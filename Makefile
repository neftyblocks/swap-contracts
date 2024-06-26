# Makefile for an eosio smart contact

SWAP_CONTRACT=swap
TOKEN_CONTRACT=lptoken

TEST_SWAP_ACCOUNT=swap.nefty
TEST_TOKEN_ACCOUNT=lp.nefty

TEST_URL=https://wax-testnet.neftyblocks.com
PROD_URL=https://wax.neftyblocks.com

build-swap:
	./build.sh ${SWAP_CONTRACT}

build-lptoken:
	./build.sh ${TOKEN_CONTRACT}

build-swap-test:
	./build.sh ${SWAP_CONTRACT} -D=BUILD_TEST=1

build-lptoken-test:
	./build.sh ${TOKEN_CONTRACT}

use-test:
	cleos wallet open -n test
	cleos wallet unlock -n test

deploy-test:
	cleos -u ${TEST_URL} set contract ${TEST_SWAP_ACCOUNT} ./artifacts $(SWAP_CONTRACT).wasm $(SWAP_CONTRACT).abi -p ${TEST_SWAP_ACCOUNT}@active
	cleos -u ${TEST_URL} set contract ${TEST_TOKEN_ACCOUNT} ./artifacts $(TOKEN_CONTRACT).wasm $(TOKEN_CONTRACT).abi -p ${TEST_TOKEN_ACCOUNT}@active

install-test: build-swap build-lptoken deploy-test
