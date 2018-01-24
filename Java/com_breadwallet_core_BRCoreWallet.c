//  Created by Ed Gamble on 1/23/2018
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
#include "BRWallet.h"
#include "BRAddress.h"
#include "BRCoreJni.h"
#include "com_breadwallet_core_BRCoreWallet.h"

/* Forward Declarations */
static void balanceChanged(void *info, uint64_t balance);
static void txAdded(void *info, BRTransaction *tx);
static void txUpdated(void *info, const UInt256 txHashes[], size_t count,
                      uint32_t blockHeight,
                      uint32_t timestamp);
static void txDeleted(void *info, UInt256 txHash, int notifyUser, int recommendRescan);

/*
 * Class:     com_breadwallet_core_BRCoreWallet
 * Method:    createJniCoreWallet
 * Signature: ([Lcom/breadwallet/core/BRCoreTransaction;Lcom/breadwallet/core/BRCoreMasterPubKey;Lcom/breadwallet/core/BRCoreWallet/Listener;)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_BRCoreWallet_createJniCoreWallet
        (JNIEnv *env, jclass thisClass,
         jobjectArray objTransactionsArray,
         jobject objMasterPubKey,
         jobject objListener) {

    BRMasterPubKey *masterPubKey = (BRMasterPubKey *) getJNIReference(env, objMasterPubKey);

    // Transactions
    size_t transactionsCount = (*env)->GetArrayLength(env, objTransactionsArray);
    BRTransaction **transactions = (BRTransaction **) calloc(transactionsCount, sizeof(BRTransaction *));

    for (int index = 0; index < transactionsCount; index++) {
        jobject objTransaction = (*env)->GetObjectArrayElement (env, objTransactionsArray, index);
        BRTransaction *transaction = (BRTransaction *) getJNIReference(env, objTransaction);
        transactions[index] = transaction;
    }

    BRWallet *result = BRWalletNew(transactions, transactionsCount, *masterPubKey);

    // TODO: Reclaim the globalListener
    //   Save in the PeerManager simply as Object; reference then delete on dispose.
    //
    // 'WeakGlobal' allows GC and prevents cross-thread SEGV
    jobject globalListener = (*env)->NewWeakGlobalRef (env, objListener);

    BRWalletSetCallbacks(result, globalListener,
                         balanceChanged,
                         txAdded,
                         txUpdated,
                         txDeleted);

    return (jlong) result;
}

/*
 * Class:     com_breadwallet_core_BRCoreWallet
 * Method:    getReceiveAddress
 * Signature: ()Lcom/breadwallet/core/BRCoreAddress;
 */
JNIEXPORT jobject JNICALL
Java_com_breadwallet_core_BRCoreWallet_getReceiveAddress
        (JNIEnv *env, jobject thisObject) {
    BRWallet *wallet = (BRWallet *) getJNIReference (env, thisObject);

    BRAddress *address = (BRAddress *) malloc (sizeof (BRAddress));
    *address = BRWalletReceiveAddress (wallet);

    jclass addressClass = (*env)->FindClass(env, "com/breadwallet/core/BRCoreAddress");
    jmethodID addressConstructor = (*env)->GetMethodID(env, addressClass, "<init>", "(J)V");
    assert (NULL != addressConstructor);

    return (*env)->NewObject (env, addressClass, addressConstructor, (jlong) address);
}

/*
 * Class:     com_breadwallet_core_BRCoreWallet
 * Method:    getAllAddresses
 * Signature: ()[Lcom/breadwallet/core/BRCoreAddress;
 */
JNIEXPORT jobjectArray JNICALL
Java_com_breadwallet_core_BRCoreWallet_getAllAddresses
        (JNIEnv *env, jobject thisObject) {
    BRWallet *wallet = (BRWallet *) getJNIReference (env, thisObject);

    // Get *all* addresses
    size_t addrCount = BRWalletAllAddrs (wallet, NULL, 0);

    BRAddress *addresses = (BRAddress *) calloc (addrCount, sizeof (BRAddress));
    BRWalletAllAddrs (wallet, addresses, addrCount);

    jclass addrClass = (*env)->FindClass(env, "com/breadwallet/core/BRCoreAddress");
    jmethodID addrConstructor = (*env)->GetMethodID(env, addrClass, "<init>", "(J)V");
    assert (NULL != addrConstructor);

    jobjectArray addrArray = (*env)->NewObjectArray (env, addrCount, addrClass, 0);

    for (int i = 0; i < addrCount; i++) {
        // Get the JNI Reference Address
        BRAddress *address = (BRAddress *) malloc (sizeof (BRAddress));
        *address = addresses[i];

        jobject addrObject = (*env)->NewObject (env, addrClass, addrConstructor, (jlong) address);

        (*env)->SetObjectArrayElement (env, addrArray, i, addrObject);
        (*env)->DeleteLocalRef (env, addrObject);
    }

    return addrArray;
}


/*
 * Class:     com_breadwallet_core_BRCoreWallet
 * Method:    containsAddress
 * Signature: (Lcom/breadwallet/core/BRCoreAddress;)Z
 */
JNIEXPORT jboolean JNICALL
Java_com_breadwallet_core_BRCoreWallet_containsAddress
        (JNIEnv *env, jobject thisObject, jobject objAddress) {
    BRWallet  *wallet  = (BRWallet  *) getJNIReference (env, thisObject);
    BRAddress *address = (BRAddress *) getJNIReference (env, objAddress);

    return (jboolean) BRWalletContainsAddress(wallet, (const char *) address);
}

/*
 * Class:     com_breadwallet_core_BRCoreWallet
 * Method:    addressIsUsed
 * Signature: (Lcom/breadwallet/core/BRCoreAddress;)Z
 */
JNIEXPORT jboolean JNICALL
Java_com_breadwallet_core_BRCoreWallet_addressIsUsed
        (JNIEnv *env, jobject thisObject, jobject objAddress) {
    BRWallet  *wallet  = (BRWallet  *) getJNIReference (env, thisObject);
    BRAddress *address = (BRAddress *) getJNIReference (env, objAddress);

    return (jboolean) BRWalletAddressIsUsed(wallet, (const char *) address);
}

/*
 * Class:     com_breadwallet_core_BRCoreWallet
 * Method:    getTransactions
 * Signature: ()[Lcom/breadwallet/core/BRCoreTransaction;
 */
JNIEXPORT jobjectArray JNICALL
Java_com_breadwallet_core_BRCoreWallet_getTransactions
        (JNIEnv *env, jobject thisObject) {
    BRWallet  *wallet  = (BRWallet  *) getJNIReference (env, thisObject);

    size_t transactionCount = BRWalletTransactions (wallet, NULL, 0);
    BRTransaction **transactions = (BRTransaction **) calloc (transactionCount, sizeof (BRTransaction *));
    BRWalletTransactions (wallet, transactions, transactionCount);

    jclass transactionClass = (*env)->FindClass (env, "com/breadwallet/core/BRCoreTransaction");
    jmethodID transactionConstructor = (*env)->GetMethodID(env, transactionClass, "<init>", "(J)V");
    assert (NULL != transactionConstructor);

    jobjectArray transactionArray = (*env)->NewObjectArray (env, transactionCount, transactionClass, 0);

    for (int index = 0; index < transactionCount; index++) {
        jobject transactionObject =
                (*env)->NewObject (env, transactionClass, transactionConstructor,
                                   (jlong) BRTransactionCopy(transactions[index]));

        (*env)->SetObjectArrayElement (env, transactionArray, index, transactionObject);
        (*env)->DeleteLocalRef (env, transactionObject);
    }

    return transactionArray;
}

/*
 * Class:     com_breadwallet_core_BRCoreWallet
 * Method:    getTransactionsConfirmedBefore
 * Signature: (J)[Lcom/breadwallet/core/BRCoreTransaction;
 */
JNIEXPORT jobjectArray JNICALL
Java_com_breadwallet_core_BRCoreWallet_getTransactionsConfirmedBefore
        (JNIEnv *env, jobject thisObject, jlong blockHeight) {
    BRWallet  *wallet  = (BRWallet  *) getJNIReference (env, thisObject);

    size_t transactionCount = BRWalletTxUnconfirmedBefore (wallet, NULL, 0, blockHeight);
    BRTransaction **transactions = (BRTransaction **) calloc (transactionCount, sizeof (BRTransaction *));
    BRWalletTxUnconfirmedBefore (wallet, transactions, transactionCount, blockHeight);

    jclass transactionClass = (*env)->FindClass (env, "com/breadwallet/core/BRCoreTransaction");
    jmethodID transactionConstructor = (*env)->GetMethodID(env, transactionClass, "<init>", "(J)V");
    assert (NULL != transactionConstructor);

    jobjectArray transactionArray = (*env)->NewObjectArray (env, transactionCount, transactionClass, 0);

    for (int index = 0; index < transactionCount; index++) {
        jobject transactionObject =
                (*env)->NewObject (env, transactionClass, transactionConstructor,
                                   (jlong) BRTransactionCopy(transactions[index]));

        (*env)->SetObjectArrayElement (env, transactionArray, index, transactionObject);
        (*env)->DeleteLocalRef (env, transactionObject);
    }

    return transactionArray;
}

/*
 * Class:     com_breadwallet_core_BRCoreWallet
 * Method:    getBalance
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_BRCoreWallet_getBalance
        (JNIEnv *env, jobject thisObject) {
    BRWallet  *wallet  = (BRWallet  *) getJNIReference (env, thisObject);
    return (jlong) BRWalletBalance (wallet);
}

/*
 * Class:     com_breadwallet_core_BRCoreWallet
 * Method:    getTotalSend
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_BRCoreWallet_getTotalSent
        (JNIEnv *env, jobject thisObject) {
    BRWallet  *wallet  = (BRWallet  *) getJNIReference (env, thisObject);
    return (jlong) BRWalletTotalSent (wallet);
}

/*
 * Class:     com_breadwallet_core_BRCoreWallet
 * Method:    getTotalReceived
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_BRCoreWallet_getTotalReceived
        (JNIEnv *env, jobject thisObject) {
    BRWallet  *wallet  = (BRWallet  *) getJNIReference (env, thisObject);
    return (jlong) BRWalletTotalReceived (wallet);
}

/*
 * Class:     com_breadwallet_core_BRCoreWallet
 * Method:    getFeePerKb
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_BRCoreWallet_getFeePerKb
        (JNIEnv *env, jobject thisObject) {
    BRWallet  *wallet  = (BRWallet  *) getJNIReference (env, thisObject);
    return (jlong) BRWalletFeePerKb (wallet);
}

/*
 * Class:     com_breadwallet_core_BRCoreWallet
 * Method:    setFeePerKb
 * Signature: (J)V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_BRCoreWallet_setFeePerKb
        (JNIEnv *env, jobject thisObject, jlong feePerKb) {
    BRWallet  *wallet  = (BRWallet  *) getJNIReference (env, thisObject);
    BRWalletSetFeePerKb (wallet, feePerKb);
}

/*
 * Class:     com_breadwallet_core_BRCoreWallet
 * Method:    createTransaction
 * Signature: (JLcom/breadwallet/core/BRCoreAddress;)Lcom/breadwallet/core/BRCoreTransaction;
 */
JNIEXPORT jobject JNICALL
Java_com_breadwallet_core_BRCoreWallet_createTransaction
        (JNIEnv *env, jobject thisObject, jlong amount, jobject addressObject) {
    BRWallet  *wallet  = (BRWallet  *) getJNIReference (env, thisObject);
    BRAddress *address = (BRAddress *) getJNIReference (env, addressObject);

    BRTransaction *transaction = BRWalletCreateTransaction (wallet, amount, (const char *) address);

    jclass transactionClass = (*env)->FindClass (env, "com/breadwallet/core/BRCoreTransaction");
    jmethodID transactionConstructor = (*env)->GetMethodID(env, transactionClass, "<init>", "(J)V");
    assert (NULL != transactionConstructor);

    return (*env)->NewObject (env, transactionClass, transactionConstructor, (jlong) transaction);
}

/*
 * Class:     com_breadwallet_core_BRCoreWallet
 * Method:    containsTransaction
 * Signature: (Lcom/breadwallet/core/BRCoreTransaction;)Z
 */
JNIEXPORT jboolean JNICALL
Java_com_breadwallet_core_BRCoreWallet_containsTransaction
        (JNIEnv *env, jobject thisObject, jobject transactionObject) {
    BRWallet  *wallet  = (BRWallet  *) getJNIReference (env, thisObject);
    BRTransaction *transaction = (BRTransaction *) getJNIReference (env, transactionObject);
    return (jboolean) BRWalletContainsTransaction (wallet, transaction);
}

/*
 * Class:     com_breadwallet_core_BRCoreWallet
 * Method:    registerTransaction
 * Signature: (Lcom/breadwallet/core/BRCoreTransaction;)Z
 */
JNIEXPORT jboolean JNICALL
Java_com_breadwallet_core_BRCoreWallet_registerTransaction
        (JNIEnv *env, jobject thisObject, jobject transactionObject) {
    BRWallet  *wallet  = (BRWallet  *) getJNIReference (env, thisObject);
    BRTransaction *transaction = (BRTransaction *) getJNIReference (env, transactionObject);
    return (jboolean) BRWalletRegisterTransaction (wallet, transaction);
}

/*
 * Class:     com_breadwallet_core_BRCoreWallet
 * Method:    removeTranaction
 * Signature: ([B)V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_BRCoreWallet_removeTransaction
        (JNIEnv *env, jobject thisObject, jbyteArray hashByteArray) {
    BRWallet  *wallet  = (BRWallet  *) getJNIReference (env, thisObject);

    uint8_t *hashData = (uint8_t *) (*env)->GetByteArrayElements(env, hashByteArray, 0);

    BRWalletRemoveTransaction (wallet, UInt256Get(hashData));
}

/*
 * Class:     com_breadwallet_core_BRCoreWallet
 * Method:    transactionForHash
 * Signature: ([B)Lcom/breadwallet/core/BRCoreTransaction;
 */
JNIEXPORT jobject JNICALL
Java_com_breadwallet_core_BRCoreWallet_transactionForHash
        (JNIEnv *env, jobject thisObject, jbyteArray hashByteArray) {
    BRWallet  *wallet  = (BRWallet  *) getJNIReference (env, thisObject);

    uint8_t *hashData = (uint8_t *) (*env)->GetByteArrayElements(env, hashByteArray, 0);

    BRTransaction *transaction = BRWalletTransactionForHash(wallet, UInt256Get(hashData));

    jclass transactionClass = (*env)->FindClass (env, "com/breadwallet/core/BRCoreTransaction");
    jmethodID transactionConstructor = (*env)->GetMethodID(env, transactionClass, "<init>", "(J)V");
    assert (NULL != transactionConstructor);

    return (*env)->NewObject (env, transactionClass, transactionConstructor, (jlong) transaction);
}

/*
 * Class:     com_breadwallet_core_BRCoreWallet
 * Method:    transactionIsValid
 * Signature: (Lcom/breadwallet/core/BRCoreTransaction;)I
 */
JNIEXPORT jboolean JNICALL
Java_com_breadwallet_core_BRCoreWallet_transactionIsValid
        (JNIEnv *env, jobject thisObject, jobject transactionObject) {
    BRWallet  *wallet  = (BRWallet  *) getJNIReference (env, thisObject);
    BRTransaction *transaction = (BRTransaction *) getJNIReference (env, transactionObject);
    return (jboolean) BRWalletTransactionIsValid (wallet, transaction);
}

/*
 * Class:     com_breadwallet_core_BRCoreWallet
 * Method:    transactionIsPending
 * Signature: (Lcom/breadwallet/core/BRCoreTransaction;)I
 */
JNIEXPORT jboolean JNICALL
Java_com_breadwallet_core_BRCoreWallet_transactionIsPending
        (JNIEnv *env, jobject thisObject, jobject transactionObject) {
    BRWallet  *wallet  = (BRWallet  *) getJNIReference (env, thisObject);
    BRTransaction *transaction = (BRTransaction *) getJNIReference (env, transactionObject);
    return (jboolean) BRWalletTransactionIsPending (wallet, transaction);
}

/*
 * Class:     com_breadwallet_core_BRCoreWallet
 * Method:    transactionIsVerified
 * Signature: (Lcom/breadwallet/core/BRCoreTransaction;)I
 */
JNIEXPORT jboolean JNICALL
Java_com_breadwallet_core_BRCoreWallet_transactionIsVerified
        (JNIEnv *env, jobject thisObject, jobject transactionObject) {
    BRWallet  *wallet  = (BRWallet  *) getJNIReference (env, thisObject);
    BRTransaction *transaction = (BRTransaction *) getJNIReference (env, transactionObject);
    return (jboolean) BRWalletTransactionIsVerified (wallet, transaction);
}


/*
 * Class:     com_breadwallet_core_BRCoreWallet
 * Method:    transactionFee
 * Signature: (Lcom/breadwallet/core/BRCoreTransaction;)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_BRCoreWallet_getTransactionFee
        (JNIEnv *env, jobject thisObject, jobject transactionObject) {
    BRWallet *wallet = (BRWallet *) getJNIReference (env, thisObject);
    BRTransaction *transaction = (BRTransaction *) getJNIReference (env, transactionObject);
    return BRWalletFeeForTx (wallet, transaction);
}

/*
 * Class:     com_breadwallet_core_BRCoreWallet
 * Method:    transactionAmountSent
 * Signature: (Lcom/breadwallet/core/BRCoreTransaction;)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_BRCoreWallet_getTransactionAmountSent
        (JNIEnv *env, jobject thisObject, jobject transactionObject) {
    BRWallet *wallet = (BRWallet *) getJNIReference (env, thisObject);
    BRTransaction *transaction = (BRTransaction *) getJNIReference (env, transactionObject);
    return BRWalletAmountSentByTx (wallet, transaction);
}

/*
 * Class:     com_breadwallet_core_BRCoreWallet
 * Method:    transactionAmountReceived
 * Signature: (Lcom/breadwallet/core/BRCoreTransaction;)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_BRCoreWallet_getTransactionAmountReceived
        (JNIEnv *env, jobject thisObject, jobject transactionObject) {
    BRWallet *wallet = (BRWallet *) getJNIReference (env, thisObject);
    BRTransaction *transaction = (BRTransaction *) getJNIReference (env, transactionObject);
    return BRWalletAmountReceivedFromTx (wallet, transaction);
}

/*
 * Class:     com_breadwallet_core_BRCoreWallet
 * Method:    getBalanceAfterTransaction
 * Signature: (Lcom/breadwallet/core/BRCoreTransaction;)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_BRCoreWallet_getBalanceAfterTransaction
        (JNIEnv *env, jobject thisObject, jobject transactionObject) {
    BRWallet *wallet = (BRWallet *) getJNIReference (env, thisObject);
    BRTransaction *transaction = (BRTransaction *) getJNIReference (env, transactionObject);
    return (jlong) BRWalletBalanceAfterTx (wallet, transaction);
}

/*
 * Class:     com_breadwallet_core_BRCoreWallet
 * Method:    getFeeForTransactionSize
 * Signature: (I)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_BRCoreWallet_getFeeForTransactionSize
        (JNIEnv *env, jobject thisObject, jint size) {
    BRWallet *wallet = (BRWallet *) getJNIReference (env, thisObject);
    return (jlong) BRWalletFeeForTxSize (wallet, size);
}

/*
 * Class:     com_breadwallet_core_BRCoreWallet
 * Method:    getFeeForTransactionAmount
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_BRCoreWallet_getFeeForTransactionAmount
        (JNIEnv *env, jobject thisObject, jlong amount) {
    BRWallet *wallet = (BRWallet *) getJNIReference (env, thisObject);
    return (jlong) BRWalletFeeForTxAmount (wallet, amount);
}

/*
 * Class:     com_breadwallet_core_BRCoreWallet
 * Method:    getMinOutputAmount
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_com_breadwallet_core_BRCoreWallet_getMinOutputAmount
        (JNIEnv *env, jobject thisObject) {
    BRWallet *wallet = (BRWallet *) getJNIReference (env, thisObject);
    return (jlong) BRWalletMinOutputAmount (wallet);
}

/*
 * Class:     com_breadwallet_core_BRCoreWallet
 * Method:    getMaxOutputAmount
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_com_breadwallet_core_BRCoreWallet_getMaxOutputAmount
        (JNIEnv *env, jobject thisObject) {
    BRWallet *wallet = (BRWallet *) getJNIReference (env, thisObject);
    return (jlong) BRWalletMaxOutputAmount (wallet);
}

/*
 * Class:     com_breadwallet_core_BRCoreWallet
 * Method:    disposeNative
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_BRCoreWallet_disposeNative
        (JNIEnv *env, jobject thisObject) {
    BRWallet *wallet = (BRWallet *) getJNIReference(env, thisObject);
    // TODO: Locate 'globalListener', then DeleteWeakGlobalRef() to save global reference space.
    if (NULL != wallet) BRWalletFree(wallet);
}

//
//
//
static jmethodID
lookupListenerMethod (JNIEnv *env, jobject listener, char *name, char *type) {
    return (*env)->GetMethodID(env,
                               (*env)->GetObjectClass(env, listener),
                               name,
                               type);
}

static void
balanceChanged(void *info, uint64_t balance) {
    JNIEnv *env = getEnv();
    if (NULL == env) return;

    jobject listener = (*env)->NewLocalRef (env, (jobject) info);
    if (NULL == listener) return; // GC reclaimed

    // The onBalanceChanged callback
    jmethodID listenerMethod =
            lookupListenerMethod(env, listener,
                                 "balanceChanged",
                                 "(J)V");
    assert (NULL != listenerMethod);

    (*env)->CallVoidMethod(env, listener, listenerMethod, balance);
    (*env)->DeleteLocalRef (env, listener);
}

static void
txAdded(void *info, BRTransaction *tx) {
    // void onTxAdded(BRCoreTransaction);

    JNIEnv *env = getEnv();
    if (NULL == env) return;

    jobject listener = (*env)->NewLocalRef(env, (jobject) info);
    if (NULL == listener) return; // GC reclaimed

    // The onTxAdded listener
    jmethodID listenerMethod =
            lookupListenerMethod(env, listener,
                                 "onTxAdded",
                                 "(Lcom/breadwallet/core/BRCoreTransaction;)V");
    assert (NULL != listenerMethod);

    jclass transactionClass = (*env)->FindClass (env, "com/breadwallet/core/BRCoreTransaction");
    jmethodID transactionConstructor = (*env)->GetMethodID(env, transactionClass, "<init>", "(J)V");
    assert (NULL != transactionConstructor);

    // Create the BRCoreTransaction
    jobject transaction = (*env)->NewObject (env, transactionClass, transactionConstructor,
                                             (jlong) BRTransactionCopy(tx));

    // Invoke the callback with the provided transaction
    (*env)->CallVoidMethod(env, listener,
                           listenerMethod,
                           transaction);
    (*env)->DeleteLocalRef(env, listener);
}

/*
    uint8_t buf[BRTransactionSerialize(tx, NULL, 0)];
    size_t len = BRTransactionSerialize(tx, buf, sizeof(buf));
    uint64_t fee = (BRWalletFeeForTx(_wallet, tx) == -1) ? 0 : BRWalletFeeForTx(_wallet, tx);
    jlong amount;

    if (BRWalletAmountSentByTx(_wallet, tx) == 0) {
        amount = (jlong) BRWalletAmountReceivedFromTx(_wallet, tx);
    } else {
        amount = (jlong) (
                (BRWalletAmountSentByTx(_wallet, tx) - BRWalletAmountReceivedFromTx(_wallet, tx) -
                 fee) * -1);
    }

    jbyteArray result = (*env)->NewByteArray(env, (jsize) len);

    (*env)->SetByteArrayRegion(env, result, 0, (jsize) len, (jbyte *) buf);

    UInt256 transactionHash = tx->txHash;
    const char *strHash = u256hex(transactionHash);
    jstring jstrHash = (*env)->NewStringUTF(env, strHash);

    (*env)->CallStaticVoidMethod(env, _walletManagerClass, mid,
                                 result,
                                 (jint) tx->blockHeight,
                                 (jlong) tx->timestamp,
                                 (jlong) amount, jstrHash);
    (*env)->DeleteLocalRef(env, jstrHash);
*/

static void
txUpdated(void *info, const UInt256 txHashes[], size_t count, uint32_t blockHeight,
                      uint32_t timestamp) {

    JNIEnv *env = getEnv();
    if (NULL == env) return;

    jobject listener = (*env)->NewLocalRef (env, (jobject) info);
    if (NULL == listener) return; // GC reclaimed

    // The onTxUpdated callback
    jmethodID listenerMethod =
            lookupListenerMethod(env, listener,
                                 "onTxUpdated",
                                 "(Ljava/lang/String;II)V");
    assert (NULL != listenerMethod);

    // Invoke the callback for each of txHashes.
    for (size_t i = 0; i < count; i++) {
        jstring hash = (*env)->NewStringUTF(env, u256hex(txHashes[i]));

        (*env)->CallVoidMethod(env, listener, listenerMethod,
                               hash,
                               blockHeight,
                               timestamp);
        (*env)->DeleteLocalRef(env, hash);
    }
    (*env)->DeleteLocalRef(env, listener);
}

static void
txDeleted(void *info, UInt256 txHash, int notifyUser, int recommendRescan) {
    JNIEnv *env = getEnv();
    if (NULL == env) return;

    jobject listener = (*env)->NewLocalRef (env, (jobject) info);
    if (NULL == listener) return; // GC reclaimed

    // The onTxDeleted callback
    jmethodID listenerMethod =
            lookupListenerMethod(env, listener,
                                 "onTxDeleted",
                                 "(Ljava/lang/String;II)V");
    assert (NULL != listenerMethod);

    // Invoke the callback for the provided txHash
    jstring hash = (*env)->NewStringUTF(env, u256hex(txHash));

    (*env)->CallVoidMethod(env, listener, listenerMethod,
                           hash,
                           notifyUser,
                           recommendRescan);

    (*env)->DeleteLocalRef(env, hash);
    (*env)->DeleteLocalRef(env, listener);
}
