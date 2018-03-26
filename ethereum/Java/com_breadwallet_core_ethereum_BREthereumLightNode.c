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

#include <stdlib.h>
#include <malloc.h>
#include <assert.h>
#include <BRBIP39Mnemonic.h>
#include <BREthereumLightNode.h>
#include <string.h>
#include <BREthereumWallet.h>
#include <BREthereum.h>
#include "com_breadwallet_core_ethereum_BREthereumLightNode.h"
#include "BRCoreJni.h"

//
// Forward Declarations
//
static const char *
jsonRpcGetBalance (JsonRpcContext context, BREthereumLightNode node, int id, const char *account);

static const char *
jsonRpcGetGasPrice (JsonRpcContext context, BREthereumLightNode node, int id);

static const char *
jsonRpcEstimateGas (JsonRpcContext context, BREthereumLightNode node, int id, const char *to, const char *amount, const char *data);

static const char *
jsonRpcSubmitTransaction (JsonRpcContext context, BREthereumLightNode node, int id, const char *transaction);

static void
jsonRpcGetTransactions(JsonRpcContext context, BREthereumLightNode node, int id, const char *account);

//
// Statically Initialize Java References
//
static jclass addressClass;
static jmethodID addressConstructor;

static jclass transactionClass;
static jmethodID transactionConstructor;
/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniCreateLightNodeLES
 * Signature: (Lcom/breadwallet/core/ethereum/BREthereumLightNode/Client;JLjava/lang/String;)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniCreateLightNodeLES
        (JNIEnv *env, jclass thisClass, jobject clientObject, jlong network, jstring paperKey) {
    BREthereumLightNodeConfiguration configuration =
            lightNodeConfigurationCreateLES((BREthereumNetwork) network, 0);
    return (jlong) createLightNode(configuration, (*env)->GetStringUTFChars (env, paperKey, 0));
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniCreateLightNodeJSON_RPC
 * Signature: (Lcom/breadwallet/core/ethereum/BREthereumLightNode/Client;JLjava/lang/String;)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniCreateLightNodeJSON_1RPC
        (JNIEnv *env, jclass thisClass, jobject clientObject, jlong network, jstring paperKey) {

    // Get a global reference to client; ensure the client exists in callback threads.
    jobject client = (*env)->NewGlobalRef (env, clientObject);

    BREthereumLightNodeConfiguration configuration =
            lightNodeConfigurationCreateJSON_RPC
                    ((BREthereumNetwork) network,
                     (JsonRpcContext) client,
                     jsonRpcGetBalance,
                     jsonRpcGetGasPrice,
                     jsonRpcEstimateGas,
                     jsonRpcSubmitTransaction,
                     jsonRpcGetTransactions);

    return (jlong) createLightNode(configuration, (*env)->GetStringUTFChars (env, paperKey, 0));
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniLightNodeGetAccount
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniLightNodeGetAccount
    (JNIEnv *env, jobject thisObject) {
  BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);
    return (jlong) lightNodeGetAccount(node);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniLightNodeGetWallet
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniLightNodeGetWallet
        (JNIEnv *env, jobject thisObject) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);
    return (jlong) lightNodeGetWallet(node);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniLightNodeGetWalletToken
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniLightNodeGetWalletToken
        (JNIEnv *env, jobject thisObject, jlong tokenId) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);
    // TODO: Implement
    return (jlong) NULL;
}


/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniLightNodeCreateWalletToken
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniLightNodeCreateWalletToken
        (JNIEnv *env, jobject thisObject, jlong token) {
  BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);
  return (jlong) lightNodeCreateWalletHoldingToken(node, (BREthereumToken) token);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniGetAccountPrimaryAddress
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniGetAccountPrimaryAddress
        (JNIEnv *env, jobject thisObject, jlong account) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);

    BREthereumAddress address = accountGetPrimaryAddress((BREthereumAccount) account);

    char *addressChars = addressAsString(address);

    jstring addressObject = (*env)->NewStringUTF(env, addressChars);
    free(addressChars);

    return addressObject;
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniGetWalletBalance
 * Signature: (JJ)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniGetWalletBalance
        (JNIEnv *env, jobject thisObject, jlong walletId, jlong unit) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);
    BREthereumWallet wallet = (BREthereumWallet) walletId;
    BREthereumAmount balance = walletGetBalance(wallet);

    char *number = (AMOUNT_ETHER == amountGetType(balance)
                    ? etherGetValueString(balance.u.ether, unit)
                    : tokenQuantityGetValueString(balance.u.tokenQuantity, unit));

    jstring result = (*env)->NewStringUTF(env, number);
    free(number);
    return result;
}


/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniForceWalletBalanceUpdate
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniForceWalletBalanceUpdate
        (JNIEnv *env, jobject thisObject, jlong wallet) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);
    BRCoreParseStatus status = CORE_PARSE_OK;
    lightNodeUpdateWalletBalance
            (node,
             (BREthereumLightNodeWalletId) wallet,
             &status);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniForceTransactionUpdate
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniForceTransactionUpdate
        (JNIEnv *env, jobject thisObject) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);
    lightNodeUpdateTransactions(node);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniAnnounceTransaction
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniAnnounceTransaction
        (JNIEnv *env, jobject thisObject,
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
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);
    lightNodeAnnounceTransaction(node,
                                 (*env)->GetStringUTFChars (env, hashObject, 0),
                                 (*env)->GetStringUTFChars (env, toObject, 0),
                                 (*env)->GetStringUTFChars (env, fromObject, 0),
                                 (*env)->GetStringUTFChars (env, contractObject, 0),
                                 (*env)->GetStringUTFChars (env, amountObject, 0),
                                 (*env)->GetStringUTFChars (env, gasLimitObject, 0),
                                 (*env)->GetStringUTFChars (env, gasPriceObject, 0),
                                 (*env)->GetStringUTFChars (env, dataObject, 0),
                                 (*env)->GetStringUTFChars (env, nonceObject, 0),
                                 (*env)->GetStringUTFChars (env, gasUsedObject, 0),
                                 (*env)->GetStringUTFChars (env, blockNumberObject, 0),
                                 (*env)->GetStringUTFChars (env, blockHashObject, 0),
                                 (*env)->GetStringUTFChars (env, blockConfirmationsObject, 0),
                                 (*env)->GetStringUTFChars (env, blockTransactionIndexObject, 0),
                                 (*env)->GetStringUTFChars (env, blockTimestampObject, 0),
                                 (*env)->GetStringUTFChars (env, isErrorObject, 0));
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniCreateTransaction
 * Signature: (JLjava/lang/String;Ljava/lang/String;J)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniCreateTransaction
        (JNIEnv *env, jobject thisObject,
         jlong walletId,
         jstring toObject,
         jstring amountObject,
         jlong amountUnit) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);
    BREthereumWallet wallet = (BREthereumWallet) walletId; // TODO: Unfair to simply cast (but correct at the moment).
    BREthereumToken token = walletGetToken(wallet);

    // Get an actual Amount
    BRCoreParseStatus status = CORE_PARSE_OK;
    const char *amountChars = (*env)->GetStringUTFChars(env, amountObject, 0);
    BREthereumAmount amount = NULL == token
                              ? amountCreateEtherString(amountChars, amountUnit, &status)
                              : amountCreateTokenQuantityString(token, amountChars, amountUnit, 0);

    return (jlong) lightNodeWalletCreateTransaction(node,
                                                    (BREthereumLightNodeWalletId) walletId,
                                                    (*env)->GetStringUTFChars(env, toObject, 0),
                                                    amount);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniSignTransaction
 * Signature: (JJLjava/lang/String;)V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniSignTransaction
        (JNIEnv *env, jobject thisObject,
         jlong walletId,
         jlong transactionId,
         jstring paperKey) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);
    lightNodeWalletSignTransaction(node,
                                   (BREthereumLightNodeWalletId) walletId,
                                   (BREthereumLightNodeTransactionId) transactionId,
                                   (*env)->GetStringUTFChars (env, paperKey, 0));
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniSubmitTransaction
 * Signature: (JJ)V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniSubmitTransaction
        (JNIEnv *env, jobject thisObject,
         jlong walletId,
         jlong transactionId) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);
    lightNodeWalletSubmitTransaction(node,
                                   (BREthereumLightNodeWalletId) walletId,
                                   (BREthereumLightNodeTransactionId) transactionId);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniGetTransactionProperties
 * Signature: (J[J)[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniGetTransactionProperties
        (JNIEnv *env, jobject thisObject,
         jlong transactionId,
         jlongArray propertyIndices) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);

    jsize stringsCount = (*env)->GetArrayLength (env, propertyIndices);
    jobjectArray strings = (*env)->NewObjectArray (env, stringsCount,
                                                   (*env)->FindClass(env, "java/lang/String"),
                                                   0);
    jlong *indices = (*env)->GetLongArrayElements (env, propertyIndices, 0);

    for (int i = 0; i < stringsCount; i++) {
        char *string = lightNodeGetTransactionProperty(node,
                                                       (BREthereumLightNodeWalletId) 0,
                                                       (BREthereumLightNodeTransactionId) transactionId,
                                                       (BREthereumTransactionProperty) indices[i]);
        (*env)->SetObjectArrayElement(env, strings, i, (*env)->NewStringUTF(env, string));
        free(string);
    }
    return strings;
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniGetTransactionAmount
 * Signature: (JJ)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniGetTransactionAmount
        (JNIEnv *env, jobject thisObject, jlong transactionId, jlong unit) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);
    BREthereumTransaction transaction = (BREthereumTransaction) transactionId;
    BREthereumAmount amount = transactionGetAmount(transaction);

    char *amountString = (AMOUNT_ETHER == amountGetType(amount)
                          ? etherGetValueString(amount.u.ether, (BREthereumEtherUnit) unit)
                          : tokenQuantityGetValueString(amount.u.tokenQuantity,
                                                        (BREthereumTokenQuantityUnit) unit));

    jstring amountObject = (*env)->NewStringUTF(env, amountString);

    free(amountString);
    return amountObject;
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniTransactionHasToken
 * Signature: (J)Z
 */
JNIEXPORT jboolean JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniTransactionHasToken
        (JNIEnv *env, jobject thisObject, jlong transactionId) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);
    BREthereumTransaction transaction = (BREthereumTransaction) transactionId;
    return (jboolean) (NULL != transactionGetToken(transaction)
                       ? JNI_TRUE
                       : JNI_FALSE);
}


/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniLightNodeConnect
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniLightNodeConnect
        (JNIEnv *env, jobject thisObject) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);
    return (jboolean) (ETHEREUM_BOOLEAN_TRUE == lightNodeConnect(node) ? JNI_TRUE : JNI_FALSE);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniLightNodeDisconnect
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniLightNodeDisconnect
        (JNIEnv *env, jobject thisObject) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);
    return (jboolean) (ETHEREUM_BOOLEAN_TRUE == lightNodeDisconnect(node) ? JNI_TRUE : JNI_FALSE);
}


/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    initializeNative
 * Signature: ()V
 */
JNIEXPORT void JNICALL 
Java_com_breadwallet_core_ethereum_BREthereumLightNode_initializeNative
  (JNIEnv *env, jclass thisClass) {

//  addressClass = (*env)->FindClass(env, "com/breadwallet/core/BRCoreAddress");
//  assert (NULL != addressClass);
//  addressClass = (*env)->NewGlobalRef (env, addressClass);
//
//  addressConstructor = (*env)->GetMethodID(env, addressClass, "<init>", "(J)V");
//  assert (NULL != addressConstructor);
//
//  transactionClass = (*env)->FindClass (env, "com/breadwallet/core/BRCoreTransaction");
//  assert (NULL != transactionClass);
//  transactionClass = (*env)->NewGlobalRef (env, transactionClass);
//
//  transactionConstructor = (*env)->GetMethodID(env, transactionClass, "<init>", "(J)V");
//  assert (NULL != transactionConstructor);

}

//
// JSON RPC Callback
//
static jmethodID
lookupListenerMethod (JNIEnv *env, jobject listener, char *name, char *type) {
    return (*env)->GetMethodID(env,
                               (*env)->GetObjectClass(env, listener),
                               name,
                               type);
}


static const char *
jsonRpcGetBalance(JsonRpcContext context, BREthereumLightNode node, int id, const char *account) {
    JNIEnv *env = getEnv();
    if (NULL == env) return NULL;

    jobject listener = (*env)->NewLocalRef(env, (jobject) context);
    if ((*env)->IsSameObject(env, listener, NULL)) return NULL; // GC reclaimed

    // String getBalance (int id, String account);
    jmethodID listenerMethod =
            lookupListenerMethod(env, listener,
                                 "getBalance",
                                 "(ILjava/lang/String;)Ljava/lang/String;");
    assert (NULL != listenerMethod);

    jobject accountObject = (*env)->NewStringUTF(env, account);

    jstring resultObject =
            (*env)->CallObjectMethod(env, listener, listenerMethod,
                                     id,
                                     accountObject);

    const char *resultChars = (*env)->GetStringUTFChars(env, resultObject, 0);
    char *result = malloc(strlen(resultChars) + 1);
    strcpy(result, resultChars);

    (*env)->DeleteLocalRef(env, listener);
    (*env)->DeleteLocalRef(env, accountObject);
    (*env)->DeleteLocalRef(env, resultObject);

    return result;
}

static const char *
jsonRpcGetGasPrice (JsonRpcContext context, BREthereumLightNode node, int id) {
    JNIEnv *env = getEnv();
    if (NULL == env) return NULL;

    jobject listener = (*env)->NewLocalRef(env, (jobject) context);
    if ((*env)->IsSameObject(env, listener, NULL)) return NULL; // GC reclaimed

    //String getGasPrice (int id);
    jmethodID listenerMethod =
            lookupListenerMethod(env, listener,
                                 "getGasPrice",
                                 "(I)Ljava/lang/String;");
    assert (NULL != listenerMethod);

    jstring resultObject =
            (*env)->CallObjectMethod(env, listener, listenerMethod,
                                     id);

    const char *resultChars = (*env)->GetStringUTFChars(env, resultObject, 0);
    char *result = malloc(strlen(resultChars) + 1);
    strcpy(result, resultChars);

    (*env)->DeleteLocalRef(env, listener);
    (*env)->DeleteLocalRef(env, resultObject);

    return result;
}

static const char *
jsonRpcEstimateGas (JsonRpcContext context, BREthereumLightNode node, int id, const char *to, const char *amount, const char *data) {
    JNIEnv *env = getEnv();
    if (NULL == env) return NULL;

    jobject listener = (*env)->NewLocalRef(env, (jobject) context);
    if ((*env)->IsSameObject(env, listener, NULL)) return NULL; // GC reclaimed

    // String getGasEstimate (int id, String to, String amount, String data);
    jmethodID listenerMethod =
            lookupListenerMethod(env, listener,
                                 "getGasEstimate",
                                 "(ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
    assert (NULL != listenerMethod);

    jobject toObject = (*env)->NewStringUTF(env, to);
    jobject amountObject = (*env)->NewStringUTF(env, amount);
    jobject dataObject = (*env)->NewStringUTF(env, data);

    jstring resultObject =
            (*env)->CallObjectMethod(env, listener, listenerMethod,
                                     id,
                                     toObject,
                                     amountObject,
                                     dataObject);

    const char *resultChars = (*env)->GetStringUTFChars(env, resultObject, 0);
    char *result = malloc(strlen(resultChars) + 1);
    strcpy(result, resultChars);

    (*env)->DeleteLocalRef(env, listener);
    (*env)->DeleteLocalRef(env, toObject);
    (*env)->DeleteLocalRef(env, amountObject);
    (*env)->DeleteLocalRef(env, dataObject);
    (*env)->DeleteLocalRef(env, resultObject);

    return result;
}

static const char *
jsonRpcSubmitTransaction (JsonRpcContext context, BREthereumLightNode node, int id, const char *transaction) {
    JNIEnv *env = getEnv();
    if (NULL == env) return NULL;

    jobject listener = (*env)->NewLocalRef(env, (jobject) context);
    if ((*env)->IsSameObject(env, listener, NULL)) return NULL; // GC reclaimed

    // String submitTransaction (int id, String rawTransaction);
    jmethodID listenerMethod =
            lookupListenerMethod(env, listener,
                                 "submitTransaction",
                                 "(ILjava/lang/String;)Ljava/lang/String;");
    assert (NULL != listenerMethod);

    jobject transactionObject = (*env)->NewStringUTF(env, transaction);

    jstring resultObject =
            (*env)->CallObjectMethod(env, listener, listenerMethod,
                                     id,
                                     transactionObject);

    const char *resultChars = (*env)->GetStringUTFChars(env, resultObject, 0);
    char *result = malloc(strlen(resultChars) + 1);
    strcpy(result, resultChars);

    (*env)->DeleteLocalRef(env, listener);
    (*env)->DeleteLocalRef(env, transactionObject);
    (*env)->DeleteLocalRef(env, resultObject);

    return result;
}

static void
jsonRpcGetTransactions(JsonRpcContext context, BREthereumLightNode node, int id, const char *address) {
    JNIEnv *env = getEnv();
    if (NULL == env) return;

    jobject listener = (*env)->NewLocalRef(env, (jobject) context);
    if ((*env)->IsSameObject(env, listener, NULL)) return; // GC reclaimed

//    jmethodID assignNodeMethod =
//            lookupListenerMethod(env, listener,
//            "assignNode",
//            "(Lcom/breadwallet/core/ethereum/BREthereumLightNode;)V");
//    assert (NULL != assignNodeMethod);
////    (*env)->CallVoidMethod (env, listener, assignNodeMethod, damn);

    // void getTransactions(int id, String account);
    jmethodID listenerMethod =
            lookupListenerMethod(env, listener,
                                 "getTransactions",
                                 "(ILjava/lang/String;)V");
    assert (NULL != listenerMethod);

    jobject addressObject = (*env)->NewStringUTF(env, address);

    (*env)->CallVoidMethod(env, listener, listenerMethod,
                           id,
                           addressObject);

    (*env)->DeleteLocalRef(env, listener);
    (*env)->DeleteLocalRef(env, addressObject);

    return;
}

