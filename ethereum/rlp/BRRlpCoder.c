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
#include <stdarg.h>
#include <memory.h>
#include <regex.h>
#include <assert.h>
#include "BRRlpCoder.h"

/**
 * An RLP Encoding is comprised of two types: an ITEM and a LIST (of ITEM).
 *
 */
typedef enum {
  CODER_ITEM,
  CODER_LIST
} BRRlpItemType;

/**
 * An RLP Context holds encoding results for each of the encoding types, either ITEM or LIST.
 * The ITEM type holds the bytes directly; the LIST type holds a list/array of ITEMS.
 *
 * The upcoming RLP Coder is going to hold multiple Contexts.  The public interface for RLP Item
 * holds an 'indexer' which is the index to a Context in the Coder.
 */
typedef struct {
  BRRlpCoder coder;  // validation
  BRRlpItemType type;

  // The encoding
  size_t bytesCount;
  uint8_t *bytes;

  // If CODER_LIST, then the component items.
  size_t itemsCount;
  BRRlpItem *items;

} BRRlpContext;

static BRRlpContext contextEmpty = { NULL, CODER_ITEM, 0, NULL, 0, NULL };

static int
contextIsEmpty (BRRlpContext context) {
  return NULL == context.coder;
}

static void
contextRelease (BRRlpContext context) {
  if (NULL != context.bytes) free (context.bytes);
  if (NULL != context.items) free (context.items);
}

static BRRlpContext
createContextItem (BRRlpCoder coder, uint8_t *bytes, size_t bytesCount, int takeBytes) {
  // assert (bytesCount > 0);
  BRRlpContext context = contextEmpty;

  context.coder = coder;
  context.type = CODER_ITEM;
  context.bytesCount = bytesCount;
  if (takeBytes)
    context.bytes = bytes;
  else {
    uint8_t *myBytes = malloc (bytesCount);
    memcpy (myBytes, bytes, bytesCount);
    context.bytes = myBytes;
  }
  return context;
}

static BRRlpContext
createContextList (BRRlpCoder coder, uint8_t *bytes, size_t bytesCount, int takeBytes, BRRlpItem *items, size_t itemsCount) {
  BRRlpContext context = createContextItem(coder, bytes, bytesCount, takeBytes);
  context.type = CODER_LIST;
  context.itemsCount = itemsCount;

  context.items = calloc (itemsCount, sizeof (BRRlpItem));
  for (int i = 0; i < itemsCount; i++)
    context.items[i] = items[i];

  return context;
}

/**
 * Return a new BRRlpContext by appending the two proviedd contexts.  Both provided contexts
 * must be for CODER_ITEM (othewise an 'assert' is raised); the appending is performed by simply
 * concatenating the two context's byte arrays.
 *
 * If release is TRUE, then both the provided contexts are released; thereby freeing their memory.
 *
 */
static BRRlpContext
createContextItemAppend (BRRlpCoder coder, BRRlpContext context1, BRRlpContext context2, int release) {
  assert (CODER_ITEM == context1.type && CODER_ITEM == context2.type);
  assert (coder == context1.coder     && coder == context2.coder);

  BRRlpContext context = contextEmpty;

  context.coder = coder;
  context.type = CODER_ITEM;

  context.bytesCount = context1.bytesCount + context2.bytesCount;
  context.bytes = malloc (context.bytesCount);
  memcpy (&context.bytes[0], context1.bytes, context1.bytesCount);
  memcpy (&context.bytes[context1.bytesCount], context2.bytes, context2.bytesCount);

  if (release) {
    contextRelease(context1);
    contextRelease(context2);
  }

  return context;
}

/**
 * And RLP Coder holds Contexts; any held Context can be encode into an array of bytes (uint8_t)
 * using coderContextFillData() or the public funtion rlpGetData().
 */
struct BRRlpCoderRecord {
  BRRlpContext *contexts;
  size_t contextsCount;
  size_t contextsAllocated;
};

#define CODER_DEFAULT_CONTEXTS 10

extern BRRlpCoder
rlpCoderCreate (void) {
  BRRlpCoder coder = (BRRlpCoder) malloc (sizeof (struct BRRlpCoderRecord));

  coder->contextsCount = 0;
  coder->contextsAllocated = CODER_DEFAULT_CONTEXTS;
  coder->contexts = (BRRlpContext *) calloc (CODER_DEFAULT_CONTEXTS, sizeof (BRRlpContext));

  return coder;
}

static void
coderRelease (BRRlpCoder coder) {
  for (int i = 0; i < coder->contextsCount; i++) {
    contextRelease(coder->contexts[i]);
  }
  free (coder->contexts);
  free (coder);
}

static int
coderIsValidItem (BRRlpCoder coder, BRRlpItem item) {
  return item.indexer < coder->contextsCount && item.identifier == coder;
}

/**
 * Return the RLP Context corresponding to the provided RLP Item; if `item` is invalid, then
 * an empty context is returned.
 */
static BRRlpContext
coderLookupContext (BRRlpCoder coder, BRRlpItem item) {
  return (coderIsValidItem(coder, item)
          ? coder->contexts[item.indexer]
          : contextEmpty);
}

/**
 * Add `context` to `coder` and return the corresponding RLP Item.  Extends coder's context
 * array if required.
 */
static BRRlpItem
coderAddContext (BRRlpCoder coder, BRRlpContext context) {
  if (coder->contextsCount + 1 >= coder->contextsAllocated) {
    coder->contextsAllocated += CODER_DEFAULT_CONTEXTS;
    coder->contexts = (BRRlpContext *) realloc (coder->contexts, coder->contextsAllocated * sizeof (BRRlpContext));
    return coderAddContext(coder, context);
  }
  else {
    BRRlpItem item;
    item.identifier = coder;
    item.indexer = coder->contextsCount;
    coder->contexts[item.indexer] = context;
    coder->contextsCount += 1;
    return item;
  }
}

// The largest number supported for encoding is a UInt256 - which is representable as 32 bytes.
#define CODER_NUMBER_BYTES_LIMIT    (256/8)

/**
 * Return the index of the first non-zero byte; if all bytes are zero, bytesCount is returned
 */
static int
coderNonZeroIndex (uint8_t *bytes, size_t bytesCount) {
  for (int i = 0; i < bytesCount; i++)
    if (bytes[i] != 0) return i;
  return (int) bytesCount;
}

/**
 * Fill `target` with `source` converted to BIG_ENDIAN.
 *
 * Note: target and source must not overlap.
 */
static void
coderConvertToBigEndian (uint8_t *target, uint8_t *source, size_t count) {
  assert (target != source);  // common overlap case, but wholely insufficient.
  for (int i = 0; i < count; i++) {
#if BYTE_ORDER == LITTLE_ENDIAN
    target[i] = source[count - 1 - i];
#else
    target[i] = source[i]
#endif
  }
}

static void
coderConvertToBigEndianAndNormalize (uint8_t *target, uint8_t *source, size_t length, size_t *targetIndex, size_t *targetCount) {
  assert (length <= CODER_NUMBER_BYTES_LIMIT);

  coderConvertToBigEndian (target, source, length);

  *targetIndex = coderNonZeroIndex(target, length);
  *targetCount = length - *targetIndex;

  if (0 == *targetCount) {
    *targetCount = 1;
    *targetIndex = 0;
  }
}

static BRRlpContext
coderEncodeLength (BRRlpCoder coder, uint64_t length, uint8_t baseline) {
  // If the length is small, simply encode a single byte as (baseline + length)
  if (length < 56) {
    uint8_t encoding = baseline + length;
    return createContextItem (coder, &encoding, 1, 0);
  }
  // Otherwise, encode the length as bytes.
  else {
    size_t lengthSize = sizeof (uint64_t);

    uint8_t bytes [lengthSize]; // big_endian representation of the bytes in 'length'
    size_t bytesIndex;          // Index of the first non-zero byte
    size_t bytesCount;          // The number of bytes to encode (beyond index)

    coderConvertToBigEndianAndNormalize (bytes, (uint8_t *) &length, lengthSize, &bytesIndex, &bytesCount);

    // The encoding a a header byte with the bytesCount and then the big_endian bytes themselves.
    uint8_t encoding [1 + bytesCount];
    encoding[0] = baseline + 55 + bytesCount;
    memcpy (&encoding[1], &bytes[bytesIndex], bytesCount);
    return createContextItem(coder, encoding, 1 + bytesCount, 0);
  }
}

static BRRlpContext
coderEncodeBytes(BRRlpCoder coder, uint8_t *bytes, size_t bytesCount) {
  // Encode a single byte directly
  if (1 == bytesCount && bytes[0] < 0x80) {
    return createContextItem(coder, bytes, 1, 0);
  }

  // otherwise, encode the length and then the bytes themselves
  else {
    return createContextItemAppend(coder,
                                   coderEncodeLength(coder, bytesCount, 0x80),
                                   createContextItem(coder, bytes, bytesCount, 0),
                                   1);
  }
}

static BRRlpContext
coderEncodeNumber (BRRlpCoder coder, uint8_t *source, size_t sourceCount) {
  // Encode a number by converting the number to a big_endian representation and then simply
  // encoding those bytes.
  uint8_t bytes [sourceCount]; // big_endian representation of the bytes in 'length'
  size_t bytesIndex;           // Index of the first non-zero byte
  size_t bytesCount;           // The number of bytes to encode

  coderConvertToBigEndianAndNormalize (bytes, source, sourceCount, &bytesIndex, &bytesCount);

  return coderEncodeBytes(coder, &bytes[bytesIndex], bytesCount);
}

static BRRlpContext
coderEncodeUInt64 (BRRlpCoder coder, uint64_t value) {
  return coderEncodeNumber(coder, (uint8_t *) &value, sizeof(value));
}

static BRRlpContext
coderEncodeUInt256 (BRRlpCoder coder, UInt256 value) {
  return coderEncodeNumber(coder, (uint8_t *) &value, sizeof(value));
}

static BRRlpContext
coderEncodeList (BRRlpCoder coder, BRRlpItem *items, size_t itemsCount) {
  // Validate the items
  for (int i = 0; i < itemsCount; i++) {
    assert (coderIsValidItem(coder, items[i]));
  }

  // Eventually fill these with concatenated item encodings.
  size_t bytesCount = 0;
  uint8_t *bytes = NULL;

  for (int i = 0; i < itemsCount; i++)
    bytesCount += coderLookupContext(coder, items[i]).bytesCount;

  bytes = malloc (bytesCount);

  {
    size_t bytesIndex = 0;
    for (int i = 0; i < itemsCount; i++) {
      BRRlpContext itemContext = coderLookupContext(coder, items[i]);
      memcpy (&bytes[bytesIndex], itemContext.bytes, itemContext.bytesCount);
      bytesIndex += itemContext.bytesCount;
    }
  }

  BRRlpContext encodedBytesContext = (0 == bytesCount
                                      ? coderEncodeLength(coder, bytesCount, 0xc0)
                                      : createContextItemAppend(coder,
                                                                coderEncodeLength(coder, bytesCount, 0xc0),
                                                                createContextItem(coder, bytes, bytesCount, 1),
                                                                1));

  return createContextList(coder,
                           encodedBytesContext.bytes,
                           encodedBytesContext.bytesCount,
                           1,
                           items,
                           itemsCount);
}


//
// Public Interface
//
extern void
rlpCoderRelease (BRRlpCoder coder) {
  coderRelease (coder);
}

extern BRRlpItem
rlpEncodeItemUInt64(BRRlpCoder coder, uint64_t value) {
  return coderAddContext(coder, coderEncodeUInt64(coder, value));
}

extern BRRlpItem
rlpEncodeItemUInt256(BRRlpCoder coder, UInt256 value) {
  return coderAddContext(coder, coderEncodeUInt256(coder, value));
}

extern BRRlpItem
rlpEncodeItemBytes(BRRlpCoder coder, uint8_t *bytes, size_t bytesCount) {
  return coderAddContext(coder, coderEncodeBytes(coder, bytes, bytesCount));
}

extern BRRlpItem
rlpEncodeItemString (BRRlpCoder coder, char *string) {
  if (NULL == string) string = "";
  return rlpEncodeItemBytes(coder, (uint8_t *) string, strlen (string));
}

extern BRRlpItem
rlpEncodeList1 (BRRlpCoder coder, BRRlpItem item) {
  assert (coderIsValidItem(coder, item));
  BRRlpItem items[1];

  items[0] = item;

  return coderAddContext(coder, coderEncodeList(coder, items, 1));
}

extern BRRlpItem
rlpEncodeList2 (BRRlpCoder coder, BRRlpItem item1, BRRlpItem item2) {
  assert (coderIsValidItem(coder, item1));
  assert (coderIsValidItem(coder, item1));

  BRRlpItem items[2];

  items[0] = item1;
  items[1] = item2;

  return coderAddContext(coder, coderEncodeList(coder, items, 2));
}

extern BRRlpItem
rlpEncodeList (BRRlpCoder coder, size_t count, ...) {
  BRRlpItem items[count];

  va_list args;
  va_start (args, count);
  for (int i = 0; i < count; i++)
    items[i] = va_arg (args, BRRlpItem);
  va_end(args);

  return coderAddContext(coder, coderEncodeList(coder, items, count));
}

extern BRRlpItem
rlpEncodeListItems (BRRlpCoder coder, BRRlpItem *items, size_t itemsCount) {
  return coderAddContext(coder, coderEncodeList(coder, items, itemsCount));
}

extern void
rlpGetData (BRRlpCoder coder, BRRlpItem item, uint8_t **bytes, size_t *bytesCount) {
  assert (coderIsValidItem(coder, item));
  assert (NULL != bytes && NULL != bytesCount);

  BRRlpContext context = coderLookupContext(coder, item);
  *bytesCount = context.bytesCount;
  *bytes = malloc (*bytesCount);
  memcpy (*bytes, context.bytes, context.bytesCount);
}

extern BRRlpData
createRlpDataEmpty (void) {
  BRRlpData data;
  data.bytesCount = 0;
  data.bytes = NULL;
  return data;
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

#define HEX_REGEX "^([0-9A-Fa-f]{2})+$" // "^[0-9A-Fa-f]+$"

extern int
encodeHexValidate (const char *string) {
    static regex_t hexCharRegex;
    static int hexCharRegexInitialized = 0;

    if (!hexCharRegexInitialized) {
        // Has pairs of hex digits
      //regcomp(&hexCharRegex, "^([0-9A-Fa-f]{2})+$", REG_BASIC);
        regcomp(&hexCharRegex, HEX_REGEX, REG_EXTENDED);
        hexCharRegexInitialized = 1;
    }

    return 0 == regexec (&hexCharRegex, string, 0, NULL, 0);
}
