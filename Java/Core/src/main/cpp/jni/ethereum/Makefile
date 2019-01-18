DIR=$(shell pwd)
JAVA_DIR=${JAVA_HOME}
CINC_DIR=/usr/include

JNI_LIB=libCore.jnilib

JNI_SRCS=com_breadwallet_core_ethereum_BREthereumEWM.c \
	com_breadwallet_core_ethereum_BREthereumNetwork.c \
	com_breadwallet_core_ethereum_BREthereumToken.c
#	BRCoreJni.c

JNI_OBJS=$(JNI_SRCS:.c=.o)

# JNI Header Files that we are interest in keeping (that are not empty)
JNI_HDRS=$(JNI_SRCS:.c=.h)

JAVA_SRCS=../../../java/com/breadwallet/core/BRCoreJniReference.java \
	../../../java/com/breadwallet/core/ethereum/BREthereumAccount.java \
	../../../java/com/breadwallet/core/ethereum/BREthereumAmount.java \
	../../../java/com/breadwallet/core/ethereum/BREthereumEWM.java \
	../../../java/com/breadwallet/core/ethereum/BREthereumNetwork.java \
	../../../java/com/breadwallet/core/ethereum/BREthereumToken.java \
	../../../java/com/breadwallet/core/ethereum/BREthereumTransfer.java \
	../../../java/com/breadwallet/core/ethereum/BREthereumBlock.java \
	../../../java/com/breadwallet/core/ethereum/BREthereumWallet.java
#	../../../java/com/breadwallet/core/ethereum/test/BREthereumEWMClientTest.java \

JAVA_OBJS=$(JAVA_SRCS:.java=.class)

CORE_DIR=../../../../../../../

CORE_SRCS=$(CORE_DIR)/BRAddress.c \
	$(CORE_DIR)/BRBIP32Sequence.c \
	$(CORE_DIR)/BRBIP38Key.c \
	$(CORE_DIR)/BRBIP39Mnemonic.c \
	$(CORE_DIR)/BRBase58.c \
	$(CORE_DIR)/BRBloomFilter.c \
	$(CORE_DIR)/BRCrypto.c \
	$(CORE_DIR)/BRKey.c \
	$(CORE_DIR)/BRKeyECIES.c \
	$(CORE_DIR)/BRMerkleBlock.c \
	$(CORE_DIR)/BRPaymentProtocol.c \
	$(CORE_DIR)/BRBech32.c \
	$(CORE_DIR)/BRPeer.c \
	$(CORE_DIR)/BRPeerManager.c \
	$(CORE_DIR)/BRSet.c \
	$(CORE_DIR)/BRTransaction.c \
	$(CORE_DIR)/BRWallet.c \
	$(CORE_DIR)/bcash/BRBCashAddr.c \
	../../jni/BRCoreJni.c \
	../../jni/com_breadwallet_core_BRCoreJniReference.c \
	$(CORE_DIR)/ethereum/rlp/BRRlpCoder.c \
	$(CORE_DIR)/ethereum/util/BRKeccak.c \
	$(CORE_DIR)/ethereum/util/BRUtilHex.c \
	$(CORE_DIR)/ethereum/util/BRUtilMath.c \
	$(CORE_DIR)/ethereum/util/BRUtilMathParse.c \
	$(CORE_DIR)/ethereum/event/BREvent.c \
	$(CORE_DIR)/ethereum/event/BREventAlarm.c \
	$(CORE_DIR)/ethereum/event/BREventQueue.c \
	$(CORE_DIR)/ethereum/base/BREthereumAddress.c \
	$(CORE_DIR)/ethereum/base/BREthereumData.c \
	$(CORE_DIR)/ethereum/base/BREthereumEther.c \
	$(CORE_DIR)/ethereum/base/BREthereumGas.c \
	$(CORE_DIR)/ethereum/base/BREthereumHash.c \
	$(CORE_DIR)/ethereum/base/BREthereumSignature.c \
	$(CORE_DIR)/ethereum/blockchain/BREthereumAccountState.c \
	$(CORE_DIR)/ethereum/blockchain/BREthereumBlock.c \
	$(CORE_DIR)/ethereum/blockchain/BREthereumBloomFilter.c \
	$(CORE_DIR)/ethereum/blockchain/BREthereumLog.c \
	$(CORE_DIR)/ethereum/blockchain/BREthereumNetwork.c \
	$(CORE_DIR)/ethereum/blockchain/BREthereumTransaction.c \
	$(CORE_DIR)/ethereum/blockchain/BREthereumTransactionReceipt.c \
	$(CORE_DIR)/ethereum/blockchain/BREthereumTransactionStatus.c \
	$(CORE_DIR)/ethereum/mpt/BREthereumMPT.c \
	$(CORE_DIR)/ethereum/contract/BREthereumContract.c \
	$(CORE_DIR)/ethereum/contract/BREthereumToken.c \
	$(CORE_DIR)/ethereum/les/BREthereumLES.c \
	$(CORE_DIR)/ethereum/les/BREthereumLESFrameCoder.c \
	$(CORE_DIR)/ethereum/les/BREthereumLESRandom.c \
	$(CORE_DIR)/ethereum/les/BREthereumMessage.c \
	$(CORE_DIR)/ethereum/les/BREthereumNode.c \
	$(CORE_DIR)/ethereum/les/BREthereumNodeEndpoint.c \
	$(CORE_DIR)/ethereum/les/BREthereumProvision.c \
	$(CORE_DIR)/ethereum/les/msg/BREthereumMessageDIS.c \
	$(CORE_DIR)/ethereum/les/msg/BREthereumMessageLES.c \
	$(CORE_DIR)/ethereum/les/msg/BREthereumMessageP2P.c \
	$(CORE_DIR)/ethereum/les/msg/BREthereumMessagePIP.c \
	$(CORE_DIR)/ethereum/bcs/BREthereumBCS.c \
	$(CORE_DIR)/ethereum/bcs/BREthereumBCSEvent.c \
	$(CORE_DIR)/ethereum/bcs/BREthereumBCSSync.c \
	$(CORE_DIR)/ethereum/ewm/BREthereumAccount.c \
	$(CORE_DIR)/ethereum/ewm/BREthereumAmount.c \
	$(CORE_DIR)/ethereum/ewm/BREthereumEWM.c \
	$(CORE_DIR)/ethereum/ewm/BREthereumEWMClient.c \
	$(CORE_DIR)/ethereum/ewm/BREthereumEWMEvent.c \
	$(CORE_DIR)/ethereum/ewm/BREthereumTransfer.c \
	$(CORE_DIR)/ethereum/ewm/BREthereumWallet.c

CORE_OBJS=$(CORE_SRCS:.c=.o)

CFLAGS=-I$(JAVA_HOME)/include \
	-I$(JAVA_HOME)/include/darwin \
	-I$(CINC_DIR) \
	-I$(CINC_DIR)/malloc \
	-I. -I./util -I./rlp \
	-I.. -I../util -I../rlp \
	-I../.. -I../../Java \
	-I../../secp256k1 \
	-Wno-nullability-completeness -Wno-format-extra-args -Wno-unknown-warning-option

compile: $(JNI_LIB) java_comp

test: $(JNI_LIB) java_comp
	java -Xss1m -Dlight.node.test -classpath build -Djava.library.path=. \
		 com.breadwallet.core.ethereum.test.BREthereumEWMClientTest $(ARGS) # -D.

debug: $(JNI_LIB) java_comp
	java -Xss1m -Xdebug -Xrunjdwp:transport=dt_socket,address=8008,server=y,suspend=n \
		 -Dlight.node.test -classpath build -Djava.library.path=. \
		 com.breadwallet.core.ethereum.test.BREthereumEWMClientTest $(ARGS) # -D.

$(JNI_LIB): $(JNI_OBJS) $(CORE_OBJS)
	cc -dynamiclib -o $(JNI_LIB) $(JNI_OBJS) $(CORE_OBJS)

java_comp:	FORCE
	@mkdir -p build
	javac -d build $(JAVA_SRCS)

jni_hdr: java_comp
	@(cd build/com/breadwallet/core/ethereum; \
	  for class in BREthereum*.class; do \
	      javah -jni -d $(DIR) -classpath $(DIR)/build com.breadwallet.core.ethereum.$${class%%.class}; \
	  done)
	@rm -f com_breadwallet_core_ethereum_BREthereum*_*.h .h

clean:
	rm -rf build $(JNI_OBJS) $(CORE_OBJS) $(JAVA_OBJS) $(JNI_LIB)

FORCE:

#	-Wno-nullability-completeness


# Generate Headers:
# 	javac Foo.java
# 	javah -jni -d <location> Foo
# Implement Foo.c
# Generate Foo.so
# 	cc -I<path to jni.h> -I<path to jni_md.h> -I<path to std> -c Foo.c
# 	cc -dynamiclib -o libFoo.jnilib Foo.o Bar.o

# 	<path-to-jni-md>=/Library/Java/JavaVirtualMachines/jdk1.8.0_151.jdk/Contents/Home/include/darwin
# 	<path-to-jni>=/Library/Java/JavaVirtualMachines/jdk1.8.0_151.jdk/Contents/Home/include
# 	<path-to-std>=/…/Xcode.app/Contents/Dev…/Platforms/MacOSX.platform/Dev…/SDKs/MacOSX.sdk/usr/include
# Run Foo
# 	java -Djava.library.path=/Users/ebg Foo
#

# (cd ${APP}/app/build/intermediates/classes/debug/com/breadwallet/core; for class in BRCore*.class; do \
#     javah -jni -d ${APP}/app/src/main/cpp/breadwallet-core/Java/ \
#	-classpath ${APP}/app/build/intermediates/classes/debug/ \
#	com.breadwallet.core.${class%%.class}; \
#	done)
# 15RBcXQMTfebbAfUFeBbcDfs1fVvPayWdU
