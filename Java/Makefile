DIR=$(shell pwd)
JAVA_DIR=${JAVA_HOME}
#CINC_DIR=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include
CINC_DIR=/usr/include

JAVA_LIB=/Users/ebg/Library/Android//sdk/extras/android/m2repository/com/android/support/support-annotations/25.3.1/support-annotations-25.3.1.jar


JNI_LIB=libCore.jnilib

JNI_SDIR=Core/src/main/cpp/jni

JNI_SRCS=$(JNI_SDIR)/BRCoreJni.c \
	$(JNI_SDIR)/com_breadwallet_core_BRCoreAddress.c \
	$(JNI_SDIR)/com_breadwallet_core_BRCoreChainParams.c \
	$(JNI_SDIR)/com_breadwallet_core_BRCoreJniReference.c \
	$(JNI_SDIR)/com_breadwallet_core_BRCoreKey.c \
	$(JNI_SDIR)/com_breadwallet_core_BRCoreMasterPubKey.c \
	$(JNI_SDIR)/com_breadwallet_core_BRCoreMerkleBlock.c \
	$(JNI_SDIR)/com_breadwallet_core_BRCorePaymentProtocol.c \
	$(JNI_SDIR)/com_breadwallet_core_BRCorePeer.c \
	$(JNI_SDIR)/com_breadwallet_core_BRCorePeerManager.c \
	$(JNI_SDIR)/com_breadwallet_core_BRCoreTransaction.c \
	$(JNI_SDIR)/com_breadwallet_core_BRCoreTransactionInput.c \
	$(JNI_SDIR)/com_breadwallet_core_BRCoreTransactionOutput.c \
	$(JNI_SDIR)/com_breadwallet_core_BRCoreWallet.c \
	$(JNI_SDIR)/com_breadwallet_core_BRCoreWallet.c \
	$(JNI_SDIR)/ethereum/com_breadwallet_core_ethereum_BREthereumEWM.c \
	$(JNI_SDIR)/ethereum/com_breadwallet_core_ethereum_BREthereumNetwork.c \
	$(JNI_SDIR)/ethereum/com_breadwallet_core_ethereum_BREthereumToken.c

JNI_OBJS=$(JNI_SRCS:.c=.o)

# JNI Header Files that we are interest in keeping (because they are not empty)
JNI_HDRS=$(JNI_SRCS:.c=.h)

JAVA_SDIR=Core/src/main/java/com/breadwallet/core

JAVA_SRCS=$(JAVA_SDIR)/BRCoreAddress.java \
	$(JAVA_SDIR)/BRCoreChainParams.java \
	$(JAVA_SDIR)/BRCoreJniReference.java \
	$(JAVA_SDIR)/BRCoreKey.java \
	$(JAVA_SDIR)/BRCoreMasterPubKey.java \
	$(JAVA_SDIR)/BRCoreMerkleBlock.java \
	$(JAVA_SDIR)/BRCorePaymentProtocolEncryptedMessage.java \
	$(JAVA_SDIR)/BRCorePaymentProtocolInvoiceRequest.java \
	$(JAVA_SDIR)/BRCorePaymentProtocolMessage.java \
	$(JAVA_SDIR)/BRCorePaymentProtocolPayment.java \
	$(JAVA_SDIR)/BRCorePaymentProtocolACK.java \
	$(JAVA_SDIR)/BRCorePaymentProtocolRequest.java \
	$(JAVA_SDIR)/BRCorePeer.java \
	$(JAVA_SDIR)/BRCorePeerManager.java \
	$(JAVA_SDIR)/BRCoreTransaction.java \
	$(JAVA_SDIR)/BRCoreTransactionInput.java \
	$(JAVA_SDIR)/BRCoreTransactionOutput.java \
	$(JAVA_SDIR)/BRCoreWallet.java \
	$(JAVA_SDIR)/BRCoreWalletManager.java \
	$(JAVA_SDIR)/ethereum/BREthereumAccount.java \
	$(JAVA_SDIR)/ethereum/BREthereumAmount.java \
	$(JAVA_SDIR)/ethereum/BREthereumBlock.java \
	$(JAVA_SDIR)/ethereum/BREthereumEWM.java \
	$(JAVA_SDIR)/ethereum/BREthereumNetwork.java \
	$(JAVA_SDIR)/ethereum/BREthereumToken.java \
	$(JAVA_SDIR)/ethereum/BREthereumTransfer.java \
	$(JAVA_SDIR)/ethereum/BREthereumWallet.java
#	$(JAVA_SDIR)/test/BRWalletManager.java

JAVA_OBJS=$(JAVA_SRCS:.java=.class)

#
# Core (Bitcoin)
#
CORE_SDIR=Core/src/main/cpp/core

CORE_SRCS=$(CORE_SDIR)/BRAddress.c \
	$(CORE_SDIR)/BRBIP32Sequence.c \
	$(CORE_SDIR)/BRBIP38Key.c \
	$(CORE_SDIR)/BRBIP39Mnemonic.c \
	$(CORE_SDIR)/BRBase58.c \
	$(CORE_SDIR)/BRBech32.c \
	$(CORE_SDIR)/BRBloomFilter.c \
	$(CORE_SDIR)/BRCrypto.c \
	$(CORE_SDIR)/BRKey.c \
	$(CORE_SDIR)/BRKeyECIES.c \
	$(CORE_SDIR)/BRMerkleBlock.c \
	$(CORE_SDIR)/BRPaymentProtocol.c \
	$(CORE_SDIR)/BRPeer.c \
	$(CORE_SDIR)/BRPeerManager.c \
	$(CORE_SDIR)/BRSet.c \
	$(CORE_SDIR)/BRTransaction.c \
	$(CORE_SDIR)/BRWallet.c \
	$(CORE_SDIR)/bcash/BRBCashAddr.c

CORE_OBJS=$(CORE_SRCS:.c=.o)

#
# Core (Ethereum)
#
ETH_SRCS=$(CORE_SDIR)/ethereum/util/BRKeccak.c \
	$(CORE_SDIR)/ethereum/util/BRUtilHex.c \
	$(CORE_SDIR)/ethereum/util/BRUtilMath.c \
	$(CORE_SDIR)/ethereum/util/BRUtilMathParse.c

ETH_OBJS=$(ETH_SRCS:.c=.o)

#
#
#
CFLAGS=-I$(JAVA_HOME)/include \
	-I$(JAVA_HOME)/include/darwin \
	-I$(CINC_DIR) \
	-I$(CINC_DIR)/malloc \
	-I.. \
	-I../secp256k1 \
	-Wno-nullability-completeness -Wno-format-extra-args -Wno-unknown-warning-option

compile: $(JNI_LIB) java_comp

test: $(JNI_LIB) java_comp
	java -Xss1m -Dwallet.test -classpath build -Djava.library.path=. \
		 com.breadwallet.core.test.BRWalletManager $(ARGS) # -D.

debug: $(JNI_LIB) java_comp
	java -Xss1m -Xdebug -Xrunjdwp:transport=dt_socket,address=8008,server=y,suspend=n \
		 -Dwallet.test -classpath build -Djava.library.path=. \
		 com.breadwallet.core.test.BRWalletManager $(ARGS) # -D.

$(JNI_LIB): $(JNI_OBJS) $(CORE_OBJS)
	cc -dynamiclib -o $(JNI_LIB) $(JNI_OBJS) $(CORE_OBJS)

java_comp:	FORCE
	@mkdir -p build
	@echo "Core & Ethereum Java"
	@javac -cp $(JAVA_LIB) -d build $(JAVA_SRCS)

jni_hdr_core: 	FORCE
	@echo Core JNI Headers
	@(cd build/com/breadwallet/core; \
	  for class in BRCore*.class; do \
	      javah -jni -d $(DIR)/$(JNI_SDIR) -classpath $(DIR)/build com.breadwallet.core.$${class%%.class}; \
	  done)
	@(cd $(JNI_SDIR); \
	  rm -f .h com_breadwallet_core_BRCoreWalletManager.h com_breadwallet_core_BRCore*_*.h)

jni_hdr_eth: 	FORCE
	@echo Ethereum JNI Headers
	@(cd build/com/breadwallet/core/ethereum; \
	  for class in BREthereum*.class; do \
	      javah -jni -d $(DIR)/$(JNI_SDIR)/ethereum -classpath $(DIR)/build com.breadwallet.core.ethereum.$${class%%.class}; \
	  done)
	@(cd $(JNI_SDIR)/ethereum; \
	  rm -f .h com_breadwallet_core_ethereum_BREthereum*_*.h)

jni_hdr: java_comp jni_hdr_core jni_hdr_eth

#            if [[ "$${class}" != "*\"$\"*" ]]; then
#           fi

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
