# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# now build app's shared lib
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

#
# sqlite3 build
#

add_library(sqlite3
            STATIC)

target_sources (sqlite3
                PRIVATE
                src/main/cpp/core/vendor/sqlite3/sqlite3.c
                src/main/cpp/core/vendor/sqlite3/sqlite3.h)

target_include_directories(sqlite3
                           INTERFACE
                           ${PROJECT_SOURCE_DIR}/src/main/cpp/core/vendor/sqlite3)

target_compile_definitions (sqlite3
                            PRIVATE
                            _HAVE_SQLITE_CONFIG_H=1)

#
# crypto build
#

add_library( # Sets the name of the library.
             corecrypto
             # Sets the library as a shared library.
             SHARED)

# Support
target_sources (corecrypto
                PRIVATE
                src/main/cpp/core/support/BRAddress.c
                src/main/cpp/core/support/BRAddress.h
                src/main/cpp/core/support/BRAssert.c
                src/main/cpp/core/support/BRAssert.h
                src/main/cpp/core/support/BRArray.h
                src/main/cpp/core/support/BRBase.h
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
                src/main/cpp/core/support/BRSet.h)

# Support Tests
if (CMAKE_BUILD_TYPE MATCHES Debug)
    target_sources (corecrypto
                    PRIVATE
                    src/main/cpp/core/support/testSup.c)
endif(CMAKE_BUILD_TYPE MATCHES Debug)

# Bitcoin
target_sources (corecrypto
                PRIVATE
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
                src/main/cpp/core/bitcoin/BRSyncManager.c
                src/main/cpp/core/bitcoin/BRSyncManager.h
                src/main/cpp/core/bitcoin/BRTransaction.c
                src/main/cpp/core/bitcoin/BRTransaction.h
                src/main/cpp/core/bitcoin/BRWallet.c
                src/main/cpp/core/bitcoin/BRWallet.h
                src/main/cpp/core/bitcoin/BRWalletManager.c
                src/main/cpp/core/bitcoin/BRWalletManager.h
                src/main/cpp/core/bitcoin/BRWalletManagerEvent.c
                src/main/cpp/core/bitcoin/BRWalletManagerPrivate.h)

# Bitcoin Tests
if (CMAKE_BUILD_TYPE MATCHES Debug)
    target_sources (corecrypto
                    PRIVATE
                    src/main/cpp/core/bitcoin/test.c
                    src/main/cpp/core/bitcoin/testBwm.c)
endif(CMAKE_BUILD_TYPE MATCHES Debug)

# BCash
target_sources (corecrypto
                PRIVATE
                src/main/cpp/core/bcash/BRBCashAddr.c
                src/main/cpp/core/bcash/BRBCashAddr.h
                src/main/cpp/core/bcash/BRBCashParams.h
                src/main/cpp/core/bcash/BRBCashParams.c)

# Ethereum
target_sources (corecrypto
                PRIVATE
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
                # RLP
                src/main/cpp/core/ethereum/rlp/BRRlp.h
                src/main/cpp/core/ethereum/rlp/BRRlpCoder.c
                src/main/cpp/core/ethereum/rlp/BRRlpCoder.h
                # Event
                src/main/cpp/core/ethereum/event/BREvent.c
                src/main/cpp/core/ethereum/event/BREvent.h
                src/main/cpp/core/ethereum/event/BREventAlarm.c
                src/main/cpp/core/ethereum/event/BREventAlarm.h
                src/main/cpp/core/ethereum/event/BREventQueue.c
                src/main/cpp/core/ethereum/event/BREventQueue.h
                # Base
                src/main/cpp/core/ethereum/base/BREthereumAddress.c
                src/main/cpp/core/ethereum/base/BREthereumAddress.h
                src/main/cpp/core/ethereum/base/BREthereumBase.h
                src/main/cpp/core/ethereum/base/BREthereumEther.c
                src/main/cpp/core/ethereum/base/BREthereumEther.h
                src/main/cpp/core/ethereum/base/BREthereumGas.c
                src/main/cpp/core/ethereum/base/BREthereumGas.h
                src/main/cpp/core/ethereum/base/BREthereumFeeBasis.c
                src/main/cpp/core/ethereum/base/BREthereumFeeBasis.h
                src/main/cpp/core/ethereum/base/BREthereumHash.c
                src/main/cpp/core/ethereum/base/BREthereumHash.h
                src/main/cpp/core/ethereum/base/BREthereumData.c
                src/main/cpp/core/ethereum/base/BREthereumData.h
                src/main/cpp/core/ethereum/base/BREthereumLogic.h
                src/main/cpp/core/ethereum/base/BREthereumSignature.c
                src/main/cpp/core/ethereum/base/BREthereumSignature.h
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
                # Contract
                src/main/cpp/core/ethereum/contract/BREthereumContract.c
                src/main/cpp/core/ethereum/contract/BREthereumContract.h
                src/main/cpp/core/ethereum/contract/BREthereumToken.c
                src/main/cpp/core/ethereum/contract/BREthereumToken.h
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
                src/main/cpp/core/ethereum/ewm/BREthereumEWMPersist.c
                src/main/cpp/core/ethereum/ewm/BREthereumEWMPrivate.h)

# Ethereum Tests
if (CMAKE_BUILD_TYPE MATCHES Debug)
    target_sources (corecrypto
                    PRIVATE
                    src/main/cpp/core/ethereum/base/testBase.c
                    src/main/cpp/core/ethereum/blockchain/testBc.c
                    src/main/cpp/core/ethereum/contract/testContract.c
                    src/main/cpp/core/ethereum/event/testEvent.c
                    src/main/cpp/core/ethereum/ewm/testEwm.c
                    src/main/cpp/core/ethereum/rlp/testRlp.c
                    src/main/cpp/core/ethereum/les/testLES.c
                    src/main/cpp/core/ethereum/util/testUtil.c
                    src/main/cpp/core/ethereum/test.c)
endif(CMAKE_BUILD_TYPE MATCHES Debug)

# Ripple
target_sources (corecrypto
                PRIVATE
                src/main/cpp/core/ripple/BRRipplePrivateStructs.h
                src/main/cpp/core/ripple/BRRippleBase.h
                src/main/cpp/core/ripple/BRRipple.h
                src/main/cpp/core/ripple/BRRippleAccount.c
                src/main/cpp/core/ripple/BRRippleAccount.h
                src/main/cpp/core/ripple/BRRippleAddress.c
                src/main/cpp/core/ripple/BRRippleAddress.h
                src/main/cpp/core/ripple/BRRippleBase58.c
                src/main/cpp/core/ripple/BRRippleBase58.h
                src/main/cpp/core/ripple/BRRippleFeeBasis.c
                src/main/cpp/core/ripple/BRRippleFeeBasis.h
                src/main/cpp/core/ripple/BRRippleSerialize.c
                src/main/cpp/core/ripple/BRRippleSerialize.h
                src/main/cpp/core/ripple/BRRippleSignature.c
                src/main/cpp/core/ripple/BRRippleSignature.h
                src/main/cpp/core/ripple/BRRippleTransaction.c
                src/main/cpp/core/ripple/BRRippleTransaction.h
                src/main/cpp/core/ripple/BRRippleTransfer.c
                src/main/cpp/core/ripple/BRRippleTransfer.h
                src/main/cpp/core/ripple/BRRippleUtils.c
                src/main/cpp/core/ripple/BRRippleUtils.h
                src/main/cpp/core/ripple/BRRippleWallet.c
                src/main/cpp/core/ripple/BRRippleWallet.h)

# Ripple Tests
if (CMAKE_BUILD_TYPE MATCHES Debug)
    target_sources (corecrypto
                    PRIVATE
                    src/main/cpp/core/ripple/testRipple.c
                    src/main/cpp/core/ripple/testRippleTxList1.h
                    src/main/cpp/core/ripple/testRippleTxList2.h)
endif(CMAKE_BUILD_TYPE MATCHES Debug)

# Generic
target_sources (corecrypto
                PRIVATE
                src/main/cpp/core/generic/BRGenericBase.h
                src/main/cpp/core/generic/BRGeneric.c
                src/main/cpp/core/generic/BRGeneric.h
                src/main/cpp/core/generic/BRGenericClient.c
                src/main/cpp/core/generic/BRGenericClient.h
                src/main/cpp/core/generic/BRGenericHandlers.c
                src/main/cpp/core/generic/BRGenericHandlers.h
                src/main/cpp/core/generic/BRGenericManager.c
                src/main/cpp/core/generic/BRGenericRipple.h
                src/main/cpp/core/generic/BRGenericRipple.c)

# Crypto
target_sources (corecrypto
                PRIVATE
                #  Dependency Order
                src/main/cpp/core/include/BRCryptoBase.h
                src/main/cpp/core/crypto/BRCryptoBaseP.h

                src/main/cpp/core/include/BRCryptoStatus.h
                src/main/cpp/core/crypto/BRCryptoStatusP.h
                src/main/cpp/core/crypto/BRCryptoStatus.c

                src/main/cpp/core/include/BRCryptoHash.h
                src/main/cpp/core/crypto/BRCryptoHashP.h
                src/main/cpp/core/crypto/BRCryptoHash.c

                src/main/cpp/core/include/BRCryptoCurrency.h
                src/main/cpp/core/crypto/BRCryptoCurrency.c

                src/main/cpp/core/include/BRCryptoUnit.h
                src/main/cpp/core/crypto/BRCryptoUnit.c

                src/main/cpp/core/include/BRCryptoAmount.h
                src/main/cpp/core/crypto/BRCryptoAmountP.h
                src/main/cpp/core/crypto/BRCryptoAmount.c

                src/main/cpp/core/include/BRCryptoKey.h
                src/main/cpp/core/crypto/BRCryptoKeyP.h
                src/main/cpp/core/crypto/BRCryptoKey.c

                src/main/cpp/core/include/BRCryptoAccount.h
                src/main/cpp/core/crypto/BRCryptoAccountP.h
                src/main/cpp/core/crypto/BRCryptoAccount.c

                src/main/cpp/core/include/BRCryptoFeeBasis.h
                src/main/cpp/core/crypto/BRCryptoFeeBasisP.h
                src/main/cpp/core/crypto/BRCryptoFeeBasis.c

                src/main/cpp/core/include/BRCryptoNetwork.h
                src/main/cpp/core/crypto/BRCryptoNetworkP.h
                src/main/cpp/core/crypto/BRCryptoNetwork.c

                src/main/cpp/core/include/BRCryptoAddress.h
                src/main/cpp/core/crypto/BRCryptoAddressP.h
                src/main/cpp/core/crypto/BRCryptoAddress.c

                src/main/cpp/core/include/BRCryptoPeer.h
                src/main/cpp/core/crypto/BRCryptoPeer.c

                src/main/cpp/core/include/BRCryptoTransfer.h
                src/main/cpp/core/crypto/BRCryptoTransferP.h
                src/main/cpp/core/crypto/BRCryptoTransfer.c

                src/main/cpp/core/include/BRCryptoPayment.h
                src/main/cpp/core/crypto/BRCryptoPaymentP.h
                src/main/cpp/core/crypto/BRCryptoPayment.c

                src/main/cpp/core/include/BRCryptoWallet.h
                src/main/cpp/core/crypto/BRCryptoWalletP.h
                src/main/cpp/core/crypto/BRCryptoWallet.c

                src/main/cpp/core/include/BRCryptoSync.h

                src/main/cpp/core/include/BRCryptoWalletManagerClient.h
                src/main/cpp/core/crypto/BRCryptoWalletManagerClient.c

                src/main/cpp/core/include/BRCryptoWalletManager.h
                src/main/cpp/core/crypto/BRCryptoWalletManagerP.h
                src/main/cpp/core/crypto/BRCryptoWalletManager.c

                src/main/cpp/core/include/BRCryptoCipher.h
                src/main/cpp/core/crypto/BRCryptoCipher.c

                src/main/cpp/core/include/BRCryptoCoder.h
                src/main/cpp/core/crypto/BRCryptoCoder.c

                src/main/cpp/core/include/BRCryptoHasher.h
                src/main/cpp/core/crypto/BRCryptoHasher.c

                src/main/cpp/core/include/BRCryptoSigner.h
                src/main/cpp/core/crypto/BRCryptoSigner.c
                )

# Crypto Tests
if (CMAKE_BUILD_TYPE MATCHES Debug)
    target_sources (corecrypto
                    PRIVATE
                    src/main/cpp/core/crypto/testCrypto.c)
endif(CMAKE_BUILD_TYPE MATCHES Debug)

# Setup all the include paths
target_include_directories (corecrypto
                            PUBLIC
                            ${PROJECT_SOURCE_DIR}/src/main/cpp/core
                            ${PROJECT_SOURCE_DIR}/src/main/cpp/core/include
                            ${PROJECT_SOURCE_DIR}/src/main/cpp/core/support
                            ${PROJECT_SOURCE_DIR}/src/main/cpp/core/bitcoin
                            ${PROJECT_SOURCE_DIR}/src/main/cpp/core/bcash
                            ${PROJECT_SOURCE_DIR}/src/main/cpp/core/ethereum
                            ${PROJECT_SOURCE_DIR}/src/main/cpp/core/vendor/secp256k1
                            ${PROJECT_SOURCE_DIR}/src/main/cpp/core/crypto)

# Set the DEBUG define when building DEBUG builds
if (CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_definitions(corecrypto
                               PUBLIC
                               "DEBUG"
                               "BITCOIN_DEBUG")
endif (CMAKE_BUILD_TYPE MATCHES Debug)


# Link in the static sqlite3 library
target_link_libraries (corecrypto
                       sqlite3)

# On Android, link in the log library
if (ANDROID)
    find_library (log-lib
                  log)
    target_link_libraries (corecrypto
                           ${log-lib})
endif(ANDROID)

# On Apple (iOS/macOS), link in the resolv library and Security framework
if (APPLE)
    find_library (log-lib
                  log)
    target_link_libraries (corecrypto
                           "resolv"
                           "-framework Security")
endif(APPLE)
