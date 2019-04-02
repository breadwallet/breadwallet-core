CORE_SRCS=../support/BRAddress.c \
	../support/BRAssert.c \
	../support/BRBIP32Sequence.c \
	../support/BRBIP39Mnemonic.c \
	../support/BRBase58.c \
	../support/BRBech32.c \
	../support/BRCrypto.c \
	../support/BRFileService.c \
	../support/BRKey.c \
	../support/BRKeyECIES.c \
	../support/BRSet.c \
	../bitcoin/BRBIP38Key.c \
	../bitcoin/BRBloomFilter.c \
	../bitcoin/BRChainParams.c \
	../bitcoin/BRMerkleBlock.c \
	../bitcoin/BRPaymentProtocol.c \
	../bitcoin/BRPeer.c \
	../bitcoin/BRPeerManager.c \
	../bitcoin/BRTransaction.c \
	../bitcoin/BRWallet.c \
	../bitcoin/BRWalletManager.c \
	../bcash/BRBCashAddr.c \
	../bcash/BRBCashParams.c

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
	./blockchain/BREthereumProofOfWork.c \
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
	./les/testLES.c \
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
	@echo "== Building"
	gcc -o test -Wno-format-extra-args -Wno-nullability-completeness -Wno-unknown-warning-option \
		-I. -I.. -I../support -I../secp256k1 \
		-DDEBUG -DBITCOIN_DEBUG=1 -DTEST_ETHEREUM_NEED_MAIN \
                $(CORE_SRCS) $(ETH_SRCS) $(ETH_TST_SRCS) -lc -lresolv

clean:
	@echo "== Cleaning"
	@rm -rf $(CORE_OBJS) $(ETH_OBJS) $(ETH_TST_OBJS)

