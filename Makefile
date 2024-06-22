# Makefile for an eosio smart contact

SWAP_CONTRACT=swap
TOKEN_CONTRACT=lptoken

TEST_SWAP_ACCOUNT=amms.nefty
TEST_TOKEN_ACCOUNT=lp.nefty

TEST_URL=https://wax-testnet.neftyblocks.com
PROD_URL=https://wax.neftyblocks.com

build:
	./build.sh

use-test:
	cleos wallet open -n test
	cleos wallet unlock -n test

deploy-test:
	cleos -u ${TEST_URL} set contract ${TEST_SWAP_ACCOUNT} ./artifacts $(SWAP_CONTRACT).wasm $(SWAP_CONTRACT).abi -p ${TEST_SWAP_ACCOUNT}@active
	cleos -u ${TEST_URL} set contract ${TEST_TOKEN_ACCOUNT} ./artifacts $(TOKEN_CONTRACT).wasm $(TOKEN_CONTRACT).abi -p ${TEST_TOKEN_ACCOUNT}@active

install-test: build deploy-test
