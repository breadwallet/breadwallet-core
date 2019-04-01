//  Created by Ed Gamble on 1/23/2018
//  Copyright (c) 2018 Breadwinner AG.  All right reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <assert.h>
#include "BRCoreJni.h"
#include "bitcoin/BRMerkleBlock.h"
#include "com_breadwallet_core_BRCoreMerkleBlock.h"

/*
 * Class:     com_breadwallet_core_BRCoreMerkleBlock
 * Method:    createJniCoreMerkleBlock
 * Signature: ([BI)J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_BRCoreMerkleBlock_createJniCoreMerkleBlock
        (JNIEnv *env, jclass thisClass,
         jbyteArray blockArray,
         jint blockHeight) {

    int blockLength   = (*env)->GetArrayLength(env, blockArray);
    jbyte *blockBytes = (*env)->GetByteArrayElements(env, blockArray, 0);

    assert (NULL != blockBytes);
    BRMerkleBlock *block = BRMerkleBlockParse((const uint8_t *) blockBytes, (size_t) blockLength);
    assert (NULL != block);
    if (blockHeight != -1)
        block->height = (uint32_t) blockHeight;

    return (jlong) block;
}

/*
 * Class:     com_breadwallet_core_BRCoreMerkleBlock
 * Method:    createJniCoreMerkleBlockEmpty
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_com_breadwallet_core_BRCoreMerkleBlock_createJniCoreMerkleBlockEmpty
        (JNIEnv *env, jclass thisClass) {
    // Test only
    BRMerkleBlock *block = BRMerkleBlockNew();
    block->height = BLOCK_UNKNOWN_HEIGHT;
    return (jlong) block;
}


/*
 * Class:     com_breadwallet_core_BRCoreMerkleBlock
 * Method:    getBlockHash
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL
Java_com_breadwallet_core_BRCoreMerkleBlock_getBlockHash
        (JNIEnv *env, jobject thisObject) {
    BRMerkleBlock *block = (BRMerkleBlock *) getJNIReference(env, thisObject);

    UInt256 hash = block->blockHash;

    jbyteArray hashObject = (*env)->NewByteArray (env, sizeof (UInt256));
    (*env)->SetByteArrayRegion (env, hashObject, 0, sizeof (UInt256), (const jbyte *) hash.u8);
    return hashObject;
}

/*
 * Class:     com_breadwallet_core_BRCoreMerkleBlock
 * Method:    getVersion
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_BRCoreMerkleBlock_getVersion
        (JNIEnv *env, jobject thisObject)  {
    BRMerkleBlock *block = (BRMerkleBlock *) getJNIReference(env, thisObject);
    return (jlong) block->version;
}

/*
 * Class:     com_breadwallet_core_BRCoreMerkleBlock
 * Method:    getPrevBlockHash
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL
Java_com_breadwallet_core_BRCoreMerkleBlock_getPrevBlockHash
        (JNIEnv *env, jobject thisObject) {
    BRMerkleBlock *block = (BRMerkleBlock *) getJNIReference(env, thisObject);

    UInt256 hash = block->prevBlock;

    jbyteArray hashObject = (*env)->NewByteArray (env, sizeof (UInt256));
    (*env)->SetByteArrayRegion (env, hashObject, 0, sizeof (UInt256), (const jbyte *) hash.u8);
    return hashObject;
}

/*
 * Class:     com_breadwallet_core_BRCoreMerkleBlock
 * Method:    getRootBlockHash
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL
Java_com_breadwallet_core_BRCoreMerkleBlock_getRootBlockHash
        (JNIEnv *env, jobject thisObject) {
    BRMerkleBlock *block = (BRMerkleBlock *) getJNIReference(env, thisObject);

    UInt256 hash = block->merkleRoot;

    jbyteArray hashObject = (*env)->NewByteArray (env, sizeof (UInt256));
    (*env)->SetByteArrayRegion (env, hashObject, 0, sizeof (UInt256), (const jbyte *) hash.u8);
    return hashObject;
}

/*
 * Class:     com_breadwallet_core_BRCoreMerkleBlock
 * Method:    getTimestamp
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_BRCoreMerkleBlock_getTimestamp
        (JNIEnv *env, jobject thisObject) {
    BRMerkleBlock *block = (BRMerkleBlock *) getJNIReference(env, thisObject);
    return (jlong) block->timestamp;
}

/*
 * Class:     com_breadwallet_core_BRCoreMerkleBlock
 * Method:    getTarget
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_BRCoreMerkleBlock_getTarget
        (JNIEnv *env, jobject thisObject) {
    BRMerkleBlock *block = (BRMerkleBlock *) getJNIReference(env, thisObject);
    return (jlong) block->target;
}

/*
 * Class:     com_breadwallet_core_BRCoreMerkleBlock
 * Method:    getNonce
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_BRCoreMerkleBlock_getNonce
        (JNIEnv *env, jobject thisObject) {
    BRMerkleBlock *block = (BRMerkleBlock *) getJNIReference(env, thisObject);
    return (jlong) block->nonce;
}

/*
 * Class:     com_breadwallet_core_BRCoreMerkleBlock
 * Method:    getTransactionCount
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_BRCoreMerkleBlock_getTransactionCount
        (JNIEnv *env, jobject thisObject) {
    BRMerkleBlock *block = (BRMerkleBlock *) getJNIReference(env, thisObject);
    return (jlong) block->totalTx;
}

/*
 * Class:     com_breadwallet_core_BRCoreMerkleBlock
 * Method:    getHeight
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL
Java_com_breadwallet_core_BRCoreMerkleBlock_getHeight
        (JNIEnv *env, jobject thisObject) {
    BRMerkleBlock *block = (BRMerkleBlock *) getJNIReference(env, thisObject);
    return (jlong) block->height;
}

/*
 * Class:     com_breadwallet_core_BRCoreMerkleBlock
 * Method:    serialize
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_breadwallet_core_BRCoreMerkleBlock_serialize
        (JNIEnv *env, jobject thisObject) {
    BRMerkleBlock *block = (BRMerkleBlock *) getJNIReference(env, thisObject);

    size_t      byteArraySize     = BRMerkleBlockSerialize(block, NULL, 0);
    jbyteArray  byteArray         = (*env)->NewByteArray (env, (jsize) byteArraySize);
    jbyte      *byteArrayElements = (*env)->GetByteArrayElements (env, byteArray, JNI_FALSE);

    BRMerkleBlockSerialize(block, (uint8_t *) byteArrayElements, byteArraySize);

    // Ensure ELEMENTS 'written' back to byteArray
    (*env)->ReleaseByteArrayElements (env, byteArray, byteArrayElements, JNI_COMMIT);

    return byteArray;
}


/*
 * Class:     com_breadwallet_core_BRCoreMerkleBlock
 * Method:    isValid
 * Signature: (J)Z
 */
JNIEXPORT jboolean JNICALL
Java_com_breadwallet_core_BRCoreMerkleBlock_isValid
        (JNIEnv *env, jobject thisObject, jlong currentTime) {
    BRMerkleBlock *block = (BRMerkleBlock *) getJNIReference(env, thisObject);
    return (jboolean) BRMerkleBlockIsValid (block, (uint32_t) currentTime);
}

/*
 * Class:     com_breadwallet_core_BRCoreMerkleBlock
 * Method:    containsTransactionHash
 * Signature: ([B)Z
 */
JNIEXPORT jboolean JNICALL
Java_com_breadwallet_core_BRCoreMerkleBlock_containsTransactionHash
        (JNIEnv *env, jobject thisObject, jbyteArray hashByteArray) {
    BRMerkleBlock *block = (BRMerkleBlock *) getJNIReference(env, thisObject);

    UInt256 *hash = (UInt256 *) (*env)->GetByteArrayElements (env, hashByteArray, JNI_FALSE);
    return (jboolean) BRMerkleBlockContainsTxHash (block, *hash);
}

/*
 * Class:     com_breadwallet_core_BRCoreMerkleBlock
 * Method:    disposeNative
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_com_breadwallet_core_BRCoreMerkleBlock_disposeNative
        (JNIEnv *env, jobject thisObject) {
    BRMerkleBlock *block = (BRMerkleBlock *) getJNIReference(env, thisObject);
    if (NULL != block) BRMerkleBlockFree(block);
}

