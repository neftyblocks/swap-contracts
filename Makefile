# Makefile for an eosio smart contact

SWAP_CONTRACT=swap
TOKEN_CONTRACT=lptoken

SWAP_ACCOUNT=swap.nefty
TOKEN_ACCOUNT=lp.nefty

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

build: build-swap build-lptoken

build-test: build-swap-test build-lptoken-test

use-test:
	cleos wallet open -n test
	cleos wallet unlock -n test

use-prod:
	cleos wallet open -n prod
	cleos wallet unlock -n prod

deploy-test:
	cleos -u ${TEST_URL} set contract ${SWAP_ACCOUNT} ./artifacts $(SWAP_CONTRACT).wasm $(SWAP_CONTRACT).abi -p ${SWAP_ACCOUNT}@active
	cleos -u ${TEST_URL} set contract ${TOKEN_ACCOUNT} ./artifacts $(TOKEN_CONTRACT).wasm $(TOKEN_CONTRACT).abi -p ${TOKEN_ACCOUNT}@active

deploy-prod:
	cleos -u ${PROD_URL} set contract ${SWAP_ACCOUNT} ./artifacts $(SWAP_CONTRACT).wasm $(SWAP_CONTRACT).abi -p ${SWAP_ACCOUNT}@active
	cleos -u ${PROD_URL} set contract ${TOKEN_ACCOUNT} ./artifacts $(TOKEN_CONTRACT).wasm $(TOKEN_CONTRACT).abi -p ${TOKEN_ACCOUNT}@active

install-test: build-swap build-lptoken deploy-test

install-prod: build-swap build-lptoken deploy-prod
