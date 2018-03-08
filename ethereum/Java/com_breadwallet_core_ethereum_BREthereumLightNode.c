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
#include "com_breadwallet_core_ethereum_BREthereumLightNode.h"

//
// Statically Initialize Java References
//
static jclass addressClass;
static jmethodID addressConstructor;

static jclass transactionClass;
static jmethodID transactionConstructor;

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniCreateEthereumLightNode
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL 
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniCreateEthereumLightNode
    (JNIEnv *env, jclass thisClass) {
  BREthereumLightNodeConfiguration configuration;
  BREthereumLightNode node = createLightNode (configuration);
  return (jlong) node;
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniCreateEthereumLightNodeAccount
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL 
Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniCreateEthereumLightNodeAccount
    (JNIEnv *env, jobject thisObject, jstring paperKeyString) {
  BREthereumLightNode node = NULL;

  jstring paperKey = NULL; // (*env)->NewStringUTF (env, paperKeyString);

  BREthereumLightNodeAccountId accountId = lightNodeCreateAccount (node, paperKey);
  //    (*env)->DeleteStringUTF (env, paperKey);
  return (jlong) accountId;

}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    jniGetAccountPrimaryAddress
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_breadwallet_core_ethereum_BREthereumLightNode_jniGetAccountPrimaryAddress
(JNIEnv *env, jobject thisObject, jlong addressId) {
  return NULL;
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumLightNode
 * Method:    initializeNative
 * Signature: ()V
 */
JNIEXPORT void JNICALL 
Java_com_breadwallet_core_ethereum_BREthereumLightNode_initializeNative
  (JNIEnv *env, jclass thisClass) {
}


