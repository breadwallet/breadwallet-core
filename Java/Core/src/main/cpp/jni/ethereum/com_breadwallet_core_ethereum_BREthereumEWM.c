//  Created by Ed Gamble on 3/7/2018
//  Copyright (c) 2018 Breadwinner AG.  All right reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include "../BRCoreJni.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "support/BRBIP39Mnemonic.h"
#include "support/BRKey.h"
#include "ethereum/ewm/BREthereumAmount.h"
#include "ethereum/base/BREthereumHash.h"
#include "ethereum/base/BREthereumData.h"
#include "ethereum/BREthereum.h"
#include "com_breadwallet_core_ethereum_BREthereumEWM.h"

static BREthereumWallet
getWallet (JNIEnv *env,
           jlong wid) {
    return (BREthereumWallet) wid;
}

static BREthereumTransfer
getTransfer (JNIEnv *env,
             jlong tid) {
    return (BREthereumTransfer) tid;
}

//
// Forward Declarations - Client Interface
//
static void
clientGetGasPrice(BREthereumClientContext context,
                  BREthereumEWM node,
                  BREthereumWallet wid,
                  int id);

static void
clientEstimateGas(BREthereumClientContext context,
                  BREthereumEWM node,
                  BREthereumWallet wid,
                  BREthereumCookie cookie,
                  const char *from,
                  const char *to,
                  const char *amount,
                  const char *gasPrice,
                  const char *data,
                  int id);

static void
clientGetBalance(BREthereumClientContext context,
                 BREthereumEWM node,
                 BREthereumWallet wid,
                 const char *address,
                 int id);

static void
clientSubmitTransaction(BREthereumClientContext context,
                        BREthereumEWM node,
                        BREthereumWallet wid,
                        BREthereumTransfer tid,
                        const char *transaction,
                        int id);

static void
clientGetTransactions(BREthereumClientContext context,
                      BREthereumEWM node,
                      const char *account,
                      uint64_t begBlockNumber,
                      uint64_t endBlockNumber,
                      int id);

static void
clientGetLogs(BREthereumClientContext context,
              BREthereumEWM node,
              const char *contract,
              const char *address,
              const char *event,
              uint64_t begBlockNumber,
              uint64_t endBlockNumber,
              int rid);

static void
clientGetTokens (BREthereumClientContext context,
                 BREthereumEWM ewm,
                 int rid);

static void
clientGetBlockNumber(BREthereumClientContext context,
                     BREthereumEWM node,
                     int id);

static void
clientGetNonce(BREthereumClientContext context,
               BREthereumEWM node,
               const char *address,
               int id);

static void
clientGetBlocks (BREthereumClientContext context,
                 BREthereumEWM ewm,
                 const char *address,
                 BREthereumSyncInterestSet interests,
                 uint64_t blockNumberStart,
                 uint64_t blockNumberStop,
                 int rid);

static void
clientEWMEventHandler (BREthereumClientContext context,
                       BREthereumEWM ewm,
                       BREthereumEWMEvent event,
                       BREthereumStatus status,
                       const char *errorDescription);

static void
clientPeerEventHandler (BREthereumClientContext context,
                        BREthereumEWM ewm,
                        BREthereumPeerEvent event,
                        BREthereumStatus status,
                        const char *errorDescription);

static void
clientWalletEventHandler(BREthereumClientContext context,
                         BREthereumEWM node,
                         BREthereumWallet wid,
                         BREthereumWalletEvent event,
                         BREthereumStatus status,
                         const char *errorDescription);

static void
clientTokenEventHandler(BREthereumClientContext context,
                        BREthereumEWM ewm,
                        BREthereumToken token,
                        BREthereumTokenEvent event);

#if 0
static void
clientBlockEventHandler(BREthereumClientContext context,
                        BREthereumEWM node,
                        BREthereumBlock bid,
                        BREthereumBlockEvent event,
                        BREthereumStatus status,
                        const char *errorDescription);
#endif
static void
clientTransferEventHandler(BREthereumClientContext context,
                           BREthereumEWM node,
                           BREthereumWallet wid,
                           BREthereumTransfer tid,
                           BREthereumTransferEvent event,
                           BREthereumStatus status,
                           const char *errorDescription);

static jstring
asJniString(JNIEnv *env, char *string) {
    jstring result = (*env)->NewStringUTF(env, string);
    free(string);
    return result;
}

//
// Trampoline (C -> Java) Methods
//
static jclass trampolineClass = NULL;
static jmethodID trampolineGetGasPrice = NULL;
static jmethodID trampolineGetGasEstimate = NULL;
static jmethodID trampolineGetBalance = NULL;
static jmethodID trampolineSubmitTransaction = NULL;
static jmethodID trampolineGetTransactions = NULL;
static jmethodID trampolineGetLogs = NULL;
static jmethodID trampolineGetBlocks = NULL;
static jmethodID trampolineGetTokens = NULL;
static jmethodID trampolineGetBlockNumber = NULL;
static jmethodID trampolineGetNonce = NULL;
static jmethodID trampolineEWMEvent = NULL;
static jmethodID trampolinePeerEvent = NULL;
static jmethodID trampolineWalletEvent = NULL;
static jmethodID trampolineTokenEvent = NULL;
//static jmethodID trampolineBlockEvent = NULL;
static jmethodID trampolineTransferEvent = NULL;

static jmethodID
trampolineOrFatal (JNIEnv *env, const char *name, const char *signature) {
    jmethodID method = (*env)->GetStaticMethodID (env, trampolineClass, name, signature);
    assert (NULL != method);
    return method;
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    initializeNative
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_initializeNative
        (JNIEnv *env, jclass thisClass) {
    if (NULL != trampolineClass) return;
    trampolineClass = (*env)->NewGlobalRef(env, thisClass);

    trampolineGetGasPrice       = trampolineOrFatal (env, "trampolineGetGasPrice",       "(JJI)V");
    trampolineGetGasEstimate    = trampolineOrFatal (env, "trampolineGetGasEstimate",    "(JJJLjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V");
    trampolineGetBalance        = trampolineOrFatal (env, "trampolineGetBalance",        "(JJLjava/lang/String;I)V");
    trampolineSubmitTransaction = trampolineOrFatal (env, "trampolineSubmitTransaction", "(JJJLjava/lang/String;I)V");
    trampolineGetTransactions   = trampolineOrFatal (env, "trampolineGetTransactions",   "(JLjava/lang/String;JJI)V");
    trampolineGetLogs           = trampolineOrFatal (env, "trampolineGetLogs",           "(JLjava/lang/String;Ljava/lang/String;Ljava/lang/String;JJI)V");
    trampolineGetBlocks         = trampolineOrFatal (env, "trampolineGetBlocks",         "(JLjava/lang/String;IJJI)V");
    trampolineGetTokens         = trampolineOrFatal (env, "trampolineGetTokens",         "(JI)V");
    trampolineGetBlockNumber    = trampolineOrFatal (env, "trampolineGetBlockNumber",    "(JI)V");
    trampolineGetNonce          = trampolineOrFatal (env, "trampolineGetNonce",          "(JLjava/lang/String;I)V");
    trampolineEWMEvent          = trampolineOrFatal (env, "trampolineEWMEvent",          "(JIILjava/lang/String;)V");
    trampolinePeerEvent         = trampolineOrFatal (env, "trampolinePeerEvent",         "(JIILjava/lang/String;)V");
    trampolineWalletEvent       = trampolineOrFatal (env, "trampolineWalletEvent",       "(JJIILjava/lang/String;)V");
    trampolineTokenEvent        = trampolineOrFatal (env, "trampolineTokenEvent",        "(JJI)V");
//    trampolineBlockEvent        = trampolineOrFatal (env, "trampolineBlockEvent",        "(JIIILjava/lang/String;)V");
    trampolineTransferEvent     = trampolineOrFatal (env, "trampolineTransferEvent",     "(JJJIILjava/lang/String;)V");
}

static BRSetOf(BREthereumHashDataPair)
mapToHashDataPair (JNIEnv *env, jobject arrayObject) {
    if ((*env)->IsSameObject(env, arrayObject, NULL)) { return NULL; }

    size_t pairsCount = (*env)->GetArrayLength (env, arrayObject);

    BRSetOf(BREthereumHashDataPair) pairs = hashDataPairSetCreateEmpty (pairsCount);

    for (size_t index = 0; index < pairsCount; index++) {
        jobject item = (*env)->GetObjectArrayElement (env, arrayObject, index);
        jstring itemHash = (*env)->GetObjectArrayElement (env, item, 0);
        jstring itemData = (*env)->GetObjectArrayElement (env, item, 1);

        const char *stringHash = (*env)->GetStringUTFChars (env, itemHash, 0);
        const char *stringData = (*env)->GetStringUTFChars (env, itemData, 0);

        if ((2*ETHEREUM_HASH_BYTES) != strlen (stringHash)) {
            assert (0);
            (*env)->ReleaseStringUTFChars (env, itemHash, stringHash);
            (*env)->ReleaseStringUTFChars (env, itemData, stringData);
            (*env)->DeleteLocalRef (env, itemData);
            (*env)->DeleteLocalRef (env, itemHash);
            (*env)->DeleteLocalRef (env, item);
            return NULL;
        }

        BREthereumHash hash;
        BREthereumData data;

        decodeHex(hash.bytes, ETHEREUM_HASH_BYTES, stringHash, 2 * ETHEREUM_HASH_BYTES);
        data.bytes = decodeHexCreate(&data.count, stringData, strlen(stringData));

        (*env)->ReleaseStringUTFChars (env, itemHash, stringHash);
        (*env)->ReleaseStringUTFChars (env, itemData, stringData);
        (*env)->DeleteLocalRef (env, itemData);
        (*env)->DeleteLocalRef (env, itemHash);
        (*env)->DeleteLocalRef (env, item);

        BREthereumHashDataPair pair = hashDataPairCreate(hash, data);
        BRSetAdd (pairs, pair);
    }
    return pairs;
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniCreateEWM
 * Signature: (Lcom/breadwallet/core/ethereum/BREthereumEWM/Client;IJLjava/lang/String;Ljava/lang/String;[Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniCreateEWM
        (JNIEnv *env, jclass thisClass,
         jobject clientObject,
         jint mode,
         jlong network,
         jstring storagePathString,
         jstring paperKeyString,
         jobjectArray wordsArrayObject) {

    // Install the wordList
    int wordsCount = (*env)->GetArrayLength(env, wordsArrayObject);
    assert (BIP39_WORDLIST_COUNT == wordsCount);
    char *wordList[wordsCount];

    for (int i = 0; i < wordsCount; i++) {
        jstring string = (jstring) (*env)->GetObjectArrayElement(env, wordsArrayObject, i);
        const char *rawString = (*env)->GetStringUTFChars(env, string, 0);

        wordList[i] = strdup (rawString);

        (*env)->ReleaseStringUTFChars(env, string, rawString);
        (*env)->DeleteLocalRef(env, string);
    }

    installSharedWordList((const char **) wordList, BIP39_WORDLIST_COUNT);

    const char *paperKey    = (*env)->GetStringUTFChars (env, paperKeyString, 0);
    const char *storagePath = (*env)->GetStringUTFChars (env, storagePathString, 0);

    BREthereumClient client = {
            NULL,
            clientGetBalance,
            clientGetGasPrice,
            clientEstimateGas,
            clientSubmitTransaction,
            clientGetTransactions,
            clientGetLogs,
            clientGetBlocks,
            clientGetTokens,
            clientGetBlockNumber,
            clientGetNonce,

            clientEWMEventHandler,
            clientPeerEventHandler,
            clientWalletEventHandler,
            clientTokenEventHandler,
            clientTransferEventHandler
    };

    BREthereumEWM node = ewmCreateWithPaperKey((BREthereumNetwork) network,
                                        paperKey,
                                        ETHEREUM_TIMESTAMP_UNKNOWN,
                                        (BREthereumMode) mode,
                                        client,
                                        storagePath,
                                        0);

    (*env)->ReleaseStringUTFChars (env, paperKeyString,    paperKey);
    (*env)->ReleaseStringUTFChars (env, storagePathString, storagePath);
    return (jlong) node;
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniCreateEWM_PublicKey
 * Signature: (Lcom/breadwallet/core/ethereum/BREthereumEWM/Client;IJLjava/lang/String;[B)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniCreateEWM_1PublicKey
        (JNIEnv *env, jclass thisClass,
         jobject clientObject,
         jint mode,
         jlong network,
         jstring storagePathString,
         jbyteArray publicKey) {

    assert (65 == (*env)->GetArrayLength(env, publicKey));

    jbyte *publicKeyBytes = (*env)->GetByteArrayElements(env, publicKey, 0);
    BRKey key;

    memcpy (key.pubKey, publicKeyBytes, 65);

    BREthereumClient client = {
            NULL,
            clientGetBalance,
            clientGetGasPrice,
            clientEstimateGas,
            clientSubmitTransaction,
            clientGetTransactions,
            clientGetLogs,
            clientGetBlocks,
            clientGetTokens,
            clientGetBlockNumber,
            clientGetNonce,

            clientEWMEventHandler,
            clientPeerEventHandler,
            clientWalletEventHandler,
            clientTokenEventHandler,
//            clientBlockEventHandler,
            clientTransferEventHandler
    };

    const char *storagePath = (*env)->GetStringUTFChars (env, storagePathString, 0);

    BREthereumEWM node = ewmCreateWithPublicKey((BREthereumNetwork) network,
                                                     key,
                                                     ETHEREUM_TIMESTAMP_UNKNOWN,
                                                    (BREthereumMode) mode,
                                                     client,
                                                     storagePath,
                                                     0);


    (*env)->ReleaseByteArrayElements(env, publicKey, publicKeyBytes, 0);
    (*env)->ReleaseStringUTFChars (env, storagePathString, storagePath);
    return (jlong) node;
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniAddressIsValid
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_com_breadwallet_core_ethereum_BREthereumEWM_jniAddressIsValid
        (JNIEnv *env, jclass thisClass, jstring addressObject) {

    const char *address = (*env)->GetStringUTFChars(env, addressObject, 0);

    jboolean result = ETHEREUM_BOOLEAN_IS_TRUE (addressValidateString(address))
                      ? JNI_TRUE
                      : JNI_FALSE;

    (*env)->ReleaseStringUTFChars(env, addressObject, address);
    return result;
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniEWMGetAccount
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniEWMGetAccount
    (JNIEnv *env, jobject thisObject) {
  BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    return (jlong) ewmGetAccount(node);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniEWMGetWallet
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniEWMGetWallet
        (JNIEnv *env, jobject thisObject) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    return (jlong) ewmGetWallet(node);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniEWMGetWalletToken
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniEWMGetWalletToken
        (JNIEnv *env, jobject thisObject, jlong tokenId) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    return (jlong) ewmGetWalletHoldingToken(node, (BREthereumToken) tokenId);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniEWMWalletGetToken
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_com_breadwallet_core_ethereum_BREthereumEWM_jniEWMWalletGetToken
        (JNIEnv *env, jobject thisObject, jlong wid) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    return (jlong) ewmWalletGetToken(node, (BREthereumWallet) wid);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniGetAccountPrimaryAddress
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniGetAccountPrimaryAddress
        (JNIEnv *env, jobject thisObject, jlong account) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);

    char *addressChars = ewmGetAccountPrimaryAddress(node);
    jstring addressObject = (*env)->NewStringUTF(env, addressChars);
    free(addressChars);

    return addressObject;
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniGetAccountPrimaryAddressPublicKey
 * Signature: (J)[B
 */
JNIEXPORT jbyteArray JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniGetAccountPrimaryAddressPublicKey
        (JNIEnv *env, jobject thisObject, jlong account) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);

    BRKey key = ewmGetAccountPrimaryAddressPublicKey(node);
    jbyteArray publicKey = (*env)->NewByteArray (env, 65);
    (*env)->SetByteArrayRegion (env, publicKey, 0, 65, (const jbyte *) key.pubKey);

    return publicKey;
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniGetAccountPrimaryAddressPrivateKey
 * Signature: (JLjava/lang/String;)[B
 */
JNIEXPORT jbyteArray JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniGetAccountPrimaryAddressPrivateKey
        (JNIEnv *env, jobject thisObject,
         jlong account,
         jstring paperKeyString) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);

    const char *paperKey = (*env)->GetStringUTFChars(env, paperKeyString, 0);
    BRKey key = ewmGetAccountPrimaryAddressPrivateKey(node, paperKey);
    (*env)->ReleaseStringUTFChars(env, paperKeyString, paperKey);

    jbyteArray privateKey = (*env)->NewByteArray(env, sizeof(BRKey));
    (*env)->SetByteArrayRegion(env, privateKey, 0, sizeof(BRKey), (const jbyte *) &key);

    return privateKey;
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniGetWalletBalance
 * Signature: (JJ)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniGetWalletBalance
        (JNIEnv *env, jobject thisObject, jlong wid, jlong unit) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    BREthereumWallet wallet = getWallet (env, wid);
    BREthereumAmount balance = ewmWalletGetBalance(node, wallet);

    char *number = (AMOUNT_ETHER == amountGetType(balance)
                    ? etherGetValueString(balance.u.ether, unit)
                    : tokenQuantityGetValueString(balance.u.tokenQuantity, unit));

    jstring result = (*env)->NewStringUTF(env, number);
    free(number);
    return result;
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniEstimateWalletGasPrice
 * Signature: (J)V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniEstimateWalletGasPrice
        (JNIEnv *env, jobject thisObject, jlong wid) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    BREthereumWallet wallet = getWallet (env, wid);

    ewmUpdateGasPrice (node, wallet);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniWalletGetDefaultGasPrice
 * Signature: (J)J
 */
JNIEXPORT jlong
JNICALL Java_com_breadwallet_core_ethereum_BREthereumEWM_jniWalletGetDefaultGasPrice
        (JNIEnv *env, jobject thisObject,
         jlong wid) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    BREthereumWallet wallet = getWallet (env, wid);
    BREthereumGasPrice price = ewmWalletGetDefaultGasPrice(node, wallet);

    return price.etherPerGas.valueInWEI.u64[0];

}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniWalletSetDefaultGasPrice
 * Signature: (JJ)V
 */
JNIEXPORT void
JNICALL Java_com_breadwallet_core_ethereum_BREthereumEWM_jniWalletSetDefaultGasPrice
        (JNIEnv *env, jobject thisObject,
         jlong wid,
         jlong value) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    BREthereumWallet wallet = getWallet (env, wid);
    BREthereumGasPrice price = gasPriceCreate (etherCreateNumber(value, WEI));

    ewmWalletSetDefaultGasPrice(node, wallet, price);

}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniWalletGetDefaultGasLimit
 * Signature: (J)J
 */
JNIEXPORT jlong
JNICALL Java_com_breadwallet_core_ethereum_BREthereumEWM_jniWalletGetDefaultGasLimit
        (JNIEnv *env, jobject thisObject,
         jlong wid) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    BREthereumWallet wallet = getWallet (env, wid);
    BREthereumGas limit = ewmWalletGetDefaultGasLimit (node, wallet);
    return limit.amountOfGas;

}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniWalletSetDefaultGasLimit
 * Signature: (JJ)V
 */
JNIEXPORT void
JNICALL Java_com_breadwallet_core_ethereum_BREthereumEWM_jniWalletSetDefaultGasLimit
        (JNIEnv *env, jobject thisObject,
         jlong wid,
         jlong value) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    BREthereumWallet wallet = getWallet (env, wid);
    BREthereumGas limit = gasCreate(value);

    ewmWalletSetDefaultGasLimit(node, wallet, limit);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniAnnounceTransaction
 * Signature: (ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniAnnounceTransaction
        (JNIEnv *env, jobject thisObject,
         jint id,
         jstring hashObject,
         jstring toObject,
         jstring fromObject,
         jstring contractObject,
         jstring amountObject,
         jstring gasLimitObject,
         jstring gasPriceObject,
         jstring dataObject,
         jstring nonceObject,
         jstring gasUsedObject,
         jstring blockNumberObject,
         jstring blockHashObject,
         jstring blockConfirmationsObject,
         jstring blockTransactionIndexObject,
         jstring blockTimestampObject,
         jstring isErrorObject) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);

    const char *hash = (*env)->GetStringUTFChars(env, hashObject, 0);
    const char *to = (*env)->GetStringUTFChars(env, toObject, 0);
    const char *from = (*env)->GetStringUTFChars(env, fromObject, 0);
    const char *contract = (*env)->GetStringUTFChars(env, contractObject, 0);
    const char *amount = (*env)->GetStringUTFChars(env, amountObject, 0);
    const char *gasLimit = (*env)->GetStringUTFChars(env, gasLimitObject, 0);
    const char *gasPrice = (*env)->GetStringUTFChars(env, gasPriceObject, 0);
    const char *data = (*env)->GetStringUTFChars(env, dataObject, 0);
    const char *nonce = (*env)->GetStringUTFChars(env, nonceObject, 0);
    const char *gasUsed = (*env)->GetStringUTFChars(env, gasUsedObject, 0);
    const char *blockNumber = (*env)->GetStringUTFChars(env, blockNumberObject, 0);
    const char *blockHash = (*env)->GetStringUTFChars(env, blockHashObject, 0);
    const char *blockConfirmation = (*env)->GetStringUTFChars(env, blockConfirmationsObject, 0);
    const char *blockTransactionIndex = (*env)->GetStringUTFChars(env, blockTransactionIndexObject, 0);
    const char *blockTimestamp = (*env)->GetStringUTFChars(env, blockTimestampObject, 0);
    const char *isError = (*env)->GetStringUTFChars(env, isErrorObject, 0);

    ewmAnnounceTransaction(node, id,
                                 hash, to, from, contract,
                                 amount, gasLimit, gasPrice,
                                 data, nonce, gasUsed,
                                 blockNumber, blockHash, blockConfirmation, blockTransactionIndex,
                                 blockTimestamp,
                                 isError);

    (*env)->ReleaseStringUTFChars(env, hashObject, hash);
    (*env)->ReleaseStringUTFChars(env, toObject, to);
    (*env)->ReleaseStringUTFChars(env, fromObject, from);
    (*env)->ReleaseStringUTFChars(env, contractObject, contract);
    (*env)->ReleaseStringUTFChars(env, amountObject, amount);
    (*env)->ReleaseStringUTFChars(env, gasLimitObject, gasLimit);
    (*env)->ReleaseStringUTFChars(env, gasPriceObject, gasPrice);
    (*env)->ReleaseStringUTFChars(env, dataObject, data);
    (*env)->ReleaseStringUTFChars(env, nonceObject, nonce);
    (*env)->ReleaseStringUTFChars(env, gasUsedObject, gasUsed);
    (*env)->ReleaseStringUTFChars(env, blockNumberObject, blockNumber);
    (*env)->ReleaseStringUTFChars(env, blockHashObject, blockHash);
    (*env)->ReleaseStringUTFChars(env, blockConfirmationsObject, blockConfirmation);
    (*env)->ReleaseStringUTFChars(env, blockTransactionIndexObject, blockTransactionIndex);
    (*env)->ReleaseStringUTFChars(env, blockTimestampObject, blockTimestamp);
    (*env)->ReleaseStringUTFChars(env, isErrorObject, isError);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniAnnounceTransactionComplete
 * Signature: (IZ)V
 */
JNIEXPORT void JNICALL Java_com_breadwallet_core_ethereum_BREthereumEWM_jniAnnounceTransactionComplete
        (JNIEnv *env, jobject thisObject, jint id, jboolean success) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);

    ewmAnnounceTransactionComplete (node, id, AS_ETHEREUM_BOOLEAN(success));
}
/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniAnnounceLog
 * Signature: (ILjava/lang/String;Ljava/lang/String;[Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniAnnounceLog
        (JNIEnv *env, jobject thisObject,
         jint id,
         jstring hashObject,
         jstring contractObject,
         jobjectArray topicsArray,
         jstring dataObject,
         jstring gasPriceObject,
         jstring gasUsedObject,
         jstring logIndexObject,
         jstring blockNumberObject,
         jstring blockTransactionIndexObject,
         jstring blockTimestampObject) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);

    size_t topicsCount = (size_t) (*env)->GetArrayLength(env, topicsArray);
    const char *topics[topicsCount];

    for (int i = 0; i < topicsCount; i++) {
        jstring topic = (*env)->GetObjectArrayElement(env, topicsArray, i);
        topics[i] = (*env)->GetStringUTFChars(env, topic, 0);
        (*env)->DeleteLocalRef(env, topic);
    }

    const char *hash = (*env)->GetStringUTFChars(env, hashObject, 0);
    const char *contract = (*env)->GetStringUTFChars(env, contractObject, 0);
    const char *data = (*env)->GetStringUTFChars(env, dataObject, 0);
    const char *gasPrice = (*env)->GetStringUTFChars(env, gasPriceObject, 0);
    const char *gasUsed = (*env)->GetStringUTFChars(env, gasUsedObject, 0);
    const char *logIndex = (*env)->GetStringUTFChars(env, logIndexObject, 0);
    const char *blockNumber = (*env)->GetStringUTFChars(env, blockNumberObject, 0);
    const char *blockTransactionIndex = (*env)->GetStringUTFChars(env, blockTransactionIndexObject, 0);
    const char *blockTimestamp = (*env)->GetStringUTFChars(env, blockTimestampObject, 0);

    ewmAnnounceLog(node, id,
                         hash, contract,
                         topicsCount,
                         topics,
                         data, gasPrice, gasUsed,
                         logIndex,
                         blockNumber, blockTransactionIndex, blockTimestamp);

    (*env)->ReleaseStringUTFChars(env, hashObject, hash);
    (*env)->ReleaseStringUTFChars(env, contractObject, contract);
    (*env)->ReleaseStringUTFChars(env, dataObject, data);
    (*env)->ReleaseStringUTFChars(env, gasPriceObject, gasPrice);
    (*env)->ReleaseStringUTFChars(env, gasUsedObject, gasUsed);
    (*env)->ReleaseStringUTFChars(env, logIndexObject, logIndex);
    (*env)->ReleaseStringUTFChars(env, blockNumberObject, blockNumber);
    (*env)->ReleaseStringUTFChars(env, blockTransactionIndexObject, blockTransactionIndex);
    (*env)->ReleaseStringUTFChars(env, blockTimestampObject, blockTimestamp);

    for (int i = 0; i < topicsCount; i++) {
        jstring topic = (*env)->GetObjectArrayElement(env, topicsArray, i);
        (*env)->ReleaseStringUTFChars(env, topic, topics[i]);
        (*env)->DeleteLocalRef(env, topic);
    }
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniAnnounceLogComplete
 * Signature: (IZ)V
 */
JNIEXPORT void JNICALL Java_com_breadwallet_core_ethereum_BREthereumEWM_jniAnnounceLogComplete
        (JNIEnv *env, jobject thisObject, jint id, jboolean success) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);

    ewmAnnounceLogComplete (node, id, AS_ETHEREUM_BOOLEAN(success));
}


/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniAnnounceBalance
 * Signature: (JLjava/lang/String;I)V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniAnnounceBalance
        (JNIEnv *env, jobject thisObject,
         jlong wid,
         jstring balanceString,
         jint rid) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    BREthereumWallet wallet = getWallet (env, wid);

    const char *balance = (*env)->GetStringUTFChars(env, balanceString, 0);
    ewmAnnounceWalletBalance(node, wallet, balance, rid);

    (*env)->ReleaseStringUTFChars (env, balanceString, balance);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniAnnounceGasPrice
 * Signature: (JLjava/lang/String;I)V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniAnnounceGasPrice
        (JNIEnv *env, jobject thisObject,
         jlong wid,
         jstring gasPrice,
         jint rid) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    const char *strGasPrice = (*env)->GetStringUTFChars (env, gasPrice, 0);
    ewmAnnounceGasPrice(node,
                              (BREthereumWallet) wid,
                              strGasPrice,
                              rid);
    (*env)->ReleaseStringUTFChars (env, gasPrice, strGasPrice);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniAnnounceGasEstimate
 * Signature: (JJLjava/lang/String;I)V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniAnnounceGasEstimate
        (JNIEnv *env, jobject thisObject,
         jlong wid,
         jlong tid,
         jstring gasEstimate,
         jint rid) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    BREthereumWallet wallet = getWallet (env, wid);
    BREthereumTransfer transfer = getTransfer (env, tid);
    const char *strGasEstimate = (*env)->GetStringUTFChars(env, gasEstimate, 0);
    // ewmAnnounceGasEstimate(node,
    //                              wallet,
    //                              transfer,
    //                              strGasEstimate,
    //                              rid);
    (*env)->ReleaseStringUTFChars(env, gasEstimate, strGasEstimate);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniAnnounceSubmitTransaction
 * Signature: (JJLjava/lang/String;ILjava/lang/String;I)V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniAnnounceSubmitTransaction
        (JNIEnv *env, jobject thisObject,
         jlong wid,
         jlong tid,
         jstring hash,
         jint errorCode,
         jstring errorMessage,
         jint rid) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    BREthereumWallet wallet = getWallet(env, wid);
    BREthereumTransfer transfer = getTransfer(env, tid);
    const char *hashStr = ((*env)->IsSameObject(env, hash, NULL)
                           ? NULL
                           : (*env)->GetStringUTFChars(env, hash, 0));
    const char *errStr = ((*env)->IsSameObject(env, errorMessage, NULL)
                          ? NULL
                          : (*env)->GetStringUTFChars(env, errorMessage, 0));
    ewmAnnounceSubmitTransfer(node, wallet, transfer, hashStr, errorCode, errStr, rid);
    if (NULL != hashStr) (*env)->ReleaseStringUTFChars(env, errorMessage, errStr);
    if (NULL != errStr)  (*env)->ReleaseStringUTFChars(env, hash, hashStr);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniAnnounceBlockNumber
 * Signature: (Ljava/lang/String;I)V
 */
JNIEXPORT void JNICALL Java_com_breadwallet_core_ethereum_BREthereumEWM_jniAnnounceBlockNumber
        (JNIEnv *env, jobject thisObject,
         jstring blockNumber,
         jint rid) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    const char *strBlockNumber = (*env)->GetStringUTFChars(env, blockNumber, 0);
    ewmAnnounceBlockNumber(node, strBlockNumber, rid);
    (*env)->ReleaseStringUTFChars(env, blockNumber, strBlockNumber);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniAnnounceNonce
 * Signature: (Ljava/lang/String;Ljava/lang/String;I)V
 */
JNIEXPORT void JNICALL Java_com_breadwallet_core_ethereum_BREthereumEWM_jniAnnounceNonce
        (JNIEnv *env, jobject thisObject,
         jstring address,
         jstring nonce,
         jint rid) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    const char *strAddress = (*env)->GetStringUTFChars(env, address, 0);
    const char *strNonce = (*env)->GetStringUTFChars(env, nonce, 0);
    ewmAnnounceNonce(node, strAddress, strNonce, rid);
    (*env)->ReleaseStringUTFChars(env, address, strAddress);
    (*env)->ReleaseStringUTFChars(env, nonce, strNonce);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniAnnounceToken
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;I)V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniAnnounceToken
        (JNIEnv *env, jobject thisObject,
         jstring address,
         jstring symbol,
         jstring name,
         jstring description,
         jint decimals,
         jstring defaultGasLimit,
         jstring defaultGasPrice,
         jint rid) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);

    const char *strAddress  = (*env)->GetStringUTFChars (env, address, 0);
    const char *strSymbol   = (*env)->GetStringUTFChars (env, symbol, 0);
    const char *strName     = (*env)->GetStringUTFChars (env, name, 0);
    const char *strDescription = (*env)->GetStringUTFChars (env, description, 0);
    const char *strGasLimit = (*env)->IsSameObject(env, defaultGasLimit, NULL)
                              ? NULL
                              : (*env)->GetStringUTFChars (env, defaultGasLimit, 0);
    const char *strGasPrice = (*env)->IsSameObject(env, defaultGasPrice, NULL)
                              ? NULL
                              : (*env)->GetStringUTFChars (env, defaultGasPrice, 0);

    ewmAnnounceToken(node,
                     rid,
                     strAddress, strSymbol, strName, strDescription,
                     decimals, strGasLimit, strGasPrice);

    (*env)->ReleaseStringUTFChars (env, address, strAddress);
    (*env)->ReleaseStringUTFChars (env, symbol, strSymbol);
    (*env)->ReleaseStringUTFChars (env, name, strName);
    (*env)->ReleaseStringUTFChars (env, description, strDescription);
    if (!(*env)->IsSameObject(env, defaultGasLimit, NULL))
        (*env)->ReleaseStringUTFChars (env, defaultGasLimit, strGasLimit);
    if (!(*env)->IsSameObject(env, defaultGasPrice, NULL))
        (*env)->ReleaseStringUTFChars (env, defaultGasPrice, strGasPrice);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniAnnounceTokenComplete
 * Signature: (IZ)V
 */
JNIEXPORT void JNICALL Java_com_breadwallet_core_ethereum_BREthereumEWM_jniAnnounceTokenComplete
        (JNIEnv *env, jobject thisObject,
         jint rid,
         jboolean success) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);

    ewmAnnounceTokenComplete(node, rid, AS_ETHEREUM_BOOLEAN(success));
}


/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniCreateTransaction
 * Signature: (JLjava/lang/String;Ljava/lang/String;J)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniCreateTransaction
        (JNIEnv *env, jobject thisObject,
         jlong wid,
         jstring toObject,
         jstring amountObject,
         jlong amountUnit) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    BREthereumWallet wallet = getWallet (env, wid);
    BREthereumToken token = ewmWalletGetToken(node, wallet);

    // Get an actual Amount
    BRCoreParseStatus status = CORE_PARSE_OK;
    const char *amountChars = (*env)->GetStringUTFChars(env, amountObject, 0);
    BREthereumAmount amount = NULL == token
                              ? amountCreateEtherString(amountChars, amountUnit, &status)
                              : amountCreateTokenQuantityString(token, amountChars, amountUnit,
                                                                &status);
    (*env)->ReleaseStringUTFChars (env, amountObject, amountChars);

    const char *to = (*env)->GetStringUTFChars(env, toObject, 0);
    BREthereumTransfer tid = ewmWalletCreateTransfer(node, wallet, to, amount);
    (*env)->ReleaseStringUTFChars(env, toObject, to);
    return (jlong) tid;
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniCreateTransactionGeneric
 * Signature: (JLjava/lang/String;Ljava/lang/String;JLjava/lang/String;JLjava/lang/String;Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniCreateTransactionGeneric
        (JNIEnv *env, jobject thisObject,
         jlong wid,
         jstring toObject,
         jstring amountObject,
         jlong amountUnit,
         jstring gasPriceObject,
         jlong gasPriceUnit,
         jstring gasLimitObject,
         jstring dataObject) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    BRCoreParseStatus status = CORE_PARSE_OK;

    const char *to = (*env)->GetStringUTFChars(env, toObject, 0);
    const char *data = (*env)->GetStringUTFChars(env, dataObject, 0);

    // Get an actual Amount
    const char *amountChars = (*env)->GetStringUTFChars(env, amountObject, 0);
    BREthereumEther amount = etherCreateString(amountChars, amountUnit, &status);
    (*env)->ReleaseStringUTFChars(env, amountObject, amountChars);

    const char *gasPriceChars = (*env)->GetStringUTFChars(env, gasPriceObject, 0);
    BREthereumGasPrice gasPrice = gasPriceCreate(
            etherCreateString(gasPriceChars, gasPriceUnit, &status));
    (*env)->ReleaseStringUTFChars(env, gasPriceObject, gasPriceChars);

    const char *gasLimitChars = (*env)->GetStringUTFChars(env, gasLimitObject, 0);
    BREthereumGas gasLimit = gasCreate(strtoull(gasLimitChars, NULL, 0));
    (*env)->ReleaseStringUTFChars(env, gasLimitObject, gasLimitChars);

    BREthereumTransfer tid =
            ewmWalletCreateTransferGeneric(node,
                                           (BREthereumWallet) wid,
                                           to,
                                           amount,
                                           gasPrice,
                                           gasLimit,
                                           data);
    (*env)->ReleaseStringUTFChars(env, toObject, to);
    (*env)->ReleaseStringUTFChars(env, dataObject, data);

    return (jlong) tid;
}


/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniSignTransaction
 * Signature: (JJLjava/lang/String;)V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniSignTransaction
        (JNIEnv *env, jobject thisObject,
         jlong walletId,
         jlong transactionId,
         jstring paperKeyString) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);

    const char *paperKey = (*env)->GetStringUTFChars (env, paperKeyString, 0);
    ewmWalletSignTransferWithPaperKey(node,
                                  (BREthereumWallet) walletId,
                                  (BREthereumTransfer) transactionId,
				  paperKey);
    (*env)->ReleaseStringUTFChars(env, paperKeyString, paperKey);
}


/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniSignTransactionWithPrivateKey
 * Signature: (JJ[B)V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniSignTransactionWithPrivateKey
        (JNIEnv *env, jobject thisObject,
         jlong walletId,
         jlong transactionId,
         jbyteArray privateKeyByteArray) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);

    BRKey *key = (BRKey *) (*env)->GetByteArrayElements(env, privateKeyByteArray, 0);

    ewmWalletSignTransfer(node,
                          (BREthereumWallet) walletId,
                          (BREthereumTransfer) transactionId,
                          *key);

    (*env)->ReleaseByteArrayElements(env, privateKeyByteArray, (jbyte *) key, 0);
}



/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniSubmitTransaction
 * Signature: (JJ)V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniSubmitTransaction
        (JNIEnv *env, jobject thisObject,
         jlong wid,
         jlong tid) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    ewmWalletSubmitTransfer(node,
                            (BREthereumWallet) wid,
                            (BREthereumTransfer) tid);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniGetTransactions
 * Signature: (J)[J
 */
JNIEXPORT jlongArray JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniGetTransactions
        (JNIEnv *env, jobject thisObject,
         jlong wid) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    BREthereumWallet wallet = getWallet (env, wid);

    int count = ewmWalletGetTransferCount(node, wallet);
    assert (-1 != count);

    // uint32_t array - need a long
    BREthereumTransfer *transactionIds =
            ewmWalletGetTransfers(node, wallet);

    jlong ids[count];
    for (int i = 0; i < count; i++) ids[i] = (jlong) transactionIds[i];

    jlongArray transactions = (*env)->NewLongArray (env, (jsize) count);
    (*env)->SetLongArrayRegion (env, transactions, 0, (jsize) count, (jlong*) ids);

    free (transactionIds);
    return transactions;
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniTransactionGetAmount
 * Signature: (JJ)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniTransactionGetAmount
        (JNIEnv *env, jobject thisObject,
         jlong tid,
         jlong unit) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    BREthereumTransfer transfer = getTransfer (env, tid);
    BREthereumAmount amount = ewmTransferGetAmount(node, transfer);


    return asJniString(env,
                       (ETHEREUM_BOOLEAN_TRUE == ewmTransferHoldsToken(node, transfer, NULL)
                        ? ewmCoerceEtherAmountToString(node, amount.u.ether, (BREthereumEtherUnit) unit)
                        : ewmCoerceTokenAmountToString(node, amount.u.tokenQuantity, (BREthereumTokenQuantityUnit) unit)));
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniTransactionGetFee
 * Signature: (JJ)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniTransactionGetFee
        (JNIEnv *env, jobject thisObject,
         jlong tid,
         jlong unit) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    BREthereumTransfer transfer = getTransfer (env, tid);

    int overflow = 0;
    BREthereumEther fee = ewmTransferGetFee(node, transfer, &overflow);

    // Return the FEE in `resultUnit`
    char *feeString = (0 != overflow
                       ? ""
                       : ewmCoerceEtherAmountToString(node, fee,
                                                           (BREthereumEtherUnit) unit));
    jstring result = (*env)->NewStringUTF(env, feeString);
    if (0 != strcmp("", feeString)) free(feeString);

    return result;
}


/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniTransactionHasToken
 * Signature: (J)Z
 */
JNIEXPORT jboolean JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniTransactionHasToken
        (JNIEnv *env, jobject thisObject,
         jlong tid) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    BREthereumTransfer transfer = getTransfer (env, tid);
    return (jboolean) (ETHEREUM_BOOLEAN_FALSE == ewmTransferHoldsToken(node, transfer, NULL)
                       ? JNI_TRUE
                       : JNI_FALSE);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniTransactionEstimateGas
 * Signature: (JJ)V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniTransactionEstimateGas
        (JNIEnv *env, jobject thisObject,
         jlong wid,
         jlong tid) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    BREthereumWallet   wallet   = getWallet (env, wid);
    BREthereumTransfer transfer = getTransfer (env, tid);

    // ewmUpdateGasEstimate (node, wallet, transfer);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniTransactionEstimateFee
 * Signature: (JLjava/lang/String;JJ)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniTransactionEstimateFee
        (JNIEnv *env, jobject thisObject,
         jlong wid,
         jstring amountString,
         jlong amountUnit,
         jlong resultUnit) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    BREthereumWallet   wallet   = getWallet (env, wid);

    int overflow;
    const char *number = (*env)->GetStringUTFChars(env, amountString, 0);
    BRCoreParseStatus status;

    // Get the `amount` as ETHER or TOKEN QUANTITY
    BREthereumToken token = ewmWalletGetToken(node, wallet);
    BREthereumAmount amount = (NULL == token
                               ? ewmCreateEtherAmountString(node, number,
                                                                 (BREthereumEtherUnit) amountUnit,
                                                                 &status)
                               : ewmCreateTokenAmountString(node, token, number,
                                                                 (BREthereumTokenQuantityUnit) amountUnit,
                                                                 &status));
    (*env)->ReleaseStringUTFChars(env, amountString, number);

    // Get the estimated FEE
    BREthereumEther fee = ewmWalletEstimateTransferFee(node, wallet, amount, &overflow);

    // Return the FEE in `resultUnit`
    char *feeString = (status != CORE_PARSE_OK || 0 != overflow
                       ? ""
                       : ewmCoerceEtherAmountToString(node, fee,
                                                           (BREthereumEtherUnit) resultUnit));

    jstring result = (*env)->NewStringUTF(env, feeString);
    if (0 != strcmp("", feeString)) free(feeString);

    return result;
}


/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniTransactionSourceAddress
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniTransactionSourceAddress
        (JNIEnv *env, jobject thisObject, jlong tid) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    BREthereumAddress source = ewmTransferGetSource (node, (BREthereumTransfer) tid);
    return asJniString(env, addressGetEncodedString (source, 1));
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniTransactionTargetAddress
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniTransactionTargetAddress
        (JNIEnv *env, jobject thisObject, jlong tid) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    BREthereumTransfer transfer = getTransfer (env, tid);
    BREthereumAddress target = ewmTransferGetTarget (node, transfer);
    return asJniString(env, addressGetEncodedString(target, 1));
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniTransactionGetIdentifier
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_breadwallet_core_ethereum_BREthereumEWM_jniTransactionGetIdentifier
        (JNIEnv *env, jobject thisObject, jlong tid) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    BREthereumTransfer transfer = getTransfer (env, tid);
    BREthereumHash hash = ewmTransferGetIdentifier(node, transfer);
    return asJniString(env, hashAsString(hash));
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniTransactionOriginatingTransactionHash
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_breadwallet_core_ethereum_BREthereumEWM_jniTransactionOriginatingTransactionHash
        (JNIEnv *env, jobject thisObject, jlong tid) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    BREthereumTransfer transfer = getTransfer (env, tid);
    BREthereumHash hash = ewmTransferGetOriginatingTransactionHash(node, transfer);
    return asJniString(env, hashAsString(hash));
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniTransactionGetGasPrice
 * Signature: (JJ)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniTransactionGetGasPrice
        (JNIEnv *env, jobject thisObject, jlong tid, jlong unit) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    BREthereumTransfer transfer = getTransfer (env, tid);
    BREthereumGasPrice price = ewmTransferGetGasPrice (node, transfer,
             (BREthereumEtherUnit) unit);
    return asJniString(env, ewmCoerceEtherAmountToString(node,
                                                         price.etherPerGas,
                                                         (BREthereumEtherUnit) unit));
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniTransactionGetGasLimit
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniTransactionGetGasLimit
        (JNIEnv *env, jobject thisObject, jlong tid) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    BREthereumTransfer transfer = getTransfer (env, tid);
    BREthereumGas limit =  ewmTransferGetGasLimit (node, transfer);
    return (jlong) limit.amountOfGas ;
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniTransactionGetGasUsed
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniTransactionGetGasUsed
        (JNIEnv *env, jobject thisObject, jlong tid) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    BREthereumTransfer transfer = getTransfer (env, tid);
    BREthereumGas gas = ewmTransferGetGasUsed(node, transfer);
    return (jlong) gas.amountOfGas;
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniTransactionGetNonce
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniTransactionGetNonce
        (JNIEnv *env, jobject thisObject, jlong tid) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    BREthereumTransfer transfer = getTransfer (env, tid);
    return (jlong) ewmTransferGetNonce (node, transfer);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniTransactionGetBlockNumber
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniTransactionGetBlockNumber
        (JNIEnv *env, jobject thisObject, jlong tid) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    BREthereumTransfer transfer = getTransfer (env, tid);
    return (jlong) ewmTransferGetBlockNumber (node, transfer);

}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniTransactionGetBlockTimestamp
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_com_breadwallet_core_ethereum_BREthereumEWM_jniTransactionGetBlockTimestamp
        (JNIEnv *env, jobject thisObject, jlong tid) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    BREthereumTransfer transfer = getTransfer (env, tid);
    return (jlong) ewmTransferGetBlockTimestamp (node, transfer);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniTransactionGetBlockConfirmations
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniTransactionGetBlockConfirmations
        (JNIEnv *env, jobject thisObject, jlong tid) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    BREthereumTransfer transfer = getTransfer (env, tid);
    return (jlong) ewmTransferGetBlockConfirmations (node, transfer);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniTransactionGetToken
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_com_breadwallet_core_ethereum_BREthereumEWM_jniTransactionGetToken
        (JNIEnv *env, jobject thisObject, jlong tid) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    BREthereumTransfer transfer = getTransfer (env, tid);
    return (jlong) ewmTransferGetToken(node, transfer);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniTransactionIsConfirmed
 * Signature: (J)Z
 */
JNIEXPORT jboolean JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniTransactionIsConfirmed
        (JNIEnv *env, jobject thisObject, jlong tid) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    BREthereumTransfer transfer = getTransfer (env, tid);
    return (jboolean) (ETHEREUM_BOOLEAN_TRUE == ewmTransferIsConfirmed (node,transfer)
                       ? JNI_TRUE
                       : JNI_FALSE);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniTransactionIsSubmitted
 * Signature: (J)Z
 */
JNIEXPORT jboolean JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniTransactionIsSubmitted
        (JNIEnv *env, jobject thisObject, jlong tid) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    BREthereumTransfer transfer = getTransfer (env, tid);
    return (jboolean) (ETHEREUM_BOOLEAN_TRUE == ewmTransferIsSubmitted (node, transfer)
                       ? JNI_TRUE
                       : JNI_FALSE);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniTransactionIsErrored
 * Signature: (J)Z
 */
JNIEXPORT jboolean JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniTransactionIsErrored
        (JNIEnv *env, jobject thisObject, jlong tid) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    BREthereumTransfer transfer = getTransfer (env, tid);
    return (jboolean) (TRANSFER_STATUS_ERRORED == ewmTransferGetStatus (node, transfer)
                       ? JNI_TRUE
                       : JNI_FALSE);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniTransactionGetErrorDescription
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_breadwallet_core_ethereum_BREthereumEWM_jniTransactionGetErrorDescription
        (JNIEnv *env, jobject thisObject, jlong tid) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    BREthereumTransfer transfer = getTransfer (env, tid);
    char *errorDescription = ewmTransferStatusGetError(node, transfer);
    return NULL == errorDescription ? NULL : asJniString(env, errorDescription);
}


/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniUpdateTokens
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_breadwallet_core_ethereum_BREthereumEWM_jniUpdateTokens
        (JNIEnv *env, jobject thisObject) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    ewmUpdateTokens(node);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniEWMGetBlockHeight
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_com_breadwallet_core_ethereum_BREthereumEWM_jniEWMGetBlockHeight
        (JNIEnv *env, jobject thisObject) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    return (jlong) ewmGetBlockHeight(node);
}

#if 0
/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniBlockGetNumber
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_com_breadwallet_core_ethereum_BREthereumEWM_jniBlockGetNumber
        (JNIEnv *env, jobject thisObject, jlong bid) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    return ethereumBlockGetNumber(node, bid);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniBlockGetTimestamp
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_com_breadwallet_core_ethereum_BREthereumEWM_jniBlockGetTimestamp
        (JNIEnv *env, jobject thisObject, jlong bid) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    return ethereumBlockGetTimestamp(node, bid);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniBlockGetHash
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_breadwallet_core_ethereum_BREthereumEWM_jniBlockGetHash
        (JNIEnv *env, jobject thisObject, jlong bid) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    char *hash = ethereumBlockGetHash(node, bid);
    jstring result = (*env)->NewStringUTF (env, hash);
    free (hash);
    return result;
}
#endif

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniEWMConnect
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniEWMConnect
        (JNIEnv *env, jobject thisObject) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    return (jboolean) (ETHEREUM_BOOLEAN_TRUE == ewmConnect(node)
                       ? JNI_TRUE
                       : JNI_FALSE);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniEWMDisconnect
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniEWMDisconnect
        (JNIEnv *env, jobject thisObject) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    return (jboolean) (ETHEREUM_BOOLEAN_TRUE == ewmDisconnect(node) ? JNI_TRUE : JNI_FALSE);
}

//
// JSON RPC Callback
//
//static jmethodID
//lookupListenerMethod (JNIEnv *env, jobject listener, char *name, char *type) {
//    // Class with desired method.
//    jclass listenerClass = (*env)->GetObjectClass(env, listener);
//
//    // Method, if found.
//    jmethodID listenerMethod = (*env)->GetMethodID(env, listenerClass, name, type);
//
//    // Clean up and return.
//    (*env)->DeleteLocalRef (env, listenerClass);
//    return listenerMethod;
//}


static void
clientGetGasPrice(BREthereumClientContext context,
                  BREthereumEWM node,
                  BREthereumWallet wid,
                  int id) {
    JNIEnv *env = getEnv();
    if (NULL == env) return;

    (*env)->CallStaticVoidMethod(env, trampolineClass, trampolineGetGasPrice,
                                 (jlong) node,
                                 (jlong) wid,
                                 (jint) id);
}

static void
clientEstimateGas(BREthereumClientContext context,
                  BREthereumEWM node,
                  BREthereumWallet wid,
                  BREthereumCookie cookie,
                  const char *fromStr,
                  const char *toStr,
                  const char *amountStr,
                  const char *gasPriceStr,
                  const char *dataStr,
                  int id) {
    JNIEnv *env = getEnv();
    if (NULL == env) return;

    jobject from = (*env)->NewStringUTF(env, fromStr);
    jobject to = (*env)->NewStringUTF(env, toStr);
    jobject amount = (*env)->NewStringUTF(env, amountStr);
    jobject data = (*env)->NewStringUTF(env, dataStr);

    // (*env)->CallStaticVoidMethod(env, trampolineClass, trampolineGetGasEstimate,
    //                              (jlong) node,
    //                              (jlong) wid,
    //                              (jlong) tid,
    //                              from,
    //                              to,
    //                              amount,
    //                              data,
    //                              (jint) id);

    (*env)->DeleteLocalRef(env, data);
    (*env)->DeleteLocalRef(env, amount);
    (*env)->DeleteLocalRef(env, to);
    (*env)->DeleteLocalRef(env, from);
}

static void
clientGetBalance(BREthereumClientContext context,
                 BREthereumEWM node,
                 BREthereumWallet wid,
                 const char *accountStr,
                 int id) {
    JNIEnv *env = getEnv();
    if (NULL == env) return;

    jobject account = (*env)->NewStringUTF(env, accountStr);

    (*env)->CallStaticVoidMethod(env, trampolineClass, trampolineGetBalance,
                                 (jlong) node,
                                 (jlong) wid,
                                 account,
                                 (jint) id);

    (*env)->DeleteLocalRef(env, account);
}

static void
clientSubmitTransaction(BREthereumClientContext context,
                        BREthereumEWM node,
                        BREthereumWallet wid,
                        BREthereumTransfer tid,
                        const char *transactionStr,
                        int id) {
    JNIEnv *env = getEnv();
    if (NULL == env) return;

    jobject transaction = (*env)->NewStringUTF(env, transactionStr);

    (*env)->CallStaticVoidMethod(env, trampolineClass, trampolineSubmitTransaction,
                                 (jlong) node,
                                 (jlong) wid,
                                 (jlong) tid,
                                 transaction,
                                 (jint) id);

    (*env)->DeleteLocalRef(env, transaction);
}

static void
clientGetTransactions(BREthereumClientContext context,
                      BREthereumEWM node,
                      const char *addressStr,
                      uint64_t begBlockNumber,
                      uint64_t endBlockNumber,
                      int id) {
    JNIEnv *env = getEnv();
    if (NULL == env) return;

    jobject address = (*env)->NewStringUTF(env, addressStr);

    (*env)->CallStaticVoidMethod(env, trampolineClass, trampolineGetTransactions,
                                 (jlong) node,
                                 address,
                                 (jlong) begBlockNumber,
                                 (jlong) endBlockNumber,
                                 (jint) id);

    (*env)->DeleteLocalRef(env, address);
}

static void
clientGetLogs(BREthereumClientContext context,
              BREthereumEWM node,
              const char *contract,
              const char *address,
              const char *event,
              uint64_t begBlockNumber,
              uint64_t endBlockNumber,
              int rid) {
    JNIEnv *env = getEnv();
    if (NULL == env) return;

    jobject contractObject = (*env)->NewStringUTF(env, contract);
    jobject addressObject = (*env)->NewStringUTF(env, address);
    jobject eventObject = (*env)->NewStringUTF(env, event);

    (*env)->CallStaticVoidMethod(env, trampolineClass, trampolineGetLogs,
                                 (jlong) node,
                                 contractObject,
                                 addressObject,
                                 eventObject,
                                 (jlong) begBlockNumber,
                                 (jlong) endBlockNumber,
                                 (jint) rid);

    (*env)->DeleteLocalRef(env, eventObject);
    (*env)->DeleteLocalRef(env, addressObject);
    (*env)->DeleteLocalRef(env, contractObject);
}

static void
clientGetBlocks(BREthereumClientContext context,
                BREthereumEWM ewm,
                const char *addressStr,
                BREthereumSyncInterestSet interests,
                uint64_t blockNumberStart,
                uint64_t blockNumberStop,
                int rid) {
    JNIEnv *env = getEnv();
    if (NULL == env) return;

    jobject address = (*env)->NewStringUTF(env, addressStr);

    (*env)->CallStaticVoidMethod(env, trampolineClass, trampolineGetBlocks,
                                 (jlong) ewm,
                                 address,
                                 (jint) interests,
                                 (jlong) blockNumberStart,
                                 (jlong) blockNumberStop,
                                 (jint) rid);

    (*env)->DeleteLocalRef(env, address);
}

static void
clientGetTokens(BREthereumClientContext context,
                BREthereumEWM ewm,
                int rid) {
    JNIEnv *env = getEnv();
    if (NULL == env) return;


    (*env)->CallStaticVoidMethod(env, trampolineClass, trampolineGetTokens,
                                 (jlong) ewm,
                                 (jint) rid);
}


static void
clientGetBlockNumber(BREthereumClientContext context,
                     BREthereumEWM node,
                     int id) {
    JNIEnv *env = getEnv();
    if (NULL == env) return;

    (*env)->CallStaticVoidMethod(env, trampolineClass, trampolineGetBlockNumber,
                                 (jlong) node,
                                 (jint) id);
}

static void
clientGetNonce(BREthereumClientContext context,
               BREthereumEWM node,
               const char *address,
               int id) {
    JNIEnv *env = getEnv();
    if (NULL == env) return;

    jobject addressObject = (*env)->NewStringUTF(env, address);

    (*env)->CallStaticVoidMethod(env, trampolineClass, trampolineGetNonce,
                                 (jlong) node,
                                 addressObject,
                                 (jint) id);

    (*env)->DeleteLocalRef(env, addressObject);
}


static void
clientEWMEventHandler(BREthereumClientContext context,
                      BREthereumEWM ewm,
                      BREthereumEWMEvent event,
                      BREthereumStatus status,
                      const char *errorDescription) {
    JNIEnv *env = getEnv();
    if (NULL == env) return;

    jstring errorDescriptionString = (NULL == errorDescription
                                      ? NULL
                                      : (*env)->NewStringUTF(env, errorDescription));

    // Callback
    (*env)->CallStaticVoidMethod(env, trampolineClass, trampolineEWMEvent,
                                 (jlong) ewm,
                                 (jint) event,
                                 (jint) status,
                                 errorDescriptionString);

    // Cleanup
    if (NULL != errorDescriptionString) (*env)->DeleteLocalRef(env, errorDescriptionString);
}

static void
clientPeerEventHandler(BREthereumClientContext context,
                       BREthereumEWM ewm,
                       BREthereumPeerEvent event,
                       BREthereumStatus status,
                       const char *errorDescription) {
    JNIEnv *env = getEnv();
    if (NULL == env) return;

    jstring errorDescriptionString = (NULL == errorDescription
                                      ? NULL
                                      : (*env)->NewStringUTF(env, errorDescription));

    // Callback
    (*env)->CallStaticVoidMethod(env, trampolineClass, trampolinePeerEvent,
                                 (jlong) ewm,
                                 (jint) event,
                                 (jint) status,
                                 errorDescriptionString);

    // Cleanup
    if (NULL != errorDescriptionString) (*env)->DeleteLocalRef(env, errorDescriptionString);
}

static void
clientWalletEventHandler(BREthereumClientContext context,
                         BREthereumEWM node,
                         BREthereumWallet wid,
                         BREthereumWalletEvent event,
                         BREthereumStatus status,
                         const char *errorDescription) {
    JNIEnv *env = getEnv();
    if (NULL == env) return;

    jstring errorDescriptionString = (NULL == errorDescription
                                      ? NULL
                                      : (*env)->NewStringUTF(env, errorDescription));

    // (*env)->CallStaticVoidMethod(env, trampolineClass, trampolineWalletEvent,
    //                              (jlong) node,
    //                              (jlong) wid,
    //                              (jint) event,
    //                              (jint) status,
    //                              errorDescriptionString);

    if (NULL != errorDescriptionString) (*env)->DeleteLocalRef(env, errorDescriptionString);
}

static void
clientTokenEventHandler(BREthereumClientContext context,
                        BREthereumEWM ewm,
                        BREthereumToken token,
                        BREthereumTokenEvent event) {
    JNIEnv *env = getEnv();
    if (NULL == env) return;

    (*env)->CallStaticVoidMethod(env, trampolineClass, trampolineTokenEvent,
                                 (jlong) ewm,
                                 (jlong) token,
                                 (jint) event);
}

#if 0
static void
clientBlockEventHandler(BREthereumClientContext context,
                        BREthereumEWM node,
                        BREthereumBlock bid,
                        BREthereumBlockEvent event,
                        BREthereumStatus status,
                        const char *errorDescription) {
    JNIEnv *env = getEnv();
    if (NULL == env) return;

    jstring errorDescriptionString = (NULL == errorDescription
                                      ? NULL
                                      : (*env)->NewStringUTF(env, errorDescription));

    // Callback
    (*env)->CallStaticVoidMethod(env, trampolineClass, trampolineBlockEvent,
                                 (jlong) node,
                                 (jint) bid,
                                 (jint) event,
                                 (jint) status,
                                 errorDescriptionString);

    // Cleanup
    if (NULL != errorDescriptionString) (*env)->DeleteLocalRef(env, errorDescriptionString);
}
#endif
static void
clientTransferEventHandler(BREthereumClientContext context,
                           BREthereumEWM node,
                           BREthereumWallet wid,
                           BREthereumTransfer tid,
                           BREthereumTransferEvent event,
                           BREthereumStatus status,
                           const char *errorDescription) {
    JNIEnv *env = getEnv();
    if (NULL == env) return;

    jstring errorDescriptionString = (NULL == errorDescription
                                      ? NULL
                                      : (*env)->NewStringUTF(env, errorDescription));

    // Callback
    (*env)->CallStaticVoidMethod(env, trampolineClass, trampolineTransferEvent,
                                 (jlong) node,
                                 (jlong) wid,
                                 (jlong) tid,
                                 (jint) event,
                                 (jint) status,
                                 errorDescriptionString);

    // Cleanup
    if (NULL != errorDescriptionString) (*env)->DeleteLocalRef(env, errorDescriptionString);
}
