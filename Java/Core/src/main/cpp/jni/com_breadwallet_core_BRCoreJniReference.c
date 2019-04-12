//  Created by Ed Gamble on 1/23/2018
//  Copyright (c) 2018 Breadwinner AG.  All right reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <stdlib.h>
#include <assert.h>
#include "BRCoreJni.h"

#define JNI_REFERENCE_ADDRESS_FIELD_NAME "jniReferenceAddress"
#define JNI_REFERENCE_ADDRESS_FIELD_TYPE "J" // long

static jfieldID getJNIReferenceField (
        JNIEnv *env,
        jobject thisObject)
{
    jclass thisClass = (*env)->GetObjectClass (env, thisObject);
    jfieldID thisFieldId = (*env)->GetFieldID(env, thisClass,
                              JNI_REFERENCE_ADDRESS_FIELD_NAME,
                              JNI_REFERENCE_ADDRESS_FIELD_TYPE);
    (*env)->DeleteLocalRef (env, thisClass);
    return thisFieldId;
}

static jlong getJNIReferenceAddress (
        JNIEnv *env,
        jobject thisObject)
{
    jfieldID coreBRKeyAddressField = getJNIReferenceField(env, thisObject);
    assert (NULL != coreBRKeyAddressField);

    return (*env)->GetLongField (env, thisObject, coreBRKeyAddressField);
}

extern void *getJNIReference (
        JNIEnv *env,
        jobject thisObject)
{
    return (void *) getJNIReferenceAddress(env, thisObject);
}

/*
 * Class:     com_breadwallet_core_BRCoreJniReference
 * Method:    disposeNative
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_breadwallet_core_BRCoreJniReference_disposeNative
        (JNIEnv *env, jobject thisObject) {
    void *reference = getJNIReference(env, thisObject);

    // Not always free(); could be BRPeerManagerFree()
    if (NULL != reference) free (reference);
}
