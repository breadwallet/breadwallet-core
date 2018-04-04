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
static void
jsonRpcGetBalance (JsonRpcContext context,
                   BREthereumLightNode node,
                   BREthereumLightNodeWalletId wid,
                   const char *address,
                   int id);

static void
jsonRpcGetGasPrice (JsonRpcContext context,
                    BREthereumLightNode node,
                    BREthereumLightNodeWalletId wid,
                    int id);

static void
jsonRpcEstimateGas (JsonRpcContext context,
                    BREthereumLightNode node,
                    BREthereumLightNodeWalletId wid,
                    BREthereumLightNodeTransactionId tid,
                    const char *to,
                    const char *amount,
                    const char *data,
                    int id);

static void
jsonRpcSubmitTransaction (JsonRpcContext context,
                          BREthereumLightNode node,
                          BREthereumLightNodeWalletId wid,
                          BREthereumLightNodeTransactionId tid,
                          const char *transaction,
                          int id);

static void
jsonRpcGetTransactions(JsonRpcContext context,
                       BREthereumLightNode node,
                       const char *account,
                       int id);

static jstring
asJniString(JNIEnv *env, char *string) {
    jstring result = (*env)->NewStringUTF(env, string);
    free(string);
    return result;
}

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
 * Method:    jniCreateLightNodeLES_PublicKey
 * Signature: (Lcom/breadwallet/core/ethereum/BREthereumLightNode/Client;J[B)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniCreateLightNodeLES_1PublicKey
        (JNIEnv *env, jclass thisClass, jobject clientObject, jlong network, jbyteArray publicKey) {

    assert (65 == (*env)->GetArrayLength(env, publicKey));

     BREthereumLightNodeConfiguration configuration =
            lightNodeConfigurationCreateLES((BREthereumNetwork) network, 0);

    jbyte *publicKeyBytes = (*env)->GetByteArrayElements(env, publicKey, 0);
    BREthereumLightNode node =
            createLightNodeWithPublicKey(configuration, (uint8_t *) publicKeyBytes);
    (*env)->ReleaseByteArrayElements(env, publicKey, publicKeyBytes, 0);
    return (jlong) node;
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
 * Method:    jniCreateLightNodeJSON_RPC_PublicKey
 * Signature: (Lcom/breadwallet/core/ethereum/BREthereumLightNode/Client;J[B)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniCreateLightNodeJSON_1RPC_1PublicKey
    (JNIEnv *env, jclass thisClass, jobject clientObject, jlong network, jbyteArray publicKey) {

    assert (65 == (*env)->GetArrayLength(env, publicKey));

    // Get a global reference to client; ensure the client exists in callback threads.
    jobject client = (*env)->NewGlobalRef(env, clientObject);

    BREthereumLightNodeConfiguration configuration =
            lightNodeConfigurationCreateJSON_RPC
                    ((BREthereumNetwork) network,
                     (JsonRpcContext) client,
                     jsonRpcGetBalance,
                     jsonRpcGetGasPrice,
                     jsonRpcEstimateGas,
                     jsonRpcSubmitTransaction,
                     jsonRpcGetTransactions);

    jbyte *publicKeyBytes = (*env)->GetByteArrayElements(env, publicKey, 0);
    BREthereumLightNode node =
            createLightNodeWithPublicKey(configuration, (uint8_t *) publicKeyBytes);
    (*env)->ReleaseByteArrayElements(env, publicKey, publicKeyBytes, 0);
    return (jlong) node;
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

    char *addressChars = lightNodeGetAccountPrimaryAddress(node);
    jstring addressObject = (*env)->NewStringUTF(env, addressChars);
    free(addressChars);

    return addressObject;
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniGetAccountPrimaryAddressPublicKey
 * Signature: (J)[B
 */
JNIEXPORT jbyteArray JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniGetAccountPrimaryAddressPublicKey
        (JNIEnv *env, jobject thisObject, jlong account) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);

    uint8_t *publicKeyBytes = lightNodeGetAccountPrimaryAddressPublicKey(node);
    jbyteArray publicKey = (*env)->NewByteArray (env, 65);
    (*env)->SetByteArrayRegion (env, publicKey, 0, 65, (const jbyte *) publicKeyBytes);
    free (publicKeyBytes);

    return publicKey;
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniGetWalletBalance
 * Signature: (JJ)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniGetWalletBalance
        (JNIEnv *env, jobject thisObject, jlong wid, jlong unit) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);
    BREthereumAmount balance = lightNodeWalletGetBalance(node, wid);

    char *number = (AMOUNT_ETHER == amountGetType(balance)
                    ? etherGetValueString(balance.u.ether, unit)
                    : tokenQuantityGetValueString(balance.u.tokenQuantity, unit));

    jstring result = (*env)->NewStringUTF(env, number);
    free(number);
    return result;
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniEstimateWalletGasPrice
 * Signature: (J)V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniEstimateWalletGasPrice
        (JNIEnv *env, jobject thisObject, jlong wid) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);

    lightNodeUpdateWalletDefaultGasPrice
            (node,
             (BREthereumLightNodeWalletId) wid);
}


/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniForceWalletBalanceUpdate
 * Signature: (J)V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniForceWalletBalanceUpdate
        (JNIEnv *env, jobject thisObject, jlong wid) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);
    lightNodeUpdateWalletBalance
            (node,
             (BREthereumLightNodeWalletId) wid);
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
 * Signature: (ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniAnnounceTransaction
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
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);
    lightNodeAnnounceTransaction(node, id,
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
 * Method:    jniAnnounceBalance
 * Signature: (ILjava/lang/String;I)V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniAnnounceBalance
        (JNIEnv *env, jobject thisObject,
         jint wid,
         jstring balance,
         jint rid) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);
    lightNodeAnnounceBalance(node,
                             wid,
                             (*env)->GetStringUTFChars(env, balance, 0),
                             rid);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniAnnounceGasPrice
 * Signature: (ILjava/lang/String;I)V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniAnnounceGasPrice
        (JNIEnv *env, jobject thisObject,
         jint wid,
         jstring gasPrice,
         jint rid) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);
    lightNodeAnnounceGasPrice(node,
                              wid,
                              (*env)->GetStringUTFChars(env, gasPrice, 0),
                              rid);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniAnnounceGasEstimate
 * Signature: (ILjava/lang/String;I)V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniAnnounceGasEstimate
        (JNIEnv *env, jobject thisObject,
         jint tid,
         jstring gasEstimate,
         jint rid) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);
    lightNodeAnnounceGasEstimate(node,
                                 tid,
                                 (*env)->GetStringUTFChars(env, gasEstimate, 0),
                                 rid);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniAnnounceSubmitTransaction
 * Signature: (ILjava/lang/String;I)V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniAnnounceSubmitTransaction
        (JNIEnv *env, jobject thisObject,
         jint tid,
         jstring hash,
         jint rid) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);
    lightNodeAnnounceSubmitTransaction(node,
                                       tid,
                                       (*env)->GetStringUTFChars(env, hash, 0),
                                       rid);
}


/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniCreateTransaction
 * Signature: (JLjava/lang/String;Ljava/lang/String;J)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniCreateTransaction
        (JNIEnv *env, jobject thisObject,
         jlong wid,
         jstring toObject,
         jstring amountObject,
         jlong amountUnit) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);
    BREthereumToken token = lightNodeWalletGetToken(node, wid);

        // Get an actual Amount
    BRCoreParseStatus status = CORE_PARSE_OK;
    const char *amountChars = (*env)->GetStringUTFChars(env, amountObject, 0);
    BREthereumAmount amount = NULL == token
                              ? amountCreateEtherString(amountChars, amountUnit, &status)
                              : amountCreateTokenQuantityString(token, amountChars, amountUnit, 0);

    return (jlong) lightNodeWalletCreateTransaction(node,
                                                    (BREthereumLightNodeWalletId) wid,
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
 * Method:    jniGetTransactions
 * Signature: (J)[J
 */
JNIEXPORT jlongArray JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniGetTransactions
        (JNIEnv *env, jobject thisObject,
         jlong wid) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);
    int count = lightNodeWalletGetTransactionCount(node, wid);
    assert (-1 != count);

    // uint32_t array - need a long
    BREthereumLightNodeTransactionId *transactionIds =
            lightNodeWalletGetTransactions(node, (BREthereumLightNodeWalletId) wid);

    jlong ids[count];
    for (int i = 0; i < count; i++) ids[i] = (jlong) transactionIds[i];

    jlongArray transactions = (*env)->NewLongArray (env, (jsize) count);
    (*env)->SetLongArrayRegion (env, transactions, 0, (jsize) count, (jlong*) ids);

    free (transactionIds);
    return transactions;
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniTransactionGetAmount
 * Signature: (JJ)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniTransactionGetAmount
        (JNIEnv *env, jobject thisObject,
         jlong tid,
         jlong unit) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);

    return asJniString(env,
                       (ETHEREUM_BOOLEAN_TRUE == lightNodeTransactionHoldsToken(node, tid, NULL)
                        ? lightNodeTransactionGetAmountEther(node, tid,
                                                             (BREthereumEtherUnit) unit)
                        : lightNodeTransactionGetAmountTokenQuantity(node, tid,
                                                                     (BREthereumTokenQuantityUnit) unit)));
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniTransactionGetFee
 * Signature: (JJ)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniTransactionGetFee
        (JNIEnv *env, jobject thisObject,
         jlong tid,
         jlong unit) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);

    int overflow = 0;
    BREthereumEther fee = lightNodeTransactionGetFee(node,
                                                     (BREthereumLightNodeTransactionId) tid,
                                                     &overflow);

    // Return the FEE in `resultUnit`
    char *feeString = (0 != overflow
                       ? ""
                       : lightNodeCoerceEtherAmountToString(node, fee,
                                                            (BREthereumEtherUnit) unit));
    jstring result = (*env)->NewStringUTF(env, feeString);
    if (0 != strcmp("", feeString)) free(feeString);

    return result;
}


/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniTransactionHasToken
 * Signature: (J)Z
 */
JNIEXPORT jboolean JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniTransactionHasToken
        (JNIEnv *env, jobject thisObject,
         jlong tid) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);
    return (jboolean) (ETHEREUM_BOOLEAN_FALSE == lightNodeTransactionHoldsToken(node, tid, NULL)
                       ? JNI_TRUE
                       : JNI_FALSE);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniTransactionEstimateGas
 * Signature: (JJ)V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniTransactionEstimateGas
        (JNIEnv *env, jobject thisObject,
         jlong walletId,
         jlong transactionId) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);

    lightNodeUpdateTransactionGasEstimate
            (node,
             (BREthereumLightNodeWalletId) walletId,
             (BREthereumLightNodeTransactionId) transactionId);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniTransactionEstimateFee
 * Signature: (JLjava/lang/String;JJ)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniTransactionEstimateFee
        (JNIEnv *env, jobject thisObject,
         jlong wid,
         jstring amountString,
         jlong amountUnit,
         jlong resultUnit) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);

    int overflow;
    const char *number = (*env)->GetStringUTFChars(env, amountString, 0);
    BRCoreParseStatus status;

    // Get the `amount` as ETHER or TOKEN QUANTITY
    BREthereumToken token = lightNodeWalletGetToken(node, (BREthereumLightNodeWalletId) wid);
    BREthereumAmount amount = (NULL == token
                               ? lightNodeCreateEtherAmountString(node, number,
                                                                  (BREthereumEtherUnit) amountUnit,
                                                                  &status)
                               : lightNodeCreateTokenAmountString(node, token, number,
                                                                  (BREthereumTokenQuantityUnit) amountUnit,
                                                                  &status));
    (*env)->ReleaseStringUTFChars(env, amountString, number);

    // Get the estimated FEE
    BREthereumEther fee = lightNodeWalletEstimateTransactionFee(node,
                                                                (BREthereumLightNodeWalletId) wid,
                                                                amount, &overflow);

    // Return the FEE in `resultUnit`
    char *feeString = (status != CORE_PARSE_OK || 0 != overflow
                       ? ""
                       : lightNodeCoerceEtherAmountToString(node, fee,
                                                            (BREthereumEtherUnit) resultUnit));

    jstring result = (*env)->NewStringUTF(env, feeString);
    if (0 != strcmp("", feeString)) free(feeString);

    return result;
}


/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniTransactionSourceAddress
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniTransactionSourceAddress
        (JNIEnv *env, jobject thisObject, jlong transactionId) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);
    return asJniString(env, lightNodeTransactionGetSendAddress
            (node,
             (BREthereumLightNodeTransactionId) transactionId));
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniTransactionTargetAddress
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniTransactionTargetAddress
        (JNIEnv *env, jobject thisObject, jlong transactionId) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);
    return asJniString(env, lightNodeTransactionGetRecvAddress
            (node,
             (BREthereumLightNodeTransactionId) transactionId));
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniTransactionGetHash
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniTransactionGetHash
        (JNIEnv *env, jobject thisObject, jlong transactionId) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);
    return asJniString(env, lightNodeTransactionGetHash
            (node,
             (BREthereumLightNodeTransactionId) transactionId));
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniTransactionGetGasPrice
 * Signature: (JJ)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniTransactionGetGasPrice
        (JNIEnv *env, jobject thisObject, jlong transactionId, jlong unit) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);
    return asJniString(env, lightNodeTransactionGetGasPrice
            (node,
             (BREthereumLightNodeTransactionId) transactionId,
             (BREthereumEtherUnit) unit));
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniTransactionGetGasLimit
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniTransactionGetGasLimit
        (JNIEnv *env, jobject thisObject, jlong transactionId) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);
    return (jlong) lightNodeTransactionGetGasLimit
            (node,
             (BREthereumLightNodeTransactionId) transactionId);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniTransactionGetGasUsed
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniTransactionGetGasUsed
        (JNIEnv *env, jobject thisObject, jlong transactionId) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);
    return (jlong) lightNodeTransactionGetGasUsed
            (node,
             (BREthereumLightNodeTransactionId) transactionId);

}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniTransactionGetNonce
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniTransactionGetNonce
        (JNIEnv *env, jobject thisObject, jlong transactionId) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);
    return (jlong) lightNodeTransactionGetNonce
            (node,
             (BREthereumLightNodeTransactionId) transactionId);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniTransactionGetBlockNumber
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniTransactionGetBlockNumber
        (JNIEnv *env, jobject thisObject, jlong transactionId) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);
    return (jlong) lightNodeTransactionGetBlockNumber
            (node,
             (BREthereumLightNodeTransactionId) transactionId);

}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniTransactionGetBlockTimestamp
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniTransactionGetBlockTimestamp
        (JNIEnv *env, jobject thisObject, jlong transactionId) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);
    return (jlong) lightNodeTransactionGetBlockTimestamp
            (node,
             (BREthereumLightNodeTransactionId) transactionId);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniTransactionIsConfirmed
 * Signature: (J)Z
 */
JNIEXPORT jboolean JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniTransactionIsConfirmed
        (JNIEnv *env, jobject thisObject, jlong transactionId) {
    BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);
    return (jboolean) (ETHEREUM_BOOLEAN_TRUE ==
                       lightNodeTransactionIsConfirmed
                               (node,
                                (BREthereumLightNodeTransactionId) transactionId)
                       ? JNI_TRUE
                       : JNI_FALSE);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniTransactionIsSubmitted
 * Signature: (J)Z
 */
JNIEXPORT jboolean JNICALL
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniTransactionIsSubmitted
    (JNIEnv *env, jobject thisObject, jlong transactionId) {
        BREthereumLightNode node = (BREthereumLightNode) getJNIReference(env, thisObject);
        return (jboolean) (ETHEREUM_BOOLEAN_TRUE ==
                           lightNodeTransactionIsSubmitted
                                   (node,
                                    (BREthereumLightNodeTransactionId) transactionId)
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


static void
jsonRpcGetBalance(JsonRpcContext context,
                  BREthereumLightNode node,
                  BREthereumLightNodeWalletId wid,
                  const char *account,
                  int id) {
    JNIEnv *env = getEnv();
    if (NULL == env) return;

    jobject listener = (*env)->NewLocalRef(env, (jobject) context);
    if ((*env)->IsSameObject(env, listener, NULL)) return; // GC reclaimed

    // String getBalance (int id, String account);
    jmethodID listenerMethod =
            lookupListenerMethod(env, listener,
                                 "getBalance",
                                 "(ILjava/lang/String;I)V");
    assert (NULL != listenerMethod);

    jobject accountObject = (*env)->NewStringUTF(env, account);

    (*env)->CallVoidMethod(env, listener, listenerMethod,
                           wid,
                           accountObject,
                           id);

    (*env)->DeleteLocalRef(env, listener);
    (*env)->DeleteLocalRef(env, accountObject);
}

static void
jsonRpcGetGasPrice(JsonRpcContext context,
                   BREthereumLightNode node,
                   BREthereumLightNodeWalletId wid,
                   int id) {
    JNIEnv *env = getEnv();
    if (NULL == env) return;

    jobject listener = (*env)->NewLocalRef(env, (jobject) context);
    if ((*env)->IsSameObject(env, listener, NULL)) return; // GC reclaimed

    //String getGasPrice (int id);
    jmethodID listenerMethod =
            lookupListenerMethod(env, listener,
                                 "getGasPrice",
                                 "(II)V");
    assert (NULL != listenerMethod);

    (*env)->CallVoidMethod(env, listener, listenerMethod,
                           wid,
                           id);

    (*env)->DeleteLocalRef(env, listener);
}

static void
jsonRpcEstimateGas(JsonRpcContext context, BREthereumLightNode node,
                   BREthereumLightNodeWalletId wid,
                   BREthereumLightNodeTransactionId tid,
                   const char *to,
                   const char *amount,
                   const char *data,
                   int id) {
    JNIEnv *env = getEnv();
    if (NULL == env) return;

    jobject listener = (*env)->NewLocalRef(env, (jobject) context);
    if ((*env)->IsSameObject(env, listener, NULL)) return; // GC reclaimed

    // String getGasEstimate (int id, String to, String amount, String data);
    jmethodID listenerMethod =
            lookupListenerMethod(env, listener,
                                 "getGasEstimate",
                                 "(IILjava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V");
    assert (NULL != listenerMethod);

    jobject toObject = (*env)->NewStringUTF(env, to);
    jobject amountObject = (*env)->NewStringUTF(env, amount);
    jobject dataObject = (*env)->NewStringUTF(env, data);

    (*env)->CallVoidMethod(env, listener, listenerMethod,
                           wid,
                           tid,
                           toObject,
                           amountObject,
                           dataObject,
                           id);

    (*env)->DeleteLocalRef(env, listener);
    (*env)->DeleteLocalRef(env, toObject);
    (*env)->DeleteLocalRef(env, amountObject);
    (*env)->DeleteLocalRef(env, dataObject);
}

static void
jsonRpcSubmitTransaction(JsonRpcContext context,
                         BREthereumLightNode node,
                         BREthereumLightNodeWalletId wid,
                         BREthereumLightNodeTransactionId tid,
                         const char *transaction,
                         int id) {
    JNIEnv *env = getEnv();
    if (NULL == env) return;

    jobject listener = (*env)->NewLocalRef(env, (jobject) context);
    if ((*env)->IsSameObject(env, listener, NULL)) return; // GC reclaimed

    // String submitTransaction (int id, String rawTransaction);
    jmethodID listenerMethod =
            lookupListenerMethod(env, listener,
                                 "submitTransaction",
                                 "(IILjava/lang/String;I)V");
    assert (NULL != listenerMethod);

    jobject transactionObject = (*env)->NewStringUTF(env, transaction);

    (*env)->CallVoidMethod(env, listener, listenerMethod,
                             wid,
                             tid,
                             transactionObject,
                             id);

    (*env)->DeleteLocalRef(env, listener);
    (*env)->DeleteLocalRef(env, transactionObject);
}

static void
jsonRpcGetTransactions(JsonRpcContext context,
                       BREthereumLightNode node,
                       const char *address,
                       int id) {
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
                                 "(Ljava/lang/String;I)V");
    assert (NULL != listenerMethod);

    jobject addressObject = (*env)->NewStringUTF(env, address);

    (*env)->CallVoidMethod(env, listener, listenerMethod,
                           addressObject,
                           id);

    (*env)->DeleteLocalRef(env, listener);
    (*env)->DeleteLocalRef(env, addressObject);
}

