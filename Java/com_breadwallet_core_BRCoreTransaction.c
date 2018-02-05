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

#include <BRTransaction.h>
#include <stdlib.h>
#include <assert.h>
#include <malloc.h>
#include <BRInt.h>
#include "BRCoreJni.h"
#include "com_breadwallet_core_BRCoreTransaction.h"

/*
 * Class:     com_breadwallet_core_BRCoreTransaction
 * Method:    getHash
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL
Java_com_breadwallet_core_BRCoreTransaction_getHash
        (JNIEnv *env, jobject thisObject) {
    BRTransaction *transaction = (BRTransaction *) getJNIReference (env, thisObject);

    UInt256 transactionHash = transaction->txHash;

    jbyteArray hashByteArray = (*env)->NewByteArray (env, sizeof (UInt256));
    (*env)->SetByteArrayRegion (env, hashByteArray, 0, sizeof (UInt256), transactionHash.u8);

//    const char *strHash = u256hex(transactionHash);
//    return (*env)->NewStringUTF(env, strHash);

    return hashByteArray;
}


/*
 * Class:     com_breadwallet_core_BRCoreTransaction
 * Method:    getVersion
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_com_breadwallet_core_BRCoreTransaction_getVersion
        (JNIEnv *env, jobject thisObject) {
    BRTransaction *transaction = (BRTransaction *) getJNIReference (env, thisObject);
    return (jlong) transaction->version;
}

/*
 * Class:     com_breadwallet_core_BRCoreTransaction
 * Method:    getInputs
 * Signature: ()[Lcom/breadwallet/core/BRCoreTransactionInput;
 */
JNIEXPORT jobjectArray JNICALL Java_com_breadwallet_core_BRCoreTransaction_getInputs
        (JNIEnv *env, jobject thisObject) {
    BRTransaction *transaction = (BRTransaction *) getJNIReference (env, thisObject);

    size_t inputCount = transaction->inCount;

    jclass inputClass = (*env)->FindClass(env, "com/breadwallet/core/BRCoreTransactionInput");
    jmethodID inputConstructor = (*env)->GetMethodID(env, inputClass, "<init>", "(J)V");
    assert (NULL != inputConstructor);

    jobjectArray inputs = (*env)->NewObjectArray (env, inputCount, inputClass, 0);

    for (int i = 0; i < inputCount; i++) {
        BRTxInput *input = (BRTxInput *) calloc (1, sizeof (BRTxInput));
        BRTxInputCopy(input, &transaction->inputs[i]);

        jobject inputObject = (*env)->NewObject (env, inputClass, inputConstructor, (jlong) input);
        (*env)->SetObjectArrayElement (env, inputs, i, inputObject);

        (*env)->DeleteLocalRef (env, inputObject);
    }

    return inputs;
}

/*
 * Class:     com_breadwallet_core_BRCoreTransaction
 * Method:    getOutputs
 * Signature: ()[Lcom/breadwallet/core/BRCoreTransactionOutput;
 */
JNIEXPORT jobjectArray JNICALL Java_com_breadwallet_core_BRCoreTransaction_getOutputs
        (JNIEnv *env, jobject thisObject) {
    BRTransaction *transaction = (BRTransaction *) getJNIReference (env, thisObject);

    size_t outputCount = transaction->outCount;

    jclass outputClass = (*env)->FindClass(env, "com/breadwallet/core/BRCoreTransactionOutput");
    jmethodID outputConstructor = (*env)->GetMethodID(env, outputClass, "<init>", "(J)V");
    assert (NULL != outputConstructor);

    jobjectArray outputs = (*env)->NewObjectArray (env, outputCount, outputClass, 0);

    for (int i = 0; i < outputCount; i++) {
        BRTxOutput *output = (BRTxOutput *) calloc (1, sizeof (BRTxOutput));
        BRTxOutputCopy(output, &transaction->outputs[i]);

        jobject outputObject = (*env)->NewObject (env, outputClass, outputConstructor, (jlong) output);
        (*env)->SetObjectArrayElement (env, outputs, i, outputObject);

        (*env)->DeleteLocalRef (env, outputObject);
    }

    return outputs;
}


/*
 * Class:     com_breadwallet_core_BRCoreTransaction
 * Method:    getLockTime
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_com_breadwallet_core_BRCoreTransaction_getLockTime
        (JNIEnv *env, jobject thisObject) {
    BRTransaction *transaction = (BRTransaction *) getJNIReference (env, thisObject);
    return (jlong) transaction->lockTime;
}

/*
 * Class:     com_breadwallet_core_BRCoreTransaction
 * Method:    getBlockHeight
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_com_breadwallet_core_BRCoreTransaction_getBlockHeight
        (JNIEnv *env, jobject thisObject) {
    BRTransaction *transaction = (BRTransaction *) getJNIReference (env, thisObject);
    return (jlong) transaction->blockHeight;
}

/*
 * Class:     com_breadwallet_core_BRCoreTransaction
 * Method:    getTimestamp
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_BRCoreTransaction_getTimestamp
        (JNIEnv *env, jobject thisObject) {
    BRTransaction *transaction = (BRTransaction *) getJNIReference (env, thisObject);
    return (jlong) transaction->timestamp;
}

/*
 * Class:     com_breadwallet_core_BRCoreTransaction
 * Method:    serialize
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_breadwallet_core_BRCoreTransaction_serialize
        (JNIEnv *env, jobject thisObject) {
    BRTransaction *transaction = (BRTransaction *) getJNIReference (env, thisObject);

    size_t byteArraySize = 1;
    jbyteArray  byteArray         = (*env)->NewByteArray (env, byteArraySize);
    jbyte      *byteArrayElements = (*env)->GetByteArrayElements (env, byteArray, JNI_FALSE);

    BRTransactionSerialize(transaction, (uint8_t *) byteArrayElements, byteArraySize);

    // Ensure ELEMENTS 'written' back to byteArray
    (*env)->ReleaseByteArrayElements (env, byteArray, byteArrayElements, JNI_COMMIT);

    return byteArray;
}

/*
 * Class:     com_breadwallet_core_BRCoreTransaction
 * Method:    addInput
 * Signature: (Lcom/breadwallet/core/BRCoreTransactionInput;)V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_BRCoreTransaction_addInput
        (JNIEnv *env, jobject thisObject, jobject transactionInputObject) {
    BRTransaction *transaction = (BRTransaction *) getJNIReference (env, thisObject);
    BRTxInput *input = (BRTxInput *) getJNIReference (env, transactionInputObject);

    BRTransactionAddInput (transaction, input->txHash, input->index, input->amount,
                           input->script, input->scriptLen,
                           input->signature, input->sigLen,
                           input->sequence);
}

/*
 * Class:     com_breadwallet_core_BRCoreTransaction
 * Method:    addOutput
 * Signature: (Lcom/breadwallet/core/BRCoreTransactionOutput;)V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_BRCoreTransaction_addOutput
        (JNIEnv *env, jobject thisObject, jobject transactionOutputObject) {
    BRTransaction *transaction = (BRTransaction *) getJNIReference(env, thisObject);
    BRTxOutput *output = (BRTxOutput *) getJNIReference(env, transactionOutputObject);

    BRTransactionAddOutput(transaction, output->amount,
                           output->script, output->scriptLen);
}


/*
 * Class:     com_breadwallet_core_BRCoreTransaction
 * Method:    shuffleOutputs
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_breadwallet_core_BRCoreTransaction_shuffleOutputs
        (JNIEnv *env, jobject thisObject) {
    BRTransaction *transaction = (BRTransaction *) getJNIReference (env, thisObject);
    BRTransactionShuffleOutputs (transaction);
}

/*
 * Class:     com_breadwallet_core_BRCoreTransaction
 * Method:    getSize
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_com_breadwallet_core_BRCoreTransaction_getSize
        (JNIEnv *env, jobject thisObject) {
    BRTransaction *transaction = (BRTransaction *) getJNIReference (env, thisObject);
    return (jlong) BRTransactionSize (transaction);
}

/*
 * Class:     com_breadwallet_core_BRCoreTransaction
 * Method:    getStandardFee
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_com_breadwallet_core_BRCoreTransaction_getStandardFee
        (JNIEnv *env, jobject thisObject) {
    BRTransaction *transaction = (BRTransaction *) getJNIReference (env, thisObject);
    return (jlong) BRTransactionStandardFee (transaction);
}

/*
 * Class:     com_breadwallet_core_BRCoreTransaction
 * Method:    isSigned
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_breadwallet_core_BRCoreTransaction_isSigned
        (JNIEnv *env, jobject thisObject) {
    BRTransaction *transaction = (BRTransaction *) getJNIReference (env, thisObject);
    return (jboolean) BRTransactionIsSigned (transaction);
}

/*
 * Class:     com_breadwallet_core_BRCoreTransaction
 * Method:    isStandard
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_breadwallet_core_BRCoreTransaction_isStandard
        (JNIEnv *env, jobject thisObject) {
    BRTransaction *transaction = (BRTransaction *) getJNIReference (env, thisObject);
    return (jboolean) BRTransactionIsStandard (transaction);
}

/*
 * Class:     com_breadwallet_core_BRCoreTransaction
 * Method:    getMinOutputAmount
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_com_breadwallet_core_BRCoreTransaction_getMinOutputAmount
        (JNIEnv *env, jclass thisClass) {
    return TX_MIN_OUTPUT_AMOUNT;
}

/*
 * Class:     com_breadwallet_core_BRCoreTransaction
 * Method:    disposeNative
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_BRCoreTransaction_disposeNative
        (JNIEnv *env, jobject thisObject) {
    BRTransaction *transaction = (BRTransaction *) getJNIReference (env, thisObject);
    if (NULL != transaction) BRTransactionFree(transaction);
}

/*
 * Class:     com_breadwallet_core_BRCoreTransaction
 * Method:    createJniCoreTransaction
 * Signature: ([BJJ)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_BRCoreTransaction_createJniCoreTransaction
        (JNIEnv *env, jclass thisClass,
         jbyteArray transactionByteArray,
         jlong blockHeight,
         jlong timestamp) {

    // static native long createJniCoreTransaction (byte[] buffer, long blockHeight, long timeStamp);
    size_t transactionSize = (size_t) (*env)->GetArrayLength (env, transactionByteArray);
    const uint8_t *transactionData = (const uint8_t *) (*env)->GetByteArrayElements (env, transactionByteArray, 0);

    BRTransaction *transaction = BRTransactionParse(transactionData, transactionSize);
    assert (NULL != transaction);

    transaction->blockHeight = (uint32_t) blockHeight;
    transaction->timestamp =(uint32_t) timestamp;

    return (jlong) transaction;
}

/*
 * Class:     com_breadwallet_core_BRCoreTransaction
 * Method:    createJniCoreTransactionSerialized
 * Signature: ([B)J
 */
JNIEXPORT jlong JNICALL Java_com_breadwallet_core_BRCoreTransaction_createJniCoreTransactionSerialized
        (JNIEnv *env, jclass thisClass, jbyteArray transactionByteArray) {

    // static native long createJniCoreTransaction (byte[] buffer, long blockHeight, long timeStamp);
    size_t transactionSize = (size_t) (*env)->GetArrayLength (env, transactionByteArray);
    const uint8_t *transactionData = (const uint8_t *) (*env)->GetByteArrayElements (env, transactionByteArray, 0);

    BRTransaction *transaction = BRTransactionParse(transactionData, transactionSize);
    assert (NULL != transaction);

    return (jlong) transaction;
}
