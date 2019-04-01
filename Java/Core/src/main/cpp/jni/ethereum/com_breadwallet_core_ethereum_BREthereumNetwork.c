//
//  com_breadwallet_core_ethereum_BREthereumNetwork
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 3/20/18.
//  Copyright (c) 2018 Breadwinner AG.  All right reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include "ethereum/blockchain/BREthereumNetwork.h"
#include "com_breadwallet_core_ethereum_BREthereumNetwork.h"

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumNetwork
 * Method:    jniGetMainnet
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_ethereum_BREthereumNetwork_jniGetMainnet
        (JNIEnv *env, jclass thisClass) {
    return (jlong) ethereumMainnet;
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumNetwork
 * Method:    jniGetTestnet
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_ethereum_BREthereumNetwork_jniGetTestnet
        (JNIEnv *env, jclass thisClass) {
    return (jlong) ethereumTestnet;
}

/*
 * Class:     com_breadwallet_core_ethereum_BREthereumNetwork
 * Method:    jniGetRinkeby
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_ethereum_BREthereumNetwork_jniGetRinkeby
        (JNIEnv *env, jclass thisClass) {
    return (jlong) ethereumRinkeby;
}

