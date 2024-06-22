#include <eosio/system.hpp>
#include <swap.hpp>

ACTION swap::createpair(name creator, extended_symbol token0,
                        extended_symbol token1) {
  require_auth(creator);
  check(get_config("status") == 1, "contract under maintenance");
  check(token0 != token1, "can not submit with same token");
  check(token0.get_contract() != LP_TOKEN_CONTRACT &&
            token1.get_contract() != LP_TOKEN_CONTRACT,
        "can not create lp token pool now");
  auto supply0 = get_supply(token0.get_contract(), token0.get_symbol().code());
  auto supply1 = get_supply(token1.get_contract(), token1.get_symbol().code());
  check(supply0.symbol == token0.get_symbol() &&
            supply1.symbol == token1.get_symbol(),
        "invalid symbol");

  auto hash = str_hash(token0, token1);
  auto byhash_idx = _pairs.get_index<"byhash"_n>();
  auto itr = byhash_idx.find(hash);
  // check(false, itr->code.to_string() + ":" + std::to_string(hash) + "," +
  // std::to_string(itr->hash()));
  while (itr != byhash_idx.end() && itr->hash() == hash) {
    check(!is_same_pair(token0, token1, itr->reserve0.get_extended_symbol(),
                        itr->reserve1.get_extended_symbol()),
          "pair already exists");
    itr++;
  }

  // Calculate symbol based on the two pairs
  auto ftoken = token0;
  auto stoken = token1;
  if (token0 < token1) {
    ftoken = token1;
    stoken = token0;
  }

  symbol_code code =
      symbol_code(ftoken.get_symbol().code().to_string().substr(0, 3) +
                  stoken.get_symbol().code().to_string().substr(0, 3));
  uint64_t suffix_id = 1;
  while (_pairs.find(code.raw()) != _pairs.end()) {
    code = symbol_code(code.to_string() + int2code(suffix_id));
    suffix_id++;
  }

  auto sym = symbol(code, 0);
  auto timestamp = current_block_time();
  _pairs.emplace(creator, [&](auto &a) {
    a.code = sym.code();
    a.active = true;
    a.reserve0 = extended_asset(0, token0);
    a.reserve1 = extended_asset(0, token1);
    a.total_liquidity = 0;
    a.created_time = timestamp;
    a.updated_time = timestamp;
  });

  // create log
  auto logdata = std::make_tuple(code, creator, token0, token1);
  action(permission_level{_self, "active"_n}, _self, "lognewpair"_n, logdata)
      .send();

  // create symbol
  auto max_supply = asset(1000000000000000000, sym);
  auto create_data = std::make_tuple(_self, max_supply);
  action(permission_level{_self, "active"_n}, LP_TOKEN_CONTRACT, "create"_n,
         create_data)
      .send();
}

ACTION swap::removepair(symbol_code code) {
  require_auth(get_manager());
  check(get_config("status") == 1, "contract under maintenance");
  auto p_itr = _pairs.require_find(code.raw(), "pair does not exist");
  check(p_itr->total_liquidity == 0, "unable to remove active pair");

  auto now_time = current_time_point().sec_since_epoch();
  auto create_time = p_itr->created_time.to_time_point().sec_since_epoch();

  deposits_mi deposits(_self, code.raw());
  check(deposits.begin() == deposits.end(),
        "can not remove the pair which still have deposits");

  // remove lptoken
  action(permission_level{_self, "active"_n}, LP_TOKEN_CONTRACT, "destroy"_n,
         std::make_tuple(p_itr->code))
      .send();

  _pairs.erase(p_itr);
}

ACTION swap::setactive(symbol_code code, bool active) {
  require_auth(get_manager());

  auto p_itr = _pairs.require_find(code.raw(), "pair does not exist");
  _pairs.modify(p_itr, same_payer, [&](auto &a) {
    a.active = active;
    a.updated_time = current_block_time();
  });
}

ACTION swap::refund(name owner, symbol_code code) {
  require_auth(owner);

  check(get_config("status") == 1, "contract under maintenance");
  deposits_mi deposits(_self, code.raw());
  auto itr = deposits.require_find(owner.value, "you don't have any deposit");
  auto p_itr = _pairs.require_find(code.raw(), "pair does not exist");

  if (itr->quantity0.amount > 0) {
    transfer_to(p_itr->reserve0.contract, owner, itr->quantity0,
                std::string("Swap cancel refund"));
  }
  if (itr->quantity1.amount > 0) {
    transfer_to(p_itr->reserve1.contract, owner, itr->quantity1,
                std::string("Swap cancel refund"));
  }
  deposits.erase(itr);
}

ACTION swap::setconfig(name key, uint64_t value) {
  name manager = get_manager();
  require_auth(manager);
  auto itr = _configs.find(name(key).value);
  if (itr == _configs.end()) {
    _configs.emplace(manager, [&](auto &a) {
      a.key = key;
      a.value = value;
    });
  } else {
    _configs.modify(itr, same_payer, [&](auto &a) { a.value = value; });
  }
}

ACTION swap::setname(name key, name value) { setconfig(key, value.value); }

void swap::handle_transfer(name from, name to, asset quantity, string memo) {
  name token_contract = get_first_receiver();
  if (from == _self || to != _self) {
    return;
  }
  require_auth(from);
  check(get_config("status") == 1, "contract under maintenance");
  if (token_contract == name(LP_TOKEN_CONTRACT)) {
    handle_rmliquidity(from, token_contract, quantity);
    return;
  }

  if (memo.find("deposit_to_pair:") == 0) {
    const std::vector<extended_symbol> &symbols =
        symbols_from_string(memo.substr(16));

    uint64_t hash = str_hash(symbols[0], symbols[1]);
    auto byhash_idx = _pairs.get_index<"byhash"_n>();
    auto p_itr = byhash_idx.find(hash);
    while (!is_same_pair(p_itr->reserve0.get_extended_symbol(),
                         p_itr->reserve1.get_extended_symbol(), symbols[0],
                         symbols[1])) {
      p_itr++;
    }
    check(p_itr != byhash_idx.end(), "pair does not exist");
    handle_deposit(p_itr->code, from, token_contract, quantity);
    return;
  }

  std::map<string, string> dict = mappify(memo);
  check(dict.size() >= 1, "invaild memo");
  if (dict.find("deposit") != dict.end()) {
    string sid = dict.find("deposit")->second;
    check(sid.size() > 0, "invalid symbol code in memo");
    symbol_code code = symbol_code(sid);
    handle_deposit(code, from, token_contract, quantity);
  } else if (dict.find("swap") != dict.end()) {
    uint64_t min_amount = 0;
    auto itr = dict.find("min");
    if (itr != dict.end()) {
      min_amount = strtoull(itr->second.c_str(), NULL, 10);
    }
    string path = dict.find("swap")->second;

    std::vector<symbol_code> codes = split_codes(path, "-");
    handle_swap(codes, from, token_contract, quantity, min_amount);
  } else {
    check(false, "invalid memo");
  }
}

void swap::handle_swap(std::vector<symbol_code> codes, name from, name contract,
                       asset quantity, uint64_t min_amount) {
  extended_asset to_asset(quantity, contract);
  for (uint8_t i = 0; i < codes.size(); i++) {
    auto code = codes[i];
    auto from_asset = to_asset;
    to_asset = swap_token(code, from, from_asset.contract, from_asset.quantity);
  }
  check(min_amount == 0 || to_asset.quantity.amount >= min_amount,
        "returns less than expected");

  double amount_in_f =
      quantity.amount * 1.0 / pow(10, quantity.symbol.precision());
  double amount_out_f = to_asset.quantity.amount * 1.0 /
                        pow(10, to_asset.quantity.symbol.precision());
  double price_f = amount_out_f / amount_in_f;
  string memo = string_format(
      string("Swap from %s to %s with price %.8lf"),
      quantity.symbol.code().to_string().c_str(),
      to_asset.quantity.symbol.code().to_string().c_str(), price_f);
  transfer_to(to_asset.contract, from, to_asset.quantity, memo);
}

void swap::handle_rmliquidity(name owner, name contract, asset quantity) {
  auto data = make_tuple(quantity, string("retire LP token"));
  action(permission_level{_self, "active"_n}, LP_TOKEN_CONTRACT, "retire"_n,
         data)
      .send();
  remove_liquidity(owner, quantity.symbol.code(), quantity.amount);
}

void swap::remove_liquidity(name owner, symbol_code code, uint64_t amount) {
  require_auth(owner);
  check(get_config("status") == 1, "contract under maintenance");
  auto p_itr = _pairs.require_find(code.raw(), "pair does not exist");
  uint64_t reserve0 = p_itr->reserve0.quantity.amount;
  uint64_t reserve1 = p_itr->reserve1.quantity.amount;
  uint64_t amount0 = (uint128_t)reserve0 * amount / p_itr->total_liquidity;
  uint64_t amount1 = (uint128_t)reserve1 * amount / p_itr->total_liquidity;
  check(amount0 > 0 && amount1 > 0 && amount0 <= reserve0 &&
            amount1 <= reserve1,
        "insufficient liquidity");

  _pairs.modify(p_itr, same_payer, [&](auto &a) {
    a.total_liquidity = safe_sub(a.total_liquidity, amount);
  });

  transfer_to(p_itr->reserve0.contract, owner,
              asset(amount0, p_itr->reserve0.quantity.symbol),
              string("Swap remove liquidity"));
  transfer_to(p_itr->reserve1.contract, owner,
              asset(amount1, p_itr->reserve1.quantity.symbol),
              string("Swap remove liquidity"));

  uint64_t balance0 = safe_sub(reserve0, amount0);
  uint64_t balance1 = safe_sub(reserve1, amount1);
  update_pair(code, balance0, balance1, reserve0, reserve1);

  // rmliquidity log
  auto quantity0 = asset(amount0, p_itr->reserve0.quantity.symbol);
  auto quantity1 = asset(amount1, p_itr->reserve1.quantity.symbol);
  auto data =
      std::make_tuple(code, owner, amount, quantity0, quantity1,
                      p_itr->total_liquidity, p_itr->reserve0, p_itr->reserve1);
  action(permission_level{_self, "active"_n}, _self, "logremliq"_n, data)
      .send();
}

void swap::handle_deposit(symbol_code code, name owner, name contract,
                          asset quantity) {
  auto p_itr = _pairs.require_find(code.raw(), "pair does not exist");
  check(p_itr->active, "pair is not active for deposits");

  extended_symbol input(quantity.symbol, contract);
  extended_symbol token0 = p_itr->reserve0.get_extended_symbol();
  extended_symbol token1 = p_itr->reserve1.get_extended_symbol();

  check(input == token0 || input == token1, "invalid deposit");

  deposits_mi deposits(_self, code.raw());
  auto itr = deposits.find(owner.value);
  if (itr == deposits.end()) {
    itr = deposits.emplace(_self, [&](auto &a) {
      a.owner = owner;
      a.quantity0.symbol = token0.get_symbol();
      a.quantity1.symbol = token1.get_symbol();
    });
  }
  deposits.modify(itr, same_payer, [&](auto &a) {
    if (input == token0) {
      a.quantity0 += quantity;
    } else if (input == token1) {
      a.quantity1 += quantity;
    }
  });
}

void swap::addliquidity(name owner, extended_symbol token0,
                        extended_symbol token1) {
  uint64_t hash = str_hash(token0, token1);
  auto byhash_idx = _pairs.get_index<"byhash"_n>();
  auto p_itr = byhash_idx.find(hash);
  while (!is_same_pair(p_itr->reserve0.get_extended_symbol(),
                       p_itr->reserve1.get_extended_symbol(), token0, token1)) {
    p_itr++;
  }
  check(p_itr != byhash_idx.end(), "pair does not exist");
  check(p_itr->active, "pair is not active to add liquidity");
  symbol_code code = p_itr->code;

  deposits_mi deposits(_self, code.raw());
  auto d_itr = deposits.require_find(owner.value, "you don't have any deposit");
  check(d_itr->quantity0.amount > 0 && d_itr->quantity1.amount > 0,
        "you need have both tokens");

  uint64_t amount0 = d_itr->quantity0.amount;
  uint64_t amount1 = d_itr->quantity1.amount;
  uint64_t amount0_refund = 0;
  uint64_t amount1_refund = 0;
  uint64_t reserve0 = p_itr->reserve0.quantity.amount;
  uint64_t reserve1 = p_itr->reserve1.quantity.amount;

  // calc amount0 and amount1
  if (reserve0 > 0 || reserve1 > 0) {
    uint128_t amount_temp = (uint128_t)amount0 * reserve1 / reserve0;
    check(amount_temp < asset::max_amount, "input amount too large");
    uint64_t amount1_matched = amount_temp;
    if (amount1_matched <= amount1) {
      amount1_refund = amount1 - amount1_matched;
      amount1 = amount1_matched;
    } else {
      amount_temp = (uint128_t)amount1 * reserve0 / reserve1;
      check(amount_temp < asset::max_amount, "input amount too large");
      uint64_t amount0_matched = amount_temp;
      amount0_refund = amount0 - amount0_matched;
      amount0 = amount0_matched;
    }
  }

  // calc liquidity
  uint64_t liquidity = 0;
  uint64_t total_liquidity = p_itr->total_liquidity;
  if (total_liquidity == 0) {
    liquidity = sqrt((uint128_t)amount0 * amount1);
    check(liquidity >= MINIMUM_LIQUIDITY, "insufficient liquidity minted");
  } else {
    liquidity = std::min((uint128_t)amount0 * total_liquidity / reserve0,
                         (uint128_t)amount1 * total_liquidity / reserve1);
  }

  deposits.erase(d_itr);

  byhash_idx.modify(p_itr, same_payer, [&](auto &a) {
    a.total_liquidity = safe_add(a.total_liquidity, liquidity);
  });

  uint64_t balance0 = safe_add(reserve0, amount0);
  uint64_t balance1 = safe_add(reserve1, amount1);
  update_pair(code, balance0, balance1, reserve0, reserve1);

  // refund
  if (amount0_refund > 0) {
    transfer_to(p_itr->reserve0.contract, owner,
                asset(amount0_refund, p_itr->reserve0.quantity.symbol),
                string("Swap deposit refund"));
  }
  if (amount1_refund > 0) {
    transfer_to(p_itr->reserve1.contract, owner,
                asset(amount1_refund, p_itr->reserve1.quantity.symbol),
                string("Swap deposit refund"));
  }

  // issue lp tokens
  auto lp_issue = asset(liquidity, symbol(p_itr->code, 0));
  auto data = make_tuple(_self, lp_issue, string("Issue LP token"));
  action(permission_level{_self, "active"_n}, LP_TOKEN_CONTRACT, "issue"_n,
         data)
      .send();
  inline_transfer(LP_TOKEN_CONTRACT, _self, owner, lp_issue,
                  string("Issue LP token"));

  // addliquidity log
  auto quantity0 = asset(amount0, p_itr->reserve0.quantity.symbol);
  auto quantity1 = asset(amount1, p_itr->reserve1.quantity.symbol);
  auto data2 =
      std::make_tuple(code, owner, liquidity, quantity0, quantity1,
                      p_itr->total_liquidity, p_itr->reserve0, p_itr->reserve1);
  action(permission_level{_self, "active"_n}, _self, "logaddliq"_n, data2)
      .send();
}

extended_asset swap::swap_token(symbol_code code, name from, name contract,
                                asset quantity) {
  auto p_itr = _pairs.require_find(code.raw(), "pair does not exist");
  check(p_itr->active, "pair is not active for trading");
  bool is_token0 = contract == p_itr->reserve0.contract &&
                   quantity.symbol == p_itr->reserve0.quantity.symbol;
  bool is_token1 = contract == p_itr->reserve1.contract &&
                   quantity.symbol == p_itr->reserve1.quantity.symbol;
  check(is_token0 || is_token1, "contract or symbol error");

  uint64_t amount_in = quantity.amount;
  uint64_t protocol_fee =
      (uint128_t)amount_in * get_config("fee.protocol") / 10000;
  uint64_t trade_fee = (uint128_t)amount_in * get_config("fee.trade") / 10000;

  check(trade_fee + protocol_fee > 0, "swap amount too small");
  if (protocol_fee > 0) {
    amount_in -= protocol_fee;
    transfer_to(contract, get_account("fee.account"),
                asset(protocol_fee, quantity.symbol),
                string("Swap protocol fee"));
  }

  uint64_t reserve0 = p_itr->reserve0.quantity.amount;
  uint64_t reserve1 = p_itr->reserve1.quantity.amount;
  uint64_t amount_out = 0;
  extended_asset output;
  uint64_t balance0;
  uint64_t balance1;
  if (is_token0) {
    amount_out = get_output(amount_in, reserve0, reserve1, trade_fee);
    check(amount_out >= 0, "insufficient output amount");
    output.contract = p_itr->reserve1.contract;
    output.quantity = asset(amount_out, p_itr->reserve1.quantity.symbol);
    balance0 = safe_add(reserve0, amount_in);
    balance1 = safe_sub(reserve1, amount_out);
  } else {
    amount_out = get_output(amount_in, reserve1, reserve0, trade_fee);
    check(amount_out >= 0, "insufficient output amount");
    output.contract = p_itr->reserve0.contract;
    output.quantity = asset(amount_out, p_itr->reserve0.quantity.symbol);
    balance0 = safe_sub(reserve0, amount_out);
    balance1 = safe_add(reserve1, amount_in);
  }
  update_pair(code, balance0, balance1, reserve0, reserve1);

  // logs
  double amount_in_f = amount_in * 1.0 / pow(10, quantity.symbol.precision());
  double amount_out_f =
      amount_out * 1.0 / pow(10, output.quantity.symbol.precision());
  double trade_price = amount_out_f / amount_in_f;

  auto token_in = is_token0 ? p_itr->reserve0.get_extended_symbol()
                            : p_itr->reserve1.get_extended_symbol();
  auto token_out = is_token0 ? p_itr->reserve1.get_extended_symbol()
                             : p_itr->reserve0.get_extended_symbol();
  auto data = std::make_tuple(
      code, from, token_in.get_contract(), quantity, token_out.get_contract(),
      output.quantity, asset(trade_fee + protocol_fee, quantity.symbol),
      trade_price, p_itr->total_liquidity, p_itr->reserve0, p_itr->reserve1);
  action(permission_level{_self, "active"_n}, _self, "logswap"_n, data).send();
  return output;
}

void swap::update_pair(symbol_code code, uint64_t balance0, uint64_t balance1,
                       uint64_t reserve0, uint64_t reserve1) {
  check(balance0 >= 0 && balance1 >= 0, "update usertokens error");
  auto p_itr = _pairs.require_find(code.raw(), "pair does not exist");
  _pairs.modify(p_itr, same_payer, [&](auto &a) {
    a.reserve0.quantity.amount = balance0;
    a.reserve1.quantity.amount = balance1;
    a.updated_time = current_block_time();
  });
}

void swap::lognewpair(symbol_code code, name creator, extended_symbol token0,
                      extended_symbol token1) {
  require_auth(_self);
}

void swap::logaddliq(symbol_code code, name owner, uint64_t liquidity,
                     asset quantity0, asset quantity1, uint64_t total_liquidity,
                     extended_asset reserve0, extended_asset reserve1) {
  require_auth(_self);
}

void swap::logremliq(symbol_code code, name owner, uint64_t liquidity,
                     asset quantity0, asset quantity1, uint64_t total_liquidity,
                     extended_asset reserve0, extended_asset reserve1) {
  require_auth(_self);
}

void swap::logswap(symbol_code code, name owner, name contract_in,
                   asset quantity_in, name contract_out, asset quantity_out,
                   asset fee, double trade_price, uint64_t total_liquidity,
                   extended_asset reserve0, extended_asset reserve1) {
  require_auth(_self);
}
