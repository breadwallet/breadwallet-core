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

ETH_SRCS=./rlp/BRRlpCoder.c \
	./util/BRKeccak.c \
	./util/BRUtilHex.c \
	./util/BRUtilMath.c \
	./util/BRUtilMathParse.c \
	./event/BREvent.c \
	./event/BREventAlarm.c \
	./event/BREventQueue.c \
	./base/BREthereumAddress.c \
	./base/BREthereumData.c \
	./base/BREthereumEther.c \
	./base/BREthereumGas.c \
	./base/BREthereumHash.c \
	./base/BREthereumSignature.c \
	./blockchain/BREthereumAccountState.c \
	./blockchain/BREthereumBlock.c \
	./blockchain/BREthereumBloomFilter.c \
	./blockchain/BREthereumLog.c \
	./blockchain/BREthereumNetwork.c \
	./blockchain/BREthereumTransaction.c \
	./blockchain/BREthereumTransactionReceipt.c \
	./blockchain/BREthereumTransactionStatus.c \
	./mpt/BREthereumMPT.c \
	./contract/BREthereumContract.c \
	./contract/BREthereumToken.c \
	./les/BREthereumLES.c \
	./les/BREthereumLESFrameCoder.c \
	./les/BREthereumLESRandom.c \
	./les/BREthereumMessage.c \
	./les/BREthereumNode.c \
	./les/BREthereumNodeEndpoint.c \
	./les/BREthereumProvision.c \
	./les/msg/BREthereumMessageDIS.c \
	./les/msg/BREthereumMessageLES.c \
	./les/msg/BREthereumMessageP2P.c \
	./les/msg/BREthereumMessagePIP.c \
	./bcs/BREthereumBCS.c \
	./bcs/BREthereumBCSEvent.c \
	./bcs/BREthereumBCSSync.c \
	./ewm/BREthereumAccount.c \
	./ewm/BREthereumAmount.c \
	./ewm/BREthereumEWM.c \
	./ewm/BREthereumEWMClient.c \
	./ewm/BREthereumEWMEvent.c \
	./ewm/BREthereumTransfer.c \
	./ewm/BREthereumWallet.c

ETH_OBJS=$(ETH_SRCS:.c=.o)

ETH_TST_SRCS=test.c \
	./les/test-les.c \
	./base/testBase.c \
	./blockchain/testBc.c \
	./contract/testContract.c \
	./event/testEvent.c \
	./util/testUtil.c \
	./rlp/testRlp.c \
	./ewm/testEWM.c 

ETH_TST_OBJS=$(ETH_TST_SRCS:.c=.o)

test:	build
	./test

build: clean
	gcc -o test -Wno-format-extra-args -Wno-nullability-completeness -Wno-unknown-warning-option \
		-I/usr/include -I/usr/include/malloc -I/usr/include/machine -I../secp256k1 -I.. \
		-I. -I../secp256k1/include \
		-DDEBUG -DBITCOIN_DEBUG=1 -DTEST_ETHEREUM_NEED_MAIN \
                $(CORE_SRCS) $(ETH_SRCS) $(ETH_TST_SRCS) -lc

clean:
	rm -rf $(CORE_OBJS) $(ETH_OBJS)

