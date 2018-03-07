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

typedef enum {
  CODER_ITEM,
  CODER_LIST
} BRRlpItemType;

typedef struct {
  BRRlpCoder coder;  // validation
  BRRlpItemType type;
  union {
    struct {
      size_t bytesCount;
      uint8_t *bytes;
    } item;

    struct {
      size_t itemsCount;
      BRRlpItem *items;
    } list;
  } u;
} BRRlpContext;

static void
contextRelease (BRRlpContext context) {
  switch (context.type) {
    case CODER_ITEM:
      free (context.u.item.bytes);
      break;
    case CODER_LIST:
      free (context.u.list.items);
      break;
  }
}

static BRRlpContext
createContextItem (BRRlpCoder coder, uint8_t *bytes, size_t bytesCount, int takeBytes) {
  BRRlpContext context;

  context.coder = coder;
  context.type = CODER_ITEM;
  context.u.item.bytesCount = bytesCount;
  if (takeBytes)
    context.u.item.bytes = bytes;
  else {
    uint8_t *myBytes = malloc (bytesCount);
    memcpy (myBytes, bytes, bytesCount);
    context.u.item.bytes = myBytes;
  }
  return context;
}

static BRRlpContext
createContextList (BRRlpCoder coder, BRRlpItem *items, size_t itemsCount) {
  BRRlpContext context;

  context.coder = coder;
  context.type = CODER_LIST;

  context.u.list.items = calloc (itemsCount, sizeof (BRRlpItem));
  context.u.list.itemsCount = itemsCount;
  for (int i = 0; i < itemsCount; i++)
    context.u.list.items[i] = items[i];

  return context;
}

static BRRlpContext
createContextItemAppend (BRRlpCoder coder, BRRlpContext context1, BRRlpContext context2, int release) {
  assert (CODER_ITEM == context1.type && CODER_ITEM == context2.type);
  assert (coder == context1.coder     && coder == context2.coder);

  BRRlpContext context;

  context.coder = coder;
  context.type = CODER_ITEM;

  context.u.item.bytesCount = context1.u.item.bytesCount + context2.u.item.bytesCount;
  context.u.item.bytes = malloc (context.u.item.bytesCount);
  memcpy (&context.u.item.bytes[0], context1.u.item.bytes, context1.u.item.bytesCount);
  memcpy (&context.u.item.bytes[context1.u.item.bytesCount], context2.u.item.bytes, context2.u.item.bytesCount);

  if (release) {
    contextRelease(context1);
    contextRelease(context2);
  }

  return context;
}



//
// Coder
//
#define CODER_DEFAULT_CONTEXTS 10

struct BRRlpCoderRecord {
  BRRlpContext *contexts;
  size_t contextsCount;
  size_t contextsAllocated;
};

extern BRRlpCoder
rlpCoderCreate (void) {
  BRRlpCoder coder = (BRRlpCoder) malloc (sizeof (struct BRRlpCoderRecord));

  coder->contextsCount = 0;
  coder->contextsAllocated = CODER_DEFAULT_CONTEXTS;
  coder->contexts = (BRRlpContext *) calloc (CODER_DEFAULT_CONTEXTS, sizeof (BRRlpContext));

  return coder;
}

extern void
rlpCoderRelease (BRRlpCoder coder) {
  // for each context; release
  free (coder->contexts);
  free (coder);
}

static int
coderIsValidItem (BRRlpCoder coder, BRRlpItem item) {
  return item.indexer < coder->contextsCount && item.identifier == coder;
}

static BRRlpContext
coderLookupContext (BRRlpCoder coder, BRRlpItem item) {
  // TODO: Fix-aroo
  BRRlpContext nullContext;
  memset (&nullContext, 0, sizeof (BRRlpContext));
  return (coderIsValidItem(coder, item)
          ? coder->contexts[item.indexer]
          : nullContext);
}

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

//
// Contexts
//

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

static BRRlpContext
coderContextItemNumber (BRRlpCoder coder, uint8_t *bytes, size_t bytesCount, uint8_t baseline, uint8_t limit) {

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
    return createContextItem(coder, &valueBytes[nonZeroIndex], 1, 0);
  }
  else {
    uint8_t rlpBytes [1 + nonZeroBytesCount];
    rlpBytes[0] = (uint8_t) (baseline + nonZeroBytesCount);
    memcpy (&rlpBytes[1], &valueBytes[nonZeroIndex], (size_t) nonZeroBytesCount);
    return createContextItem (coder, rlpBytes, 1 + nonZeroBytesCount, 0);
  }
}

static BRRlpContext
coderContextItemUInt64 (BRRlpCoder coder, uint64_t value) {
  return coderContextItemNumber(coder, (uint8_t *) &value, sizeof(value), 0x80, 0x80);
}

static BRRlpContext
coderContextItemUInt256 (BRRlpCoder coder, UInt256 value) {
  return coderContextItemNumber(coder, (uint8_t *) &value, sizeof(value), 0x80, 0x80);
}

static BRRlpContext
coderContextItemBytes(BRRlpCoder coder, uint8_t *bytes, size_t bytesCount) {
  if (1 == bytesCount && bytes[0] < 0x80) {
    uint8_t rlpBytes[1];
    rlpBytes[0] = bytes[0];
    return createContextItem(coder, rlpBytes, 1, 0);
  }
  else if (bytesCount < 56) {
    uint8_t rlpBytes [1 + bytesCount];
    rlpBytes[0] = (uint8_t) (0x80 + bytesCount);
    memcpy (&rlpBytes[1], bytes, bytesCount);
    return createContextItem(coder, rlpBytes, 1 + bytesCount, 0);
  }
  else {
    return createContextItemAppend(coder,
                                   coderContextItemNumber(coder, (uint8_t *) &bytesCount, sizeof(bytesCount), 0xb7, 56),
                                   createContextItem(coder, bytes, bytesCount, 0),
                                   1);
  }
}

static BRRlpContext
coderContextList1 (BRRlpCoder coder, BRRlpItem item1) {
  return createContextList(coder, &item1, 1);
}

static BRRlpContext
coderContextList2 (BRRlpCoder coder, BRRlpItem item1, BRRlpItem item2) {
  BRRlpItem items[2];

  items[0] = item1;
  items[1] = item2;

  return createContextList(coder, items, 2);
}

#define COMPUTE_SIZE_FOR_LIST_HEADER 4;

static size_t
codeContextHeaderCount (BRRlpCoder coder, size_t bytesCount) {
  if (bytesCount < 56)
    return 1;
  else {
    size_t count = 0;
    while (bytesCount > 0) {
      count += 1;
      bytesCount >>= 8;
    }
    return 1 + count;
  }
}

static void
codeContextByteCount (BRRlpCoder coder, BRRlpItem item, size_t *bytesCount, size_t *headerCount) {
  BRRlpContext context = coderLookupContext(coder, item);
  switch (context.type) {
    case CODER_ITEM:
      *bytesCount = context.u.item.bytesCount;
      *headerCount = 0;
      break;

    case CODER_LIST: {
      size_t itemBytesCount;
      size_t itemHeaderCount;

      *bytesCount = 0;
      for (int i = 0; i < context.u.list.itemsCount; i++) {
        codeContextByteCount(coder, context.u.list.items[i], &itemBytesCount, &itemHeaderCount);
        *bytesCount += itemBytesCount + itemHeaderCount;
      }

      *headerCount = codeContextHeaderCount(coder, *bytesCount);
      break;
    }
  }
}

static void
coderContextFillData (BRRlpCoder coder, BRRlpItem item, uint8_t **bytes, size_t *bytesCount) {
  assert (coderIsValidItem(coder, item));
  assert (NULL != bytes && NULL != bytesCount);

  BRRlpContext context = coderLookupContext(coder, item);
  switch (context.type) {
    case CODER_ITEM:
      *bytesCount = context.u.item.bytesCount;
      *bytes = malloc (*bytesCount);
      memcpy (*bytes, context.u.item.bytes, *bytesCount);
      break;

    case CODER_LIST: {
      size_t itemBytesCount;
      size_t itemHeaderCount;

      codeContextByteCount(coder, item, &itemBytesCount, &itemHeaderCount);

      *bytesCount = itemBytesCount + itemHeaderCount;
      *bytes = malloc (*bytesCount);

      // Encode the header
      BRRlpContext headerContext  =
        coderContextItemNumber(coder, (uint8_t *) &bytesCount, sizeof(size_t), 0xc0, 56);
      memcpy (bytes, headerContext.u.item.bytes, headerContext.u.item.bytesCount);
      contextRelease(headerContext);

      //      (*bytes)[0] = 0xc0 + (itemHeaderCount - 1);
      //      memset (&(*bytes)[1], 0xee, itemHeaderCount -1);

      // We could just recurse down with coderContextFillData; however, for a non-LIST
      // we can just copy bytes into place directly.
      size_t bytesIndex = itemHeaderCount;
      for (int i = 0; i < context.u.list.itemsCount; i++) {
        BRRlpContext nextContext = coderLookupContext(coder, context.u.list.items[i]);
        switch (nextContext.type) {
          case CODER_ITEM:
            memcpy (&(*bytes)[bytesIndex], nextContext.u.item.bytes, nextContext.u.item.bytesCount);
            bytesIndex += nextContext.u.item.bytesCount;
            break;
          case CODER_LIST: {
            uint8_t *itemBytes;
            size_t   itemBytesCount;
            coderContextFillData(coder, nextContext.u.list.items[i], &itemBytes, &itemBytesCount);
            memcpy(&(*bytes)[bytesIndex], itemBytes, itemBytesCount);
            bytesIndex += itemBytesCount;
            free (itemBytes);
            break;
          } // case CODER_LIST
        } // switch nextContext.type
      } // for items
      break;
    }
  }
}

//
// Public Interface
//
extern BRRlpItem
rlpEncodeItemUInt64(BRRlpCoder coder, uint64_t value) {
  return coderAddContext(coder, coderContextItemUInt64(coder, value));
}

extern BRRlpItem
rlpEncodeItemUInt256(BRRlpCoder coder, UInt256 value) {
  return coderAddContext(coder, coderContextItemUInt256(coder, value));
}


extern BRRlpItem
rlpEncodeItemBytes(BRRlpCoder coder, uint8_t *bytes, size_t bytesCount) {
  return coderAddContext(coder, coderContextItemBytes(coder, bytes, bytesCount));
}

extern BRRlpItem
rlpEncodeItemString (BRRlpCoder coder, char *string) {
  if (NULL == string) string = "";
  return rlpEncodeItemBytes(coder, (uint8_t *) string, strlen (string));
}

extern BRRlpItem
rlpEncodeList1 (BRRlpCoder coder, BRRlpItem item1) {
  assert (coderIsValidItem(coder, item1));
  return coderAddContext(coder, coderContextList1(coder, item1));
}

extern BRRlpItem
rlpEncodeList2 (BRRlpCoder coder, BRRlpItem item1, BRRlpItem item2) {
  assert (coderIsValidItem(coder, item1));
  assert (coderIsValidItem(coder, item1));
  return coderAddContext(coder, coderContextList2(coder, item1, item2));
}

extern BRRlpItem
rlpEncodeList (BRRlpCoder coder, size_t count, ...) {
  BRRlpItem items[count];

  va_list args;
  va_start (args, count);
  for (int i = 0; i < count; i++)
    items[i] = va_arg (args, BRRlpItem);
  va_end(args);

  return coderAddContext(coder, createContextList(coder, items, count));
}

extern BRRlpItem
rlpEncodeListItems (BRRlpCoder coder, BRRlpItem *items, size_t itemsCount) {
  return coderAddContext(coder, createContextList(coder, items, itemsCount));
}

// Hold onto BRRlpItem 'forever'... then try to use... will fail because 'coder'
// will not have 'context'
extern void
rlpGetData (BRRlpCoder coder, BRRlpItem item, uint8_t **bytes, size_t *bytesCount) {
  coderContextFillData(coder, item, bytes, bytesCount);
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
