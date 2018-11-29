//  Created by Ed Gamble on 3/7/2018
//  Copyright (c) 2018 breadwallet LLC.
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#include "../BRCoreJni.h"

#include <stdlib.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <ethereum/ewm/BREthereumAmount.h>
#include "BRBIP39Mnemonic.h"
#include "BRKey.h"
#include "BREthereum.h"

#include "com_breadwallet_core_ethereum_BREthereumEWM.h"

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
                  BREthereumTransfer tid,
                  const char *to,
                  const char *amount,
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
                      int id);

static void
clientGetLogs(BREthereumClientContext context,
              BREthereumEWM node,
              const char *contract,
              const char *address,
              const char *event,
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
clientSaveNodes (BREthereumClientContext context,
                 BREthereumEWM ewm,
                 BRSetOf(BREthereumHashDataPair) data);

static void
clientSaveBlocks (BREthereumClientContext context,
                  BREthereumEWM ewm,
                  BRSetOf(BREthereumHashDataPair) data);

static void
clientChangeTransaction (BREthereumClientContext context,
                         BREthereumEWM ewm,
                         BREthereumClientChangeType type,
                         BREthereumHashDataPair dagta);

static void
clientChangeLog (BREthereumClientContext context,
                 BREthereumEWM ewm,
                 BREthereumClientChangeType type,
                 BREthereumHashDataPair data);

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
static jmethodID trampolineSaveNodes = NULL;
static jmethodID trampolineSaveBlocks = NULL;
static jmethodID trampolineChangeTransaction = NULL;
static jmethodID trampolineChangeLog = NULL;
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

//
// Hash Map
//
static jclass hashMapClass = NULL;
static jmethodID hashMapInit = NULL;
static jmethodID hashMapPut  = NULL;

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
    trampolineGetGasEstimate    = trampolineOrFatal (env, "trampolineGetGasEstimate",    "(JJJLjava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V");
    trampolineGetBalance        = trampolineOrFatal (env, "trampolineGetBalance",        "(JJLjava/lang/String;I)V");
    trampolineSubmitTransaction = trampolineOrFatal (env, "trampolineSubmitTransaction", "(JJJLjava/lang/String;I)V");
    trampolineGetTransactions   = trampolineOrFatal (env, "trampolineGetTransactions",   "(JLjava/lang/String;I)V");
    trampolineGetLogs           = trampolineOrFatal (env, "trampolineGetLogs",           "(JLjava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V");
    trampolineGetBlocks         = trampolineOrFatal (env, "trampolineGetBlocks",         "(JLjava/lang/String;IJJI)V");
    trampolineGetTokens         = trampolineOrFatal (env, "trampolineGetTokens",         "(JI)V");
    trampolineGetBlockNumber    = trampolineOrFatal (env, "trampolineGetBlockNumber",    "(JI)V");
    trampolineGetNonce          = trampolineOrFatal (env, "trampolineGetNonce",          "(JLjava/lang/String;I)V");
    trampolineSaveNodes         = trampolineOrFatal (env, "trampolineSaveNodes",         "(JLjava/util/Map;)V");
    trampolineSaveBlocks        = trampolineOrFatal (env, "trampolineSaveBlocks",        "(JLjava/util/Map;)V");
    trampolineChangeTransaction = trampolineOrFatal (env, "trampolineChangeTransaction", "(JILjava/lang/String;Ljava/lang/String;)V");
    trampolineChangeLog         = trampolineOrFatal (env, "trampolineChangeLog",         "(JILjava/lang/String;Ljava/lang/String;)V");
    trampolineEWMEvent          = trampolineOrFatal (env, "trampolineEWMEvent",          "(JIILjava/lang/String;)V");
    trampolinePeerEvent         = trampolineOrFatal (env, "trampolinePeerEvent",         "(JIILjava/lang/String;)V");
    trampolineWalletEvent       = trampolineOrFatal (env, "trampolineWalletEvent",       "(JJIILjava/lang/String;)V");
    trampolineTokenEvent        = trampolineOrFatal (env, "trampolineTokenEvent",        "(JJI)V");
//    trampolineBlockEvent        = trampolineOrFatal (env, "trampolineBlockEvent",        "(JIIILjava/lang/String;)V");
    trampolineTransferEvent     = trampolineOrFatal (env, "trampolineTransferEvent",     "(JJJIILjava/lang/String;)V");

    jclass mapClass = (*env)->FindClass(env, "java/util/HashMap");
    assert (NULL != mapClass);

    hashMapClass = (*env)->NewGlobalRef (env, mapClass);
    hashMapInit  = (*env)->GetMethodID(env, hashMapClass, "<init>", "(I)V");
    hashMapPut   = (*env)->GetMethodID(env, hashMapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    assert (NULL != hashMapInit && NULL != hashMapPut);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniCreateEWM
 * Signature: (Lcom/breadwallet/core/ethereum/BREthereumEWM/Client;JLjava/lang/String;[Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniCreateEWM
        (JNIEnv *env, jclass thisClass,
         jobject clientObject,
         jlong network,
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

    const char *paperKey = (*env)->GetStringUTFChars (env, paperKeyString, 0);

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

            clientSaveNodes,
            clientSaveBlocks,
            clientChangeTransaction,
            clientChangeLog,

            clientEWMEventHandler,
            clientPeerEventHandler,
            clientWalletEventHandler,
            clientTokenEventHandler,
            clientTransferEventHandler
    };

    BREthereumEWM node = ewmCreateWithPaperKey((BREthereumNetwork) network,
                                        paperKey,
                                        ETHEREUM_TIMESTAMP_UNKNOWN,
                                        P2P_ONLY,
                                        client,
                                        NULL,
                                        NULL,
                                        NULL,
                                        NULL);

    (*env)->ReleaseStringUTFChars (env, paperKeyString, paperKey);
    return (jlong) node;
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniCreateEWM_PublicKey
 * Signature: (Lcom/breadwallet/core/ethereum/BREthereumEWM/Client;J[B)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniCreateEWM_1PublicKey
    (JNIEnv *env, jclass thisClass, jobject clientObject, jlong network, jbyteArray publicKey) {

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

            clientSaveNodes,
            clientSaveBlocks,
            clientChangeTransaction,
            clientChangeLog,

            clientEWMEventHandler,
            clientPeerEventHandler,
            clientWalletEventHandler,
            clientTokenEventHandler,
//            clientBlockEventHandler,
            clientTransferEventHandler
    };

    BREthereumEWM node = ewmCreateWithPublicKey((BREthereumNetwork) network,
                                                     key,
                                                     ETHEREUM_TIMESTAMP_UNKNOWN,
                                                     P2P_ONLY,
                                                     client,
                                                     NULL,
                                                     NULL,
                                                     NULL,
                                                     NULL);


    (*env)->ReleaseByteArrayElements(env, publicKey, publicKeyBytes, 0);
    return (jlong) node;
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
    BREthereumAmount balance = ewmWalletGetBalance(node, wid);

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

    ewmUpdateGasPrice
            (node,
             (BREthereumWallet) wid);
}


/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniForceWalletBalanceUpdate
 * Signature: (J)V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniForceWalletBalanceUpdate
        (JNIEnv *env, jobject thisObject, jlong wid) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    ewmUpdateWalletBalance
            (node,
             (BREthereumWallet) wid);
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
    BREthereumGasPrice price = ewmWalletGetDefaultGasPrice(node, wid);
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
    BREthereumGasPrice price = gasPriceCreate (etherCreateNumber(value, WEI));
    ewmWalletSetDefaultGasPrice(node, (BREthereumWallet) wid, price);

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
    BREthereumGas limit = ewmWalletGetDefaultGasLimit (node, (BREthereumWallet) wid);
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
    BREthereumGas limit = gasCreate(value);
    ewmWalletSetDefaultGasLimit(node, (BREthereumWallet) wid, limit);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniForceTransactionUpdate
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniForceTransactionUpdate
        (JNIEnv *env, jobject thisObject) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    ewmUpdateTransactions(node);
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

    const char *balance = (*env)->GetStringUTFChars(env, balanceString, 0);
    ewmAnnounceWalletBalance(node, wid, balance, rid);

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
    const char *strGasEstimate = (*env)->GetStringUTFChars(env, gasEstimate, 0);
    ewmAnnounceGasEstimate(node,
                                 wid,
                                 tid,
                                 strGasEstimate,
                                 rid);
    (*env)->ReleaseStringUTFChars(env, gasEstimate, strGasEstimate);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniAnnounceSubmitTransaction
 * Signature: (JJLjava/lang/String;I)V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniAnnounceSubmitTransaction
        (JNIEnv *env, jobject thisObject,
         jlong wid,
         jlong tid,
         jstring hash,
         jint rid) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    const char *hashStr = (*env)->GetStringUTFChars (env, hash, 0);
    ewmAnnounceSubmitTransfer(node, wid, tid, hashStr, rid);
    (*env)->ReleaseStringUTFChars (env, hash, hashStr);
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
    ethereumClientAnnounceBlockNumber(node, strBlockNumber, rid);
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
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniAnnounceToken
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;I)V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniAnnounceToken
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
                               strAddress, strSymbol, strName, strDescription,
                            decimals, strGasLimit, strGasPrice,
                            rid);

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
    BREthereumToken token = ewmWalletGetToken(node, wid);

    // Get an actual Amount
    BRCoreParseStatus status = CORE_PARSE_OK;
    const char *amountChars = (*env)->GetStringUTFChars(env, amountObject, 0);
    BREthereumAmount amount = NULL == token
                              ? amountCreateEtherString(amountChars, amountUnit, &status)
                              : amountCreateTokenQuantityString(token, amountChars, amountUnit,
                                                                &status);
    (*env)->ReleaseStringUTFChars (env, amountObject, amountChars);

    const char *to = (*env)->GetStringUTFChars(env, toObject, 0);
    BREthereumTransfer tid =
            ewmWalletCreateTransfer(node,
                                            (BREthereumWallet) wid,
                                            to,
                                            amount);
    (*env)->ReleaseStringUTFChars(env, toObject, to);
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

    BRKey *key = (BRKey *) (*env)->GetByteArrayElements (env, privateKeyByteArray, 0);

    ewmWalletSignTransfer(node,
                                                (BREthereumWallet) walletId,
                                                (BREthereumTransfer) transactionId,
                                                *key);

    (*env)->ReleaseByteArrayElements (env, privateKeyByteArray, (jbyte*) key, 0);
}



/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniSubmitTransaction
 * Signature: (JJ)V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniSubmitTransaction
        (JNIEnv *env, jobject thisObject,
         jlong walletId,
         jlong transactionId) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    ewmWalletSubmitTransfer(node,
                                    (BREthereumWallet) walletId,
                                    (BREthereumTransfer) transactionId);
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
    int count = ewmWalletGetTransferCount(node, wid);
    assert (-1 != count);

    // uint32_t array - need a long
    BREthereumTransfer *transactionIds =
            ewmWalletGetTransfers(node, (BREthereumWallet) wid);

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
    BREthereumAmount amount = ewmTransferGetAmount(node, (BREthereumTransfer) tid);


    return asJniString(env,
                       (ETHEREUM_BOOLEAN_TRUE == ewmTransferHoldsToken(node, tid, NULL)
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

    int overflow = 0;
    BREthereumEther fee = ewmTransferGetFee(node,
                                                    (BREthereumTransfer) tid,
                                                    &overflow);

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
    return (jboolean) (ETHEREUM_BOOLEAN_FALSE == ewmTransferHoldsToken(node, tid, NULL)
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
         jlong walletId,
         jlong transactionId) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);

    ewmUpdateGasEstimate
            (node,
             (BREthereumWallet) walletId,
             (BREthereumTransfer) transactionId);
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

    int overflow;
    const char *number = (*env)->GetStringUTFChars(env, amountString, 0);
    BRCoreParseStatus status;

    // Get the `amount` as ETHER or TOKEN QUANTITY
    BREthereumToken token = ewmWalletGetToken(node, (BREthereumWallet) wid);
    BREthereumAmount amount = (NULL == token
                               ? ewmCreateEtherAmountString(node, number,
                                                                 (BREthereumEtherUnit) amountUnit,
                                                                 &status)
                               : ewmCreateTokenAmountString(node, token, number,
                                                                 (BREthereumTokenQuantityUnit) amountUnit,
                                                                 &status));
    (*env)->ReleaseStringUTFChars(env, amountString, number);

    // Get the estimated FEE
    BREthereumEther fee = ewmWalletEstimateTransferFee(node,
                                                               (BREthereumWallet) wid,
                                                               amount, &overflow);

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
    BREthereumAddress target = ewmTransferGetTarget (node, (BREthereumTransfer) tid);
    return asJniString(env, addressGetEncodedString(target, 1));
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniTransactionGetHash
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_breadwallet_core_ethereum_BREthereumEWM_jniTransactionGetHash
        (JNIEnv *env, jobject thisObject, jlong tid) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    BREthereumHash hash = ewmTransferGetHash(node, (BREthereumTransfer) tid);
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
    BREthereumGasPrice price = ewmTransferGetGasPrice (node,
             (BREthereumTransfer) tid,
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
    BREthereumGas limit =  ewmTransferGetGasLimit (node, (BREthereumTransfer) tid);
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
    BREthereumGas gas = ewmTransferGetGasUsed(node, (BREthereumTransfer) tid);
    return (jlong) gas.amountOfGas;
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniTransactionGetNonce
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniTransactionGetNonce
        (JNIEnv *env, jobject thisObject, jlong transactionId) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    return (jlong) ewmTransferGetNonce
            (node,
             (BREthereumTransfer) transactionId);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniTransactionGetBlockNumber
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniTransactionGetBlockNumber
        (JNIEnv *env, jobject thisObject, jlong transactionId) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    return (jlong) ewmTransferGetBlockNumber
            (node,
             (BREthereumTransfer) transactionId);

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
    return (jlong) ewmTransferGetBlockConfirmations
            (node,
             (BREthereumTransfer) tid);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniTransactionGetToken
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_com_breadwallet_core_ethereum_BREthereumEWM_jniTransactionGetToken
        (JNIEnv *env, jobject thisObject, jlong tid) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    return (jlong) ewmTransferGetToken(node, tid);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumEWM
 * Method:    jniTransactionIsConfirmed
 * Signature: (J)Z
 */
JNIEXPORT jboolean JNICALL
Java_com_breadwallet_core_ethereum_BREthereumEWM_jniTransactionIsConfirmed
        (JNIEnv *env, jobject thisObject, jlong transactionId) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    return (jboolean) (ETHEREUM_BOOLEAN_TRUE ==
                               ewmTransferIsConfirmed
                                       (node,
                                        (BREthereumTransfer) transactionId)
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
        (JNIEnv *env, jobject thisObject, jlong transactionId) {
    BREthereumEWM node = (BREthereumEWM) getJNIReference(env, thisObject);
    return (jboolean) (ETHEREUM_BOOLEAN_TRUE ==
                               ewmTransferIsSubmitted
                               (node,
                                (BREthereumTransfer) transactionId)
                       ? JNI_TRUE
                       : JNI_FALSE);

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

    // TODO: Hopefully
    (*env)->DeleteGlobalRef (env, thisObject);

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
clientEstimateGas(BREthereumClientContext context, BREthereumEWM node,
                  BREthereumWallet wid,
                  BREthereumTransfer tid,
                  const char *toStr,
                  const char *amountStr,
                  const char *dataStr,
                  int id) {
    JNIEnv *env = getEnv();
    if (NULL == env) return;

    jobject to = (*env)->NewStringUTF(env, toStr);
    jobject amount = (*env)->NewStringUTF(env, amountStr);
    jobject data = (*env)->NewStringUTF(env, dataStr);

    (*env)->CallStaticVoidMethod(env, trampolineClass, trampolineGetGasEstimate,
                                 (jlong) wid,
                                 (jlong) tid,
                                 to,
                                 amount,
                                 data,
                                 (jint) id);

    (*env)->DeleteLocalRef(env, data);
    (*env)->DeleteLocalRef(env, amount);
    (*env)->DeleteLocalRef(env, to);
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
                      int id) {
    JNIEnv *env = getEnv();
    if (NULL == env) return;

    jobject address = (*env)->NewStringUTF(env, addressStr);

    (*env)->CallStaticVoidMethod(env, trampolineClass, trampolineGetTransactions,
                                 (jlong) node,
                                 address,
                                 (jint) id);

    (*env)->DeleteLocalRef(env, address);
}

static void
clientGetLogs(BREthereumClientContext context,
              BREthereumEWM node,
              const char *contract,
              const char *address,
              const char *event,
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

static jobject
createHashDataPairMap (JNIEnv *env,
                       BRSetOf(BREthereumHashDataPair) pairSet) {
    char *hashStr, *dataStr;

    jobject map = (*env)->NewObject (env, hashMapClass, hashMapInit, BRSetCount (pairSet));
    FOR_SET (BREthereumHashDataPair, pair, pairSet){
        hashDataPairExtractStrings (pair, &hashStr, &dataStr);

        jstring hash = (*env)->NewStringUTF(env, hashStr);
        jstring data = (*env)->NewStringUTF(env, dataStr);

        (*env)->CallObjectMethod (env, map, hashMapPut, hash, data);

        (*env)->DeleteLocalRef(env, data);
        (*env)->DeleteLocalRef(env, hash);

        free(dataStr);
        free(hashStr);
    }

    return map;
}

static void
clientSaveNodes(BREthereumClientContext context,
                BREthereumEWM ewm,
                BRSetOf(BREthereumHashDataPair) pairSet) {
    JNIEnv *env = getEnv();
    if (NULL == env) return;

    jobject map = createHashDataPairMap (env, pairSet);

    (*env)->CallStaticVoidMethod (env, trampolineClass, trampolineSaveNodes,
                                  (jlong) ewm,
                                  map);

    (*env)->DeleteLocalRef(env, map);
}

static void
clientSaveBlocks(BREthereumClientContext context,
                 BREthereumEWM ewm,
                 BRSetOf(BREthereumHashDataPair) pairSet) {
    JNIEnv *env = getEnv();
    if (NULL == env) return;

    jobject map = createHashDataPairMap (env, pairSet);

    (*env)->CallStaticVoidMethod (env, trampolineClass, trampolineSaveBlocks,
                                  (jlong) ewm,
                                  map);

    (*env)->DeleteLocalRef(env, map);
}

static void
clientChangeTransaction(BREthereumClientContext context,
                        BREthereumEWM ewm,
                        BREthereumClientChangeType type,
                        BREthereumHashDataPair pair) {
    JNIEnv *env = getEnv();
    if (NULL == env) return;

    char *hashStr = hashDataPairGetHashAsString(pair);
    jstring hash = (*env)->NewStringUTF(env, hashStr);

    char *dataStr = hashDataPairGetHashAsString(pair);
    jstring data = (*env)->NewStringUTF(env, dataStr);

    (*env)->CallStaticVoidMethod(env, trampolineClass, trampolineChangeTransaction,
                                 (jlong) ewm,
                                 (jint) type,
                                 hash,
                                 data);

    (*env)->DeleteLocalRef(env, data);
    free(dataStr);
    (*env)->DeleteLocalRef(env, hash);
    free(hashStr);
}

static void
clientChangeLog(BREthereumClientContext context,
                BREthereumEWM ewm,
                BREthereumClientChangeType type,
                BREthereumHashDataPair pair) {
    JNIEnv *env = getEnv();
    if (NULL == env) return;

    char *hashStr = hashDataPairGetHashAsString(pair);
    jstring hash = (*env)->NewStringUTF(env, hashStr);

    char *dataStr = hashDataPairGetHashAsString(pair);
    jstring data = (*env)->NewStringUTF(env, dataStr);

    (*env)->CallStaticVoidMethod(env, trampolineClass, trampolineChangeLog,
                                 (jlong) ewm,
                                 (jint) type,
                                 hash,
                                 data);

    (*env)->DeleteLocalRef(env, data);
    free(dataStr);
    (*env)->DeleteLocalRef(env, hash);
    free(hashStr);
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

    (*env)->CallStaticVoidMethod(env, trampolineClass, trampolineWalletEvent,
                                 (jlong) node,
                                 (jlong) wid,
                                 (jint) event,
                                 (jint) status,
                                 errorDescriptionString);

    if (NULL != errorDescriptionString) (*env)->DeleteLocalRef(env, errorDescriptionString);
}

static void
clientTokenEventHandler(BREthereumClientContext context,
                        BREthereumEWM ewm,
                        BREthereumToken token,
                        BREthereumTokenEvent event) {
    JNIEnv *env = getEnv();
    if (NULL == env) return;

    (*env)->CallStaticVoidMethod(env, trampolineClass, trampolineWalletEvent,
                                 (jlong) ewm,
                                 (jint) token,
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
