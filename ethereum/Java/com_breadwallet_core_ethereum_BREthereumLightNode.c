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
jsonRpcGetBalance (JsonRpcContext context, int id, const char *account);

static const char *
jsonRpcGetGasPrice (JsonRpcContext context, int id);

static const char *
jsonRpcEstimateGas (JsonRpcContext context, int id, const char *to, const char *amount, const char *data);

static const char *
jsonRpcSubmitTransaction (JsonRpcContext context, int id, const char *transaction);

static void
jsonRpcGetTransactions(JsonRpcContext context, int id, const char *account);

//
// Statically Initialize Java References
//
static jclass addressClass;
static jmethodID addressConstructor;

static jclass transactionClass;
static jmethodID transactionConstructor;

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniCreateEthereumLightNodeLES
 * Signature: (Lcom/breadwallet/core/ethereum/BREthereumLightNode/Client;J)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniCreateEthereumLightNodeLES
        (JNIEnv *env, jclass thisClass, jobject clientObject, jlong network) {
    BREthereumLightNodeConfiguration configuration =
            lightNodeConfigurationCreateLES((BREthereumNetwork) network, 0);
    return (jlong) createLightNode(configuration);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniCreateEthereumLightNodeJSON_RPC
 * Signature: (Lcom/breadwallet/core/ethereum/BREthereumLightNode/Client;J)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniCreateEthereumLightNodeJSON_1RPC
        (JNIEnv *env, jclass thisClass, jobject clientObject, jlong network) {

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

    return (jlong) createLightNode(configuration);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniCreateEthereumLightNodeAccount
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL 
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniCreateEthereumLightNodeAccount
    (JNIEnv *env, jobject thisObject, jstring paperKeyString) {
  BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);

  const char *paperKey = (*env)->GetStringUTFChars (env, paperKeyString, 0);
  BREthereumLightNodeAccountId accountId = lightNodeCreateAccount (node, paperKey);

  return (jlong) accountId;

}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniCreateEthereumLightNodeWallet
 * Signature: (JJ)J
 */
JNIEXPORT jlong JNICALL Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniCreateEthereumLightNodeWallet
        (JNIEnv *env, jobject thisObject, jlong account, jlong network) {

  BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);

  return (jlong) lightNodeCreateWallet(node,
                                       (BREthereumLightNodeAccountId) account,
                                       (BREthereumNetwork) network);
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

    char *addressChars = lightNodeGetAccountPrimaryAddress
            (node, (BREthereumLightNodeAccountId) account);
    jstring address = (*env)->NewStringUTF(env, addressChars);
    free(addressChars);

    return address;
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
 * Method:    jniForceWalletTransactionUpdate
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniForceWalletTransactionUpdate
        (JNIEnv *env, jobject thisObject, jlong wallet) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);
    lightNodeWalletUpdateTransactions(node, (BREthereumLightNodeWalletId) wallet);
}


/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniAnnounceTransaction
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniAnnounceTransaction
        (JNIEnv *env, jobject thisObject,
         jstring toObject,
         jstring fromObject,
         jstring contractObject,
         jstring amountObject,
         jstring gasLimitObject,
         jstring gasPriceObject,
         jstring dataObject,
         jstring nonceObject) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);

    lightNodeAnnounceTransaction(node,
                                 (*env)->GetStringUTFChars (env, toObject, 0),
                                 (*env)->GetStringUTFChars (env, fromObject, 0),
                                 (*env)->GetStringUTFChars (env, contractObject, 0),
                                 (*env)->GetStringUTFChars (env, amountObject, 0),
                                 (*env)->GetStringUTFChars (env, gasLimitObject, 0),
                                 (*env)->GetStringUTFChars (env, gasPriceObject, 0),
                                 (*env)->GetStringUTFChars (env, dataObject, 0),
                                 (*env)->GetStringUTFChars (env, nonceObject, 0));
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
jsonRpcGetBalance(JsonRpcContext context, int id, const char *account) {
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
jsonRpcGetGasPrice (JsonRpcContext context, int id) {
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
jsonRpcEstimateGas (JsonRpcContext context, int id, const char *to, const char *amount, const char *data) {
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
jsonRpcSubmitTransaction (JsonRpcContext context, int id, const char *transaction) {
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
jsonRpcGetTransactions(JsonRpcContext context, int id, const char *address) {
    JNIEnv *env = getEnv();
    if (NULL == env) return;

    jobject listener = (*env)->NewLocalRef(env, (jobject) context);
    if ((*env)->IsSameObject(env, listener, NULL)) return; // GC reclaimed

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

