
CORE_SRCS=../BRAddress.c \
	../BRBIP32Sequence.c \
	../BRBIP38Key.c \
	../BRBIP39Mnemonic.c \
	../BRBase58.c \
	../BRBech32.c \
	../BRBloomFilter.c \
	../BRCrypto.c \
	../BRKey.c \
	../BRKeyECIES.c \
	../BRMerkleBlock.c \
	../BRPaymentProtocol.c \
	../BRPeer.c \
	../BRPeerManager.c \
	../BRSet.c \
	../BRTransaction.c \
	../BRWallet.c \
	../bcash/BRBCashAddr.c

CORE_OBJS=$(CORE_SRCS:.c=.o)

ETH_SRCS=./BREthereum.c \
	./BREthereumAccount.c \
	./BREthereumWallet.c \
	./base/BREthereumAddress.c \
	./base/BREthereumEther.c \
	./base/BREthereumGas.c \
	./base/BREthereumHash.c \
	./base/BREthereumSignature.c \
	./bcs/BREthereumBCS.c \
	./bcs/BREthereumBCSEvent.c \
	./blockchain/BREthereumAccountState.c \
	./blockchain/BREthereumAmount.c \
	./blockchain/BREthereumBlock.c \
	./blockchain/BREthereumBloomFilter.c \
	./blockchain/BREthereumLog.c \
	./blockchain/BREthereumNetwork.c \
	./blockchain/BREthereumTransaction.c \
	./blockchain/BREthereumTransactionReceipt.c \
	./blockchain/BREthereumTransactionStatus.c \
	./contract/BREthereumContract.c \
	./contract/BREthereumToken.c \
	./les/BREthereumEndpoint.c \
	./les/BREthereumFrameCoder.c \
	./les/BREthereumHandshake.c \
	./les/BREthereumLES.c \
	./les/BREthereumLESBase.c \
	./les/BREthereumLESCoder.c \
	./les/BREthereumNode.c \
	./les/BREthereumNodeDiscovery.c \
	./les/BREthereumNodeEventHandler.c \
	./les/BREthereumNodeManager.c \
	./les/BREthereumP2PCoder.c \
	./les/BREthereumRandom.c \
	./ewm/BREthereumEWM.c \
	./ewm/BREthereumEWMClient.c \
	./ewm/BREthereumEWMEvent.c \
	./event/BREvent.c \
	./event/BREventQueue.c \
	./event/BREventAlarm.c \
	./rlp/BRRlpCoder.c \
	./util/BRUtilHex.c \
	./util/BRUtilMath.c \
	./util/BRUtilMathParse.c


ETH_OBJS=$(ETH_SRCS:.c=.o)

test:	build
	./test

build: clean
	cc -o test -Wno-format-extra-args -Wno-nullability-completeness -Wno-unknown-warning-option \
		-I/usr/include -I/usr/include/malloc -I/usr/include/machine -I../secp256k1 -I.. \
		-I. -I../secp256k1/include \
		-DDEBUG -DBITCOIN_DEBUG=1 -DTEST_ETHEREUM_NEED_MAIN \
                $(CORE_SRCS) $(ETH_SRCS) les/test-les.c test.c -lc

clean:
	rm -rf $(CORE_OBJS) $(ETH_OBJS)

