#include <jni.h>
/* Header for class com_breadwallet_core_BRCore */

#ifndef _Included_com_breadwallet_core_BRCorePeer
#define _Included_com_breadwallet_core_BRCorePeer
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Class:     com_breadwallet_core_BRCoreTest
 * Method:    runTests
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_breadwallet_core_BRCoreTest_runTests
  (JNIEnv *, jobject);

/*
 * Class:     com_breadwallet_core_BRCoreTest
 * Method:    runTestsSync
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_breadwallet_core_BRCoreTest_runTestsSync
  (JNIEnv *, jobject, jint);



#ifdef __cplusplus
}
#endif
#endif
