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

JAVA_SRCS=../../Java/root/com/breadwallet/core/BRCoreJniReference.java \
	root/com/breadwallet/core/ethereum/BREthereumAccount.java \
	root/com/breadwallet/core/ethereum/BREthereumAmount.java \
	root/com/breadwallet/core/ethereum/BREthereumEWM.java \
	root/com/breadwallet/core/ethereum/BREthereumNetwork.java \
	root/com/breadwallet/core/ethereum/BREthereumToken.java \
	root/com/breadwallet/core/ethereum/BREthereumTransaction.java \
	root/com/breadwallet/core/ethereum/BREthereumBlock.java \
	root/com/breadwallet/core/ethereum/BREthereumWallet.java \
	root/com/breadwallet/core/ethereum/test/BREthereumEWMClientTest.java \

JAVA_OBJS=$(JAVA_SRCS:.java=.class)

CORE_SRCS=../../BRAddress.c \
	../../BRBIP32Sequence.c \
	../../BRBIP38Key.c \
	../../BRBIP39Mnemonic.c \
	../../BRBase58.c \
	../../BRBloomFilter.c \
	../../BRCrypto.c \
	../../BRKey.c \
	../../BRKeyECIES.c \
	../../BRMerkleBlock.c \
	../../BRPaymentProtocol.c \
	../../BRBech32.c \
	../../BRPeer.c \
	../../BRPeerManager.c \
	../../BRSet.c \
	../../BRTransaction.c \
	../../BRWallet.c \
	../../bcash/BRBCashAddr.c \
	../../Java/BRCoreJni.c \
	../../Java/com_breadwallet_core_BRCoreJniReference.c \
	../BREthereum.c \
	../BREthereumAccount.c \
	../BREthereumWallet.c \
	../base/BREthereumAddress.c \
	../base/BREthereumEther.c \
	../base/BREthereumGas.c \
	../base/BREthereumHash.c \
	../base/BREthereumSignature.c \
	../bcs/BREthereumBCS.c \
	../bcs/BREthereumBCSEvent.c \
	../blockchain/BREthereumAccountState.c \
	../blockchain/BREthereumAmount.c \
	../blockchain/BREthereumBlock.c \
	../blockchain/BREthereumBloomFilter.c \
	../blockchain/BREthereumLog.c \
	../blockchain/BREthereumNetwork.c \
	../blockchain/BREthereumTransaction.c \
	../blockchain/BREthereumTransactionReceipt.c \
	../blockchain/BREthereumTransactionStatus.c \
	../contract/BREthereumContract.c \
	../contract/BREthereumToken.c \
	../les/BREthereumEndpoint.c \
	../les/BREthereumFrameCoder.c \
	../les/BREthereumHandshake.c \
	../les/BREthereumLES.c \
	../les/BREthereumLESBase.c \
	../les/BREthereumLESCoder.c \
	../les/BREthereumNode.c \
	../les/BREthereumNodeDiscovery.c \
	../les/BREthereumNodeEventHandler.c \
	../les/BREthereumNodeManager.c \
	../les/BREthereumP2PCoder.c \
	../les/BREthereumRandom.c \
	../ewm/BREthereumEWM.c \
	../ewm/BREthereumEWMClient.c \
	../ewm/BREthereumEWMEvent.c \
	../event/BREvent.c \
	../event/BREventQueue.c \
	../event/BREventAlarm.c \
	../rlp/BRRlpCoder.c \
	../util/BRUtilHex.c \
	../util/BRUtilMath.c \
	../util/BRUtilMathParse.c


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
