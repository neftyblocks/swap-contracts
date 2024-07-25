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

deploy-lptoken-test:
	cleos -u ${TEST_URL} set contract ${TOKEN_ACCOUNT} ./artifacts $(TOKEN_CONTRACT).wasm $(TOKEN_CONTRACT).abi -p ${TOKEN_ACCOUNT}@active

deploy-swap-test:
	cleos -u ${TEST_URL} set contract ${SWAP_ACCOUNT} ./artifacts $(SWAP_CONTRACT).wasm $(SWAP_CONTRACT).abi -p ${SWAP_ACCOUNT}@active

deploy-lptoken-prod:
	cleos -u ${PROD_URL} set contract ${TOKEN_ACCOUNT} ./artifacts $(TOKEN_CONTRACT).wasm $(TOKEN_CONTRACT).abi -p ${TOKEN_ACCOUNT}@active

deploy-swap-prod:
	cleos -u ${PROD_URL} set contract ${SWAP_ACCOUNT} ./artifacts $(SWAP_CONTRACT).wasm $(SWAP_CONTRACT).abi -p ${SWAP_ACCOUNT}@active

install-lptoken-test: build-lptoken deploy-lptoken-test

install-swap-test: build-swap deploy-swap-test

install-lptoken-prod: build-lptoken deploy-lptoken-prod

install-swap-prod: build-swap deploy-swap-prod

install-test: install-swap-test install-lptoken-test

install-prod: install-swap-prod install-lptoken-prod
