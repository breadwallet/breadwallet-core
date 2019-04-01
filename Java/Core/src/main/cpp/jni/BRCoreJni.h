//  Created by Ed Gamble on 1/23/2018
//  Copyright (c) 2018 Breadwinner AG.  All right reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef COREJNI_BRCOREJVM_H
#define COREJNI_BRCOREJVM_H

#include <jni.h>
#include "bitcoin/BRTransaction.h"
#include "com_breadwallet_core_BRCoreJniReference.h"

/**
 *
 * @return
 */
extern JNIEnv *
getEnv();

extern void
releaseEnv ();

/**
 *
 * @param env
 * @param thisObject
 * @return
 */
extern void *
getJNIReference (
        JNIEnv *env,
        jobject thisObject);

//
// Support
//
extern void
transactionInputCopy(BRTxInput *target,
                     const BRTxInput *source);

extern void
transactionOutputCopy (BRTxOutput *target,
                       const BRTxOutput *source);

#endif //COREJNI_BRCOREJVM_H
