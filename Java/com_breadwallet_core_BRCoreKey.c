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

#include "BRCoreJni.h"
#include <BRKey.h>
#include <BRAddress.h>
#include <malloc.h>
#include <assert.h>
#include "com_breadwallet_core_BRCoreKey.h"

/*
 * Class:     com_breadwallet_core_BRCoreKey
 * Method:    getSecret
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_breadwallet_core_BRCoreKey_getSecret
        (JNIEnv *env, jobject thisObject) {
    BRKey *key = (BRKey *) getJNIReference(env, thisObject);

    size_t secretLen = sizeof (UInt256);

    jbyteArray result = (*env)->NewByteArray(env, (jsize) secretLen);
    (*env)->SetByteArrayRegion(env, result, 0, (jsize) secretLen,
                               (const jbyte *) key->secret.u8);
    return result;
}

/*
 * Class:     com_breadwallet_core_BRCoreKey
 * Method:    getPubKey
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_breadwallet_core_BRCoreKey_getPubKey
        (JNIEnv *env, jobject thisObject) {
    BRKey *key = (BRKey *) getJNIReference(env, thisObject);

    size_t pubKeyLen = sizeof (65 * sizeof(uint8_t));

    jbyteArray result = (*env)->NewByteArray(env, (jsize) pubKeyLen);
    (*env)->SetByteArrayRegion(env, result, 0, (jsize) pubKeyLen,
                               (const jbyte *) key->pubKey);
    return result;
}

/*
 * Class:     com_breadwallet_core_BRCoreKey
 * Method:    getCompressed
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_breadwallet_core_BRCoreKey_getCompressed
        (JNIEnv *env, jobject thisObject) {
    BRKey *key = (BRKey *) getJNIReference(env, thisObject);
    return (jint) key->compressed;
}

/*
 * Class:     com_breadwallet_core_BRCoreKey
 * Method:    createJniCoreKey
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_BRCoreKey_createJniCoreKey
        (JNIEnv *env, jclass thisClass) {
    BRKey *key = (BRKey *) calloc (1, sizeof(BRKey));
    return (jlong) key;
}

/*
 * Class:     com_breadwallet_core_BRCoreKey
 * Method:    setPrivKey
 * Signature: ([B)Z
 */
JNIEXPORT jboolean JNICALL
Java_com_breadwallet_core_BRCoreKey_setPrivKey
        (JNIEnv *env, jobject thisObject, jbyteArray privateKeyByteArray) {
    BRKey *key = (BRKey *) getJNIReference(env, thisObject);

    const char *privateKey = (const char *)
            (*env)->GetByteArrayElements(env, privateKeyByteArray, 0);

    return 1 == BRKeySetPrivKey(key, privateKey)
           ? JNI_TRUE
           : JNI_FALSE;
}

/*
 * Class:     com_breadwallet_core_BRCoreKey
 * Method:    setSecret
 * Signature: ([BZ)V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_BRCoreKey_setSecret
        (JNIEnv *env, jobject thisObject, jbyteArray secretByteArray, jboolean compressed) {
    BRKey *key = (BRKey *) getJNIReference(env, thisObject);

    const char *secretKey = (const char *)
            (*env)->GetByteArrayElements(env, secretByteArray, 0);

    BRKeySetSecret (key, (const UInt256 *) secretKey, JNI_TRUE == compressed);
}

/*
 * Class:     com_breadwallet_core_BRCoreKey
 * Method:    compactSign
 * Signature: ([B)[B
 */
JNIEXPORT jbyteArray JNICALL
Java_com_breadwallet_core_BRCoreKey_compactSign
        (JNIEnv *env, jobject thisObject, jbyteArray dataByteArray) {
    BRKey *key = (BRKey *) getJNIReference(env, thisObject);

    uint8_t *data = (uint8_t *) (*env)->GetByteArrayElements(env, dataByteArray, 0);
    UInt256 md32 = UInt256Get(data);

    size_t sigLen = BRKeyCompactSign(key, NULL, 0, md32);
    uint8_t compactSig[sigLen];
    sigLen = BRKeyCompactSign(key, compactSig, sizeof(compactSig), md32);

    jbyteArray result = (*env)->NewByteArray(env, (jsize) sigLen);
    (*env)->SetByteArrayRegion(env, result, 0, (jsize) sigLen, (const jbyte *) compactSig);

    return result;
}

/*
 * Class:     com_breadwallet_core_BRCoreKey
 * Method:    encryptNative
 * Signature: ([B[B)[B
 */
JNIEXPORT jbyteArray JNICALL
Java_com_breadwallet_core_BRCoreKey_encryptNative
        (JNIEnv *env, jobject thisObject, jbyteArray dataByteArray, jbyteArray nonceByteArray) {
    BRKey *key = (BRKey *) getJNIReference(env, thisObject);

    jbyte *data = (*env)->GetByteArrayElements(env, dataByteArray, NULL);
    jsize dataSize = (*env)->GetArrayLength(env, dataByteArray);

    jbyte *nonce = (*env)->GetByteArrayElements(env, nonceByteArray, NULL);
    jsize nonceSize = (*env)->GetArrayLength(env, nonceByteArray);

    uint8_t out[16 + dataSize];

    size_t outSize = BRChacha20Poly1305AEADEncrypt(out, sizeof(out), key,
                                                   (uint8_t *) nonce,
                                                   (uint8_t *) data,
                                                   (size_t) dataSize,
                                                   NULL,
                                                   0);

    jbyteArray result = (*env)->NewByteArray(env, (jsize) outSize);
    (*env)->SetByteArrayRegion(env, result, 0, (jsize) outSize, (const jbyte *) out);

    (*env)->ReleaseByteArrayElements(env, dataByteArray, data, 0);
    (*env)->ReleaseByteArrayElements(env, nonceByteArray, nonce, 0);

    return result;
}

/*
 * Class:     com_breadwallet_core_BRCoreKey
 * Method:    decryptNative
 * Signature: ([B[B)[B
 */
JNIEXPORT jbyteArray JNICALL
Java_com_breadwallet_core_BRCoreKey_decryptNative
        (JNIEnv *env, jobject thisObject, jbyteArray dataByteArray, jbyteArray nonceByteArray) {
    BRKey *key = (BRKey *) getJNIReference(env, thisObject);

    jbyte *data = (*env)->GetByteArrayElements(env, dataByteArray, NULL);
    jsize dataSize = (*env)->GetArrayLength(env, dataByteArray);

    jbyte *nonce = (*env)->GetByteArrayElements(env, nonceByteArray, NULL);
    jsize nonceSize = (*env)->GetArrayLength(env, nonceByteArray);

    uint8_t out[dataSize];

    size_t outSize = BRChacha20Poly1305AEADDecrypt(out, sizeof(out), key,
                                                   (uint8_t *) nonce,
                                                   (uint8_t *) data,
                                                   (size_t) (dataSize),
                                                   NULL,
                                                   0);

    if (sizeof(out) == 0) return NULL;

    jbyteArray result = (*env)->NewByteArray(env, (jsize) outSize);
    (*env)->SetByteArrayRegion(env, result, 0, (jsize) outSize, (const jbyte *) out);

    (*env)->ReleaseByteArrayElements(env, dataByteArray, data, 0);
    (*env)->ReleaseByteArrayElements(env, nonceByteArray, nonce, 0);

    return result;
}
/*
 * Class:     com_breadwallet_core_BRCoreKey
 * Method:    address
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_breadwallet_core_BRCoreKey_address
        (JNIEnv *env, jobject thisObject) {
    BRKey *key = (BRKey *) getJNIReference(env, thisObject);

    BRAddress address = BR_ADDRESS_NONE;
    BRKeyAddress (key, address.s, sizeof(address));
    assert(address.s[0] != '\0');

    return (*env)->NewStringUTF(env, address.s);
}
