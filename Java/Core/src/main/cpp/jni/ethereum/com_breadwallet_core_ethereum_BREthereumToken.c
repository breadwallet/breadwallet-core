//
//  com_breadwallet_core_ethereum_BREthereumToken
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 3/20/18.
//  Copyright (c) 2018 Breadwinner AG.  All right reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include "../BRCoreJni.h"
#include "ethereum/contract/BREthereumToken.h"
#include "com_breadwallet_core_ethereum_BREthereumToken.h"

#if defined (BITCOIN_TESTNET) && 1 == BITCOIN_TESTNET
static const char *tokenBRDAddress = "0x7108ca7c4718efa810457f228305c9c71390931a"; // testnet
#else
static const char *tokenBRDAddress = "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6"; // mainnet
#endif

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumToken
 * Method:    getAddress
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_com_breadwallet_core_ethereum_BREthereumToken_getAddress
        (JNIEnv *env, jobject thisObject) {
    BREthereumToken token = (BREthereumToken) getJNIReference(env, thisObject);
    return (*env)->NewStringUTF(env, tokenGetAddress(token));
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumToken
 * Method:    getSymbol
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_com_breadwallet_core_ethereum_BREthereumToken_getSymbol
        (JNIEnv *env, jobject thisObject) {
    BREthereumToken token = (BREthereumToken) getJNIReference(env, thisObject);
    return (*env)->NewStringUTF(env, tokenGetSymbol(token));
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumToken
 * Method:    getName
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_com_breadwallet_core_ethereum_BREthereumToken_getName
        (JNIEnv *env, jobject thisObject)  {
    BREthereumToken token = (BREthereumToken) getJNIReference(env, thisObject);
    return (*env)->NewStringUTF(env, tokenGetName(token));
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumToken
 * Method:    getDescription
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_com_breadwallet_core_ethereum_BREthereumToken_getDescription
        (JNIEnv *env, jobject thisObject) {
    BREthereumToken token = (BREthereumToken) getJNIReference(env, thisObject);
    return (*env)->NewStringUTF(env, tokenGetDescription(token));
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumToken
 * Method:    getDecimals
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_com_breadwallet_core_ethereum_BREthereumToken_getDecimals
        (JNIEnv *env, jobject thisObject) {
    BREthereumToken token = (BREthereumToken) getJNIReference(env, thisObject);
    return tokenGetDecimals (token);
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumToken
 * Method:    jniGetTokenBRD
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_ethereum_BREthereumToken_jniGetTokenBRD
        (JNIEnv *env, jclass thisClass) {
//    return (jlong) tokenLookup(tokenBRDAddress);
    return 0;
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumToken
 * Method:    jniTokenAll
 * Signature: ()[J
 */
JNIEXPORT jlongArray JNICALL
Java_com_breadwallet_core_ethereum_BREthereumToken_jniTokenAll
        (JNIEnv *env, jclass thisClass) {
//    int count = tokenCount();
//
//    // A uint32_t array on x86 platforms - we *require* a long array
//    BREthereumToken *tokens = tokenGetAll();
//
//    jlong ids[count];
//    for (int i = 0; i < count; i++) ids[i] = (jlong) tokens[i];
//
//    jlongArray result = (*env)->NewLongArray (env, count);
//    (*env)->SetLongArrayRegion (env, result, 0, count, ids);
//
//    free (tokens);
//    return result;
    return (*env)->NewLongArray (env, 0);
}
