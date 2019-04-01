//  Created by Ed Gamble on 1/31/2018
//  Copyright (c) 2018 Breadwinner AG.  All right reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "BRCoreJni.h"
#include "bitcoin/BRTransaction.h"
#include "com_breadwallet_core_BRCoreTransactionOutput.h"

/*
 * Class:     com_breadwallet_core_BRCoreTransactionOutput
 * Method:    createTransactionOutput
 * Signature: (J[B)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_BRCoreTransactionOutput_createTransactionOutput
        (JNIEnv *env, jclass thisClass,
         jlong amount,
         jbyteArray scriptByteArray) {
    BRTxOutput *output = (BRTxOutput *) calloc(1, sizeof(BRTxOutput));

    // script
    output->script = NULL;
    size_t scriptLen = (size_t) (*env)->GetArrayLength(env, scriptByteArray);
    const uint8_t *script = (const uint8_t *)
            (0 == scriptLen
             ? NULL
             : (*env)->GetByteArrayElements(env, scriptByteArray, 0));
    BRTxOutputSetScript(output, script, scriptLen);

    output->amount = (uint64_t) amount;

    return (jlong) output;
}

/*
 * Class:     com_breadwallet_core_BRCoreTransactionOutput
 * Method:    getAddress
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_com_breadwallet_core_BRCoreTransactionOutput_getAddress
        (JNIEnv *env, jobject thisObject) {
    BRTxOutput *output = (BRTxOutput *) getJNIReference (env, thisObject);

    size_t addressLen = sizeof (output->address);
    char address[1 + addressLen];
    memcpy (address, output->address, addressLen);
    address[addressLen] = '\0';

    return (*env)->NewStringUTF (env, address);
}

/*
 * Class:     com_breadwallet_core_BRCoreTransactionOutput
 * Method:    setAddress
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_BRCoreTransactionOutput_setAddress
        (JNIEnv *env, jobject thisObject, jstring addressObject) {
    BRTxOutput *output = (BRTxOutput *) getJNIReference (env, thisObject);

    size_t addressLen = sizeof (output->address);

    size_t addressDataLen = (size_t) (*env)->GetStringLength (env, addressObject);
    const jchar *addressData = (*env)->GetStringChars (env, addressObject, 0);
    assert (addressDataLen <= addressLen);

    memset (output->address, '\0', addressLen);
    memcpy (output->address, addressData, addressDataLen);

}

/*
 * Class:     com_breadwallet_core_BRCoreTransactionOutput
 * Method:    getAmount
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_BRCoreTransactionOutput_getAmount
        (JNIEnv *env, jobject thisObject) {
    BRTxOutput *output = (BRTxOutput *) getJNIReference (env, thisObject);
    return (jlong) output->amount;
}

/*
 * Class:     com_breadwallet_core_BRCoreTransactionOutput
 * Method:    setAmount
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_breadwallet_core_BRCoreTransactionOutput_setAmount
        (JNIEnv *env, jobject thisObject, jlong amount) {
    BRTxOutput *output = (BRTxOutput *) getJNIReference (env, thisObject);
    output->amount = (uint64_t) amount;
}

/*
 * Class:     com_breadwallet_core_BRCoreTransactionOutput
 * Method:    getScript
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_breadwallet_core_BRCoreTransactionOutput_getScript
        (JNIEnv *env, jobject thisObject) {
    BRTxOutput *output = (BRTxOutput *) getJNIReference (env, thisObject);

    jbyteArray scriptByteArray = (*env)->NewByteArray (env, output->scriptLen);
    (*env)->SetByteArrayRegion (env, scriptByteArray, 0, output->scriptLen,
                                (const jbyte *) output->script);

    return scriptByteArray;
}
