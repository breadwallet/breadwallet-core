
CORE_SRCS=../BRAddress.c \
	../BRBIP32Sequence.c \
	../BRBIP38Key.c \
	../BRBIP39Mnemonic.c \
	../BRBase58.c \
	../BRBech32.c \
	../BRBloomFilter.c \
	../BRCrypto.c \
	../BRKey.c \
	../BRMerkleBlock.c \
	../BRPaymentProtocol.c \
	../BRPeer.c \
	../BRPeerManager.c \
	../BRSet.c \
	../BRTransaction.c \
	../BRWallet.c \
	../bcash/BRBCashAddr.c

CORE_OBJS=$(CORE_SRCS:.c=.o)

ETH_SRCS=./util/BRUtilMath.c \
	./util/BRUtilMathParse.c \
	./util/BRUtilHex.c \
	./rlp/BRRlpCoder.c \
	./BREthereumAccount.c \
	./BREthereumAmount.c \
	./BREthereumBlock.c \
	./BREthereumContract.c \
	./BREthereumEther.c \
	./BREthereumGas.c \
	./BREthereumLightNode.c \
	./BREthereumNetwork.c \
	./BREthereumToken.c \
	./BREthereumTransaction.c \
	./BREthereumWallet.c 

ETH_OBJS=$(ETH_SRCS:.c=.o)

test:	build
	./test

build: clean
	cc -o test -Wno-format-extra-args -Wno-nullability-completeness -Wno-unknown-warning-option \
		-I/usr/include -I/usr/include/malloc -I/usr/include/machine -I../secp256k1 -I.. \
		-I. -I./util -I./rlp \
		-DDEBUG -DTEST_ETHEREUM_NEED_MAIN $(CORE_SRCS) $(ETH_SRCS) test.c -lc

clean:
	rm -rf $(CORE_OBJS) $(ETH_OBJS)

