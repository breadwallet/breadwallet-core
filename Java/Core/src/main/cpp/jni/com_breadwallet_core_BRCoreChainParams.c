//  Created by Ed Gamble on 1/23/2018
//  Copyright (c) 2018 Breadwinner AG.  All right reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <stdlib.h>
#include <string.h>
#include "BRCoreJni.h"
#include "bitcoin/BRChainParams.h"
#include "bcash/BRBCashParams.h"
#include "com_breadwallet_core_BRCoreChainParams.h"

/*
 * Class:     com_breadwallet_core_BRCoreChainParams
 * Method:    getJniMagicNumber
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_breadwallet_core_BRCoreChainParams_getJniMagicNumber
        (JNIEnv *env, jobject thisObject)
{
    BRChainParams *params = (BRChainParams *) getJNIReference(env, thisObject);
    return params->magicNumber;
}

/*
 * Class:     com_breadwallet_core_BRCoreChainParams
 * Method:    createJniMainnetChainParams
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_com_breadwallet_core_BRCoreChainParams_createJniMainnetChainParams
        (JNIEnv *env, jclass thisClass) {
    BRChainParams *result = (BRChainParams *) calloc (1, sizeof (BRChainParams));
    memcpy (result, BRMainNetParams, sizeof (BRChainParams));
    return (jlong) result;
}

/*
 * Class:     com_breadwallet_core_BRCoreChainParams
 * Method:    createJniTestnetChainParams
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_com_breadwallet_core_BRCoreChainParams_createJniTestnetChainParams
        (JNIEnv *env, jclass thisClass) {
    BRChainParams *result = (BRChainParams *) calloc (1, sizeof (BRChainParams));
    memcpy (result, BRTestNetParams, sizeof (BRChainParams));
    return (jlong) result;
}

/*
 * Class:     com_breadwallet_core_BRCoreChainParams
 * Method:    createJniMainnetBcashChainParams
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_com_breadwallet_core_BRCoreChainParams_createJniMainnetBcashChainParams
        (JNIEnv *env, jclass thisClass) {
    BRChainParams *result = (BRChainParams *) calloc (1, sizeof (BRChainParams));
    memcpy (result, BRBCashParams, sizeof (BRChainParams));
    return (jlong) result;
}

/*
 * Class:     com_breadwallet_core_BRCoreChainParams
 * Method:    createJniTestnetBcashChainParams
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_com_breadwallet_core_BRCoreChainParams_createJniTestnetBcashChainParams
        (JNIEnv *env, jclass thisClass) {
    BRChainParams *result = (BRChainParams *) calloc(1, sizeof(BRChainParams));
    memcpy(result, BRBCashTestNetParams, sizeof(BRChainParams));
    return (jlong) result;
}
