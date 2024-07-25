#pragma once
#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>

#include <cmath>
#include <string>

#include <utils.hpp>

using std::string;
using namespace eosio;

class [[eosio::contract("lptoken")]] lptoken : public contract {
public:
  using contract::contract;

  [[eosio::action]] void create(const name &issuer,
                                const asset &maximum_supply);
  [[eosio::action]] void issue(const name &to, const asset &quantity,
                               const string &memo);
  [[eosio::action]] void transfer(const name &from, const name &to,
                                  const asset &quantity, const string &memo);
  [[eosio::action]] void retire(const asset &quantity, const string &memo);
  [[eosio::action]] void destroy(symbol_code code);
  [[eosio::action]] void open(const name &owner, const symbol &symbol,
                              const name &ram_payer);
  [[eosio::action]] void close(const name &owner, const symbol &symbol);

  /**
   * Get supply method.
   *
   * @details Gets the supply for token `sym_code`, created by
   * `token_contract_account` account.
   *
   * @param token_contract_account - the account to get the supply for,
   * @param sym_code - the symbol to get the supply for.
   */
  static asset get_supply(const name &token_contract_account,
                          const symbol_code &sym_code) {
    stats statstable(token_contract_account, sym_code.raw());
    const auto &st = statstable.get(sym_code.raw());
    return st.supply;
  }

  /**
   * Get balance method.
   *
   * @details Get the balance for a token `sym_code` created by
   * `token_contract_account` account, for account `owner`.
   *
   * @param token_contract_account - the token creator account,
   * @param owner - the account for which the token balance is returned,
   * @param sym_code - the token for which the balance is returned.
   */
  static asset get_balance(const name &token_contract_account,
                           const name &owner, const symbol_code &sym_code) {
    accounts accountstable(token_contract_account, owner.value);
    const auto &ac = accountstable.get(sym_code.raw());
    return ac.balance;
  }

private:
  struct [[eosio::table]] account {
    asset balance;
    int64_t cost0;
    int64_t cost1;
    uint64_t primary_key() const { return balance.symbol.code().raw(); }
    EOSLIB_SERIALIZE(account, (balance)(cost0)(cost1))
  };

  struct [[eosio::table]] currency_stats {
    asset supply;
    asset max_supply;
    name issuer;
    uint64_t primary_key() const { return supply.symbol.code().raw(); }
  };

  struct [[eosio::table]] lp_token {
    name owner;
    uint64_t liquidity;
    uint64_t primary_key() const { return owner.value; }
  };

  typedef eosio::multi_index<"accounts"_n, account> accounts;
  typedef eosio::multi_index<"stat"_n, currency_stats> stats;
  typedef eosio::multi_index<"lptokens"_n, lp_token> lptokens;

  uint64_t sub_balance(const name &owner, const asset &value);
  uint64_t add_balance(const name &owner, const asset &value,
                       const name &ram_payer);
};
