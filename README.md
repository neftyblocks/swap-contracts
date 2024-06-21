# Swap contracts project

Forked from Totoro Finance Swap contracts. Modified to deploy in WAX.

## Changes to deploy to different accounts

* `core/contracts/swap/include/swap.hpp`: Replace LP_TOKEN_CONTRACT with the account where the `lptoken` contract will be deployed.
* `core/contracts/swap/include/swap.hpp`: In get_manager() function replace the manager with the account that will manage the contract.
* `core/contracts/lptoken/include/utils.hpp`: Replace SWAP_CONTRACT with the account where the `swap` contract will be deployed.

## Build

### How to Build -
   - cd to 'build' directory
   - run the command 'cmake ..'
   - run the command 'make'

### After build -
   - The built smart contract is under the 'build' directory
   - You can then do a 'set contract' action with 'cleos' and point in to the './build/xxx' directory

## How to use

### deploy

cleos set contract amms.nefty ./artifacts swap.wasm swap.abi -p amms.nefty@active
cleos set contract lp.nefty ./artifacts lptoken.wasm lptoken.abi -p lp.nefty@active

### setup
cleos push action amms.nefty setname '["manager", "admin.nefty"]' -p admin.nefty  
cleos push action amms.nefty setname '["fee.account", "fees.nefty"]' -p admin.nefty  
cleos push action amms.nefty setconfig '["fee.protocol", 10]' -p admin.nefty  
cleos push action amms.nefty setconfig '["fee.trade", 20]' -p admin.nefty  
cleos push action amms.nefty setconfig '["status", 1]' -p admin.nefty  

// additional  
cleos push action amms.nefty setname '["mine.account", "mine.nefty"]' -p admin.nefty  
cleos push action amms.nefty setconfig '["mine.status", 1]' -p admin.nefty  
cleos push action amms.nefty setname '["orac.account", "oracle.nefty"]' -p admin.nefty  
cleos push action amms.nefty setconfig '["orac.status", 1]' -p admin.nefty  

### create pair
cleos push action amms.nefty createpair '{"creator":"init.nefty","token0":{"contract":"eosio.token","sym":"8,WAX"},"token1":{"contract":"usdt.alcor","sym":"4,USDT"}}' -p init.nefty  

### remove pair
cleos push action amms.nefty removepair '{"pair_id":1}' -p admin.nefty  

### add liquidity
cleos push action eosio.token transfer '["init.nefty","amms.nefty","1000.00000000 WAX","deposit:1"]' -p init.nefty  
cleos push action usdt.alcor transfer '["init.nefty","amms.nefty","2000.0000 USDT","deposit:1"]' -p init.nefty  
cleos push action amms.nefty addliquidity '["init.nefty",1]' -p init.nefty  

### cancel deposit order
cleos push action amms.nefty cancel '["init.nefty",1]' -p init.nefty  

### remove liquidity
cleos push action lptoken.ttr transfer '["init.nefty","amms.nefty","14832396 LPA",""]' -p init.nefty  

### swap
cleos push action eosio.token transfer '["init.nefty","amms.nefty","1.00000000 WAX","swap:1"]' -p init.nefty  
cleos push action usdt.alcor transfer '["init.nefty","amms.nefty","1.0000 USDT","swap:1"]' -p init.nefty  

### multi-path swap
cleos push action usdt.alcor transfer '["init.nefty","amms.nefty","10.0000 USDT","swap:1-2"]' -p init.nefty  

