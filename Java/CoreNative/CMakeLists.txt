# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# now build app's shared lib
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -DANDROID_STL=gnustl_static -DANDROID_TOOLCHAIN=clang")

# -Wimplicit-function-declaration
# -Wno-missing-prototypes -Werror=return-type -Wdocumentation -Wunreachable-code-aggressive -Wno-missing-braces
# -Wparentheses -Wswitch -Wno-unused-function -Wunused-label -Wno-unused-parameter -Wunused-variable -Wunused-value -Wempty-body
# -Wconditional-uninitialized -Wno-unknown-pragmas -pedantic -Wshadow -Wfour-char-constants -Wno-conversion -Wconstant-conversion
# -Wint-conversion -Wbool-conversion -Wenum-conversion -Wassign-enum -Wno-shorten-64-to-32 -Wpointer-sign -Wnewline-eof
# -Wdeprecated-declarations -Wno-sign-conversion


add_library( # Sets the name of the library.
             crypto

             # Sets the library as a shared library.
             SHARED)

# Support
target_sources (crypto
                PUBLIC

                src/main/cpp/core/support/BRAddress.c
                src/main/cpp/core/support/BRAddress.h
                src/main/cpp/core/support/BRAssert.c
                src/main/cpp/core/support/BRAssert.h
                src/main/cpp/core/support/BRArray.h
                src/main/cpp/core/support/BRBIP32Sequence.c
                src/main/cpp/core/support/BRBIP32Sequence.h
                src/main/cpp/core/support/BRBIP39Mnemonic.c
                src/main/cpp/core/support/BRBIP39Mnemonic.h
                src/main/cpp/core/support/BRBIP39WordsEn.h
                src/main/cpp/core/support/BRBase58.c
                src/main/cpp/core/support/BRBase58.h
                src/main/cpp/core/support/BRBech32.c
                src/main/cpp/core/support/BRBech32.h
                src/main/cpp/core/support/BRCrypto.c
                src/main/cpp/core/support/BRCrypto.h
                src/main/cpp/core/support/BRFileService.c
                src/main/cpp/core/support/BRFileService.h
                src/main/cpp/core/support/BRInt.h
                src/main/cpp/core/support/BRKey.c
                src/main/cpp/core/support/BRKey.h
                src/main/cpp/core/support/BRKeyECIES.c
                src/main/cpp/core/support/BRKeyECIES.h
                src/main/cpp/core/support/BRSet.c
                src/main/cpp/core/support/BRSet.h
                )

# Bitcoin
target_sources (crypto
                PUBLIC

                src/main/cpp/core/bitcoin/BRBIP38Key.c
                src/main/cpp/core/bitcoin/BRBIP38Key.h
                src/main/cpp/core/bitcoin/BRBloomFilter.c
                src/main/cpp/core/bitcoin/BRBloomFilter.h
                src/main/cpp/core/bitcoin/BRChainParams.h
                src/main/cpp/core/bitcoin/BRChainParams.c
                src/main/cpp/core/bitcoin/BRMerkleBlock.c
                src/main/cpp/core/bitcoin/BRMerkleBlock.h
                src/main/cpp/core/bitcoin/BRPaymentProtocol.c
                src/main/cpp/core/bitcoin/BRPaymentProtocol.h
                src/main/cpp/core/bitcoin/BRPeer.c
                src/main/cpp/core/bitcoin/BRPeer.h
                src/main/cpp/core/bitcoin/BRPeerManager.c
                src/main/cpp/core/bitcoin/BRPeerManager.h
                src/main/cpp/core/bitcoin/BRTransaction.c
                src/main/cpp/core/bitcoin/BRTransaction.h
                src/main/cpp/core/bitcoin/BRWallet.c
                src/main/cpp/core/bitcoin/BRWallet.h
                src/main/cpp/core/bitcoin/BRWalletManager.c
                src/main/cpp/core/bitcoin/BRWalletManager.h
                )

# BCash
target_sources (crypto
                PUBLIC

                src/main/cpp/core/bcash/BRBCashAddr.c
                src/main/cpp/core/bcash/BRBCashAddr.h
                src/main/cpp/core/bcash/BRBCashParams.h
                src/main/cpp/core/bcash/BRBCashParams.c
                )

# Ethereum
target_sources (crypto
                PUBLIC

                #
                src/main/cpp/core/ethereum/BREthereum.h

                # Util
                src/main/cpp/core/ethereum/util/BRKeccak.c
                src/main/cpp/core/ethereum/util/BRKeccak.h
                src/main/cpp/core/ethereum/util/BRUtil.h
                src/main/cpp/core/ethereum/util/BRUtilHex.c
                src/main/cpp/core/ethereum/util/BRUtilHex.h
                src/main/cpp/core/ethereum/util/BRUtilMath.c
                src/main/cpp/core/ethereum/util/BRUtilMath.h
                src/main/cpp/core/ethereum/util/BRUtilMathParse.c
#                src/main/cpp/core/ethereum/util/testUtil.c

                # RLP
                src/main/cpp/core/ethereum/rlp/BRRlp.h
                src/main/cpp/core/ethereum/rlp/BRRlpCoder.c
                src/main/cpp/core/ethereum/rlp/BRRlpCoder.h
#                src/main/cpp/core/ethereum/rlp/testRlp.c

                # Event
                src/main/cpp/core/ethereum/event/BREvent.c
                src/main/cpp/core/ethereum/event/BREvent.h
                src/main/cpp/core/ethereum/event/BREventAlarm.c
                src/main/cpp/core/ethereum/event/BREventAlarm.h
                src/main/cpp/core/ethereum/event/BREventQueue.c
                src/main/cpp/core/ethereum/event/BREventQueue.h
#                src/main/cpp/core/ethereum/event/testEvent.c

                # Base
                src/main/cpp/core/ethereum/base/BREthereumAddress.c
                src/main/cpp/core/ethereum/base/BREthereumAddress.h
                src/main/cpp/core/ethereum/base/BREthereumBase.h
                src/main/cpp/core/ethereum/base/BREthereumEther.c
                src/main/cpp/core/ethereum/base/BREthereumEther.h
                src/main/cpp/core/ethereum/base/BREthereumGas.c
                src/main/cpp/core/ethereum/base/BREthereumGas.h
                src/main/cpp/core/ethereum/base/BREthereumHash.c
                src/main/cpp/core/ethereum/base/BREthereumHash.h
                src/main/cpp/core/ethereum/base/BREthereumData.c
                src/main/cpp/core/ethereum/base/BREthereumData.h
                src/main/cpp/core/ethereum/base/BREthereumLogic.h
                src/main/cpp/core/ethereum/base/BREthereumSignature.c
                src/main/cpp/core/ethereum/base/BREthereumSignature.h
#                src/main/cpp/core/ethereum/base/testBase.c

                # Block Chain
                src/main/cpp/core/ethereum/blockchain/BREthereumAccountState.c
                src/main/cpp/core/ethereum/blockchain/BREthereumAccountState.h
                src/main/cpp/core/ethereum/blockchain/BREthereumBlock.c
                src/main/cpp/core/ethereum/blockchain/BREthereumBlock.h
                src/main/cpp/core/ethereum/blockchain/BREthereumBlockChain.h
                src/main/cpp/core/ethereum/blockchain/BREthereumBloomFilter.c
                src/main/cpp/core/ethereum/blockchain/BREthereumBloomFilter.h
                src/main/cpp/core/ethereum/blockchain/BREthereumLog.c
                src/main/cpp/core/ethereum/blockchain/BREthereumLog.h
                src/main/cpp/core/ethereum/blockchain/BREthereumNetwork.c
                src/main/cpp/core/ethereum/blockchain/BREthereumNetwork.h
                src/main/cpp/core/ethereum/blockchain/BREthereumTransaction.c
                src/main/cpp/core/ethereum/blockchain/BREthereumTransaction.h
                src/main/cpp/core/ethereum/blockchain/BREthereumTransactionReceipt.c
                src/main/cpp/core/ethereum/blockchain/BREthereumTransactionReceipt.h
                src/main/cpp/core/ethereum/blockchain/BREthereumTransactionStatus.c
                src/main/cpp/core/ethereum/blockchain/BREthereumTransactionStatus.h
                src/main/cpp/core/ethereum/blockchain/BREthereumProofOfWork.c
#                src/main/cpp/core/ethereum/blockchain/testBc.c

                # Contract
                src/main/cpp/core/ethereum/contract/BREthereumContract.c
                src/main/cpp/core/ethereum/contract/BREthereumContract.h
                src/main/cpp/core/ethereum/contract/BREthereumToken.c
                src/main/cpp/core/ethereum/contract/BREthereumToken.h
#                src/main/cpp/core/ethereum/contract/testContract.c

                # MPT
                src/main/cpp/core/ethereum/mpt/BREthereumMPT.c
                src/main/cpp/core/ethereum/mpt/BREthereumMPT.h

                # LES Msg
                src/main/cpp/core/ethereum/les/msg/BREthereumMessageDIS.c
                src/main/cpp/core/ethereum/les/msg/BREthereumMessageDIS.h
                src/main/cpp/core/ethereum/les/msg/BREthereumMessageETH.h
                src/main/cpp/core/ethereum/les/msg/BREthereumMessageLES.c
                src/main/cpp/core/ethereum/les/msg/BREthereumMessageLES.h
                src/main/cpp/core/ethereum/les/msg/BREthereumMessageP2P.c
                src/main/cpp/core/ethereum/les/msg/BREthereumMessageP2P.h
                src/main/cpp/core/ethereum/les/msg/BREthereumMessagePIP.c
                src/main/cpp/core/ethereum/les/msg/BREthereumMessagePIP.h

                # LES
                src/main/cpp/core/ethereum/les/BREthereumLES.c
                src/main/cpp/core/ethereum/les/BREthereumLES.h
                src/main/cpp/core/ethereum/les/BREthereumLESBase.h
                src/main/cpp/core/ethereum/les/BREthereumLESFrameCoder.c
                src/main/cpp/core/ethereum/les/BREthereumLESFrameCoder.h
                src/main/cpp/core/ethereum/les/BREthereumLESRandom.c
                src/main/cpp/core/ethereum/les/BREthereumLESRandom.h
                src/main/cpp/core/ethereum/les/BREthereumMessage.c
                src/main/cpp/core/ethereum/les/BREthereumMessage.h
                src/main/cpp/core/ethereum/les/BREthereumNode.c
                src/main/cpp/core/ethereum/les/BREthereumNode.h
                src/main/cpp/core/ethereum/les/BREthereumNodeEndpoint.c
                src/main/cpp/core/ethereum/les/BREthereumNodeEndpoint.h
                src/main/cpp/core/ethereum/les/BREthereumProvision.c
                src/main/cpp/core/ethereum/les/BREthereumProvision.h
                src/main/cpp/core/ethereum/les/msg/BREthereumMessageP2P.h
                src/main/cpp/core/ethereum/les/msg/BREthereumMessageP2P.c
                src/main/cpp/core/ethereum/les/msg/BREthereumMessageDIS.h
                src/main/cpp/core/ethereum/les/msg/BREthereumMessageDIS.c
                src/main/cpp/core/ethereum/les/msg/BREthereumMessageETH.h
                src/main/cpp/core/ethereum/les/msg/BREthereumMessageLES.h
                src/main/cpp/core/ethereum/les/msg/BREthereumMessageLES.c
                src/main/cpp/core/ethereum/les/msg/BREthereumMessagePIP.h
                src/main/cpp/core/ethereum/les/msg/BREthereumMessagePIP.c
#                src/main/cpp/core/ethereum/les/test-les.c
#                src/main/cpp/core/ethereum/les/test-les.h

                # BCS
                src/main/cpp/core/ethereum/bcs/BREthereumBCS.c
                src/main/cpp/core/ethereum/bcs/BREthereumBCS.h
                src/main/cpp/core/ethereum/bcs/BREthereumBCSEvent.c
                src/main/cpp/core/ethereum/bcs/BREthereumBCSPrivate.h
                src/main/cpp/core/ethereum/bcs/BREthereumBCSSync.c
                src/main/cpp/core/ethereum/bcs/BREthereumBlockChainSlice.h

                #EWM
                src/main/cpp/core/ethereum/ewm/BREthereumBase.h
                src/main/cpp/core/ethereum/ewm/BREthereumAccount.c
                src/main/cpp/core/ethereum/ewm/BREthereumAccount.h
                src/main/cpp/core/ethereum/ewm/BREthereumAmount.c
                src/main/cpp/core/ethereum/ewm/BREthereumAmount.h
                src/main/cpp/core/ethereum/ewm/BREthereumTransfer.c
                src/main/cpp/core/ethereum/ewm/BREthereumTransfer.h
                src/main/cpp/core/ethereum/ewm/BREthereumWallet.c
                src/main/cpp/core/ethereum/ewm/BREthereumWallet.h
                src/main/cpp/core/ethereum/ewm/BREthereumClient.h
                src/main/cpp/core/ethereum/ewm/BREthereumEWM.c
                src/main/cpp/core/ethereum/ewm/BREthereumEWM.h
                src/main/cpp/core/ethereum/ewm/BREthereumEWMClient.c
                src/main/cpp/core/ethereum/ewm/BREthereumEWMEvent.c
                src/main/cpp/core/ethereum/ewm/BREthereumEWMPrivate.h
#                src/main/cpp/core/ethereum/ewm/testEwm.c
                )

# Crypto
target_sources (crypto
                PUBLIC

                src/main/cpp/core/crypto/BRCryptoAccount.c
                src/main/cpp/core/crypto/BRCryptoAccount.h
                src/main/cpp/core/crypto/BRCryptoAddress.c
                src/main/cpp/core/crypto/BRCryptoAddress.h
                src/main/cpp/core/crypto/BRCryptoAmount.c
                src/main/cpp/core/crypto/BRCryptoAmount.h
                src/main/cpp/core/crypto/BRCryptoBase.h
                src/main/cpp/core/crypto/BRCryptoCurrency.c
                src/main/cpp/core/crypto/BRCryptoCurrency.h
                src/main/cpp/core/crypto/BRCryptoPrivate.h
                src/main/cpp/core/crypto/BRCryptoUnit.c
                src/main/cpp/core/crypto/BRCryptoUnit.h
#                src/main/cpp/core/crypto/testCrypto.c

                src/main/cpp/core/crypto/pending/BRCryptoCurrencyPair.c
                src/main/cpp/core/crypto/pending/BRCryptoCurrencyPair.h
                src/main/cpp/core/crypto/pending/BRCryptoNetwork.c
                src/main/cpp/core/crypto/pending/BRCryptoNetwork.h
                src/main/cpp/core/crypto/pending/BRCryptoSystem.c
                src/main/cpp/core/crypto/pending/BRCryptoSystem.h
                src/main/cpp/core/crypto/pending/BRCryptoTransfer.c
                src/main/cpp/core/crypto/pending/BRCryptoTransfer.h
                src/main/cpp/core/crypto/pending/BRCryptoWallet.c
                src/main/cpp/core/crypto/pending/BRCryptoWallet.h
                src/main/cpp/core/crypto/pending/BRCryptoWalletManager.c
                src/main/cpp/core/crypto/pending/BRCryptoWalletManager.h
                )

# Crypto JNI
target_sources (crypto
                PUBLIC)

include_directories(src/main/cpp/core/)
include_directories(src/main/cpp/core/support)
include_directories(src/main/cpp/core/bitcoin)
include_directories(src/main/cpp/core/bcash)
include_directories(src/main/cpp/core/ethereum)
include_directories(src/main/cpp/core/secp256k1)
include_directories(src/main/cpp/core/crypto)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                       crypto

                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib} )