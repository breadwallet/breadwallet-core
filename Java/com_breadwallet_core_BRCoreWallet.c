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

//
// Statically Initialize Java References
//
static jclass addressClass;
static jmethodID addressConstructor;

static jclass transactionClass;
static jmethodID transactionConstructor;


/*
 * Class:     com_breadwallet_core_BRCoreWallet
 * Method:    createJniCoreWallet
 * Signature: ([Lcom/breadwallet/core/BRCoreTransaction;Lcom/breadwallet/core/BRCoreMasterPubKey;Lcom/breadwallet/core/BRCoreWallet/Listener;)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_BRCoreWallet_createJniCoreWallet
        (JNIEnv *env, jclass thisClass,
         jobjectArray objTransactionsArray,
         jobject objMasterPubKey) {

    BRMasterPubKey *masterPubKey = (BRMasterPubKey *) getJNIReference(env, objMasterPubKey);

    // Transactions
    size_t transactionsCount = (*env)->GetArrayLength(env, objTransactionsArray);
    BRTransaction **transactions = (BRTransaction **) calloc(transactionsCount, sizeof(BRTransaction *));

    for (int index = 0; index < transactionsCount; index++) {
        jobject objTransaction = (*env)->GetObjectArrayElement (env, objTransactionsArray, index);
        BRTransaction *transaction = (BRTransaction *) getJNIReference(env, objTransaction);
        transactions[index] = transaction;
    }

    return (jlong) BRWalletNew(transactions, transactionsCount, *masterPubKey);
}

/*
 * Class:     com_breadwallet_core_BRCoreWallet
 * Method:    installListener
 * Signature: (Lcom/breadwallet/core/BRCoreWallet/Listener;)V
 */
JNIEXPORT void JNICALL Java_com_breadwallet_core_BRCoreWallet_installListener
        (JNIEnv *env, jobject thisObject, jobject listenerObject) {

    BRWallet *wallet = (BRWallet *) getJNIReference (env, thisObject);

    // TODO: Reclaim the globalListener
    //   Save in the PeerManager simply as Object; reference then delete on dispose.
    //
    // 'WeakGlobal' allows GC and prevents cross-thread SEGV
    jobject listenerWeakRefGlobal = (*env)->NewWeakGlobalRef (env, listenerObject);

    jfieldID listenerField = (*env)->GetFieldID (env, (*env)->GetObjectClass (env, thisObject),
                                                 "listener",
                                                 "Ljava/lang/ref/WeakReference;");
    assert (NULL != listenerField);
    (*env)->SetObjectField (env, thisObject, listenerField, listenerWeakRefGlobal);

    BRWalletSetCallbacks(wallet, listenerWeakRefGlobal,
                         balanceChanged,
                         txAdded,
                         txUpdated,
                         txDeleted);
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

    jobjectArray addrArray = (*env)->NewObjectArray (env, addrCount, addressClass, 0);

    for (int i = 0; i < addrCount; i++) {
        // Get the JNI Reference Address
        BRAddress *address = (BRAddress *) malloc (sizeof (BRAddress));
        *address = addresses[i];

        jobject addrObject = (*env)->NewObject (env, addressClass, addressConstructor, (jlong) address);

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
 * Method:    getMaxFeePerKb
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_com_breadwallet_core_BRCoreWallet_getMaxFeePerKb
        (JNIEnv *env, jobject thisObject) {
    return MAX_FEE_PER_KB;
}

/*
 * Class:     com_breadwallet_core_BRCoreWallet
 * Method:    getDefaultFeePerKb
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_com_breadwallet_core_BRCoreWallet_getDefaultFeePerKb
        (JNIEnv *env, jobject thisObject) {
    return DEFAULT_FEE_PER_KB;
}

/*
 * Class:     com_breadwallet_core_BRCoreWallet
 * Method:    createTransaction
 * Signature: (JLcom/breadwallet/core/BRCoreAddress;)Lcom/breadwallet/core/BRCoreTransaction;
 */
JNIEXPORT jobject JNICALL
Java_com_breadwallet_core_BRCoreWallet_createTransaction
        (JNIEnv *env, jobject thisObject, jlong amount, jobject addressObject) {
    BRWallet *wallet = (BRWallet *) getJNIReference(env, thisObject);
    BRAddress *address = (BRAddress *) getJNIReference(env, addressObject);

    // transaction may be NULL - like if the wallet does not have a large enough balance
    // to cover the transaction amount
    BRTransaction *transaction = BRWalletCreateTransaction(wallet,
                                                           (uint64_t) amount,
                                                           (const char *) address->s);

    return NULL == transaction
           ? NULL
           : (*env)->NewObject(env, transactionClass, transactionConstructor, (jlong) transaction);
}

/*
 * Class:     com_breadwallet_core_BRCoreWallet
 * Method:    signTransaction
 * Signature: (Lcom/breadwallet/core/BRCoreTransaction;I[B)Z
 */
JNIEXPORT jboolean JNICALL
Java_com_breadwallet_core_BRCoreWallet_signTransaction
        (JNIEnv *env, jobject thisObject,
         jobject transactionObject,
         jint forkId,
         jbyteArray seedByteArray) {
    BRWallet *wallet = (BRWallet *) getJNIReference(env, thisObject);
    BRTransaction *transaction = (BRTransaction *) getJNIReference(env, transactionObject);

    size_t seedLen = (size_t) (*env)->GetArrayLength(env, seedByteArray);
    const void *seed = (const void *) (*env)->GetByteArrayElements(env, seedByteArray, 0);

    return (jboolean) (1 == BRWalletSignTransaction(wallet, transaction, forkId, seed, seedLen)
                       ? JNI_TRUE
                       : JNI_FALSE);
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
 * Method:    removeTransaction
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
 * Method:    updateTransactions
 * Signature: ([[BJJ)V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_BRCoreWallet_updateTransactions
        (JNIEnv *env, jobject thisObject,
         jobjectArray transactionsHashesArray,
         jlong blockHeight,
         jlong timestamp) {
    BRWallet  *wallet  = (BRWallet  *) getJNIReference (env, thisObject);

    size_t txCount = (size_t) (*env)->GetArrayLength (env, transactionsHashesArray);
    UInt256 txHashes[txCount];

    for (int i = 0; i < txCount; i++) {
        jbyteArray txHashByteArray = (jbyteArray) (*env)->GetObjectArrayElement (env, transactionsHashesArray, 0);
        const jbyte *txHashBytes = (*env)->GetByteArrayElements (env, txHashByteArray, 0);
        txHashes[i] = UInt256Get(txHashBytes);
    }
    BRWalletUpdateTransactions(wallet, txHashes, txCount, (uint32_t) blockHeight, (uint32_t) timestamp);
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

    return (*env)->NewObject (env, transactionClass, transactionConstructor,
                              (jlong) BRWalletTransactionForHash(wallet, UInt256Get(hashData)));
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

    // Locate 'globalListener', then DeleteWeakGlobalRef() to save global reference space.
    if (NULL != wallet) {
        jfieldID listenerField = (*env)->GetFieldID (env, (*env)->GetObjectClass (env, thisObject),
                                                     "listener",
                                                     "Ljava/lang/ref/WeakReference;");
        assert (NULL != listenerField);

        jweak listenerWeakGlobalRef = (*env)->GetObjectField (env, thisObject, listenerField);
        if (JNIWeakGlobalRefType == (*env)->GetObjectRefType (env, listenerWeakGlobalRef)) {
            (*env)->DeleteWeakGlobalRef (env, listenerWeakGlobalRef);
        }

        BRWalletFree(wallet);
    }
}

/*
 * Class:     com_breadwallet_core_BRCoreWallet
 * Method:    initializeNative
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_breadwallet_core_BRCoreWallet_initializeNative
        (JNIEnv *env, jclass thisClass) {
    addressClass = (*env)->FindClass(env, "com/breadwallet/core/BRCoreAddress");
    assert (NULL != addressClass);
    addressClass = (*env)->NewGlobalRef (env, addressClass);

    addressConstructor = (*env)->GetMethodID(env, addressClass, "<init>", "(J)V");
    assert (NULL != addressConstructor);

    transactionClass = (*env)->FindClass (env, "com/breadwallet/core/BRCoreTransaction");
    assert (NULL != transactionClass);
    transactionClass = (*env)->NewGlobalRef (env, transactionClass);

    transactionConstructor = (*env)->GetMethodID(env, transactionClass, "<init>", "(J)V");
    assert (NULL != transactionConstructor);
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
    if ((*env)->IsSameObject (env, listener, NULL)) return; // GC reclaimed

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
    if ((*env)->IsSameObject (env, listener, NULL)) return; // GC reclaimed

    // The onTxAdded listener
    jmethodID listenerMethod =
            lookupListenerMethod(env, listener,
                                 "onTxAdded",
                                 "(Lcom/breadwallet/core/BRCoreTransaction;)V");
    assert (NULL != listenerMethod);

    // Create the BRCoreTransaction
    jobject transaction = (*env)->NewObject (env, transactionClass, transactionConstructor,
                                             (jlong) BRTransactionCopy(tx));

    // Invoke the callback with the provided transaction
    (*env)->CallVoidMethod(env, listener,
                           listenerMethod,
                           transaction);
    (*env)->DeleteLocalRef(env, listener);
}

static void
txUpdated(void *info, const UInt256 txHashes[], size_t count, uint32_t blockHeight,
                      uint32_t timestamp) {

    JNIEnv *env = getEnv();
    if (NULL == env) return;

    jobject listener = (*env)->NewLocalRef (env, (jobject) info);
    if ((*env)->IsSameObject (env, listener, NULL)) return; // GC reclaimed

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
    if ((*env)->IsSameObject (env, listener, NULL)) return; // GC reclaimed

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
