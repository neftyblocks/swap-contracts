#include <helpers.hpp>
#include <math.h>

class [[eosio::contract("swap")]] swap : public contract {
public:
  using contract::contract;
  static constexpr name LP_TOKEN_CONTRACT{name("lp.nefty")};
  static const uint64_t MINIMUM_LIQUIDITY = 10000;

  swap(name receiver, name code, datastream<const char *> ds)
      : contract(receiver, code, ds), _pairs(_self, _self.value),
        _configs(_self, _self.value) {}

  [[eosio::action]] void setconfig(name key, uint64_t value);
  [[eosio::action]] void setname(name key, name value);

  [[eosio::action]] void createpair(name creator, extended_symbol token0,
                                    extended_symbol token1);
  [[eosio::action]] void removepair(symbol_code code);
  [[eosio::action]] void addliquidity(name owner, extended_symbol token0,
                                      extended_symbol token1);
  [[eosio::action]] void refund(name owner, symbol_code code);
  [[eosio::action]] void setactive(symbol_code code, bool active);

  [[eosio::on_notify("*::transfer")]] void
  handle_transfer(name from, name to, asset quantity, string memo);

  // Log actions
  [[eosio::action]] void lognewpair(symbol_code code, name creator,
                                    extended_symbol token0,
                                    extended_symbol token1);
  [[eosio::action]] void logaddliq(symbol_code code, name owner,
                                   uint64_t liquidity, asset quantity0,
                                   asset quantity1, uint64_t total_liquidity,
                                   extended_asset reserve0,
                                   extended_asset reserve1);
  [[eosio::action]] void logremliq(symbol_code code, name owner,
                                   uint64_t liquidity, asset quantity0,
                                   asset quantity1, uint64_t total_liquidity,
                                   extended_asset reserve0,
                                   extended_asset reserve1);
  [[eosio::action]] void logswap(symbol_code code, name owner, name contract_in,
                                 asset quantity_in, name contract_out,
                                 asset quantity_out, asset fee,
                                 double trade_price, uint64_t total_liquidity,
                                 extended_asset reserve0,
                                 extended_asset reserve1);

private:
  TABLE pair {
    symbol_code code;
    bool active;
    extended_asset reserve0;
    extended_asset reserve1;
    uint64_t total_liquidity;
    block_timestamp created_time;
    block_timestamp updated_time;
    uint64_t primary_key() const { return code.raw(); }
    uint64_t hash() const {
      return str_hash(reserve0.get_extended_symbol(),
                      reserve1.get_extended_symbol());
    }
    EOSLIB_SERIALIZE(
        pair,
        (code)(active)(reserve0)(reserve1)(total_liquidity)(created_time)(updated_time))
  };

  TABLE deposit {
    name owner;
    asset quantity0;
    asset quantity1;
    uint64_t primary_key() const { return owner.value; }
    EOSLIB_SERIALIZE(deposit, (owner)(quantity0)(quantity1))
  };

  TABLE config {
    name key;
    uint64_t value;
    uint64_t primary_key() const { return key.value; }
    EOSLIB_SERIALIZE(config, (key)(value))
  };

  typedef multi_index<
      "pairs"_n, pair,
      indexed_by<"byhash"_n, const_mem_fun<pair, uint64_t, &pair::hash>>>
      pairs_mi;
  typedef multi_index<"deposits"_n, deposit> deposits_mi;
  typedef multi_index<"configs"_n, config> configs_mi;
  pairs_mi _pairs;
  configs_mi _configs;

  extended_asset swap_token(symbol_code code, name from, name contract,
                            asset quantity);
  void update_pair(symbol_code code, uint64_t balance0, uint64_t balance1,
                   uint64_t reserve0, uint64_t reserve1);
  void handle_swap(std::vector<symbol_code> codes, name from, name contract,
                   asset quantity, uint64_t min_out);
  void handle_deposit(symbol_code code, name owner, name contract,
                      asset quantity);
  void handle_rmliquidity(name owner, name contract, asset quantity);
  void remove_liquidity(name owner, symbol_code code, uint64_t amount);

  uint64_t get_config(string key) {
    auto itr = _configs.find(name(key).value);
    if (itr != _configs.end()) {
      return itr->value;
    }
    return 0;
  }

  name get_account(string key) {
    uint64_t value = get_config(key);
    check(value > 0 && is_account(name(value)), "account not set");
    return name(value);
  }

  name get_manager() {
    name manager = name(get_config("manager"));
    if (manager.value == 0) {
      manager = name("admin.nefty"); // default manager
    }
    return manager;
  }

  void transfer_to(const name &contract, const name &to, const asset &quantity,
                   string memo) {
    inline_transfer(contract, _self, to, quantity, memo);
  }
};
