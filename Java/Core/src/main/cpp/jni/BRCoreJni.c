//  Created by Ed Gamble on 1/23/2018
//  Copyright (c) 2018 Breadwinner AG.  All right reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <jni.h>
#include <assert.h>
#include "BRCoreJni.h"

static JavaVM *jvm = NULL;

extern
JNIEnv *getEnv() {
    JNIEnv *env;

    if (NULL == jvm) return NULL;

    int status = (*jvm)->GetEnv(jvm, (void **) &env, JNI_VERSION_1_6);

    if (status != JNI_OK)
      status = (*jvm)->AttachCurrentThread(jvm, (JNIEnv **) &env, NULL);

    return status == JNI_OK ? env : NULL;
}

extern
void releaseEnv () {
    if (NULL != jvm)
        (*jvm)->DetachCurrentThread (jvm);
}

JNIEXPORT jint JNICALL
JNI_OnLoad (JavaVM *theJvm, void *reserved) {
    JNIEnv *env = 0;

    if ((*theJvm)->GetEnv(theJvm, (void **)&env, JNI_VERSION_1_6)) {
        return JNI_ERR;
    }

    jvm = theJvm;

    return JNI_VERSION_1_6;
}

//
// Support
//
extern void
transactionInputCopy(BRTxInput *target,
                     const BRTxInput *source) {
    assert (target != NULL);
    assert (source != NULL);
    *target = *source;

    target->script = NULL;
    BRTxInputSetScript(target, source->script, source->scriptLen);

    target->signature = NULL;
    BRTxInputSetSignature(target, source->signature, source->sigLen);
}

extern void
transactionOutputCopy (BRTxOutput *target,
                       const BRTxOutput *source) {
    assert (target != NULL);
    assert (source != NULL);
    *target = *source;

    target->script = NULL;
    BRTxOutputSetScript(target, source->script, source->scriptLen);
}


