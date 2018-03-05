//
//  rlp
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 2/25/18.
//  Copyright (c) 2018 breadwallet LLC
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

#ifndef BR_RLP_Coder_H
#define BR_RLP_Coder_H

#include <stddef.h>
#include <stdint.h>
#include "BRInt.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BRRlpDataRecord {
    // type
    size_t bytesAllocated;
    size_t bytesCount;
    uint8_t *bytes;
} BRRlpData;

extern BRRlpData
createRlpDataEmpty (void);

extern BRRlpData
createRlpDataCopy (BRRlpData data);

/**
 * Return a '0x' prefixed hex string of the data bytes.
 *
 * @param data
 * @return
 */
extern const char *
rlpDataAsString (BRRlpData data);

//
//
//
typedef struct BRRlpCoderRecord *BRRlpCoder;

extern BRRlpCoder
createRlpCoder (void);

extern void
rlpCoderRelease (BRRlpCoder coder);

extern void
rlpEncodeItemUInt64(BRRlpCoder coder, uint64_t value);

extern void
rlpEncodeItemUInt256(BRRlpCoder coder, UInt256 value);

extern void
rlpEncodeItemString(BRRlpCoder coder, const char *string);

extern void
rlpEncodeItemBytes(BRRlpCoder coder, uint8_t *bytes, size_t bytesCount);

//extern void
//rlpEncodeList (BRRlpCoder coder, BRRlpData *data, size_t dataCount);

extern BRRlpData
rlpGetData (BRRlpCoder coder);

//
//
//
/*

 extern void
 addressRlpEncode (BREthereumAddress address, BRRlpCoder coder);

 */


//
// Support
//

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
decodeHex (uint8_t *target, size_t targetLen, char *source, size_t sourceLen);

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
decodeHexCreate (size_t *targetLen, char *source, size_t sourceLen);

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
encodeHex (char *target, size_t targetLen, uint8_t *source, size_t sourceLen);

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
encodeHexCreate (size_t *targetLen, uint8_t *source, size_t sourceLen);

#ifdef __cplusplus
}
#endif

#endif //BR_RLP_Coder_H
