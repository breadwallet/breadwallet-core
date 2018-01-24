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

#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include "BRBIP39Mnemonic.h"
#include "BRBIP32Sequence.h"
#include "BRCoreJni.h"
#include "com_breadwallet_core_BRCoreMasterPubKey.h"

JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_BRCoreMasterPubKey_createJniCoreMasterPubKey
        (JNIEnv *env, jclass thisClass,
         jbyteArray phrase) {

    if (setJvm(env) != JNI_OK) return 0;

    jsize phraseLength = (*env)->GetArrayLength (env, phrase);
    jbyte *phraseBytes = (*env)->GetByteArrayElements (env, phrase, 0);

    // The upcoming call to BRBIP39DeriveKey() calls strlen() on the phrase; THUS, a proper
    // zero terminated C-string is required!  There will certainly be trouble if the byte[]
    // phrase has a some randomly placed '0' in it.  Really, a 'String' should be passed to
    // this function.
    //
    // This conversion might not be required if `phrase` is a 'null-terminated byte array'.  But
    // that is a dangerous assumption if violated (buffer overflow errors).
    char *phraseString = malloc (1 + phraseLength);
    memcpy (phraseString, phraseBytes, phraseLength);
    phraseString[phraseLength] = '\0';

    // UInt512 seed = UINT512_ZERO;
    // BRMasterPubKey mpk = BR_MASTER_PUBKEY_NONE;
    // BRBIP39DeriveKey(seed.u8, "axis husband project any sea patch drip tip spirit tide bring belt", NULL);
    // mpk = BRBIP32MasterPubKey(&seed, sizeof(seed));

    //(*env)->GetArrayLength(env, phrase);
    //jbyte *bytePhrase = (*env)->GetByteArrayElements(env, phrase, 0);
    //UInt512 key = UINT512_ZERO;
    //char *charPhrase = (char *) bytePhrase;
    // BRBIP39DeriveKey(key.u8, charPhrase, NULL);
    // BRMasterPubKey pubKey = BRBIP32MasterPubKey(key.u8, sizeof(key));

    //    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "getMasterPubKey");
//    (*env)->GetArrayLength(env, phrase);
//    jbyte *bytePhrase = (*env)->GetByteArrayElements(env, phrase, 0);
//    UInt512 key = UINT512_ZERO;
//    char *charPhrase = (char *) bytePhrase;
//    BRBIP39DeriveKey(key.u8, charPhrase, NULL);
//    BRMasterPubKey pubKey = BRBIP32MasterPubKey(key.u8, sizeof(key));
//    size_t pubKeySize = sizeof(pubKey);
//    jbyte *pubKeyBytes = (jbyte *) &pubKey;
//    jbyteArray result = (*env)->NewByteArray(env, (jsize) pubKeySize);
//    (*env)->SetByteArrayRegion(env, result, 0, (jsize) pubKeySize, (const jbyte *) pubKeyBytes);
//    (*env)->ReleaseByteArrayElements(env, phrase, bytePhrase, JNI_ABORT);
//    //release everything

    UInt512 seed = UINT512_ZERO;
    BRBIP39DeriveKey(seed.u8, phraseString, NULL);
    free (phraseString);

    BRMasterPubKey pubKey = BRBIP32MasterPubKey(&seed, sizeof(seed));

    // Allocate, then fill, our BRMasterPubKey result with the computed pubKey
    BRMasterPubKey *resKey = (BRMasterPubKey *) calloc (1, sizeof (BRMasterPubKey));
    *resKey = pubKey;

    return (jlong) resKey;

}

/*
 * Class:     com_breadwallet_core_BRCoreMasterPubKey
 * Method:    bip32BitIDKey
 * Signature: ([BILjava/lang/String;)[B
 */
JNIEXPORT jbyteArray JNICALL
Java_com_breadwallet_core_BRCoreMasterPubKey_bip32BitIDKey
        (JNIEnv *env, jclass thisClass, jbyteArray seed, jint index, jstring strUri) {
    int seedLength = (*env)->GetArrayLength(env, seed);
    const char *uri = (*env)->GetStringUTFChars(env, strUri, NULL);
    jbyte *byteSeed = (*env)->GetByteArrayElements(env, seed, 0);

    BRKey key;

    BRBIP32BitIDKey(&key, byteSeed, (size_t) seedLength, (uint32_t) index, uri);

    char rawKey[BRKeyPrivKey(&key, NULL, 0)];
    BRKeyPrivKey(&key, rawKey, sizeof(rawKey));

    jbyteArray result = (*env)->NewByteArray(env, (jsize) sizeof(rawKey));
    (*env)->SetByteArrayRegion(env, result, 0, (jsize) sizeof(rawKey), (jbyte *) rawKey);

    return result;
}
