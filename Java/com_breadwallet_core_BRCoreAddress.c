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
#include <stdlib.h>
#include <assert.h>
#include "BRAddress.h"
#include "com_breadwallet_core_BRCoreAddress.h"

/*
 * Class:     com_breadwallet_core_BRCoreAddress
 * Method:    createCoreAddress
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_BRCoreAddress_createCoreAddress
        (JNIEnv *env, jclass thisClass, jstring stringObject) {
    BRAddress *address = (BRAddress *) calloc (1, sizeof (BRAddress));

    size_t stringLen = (size_t) (*env)->GetStringLength (env, stringObject);
    assert (stringLen <= 36);

    const char *stringChars = (const char *) (*env)->GetStringChars (env, stringObject, 0);
    memcpy(address->s, stringChars, stringLen);

    return (jlong) address;
}

/*
 * Class:     com_breadwallet_core_BRCoreAddress
 * Method:    stringify
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_breadwallet_core_BRCoreAddress_stringify
        (JNIEnv *env, jobject thisObject) {
    BRAddress *address = (BRAddress *) getJNIReference (env, thisObject);
    return (*env)->NewStringUTF (env, address->s);
}

/*
 * Class:     com_breadwallet_core_BRCoreAddress
 * Method:    isValid
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_breadwallet_core_BRCoreAddress_isValid
        (JNIEnv *env, jobject thisObject) {
    BRAddress *address = (BRAddress *) getJNIReference(env, thisObject);
    return (jboolean) (BRAddressIsValid(address->s)
                       ? JNI_TRUE
                       : JNI_FALSE);
}
