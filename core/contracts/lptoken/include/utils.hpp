#pragma once
#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>

using std::string;
using namespace eosio;

const name SWAP_CONTRACT = name("amms.nefty");

struct liquidity_cost {
  uint64_t cost0;
  uint64_t cost1;
};

struct pair {
  symbol_code code;
  extended_asset reserve0;
  extended_asset reserve1;
  uint64_t total_liquidity;
  block_timestamp block_time_create;
  block_timestamp block_time_last;
  uint64_t primary_key() const { return code.raw(); }
};
typedef multi_index<"pairs"_n, pair> pairs_mi;

liquidity_cost get_costs(symbol_code code, uint64_t liquidity) {
  pairs_mi pairs_tb(SWAP_CONTRACT, SWAP_CONTRACT.value);
  auto itr = pairs_tb.find(code.raw());
  liquidity_cost costs;
  costs.cost0 = (uint128_t)liquidity * itr->reserve0.quantity.amount /
                itr->total_liquidity;
  costs.cost1 = (uint128_t)liquidity * itr->reserve1.quantity.amount /
                itr->total_liquidity;
  return costs;
}
