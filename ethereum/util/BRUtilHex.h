//
//  BBRUtilHex.h
//  Core Ethereum
//
//  Created by Ed Gamble on 3/10/2018.
//  Copyright © 2018-2019 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_Util_Hex_H
#define BR_Util_Hex_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Convert a 'char *' string into a 'uint8_t *' byte array.  Two characters are consumed for each
 * byte.  The conversion is only valid for a source string this was derived by encoding a
 * byte array (with encodeHex).  Thus this method *WILL FATAL* if the sourceLen is not an even
 * number.
 *
 * @param target
 * @param targetLen  Must be sourceLen/2
 * @param source
 * @param sourceLen Must be even and 2*targetLen
 */
extern void
decodeHex (uint8_t *target, size_t targetLen, const char *source, size_t sourceLen);

/**
 * Return the number `uint8_t *' elements needed to decode stringLen characters.  The provided
 * stringLen is the strlen() return (and even number is required); the provided stringLen *DOES NOT*
 * include the null terminator.  the return value is appropraite for malloc() or alloca() calls.
 *
 * @param stringLen
 * @return
 */
extern size_t
decodeHexLength (size_t stringLen);

extern uint8_t *
decodeHexCreate (size_t *targetLen, const char *source, size_t sourceLen);

/**
 * Convert a 'uint8_t *' byte array into a 'char *' string.  Two characters are produced for each
 * byte.
 *
 * The resulting 'char *' string *WILL BE* zero terminated.  Thus targetLen = 2*sourceLen +1 with
 * the added '1' for the terminator.
 *
 * @param target
 * @param targetLen
 * @param source
 * @param sourceLen
 */
extern void
encodeHex (char *target, size_t targetLen, const uint8_t *source, size_t sourceLen);

/**
 * Return the number of 'char *' elements needs to encode byteArrayLen bytes.  The return value
 * includes one elements for the zero terminator.  Thus, the return value is appropriate for
 * malloc() on alloca() calls.
 *
 * @param sourceLen
 * @return
 */
extern size_t
encodeHexLength(size_t byteArrayLen);

extern char *
encodeHexCreate (size_t *targetLen, const uint8_t *source, size_t sourceLen);

/**
 * Return TRUE/1 if `string` is an encoded-hex string.
 *
 * @param string
 * @return
 */
extern int
encodeHexValidate (const char *string);

#ifdef __cplusplus
}
#endif

#endif /* BR_Util_Hex_H */
