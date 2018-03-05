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

#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <endian.h>
#include "BRRlpCoder.h"

#define RLP_DATA_DEFAULT_DATA_ALLOCATED 1024

extern BRRlpData
createRlpDataEmpty (void) {
    BRRlpData data;
    memset (&data, sizeof (BRRlpData), 0);
    return data;
}

static BRRlpData
createRlpData (uint8_t *bytes, size_t bytesCount) {
    BRRlpData data;

    data.bytesAllocated = RLP_DATA_DEFAULT_DATA_ALLOCATED;
    if (bytesCount > data.bytesAllocated)
        data.bytesAllocated = bytesCount;

    data.bytes = (uint8_t *) malloc (data.bytesAllocated);
    memcpy (data.bytes, bytes, bytesCount);
    data.bytesCount = bytesCount;

    return data;
}

extern BRRlpData
createRlpDataCopy (BRRlpData data) {
    return createRlpData(data.bytes, data.bytesCount);
}

extern const char *
rlpDataAsString (BRRlpData data) {
    return NULL;
}

static void
rlpDataAppend (BRRlpData *target, BRRlpData source) {
    if (target->bytesCount + source.bytesCount < target->bytesAllocated) {
        memcpy (&target->bytes[target->bytesCount], source.bytes, source.bytesCount);
        target->bytesCount += source.bytesCount;
    }
    else {
        target->bytes = realloc (target->bytes, target->bytesAllocated + source.bytesCount);
        target->bytesAllocated += source.bytesCount;
        rlpDataAppend(target, source);
    }
}

static size_t
rlpDataCountList(BRRlpData *sources, size_t sourcesCount) {
    size_t count = 0;
    for (int i = 0; i < sourcesCount; i++)
        count += sources[i].bytesCount;
    return count;
}

static void
rlpDataAppendList (BRRlpData *target, BRRlpData *sources, size_t sourcesCount) {
    // Additional bytes
    size_t count = rlpDataCountList(sources, sourcesCount);

    // Expand target
    target->bytes = realloc(target->bytes, target->bytesAllocated + count);
    target->bytesAllocated += count;

    // Append to target
    for (int i = 0; i < sourcesCount; i++)
        rlpDataAppend(target, sources[i]);
}

//
//
//
#define RLP_CODER_DEFAULT_DATA_ALLOCATED 25

struct BRRlpCoderRecord {
    size_t dataAllocated;
    size_t dataCount;
    BRRlpData *data;
};

extern BRRlpCoder
createRlpCoder (void) {
    BRRlpCoder coder = (BRRlpCoder) calloc (1, sizeof (struct BRRlpCoderRecord));

    coder->dataAllocated = RLP_CODER_DEFAULT_DATA_ALLOCATED;
    coder->dataCount = 0;
    coder->data = (BRRlpData *) calloc (RLP_CODER_DEFAULT_DATA_ALLOCATED, sizeof (BRRlpData *));

    return coder;
}

extern void
rlpCoderRelease (BRRlpCoder coder) {
    for (int i = 0; i < coder->dataCount; i++)
        /* something */;
    free (coder->data);
    free (coder);
}

static void
rlpAppendData (BRRlpCoder coder, BRRlpData data) {
    if (coder->dataCount + 1 < coder->dataAllocated) {
        coder->data[coder->dataCount] = data;
        coder->dataCount += 1;
    }
    else {
        coder->dataAllocated += RLP_CODER_DEFAULT_DATA_ALLOCATED;
        coder->data = (BRRlpData *) realloc(coder->data, coder->dataAllocated * sizeof (BRRlpData *));
        rlpAppendData(coder, data);
    }
}

// Return the index of the first non-zero byte; if all bytes are zero, bytesCount is returned
static int
rlpNonZeroIndex (uint8_t *bytes, size_t bytesCount) {
    for (int i = 0; i < bytesCount; i++)
        if (bytes[i] != 0) return i;
    return (int) bytesCount;
}

static void
rlpAsBigEndian (uint8_t *bytes, size_t bytesCount) {
#if BYTE_ORDER == LITTLE_ENDIAN
    for (int i = 0; i < bytesCount/2; i++) {
        uint8_t tmp = bytes[i];
        bytes[i] = bytes[bytesCount - 1 - i];
        bytes[bytesCount - 1 - i] = tmp;
    }
#endif
}

static void
rlpEncodeItemNumber (BRRlpCoder coder, uint8_t *bytes, size_t bytesCount, uint8_t baseline, uint8_t limit) {

    // TODO: Fix this tortured logic.
    uint8_t valueBytes [bytesCount];
    memcpy(valueBytes, bytes, bytesCount);
    // Ensure ValueBytes is big endian.
    rlpAsBigEndian(valueBytes, bytesCount);

    // Find the first non-zero byte in `value`
    int nonZeroIndex = rlpNonZeroIndex(valueBytes, bytesCount);
    size_t nonZeroBytesCount = bytesCount - nonZeroIndex;

    // All zeros.
    if (nonZeroIndex == bytesCount) {
        nonZeroBytesCount = 1;
        nonZeroIndex = 0;
    }

    if (nonZeroBytesCount == 1 && valueBytes[nonZeroIndex] < limit) {
        rlpAppendData(coder, createRlpData(&valueBytes[nonZeroIndex], 1));
    }
    else {
        uint8_t rlpBytes [1 + nonZeroBytesCount];
        rlpBytes[0] = (uint8_t) (baseline + nonZeroBytesCount);
        memcpy (&rlpBytes[1], &valueBytes[nonZeroIndex], (size_t) nonZeroBytesCount);

        rlpAppendData (coder, createRlpData(rlpBytes, 1 + nonZeroBytesCount));
    }
}

extern void
rlpEncodeItemUInt64(BRRlpCoder coder, uint64_t value) {
    rlpEncodeItemNumber (coder, (uint8_t *) &value, sizeof(uint64_t), 0x80, 0x80);
}

extern void
rlpEncodeItemUInt256(BRRlpCoder coder, UInt256 value) {
    rlpEncodeItemNumber (coder, (uint8_t *) &value, sizeof(UInt256), 0x80, 0x80);
}

extern void
rlpEncodeItemString(BRRlpCoder coder, const char *string) {
    if (NULL == string) string = "";
    rlpEncodeItemBytes(coder, (uint8_t *) string, strlen (string));
}

extern void
rlpEncodeItemBytes(BRRlpCoder coder, uint8_t *bytes, size_t bytesCount) {
    if (1 == bytesCount && bytes[0] < 0x80) {
        uint8_t rlpBytes[1];
        rlpBytes[0] = bytes[0];
        rlpAppendData(coder, createRlpData(rlpBytes, 1));
    }
    else if (bytesCount < 56) {
        uint8_t rlpBytes [1 + bytesCount];
        rlpBytes[0] = (uint8_t) (0x80 + bytesCount);
        memcpy (&rlpBytes[1], bytes, bytesCount);
        rlpAppendData(coder, createRlpData(rlpBytes, 1 + bytesCount));
    }
    else {
        rlpEncodeItemNumber(coder, (uint8_t *) &bytesCount, sizeof(bytesCount), 0xb7, 56);
        rlpAppendData(coder, createRlpData(bytes, bytesCount));
    }
}

//extern void
//rlpEncodeList (BRRlpCoder coder, BRRlpData *data, size_t dataCount) {
//    for (int i = 0; i < dataCount; i++)
//        rlpAppendData(coder, data[i]);
//}

extern BRRlpData
rlpGetData (BRRlpCoder coder) {
    if (1 != coder->dataCount)
        rlpDataAppendList (&coder->data[0], &coder->data[1], coder->dataCount - 1);

    return coder->data[0];
}

//
//
//

extern void
decodeHex (uint8_t *target, size_t targetLen, char *source, size_t sourceLen) {
    //
    assert (0 == sourceLen % 2);
    assert (2 * targetLen == sourceLen);

    for (int i = 0; i < targetLen; i++) {
        target[i] = (uint8_t) ((_hexu(source[2*i]) << 4) | _hexu(source[(2*i)+1]));
     }
}

extern size_t
decodeHexLength (size_t stringLen) {
    assert (0 == stringLen % 2);
    return stringLen/2;
}

extern uint8_t *
decodeHexCreate (size_t *targetLen, char *source, size_t sourceLen) {
    size_t length = decodeHexLength(sourceLen);
    if (NULL != targetLen) *targetLen = length;
    uint8_t *target = malloc (length);
    decodeHex (target, length, source, sourceLen);
    return target;
}

extern void
encodeHex (char *target, size_t targetLen, uint8_t *source, size_t sourceLen) {
    assert (targetLen == 2 * sourceLen  + 1);

    int i = 0;
    for (; i < sourceLen && 2 * i < targetLen - 1; i++) {
        target[2*i] = (uint8_t) _hexc (source[i] >> 4);
        target[2*i + 1] = (uint8_t) _hexc (source[i]);
    }
    target[2*i] = '\0';
}

extern size_t
encodeHexLength(size_t byteArrayLen) {
    return 2 * byteArrayLen + 1;
}

extern char *
encodeHexCreate (size_t *targetLen, uint8_t *source, size_t sourceLen) {
    size_t length = encodeHexLength(sourceLen);
    if (NULL != targetLen) *targetLen = length;
    char *target = malloc (length);
    encodeHex(target, length, source, sourceLen);
    return target;
}
