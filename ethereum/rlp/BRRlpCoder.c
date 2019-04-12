//
//  rlp
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 2/25/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <stdlib.h>
#include <stdarg.h>
#include <memory.h>
#include <assert.h>
#include <pthread.h>
#include "ethereum/util/BRUtil.h"
#include "BRRlpCoder.h"

static void
rlpCoderReclaimInternal (BRRlpCoder coder) ;

static int
rlpDecodeStringEmptyCheck (BRRlpCoder coder, BRRlpItem item);

static void
encodeLengthIntoBytes (uint64_t length, uint8_t baseline, uint8_t *bytes9, uint8_t *bytes9Count);

#define CODER_DEFAULT_ITEMS     (2000)

/**
 * An RLP Encoding is comprised of two types: an ITEM and a LIST (of ITEM).
 *
 */
typedef enum {
    CODER_ITEM,
    CODER_LIST,
} BRRlpItemType;

#define ITEM_DEFAULT_BYTES_COUNT  1024
#define ITEM_DEFAULT_ITEMS_COUNT    15

struct  BRRlpItemRecord {
    BRRlpItemType type;

    // The encoding
    size_t bytesCount;
    uint8_t *bytes;
    uint8_t  bytesArray [ITEM_DEFAULT_BYTES_COUNT];

    // If CODER_LIST, then reference the component items.
    size_t itemsCount;
    BRRlpItem *items;
    BRRlpItem  itemsArray [ITEM_DEFAULT_ITEMS_COUNT];

    // double linked-list of free/busy items.
    BRRlpItem next, prev;
};

static void
itemReleaseMemory (BRRlpItem item) {
    if (item->bytesArray != item->bytes && NULL != item->bytes) free (item->bytes);
    if (item->itemsArray != item->items && NULL != item->items) free (item->items);

    memset (item, 0, sizeof (struct BRRlpItemRecord));
}

/**
 *
 */
struct BRRlpCoderRecord {
    /**
     * A boolean to indicate that some coder failed.
     */
    int failed;

    /**
     * A singly-link list of available RLP items.  When a RLP item is released we'll clear out
     * the RLP item and then add it to `free`.
     */
    BRRlpItem free;

    /**
     * A doubly-linked list of busy RLP items.  Fact is, we don't need to keep this list - you
     * acquire an item and you best be sure to release it and if you don't you've leaked memory.
     *
     * Well, in order to ensure that memory is not leaked, we keep a `busy` list and then on
     * `rlpCoderRelease()` we assert that there a no busy items - ensuring all are releaded.
     *
     * This busy list caused a problem (see bugfix/CORE-152).  It was singly linked and thus to
     * remove an item we were forced to traverse the list to find the just-prior item and then
     * set prev->next = item->next.  When we had an RLP item with ~300,000 sub-items the
     * traversal was computationally unmanageable.  Thus we changed to a doubly-linked list.  Fact
     * is we probaby don't need `busy`.
     */
    BRRlpItem busy;

    /**
     * It is not likely that this lock is actually needed, base on current `BRRlpCoder` use - coders
     * are only used in one thread.  However, that use my not be generally true - so lock/unlock.
     */
    pthread_mutex_t lock;
};

extern BRRlpCoder
rlpCoderCreate (void) {
    BRRlpCoder coder = malloc (sizeof (struct BRRlpCoderRecord));
    coder->failed = 0;
    coder->free = NULL;
    coder->busy = NULL;

    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
        pthread_mutex_init (&coder->lock, &attr);
        pthread_mutexattr_destroy(&attr);
    }
#if 0
    for (size_t index = 0; index < CODER_DEFAULT_ITEMS; index++) {
        BRRlpItem item = calloc (1, sizeof (struct BRRlpItemRecord));
        item->next = coder->free;
        coder->free = item;
    }
#endif
    return coder;
}

extern void
rlpCoderRelease (BRRlpCoder coder) {
    pthread_mutex_lock(&coder->lock);

    // Every single Item must be returned!
    assert (NULL == coder->busy);
    rlpCoderReclaimInternal (coder);

    pthread_mutex_unlock(&coder->lock);
    pthread_mutex_destroy(&coder->lock);
    free (coder);
}

static void
rlpCoderReclaimInternal (BRRlpCoder coder) {
    BRRlpItem item = coder->free;
    while (item != NULL) {
        BRRlpItem next = item->next;   // save 'next' before release...
        itemReleaseMemory (item);
        free (item);
        item = next;
    }
    coder->free = NULL;
}

extern void
rlpCoderReclaim (BRRlpCoder coder) {
    pthread_mutex_lock(&coder->lock);
    rlpCoderReclaimInternal (coder);
    pthread_mutex_unlock(&coder->lock);
}

extern int
rlpCoderBusyCount (BRRlpCoder coder) {
    int count = 0;
    for (BRRlpItem found = coder->busy; NULL != found; found = found->next)
        count++;
    return count;
}

static BRRlpItem
rlpCoderAcquireItem (BRRlpCoder coder) {
    BRRlpItem item = NULL;
    pthread_mutex_lock(&coder->lock);

    // Get `item` from `coder->free` or `calloc`
    if (NULL != coder->free) {
        item = coder->free;
        coder->free = item->next;
        item->next = NULL;
    }
    else item = calloc (1, sizeof (struct BRRlpItemRecord));

    assert (NULL == item->next       && NULL == item->prev &&
            0    == item->bytesCount && 0    == item->itemsCount);

    // Doubly-link `item` to `busy`
    if (NULL != coder->busy) coder->busy->prev = item;
    item->next = coder->busy;

    // Update `coder` to show `item` as busy.
    coder->busy = item;

    pthread_mutex_unlock(&coder->lock);
    return item;
}

static void
rlpCoderReturnItem (BRRlpCoder coder, BRRlpItem prev, BRRlpItem item, BRRlpItem next) {
    pthread_mutex_lock(&coder->lock);
    assert (NULL == item->next       && NULL == item->prev &&
            0    == item->bytesCount && 0    == item->itemsCount);

    // If `item` is a the head of our `busy` list, then just move `busy`
    if (item == coder->busy)
        coder->busy = next;

    // If `item` is not at the head of our `busy` list, then splice out `item` by
    // linking as `prev` <==> `next`
    else {
        if (NULL != prev) prev->next = next;
        if (NULL != next) next->prev = prev;
    }

    // The `item` is no longer busy.  Singlely link to `free`.
    item->prev = NULL;
    item->next = coder->free;

    // Update `coder` to show `item` as free.
    coder->free = item;

    pthread_mutex_unlock(&coder->lock);
}

static void
itemRelease (BRRlpCoder coder, BRRlpItem item) {
    BRRlpItem prev = item->prev;
    BRRlpItem next = item->next;

    itemReleaseMemory(item);
    rlpCoderReturnItem(coder, prev, item, next);
}

static int
itemIsValid (BRRlpCoder coder, BRRlpItem item) {
    return 1;
}

static BRRlpItem
itemCreateEmpty (BRRlpCoder coder, BRRlpItemType type) {
    BRRlpItem item = rlpCoderAcquireItem(coder); // calloc (1, sizeof (struct BRRlpItemRecord));
    item->type = type;
    return item;
}

static uint8_t *
itemEnsureBytes (BRRlpCoder coder, BRRlpItem item, size_t bytesCount) {
    assert (NULL == item->bytes);
    item->bytesCount = bytesCount;
    item->bytes = (item->bytesCount > ITEM_DEFAULT_BYTES_COUNT
                   ? malloc (item->bytesCount)
                   : item->bytesArray);
    return item->bytes;
}

static BRRlpItem
itemFillList (BRRlpCoder coder, BRRlpItem item, BRRlpItem *items, size_t itemsCount) {
    item->type = CODER_LIST;
    item->itemsCount = itemsCount;
    item->items = (item->itemsCount > ITEM_DEFAULT_ITEMS_COUNT
                   ? calloc (item->itemsCount, sizeof (BRRlpItem))
                   : item->itemsArray);
    for (int i = 0; i < itemsCount; i++)
        item->items[i] = items[i];
    return item;
}

extern void
rlpCoderSetFailed (BRRlpCoder coder) {
    coder->failed = 1;
}

extern void
rlpCoderClrFailed (BRRlpCoder coder) {
    coder->failed = 0;
}

extern int
rlpCoderHasFailed (BRRlpCoder coder) {
    return coder->failed;
}

#if 0
static BRRlpItem
itemCreate (BRRlpCoder coder,
            uint8_t *bytes, size_t bytesCount, int takeBytes) {
    BRRlpItem item = calloc (1, sizeof (struct BRRlpItemRecord));

    item->type = CODER_ITEM;
    item->bytesCount = bytesCount;
    if (takeBytes)
        item->bytes = bytes;
    else {
        item->bytes = (item->bytesCount > ITEM_DEFAULT_BYTES_COUNT
                       ? malloc (item->bytesCount)
                       : item->bytesArray);
        memcpy (item->bytes, bytes, item->bytesCount);
    }
    item->itemsCount = 0;
    item->items = NULL;

    return item;
}

static BRRlpItem
itemCreateList (BRRlpCoder coder,
                uint8_t *bytes, size_t bytesCount, int takeBytes,
                BRRlpItem *items, size_t itemsCount) {
    BRRlpItem item = itemCreate(coder, bytes, bytesCount, takeBytes);
    item->type = CODER_LIST;
    item->itemsCount = itemsCount;
    item->items = (item->itemsCount > ITEM_DEFAULT_ITEMS_COUNT
                   ? calloc (item->itemsCount, sizeof (BRRlpItem))
                   : item->itemsArray);
    for (int i = 0; i < itemsCount; i++)
        item->items[i] = items[i];
    
    return item;
}
#endif

/**
 * Return a new BRRlpContext by appending the two provided contexts.  Both provided contexts
 * must be for CODER_ITEM (othewise an 'assert' is raised); the appending is performed by simply
 * concatenating the two context's byte arrays.
 *
 * If release is TRUE, then both the provided contexts are released; thereby freeing their memory.
 *
 */
#if 0
static BRRlpItem
itemCreateAppend (BRRlpCoder coder, BRRlpItem context1, BRRlpItem context2, int release) {
    assert (CODER_ITEM == context1->type && CODER_ITEM == context2->type);

    BRRlpItem item = calloc (1, sizeof (struct BRRlpItemRecord));

    item->type = CODER_ITEM;
    
    item->bytesCount = context1->bytesCount + context2->bytesCount;
    item->bytes = (item->bytesCount > ITEM_DEFAULT_BYTES_COUNT
                   ? malloc (item->bytesCount)
                   : item->bytesArray);

    memcpy (&item->bytes[0], context1->bytes, context1->bytesCount);
    memcpy (&item->bytes[context1->bytesCount], context2->bytes, context2->bytesCount);

    item->itemsCount = 0;
    item->items = NULL;
    
    if (release) {
        //        assert (context2.bytes != context1.bytes || context2.bytes == NULL || context1.bytes == NULL);
        //        assert (context2.items != context1.items || context2.items == NULL || context1.items == NULL);
        itemRelease(coder, context1);
        itemRelease(coder, context2);
    }
    
    return item;
}
#endif

// The largest number supported for encoding is a UInt256 - which is representable as 32 bytes.
#define CODER_NUMBER_BYTES_LIMIT    (256/8)

/**
 * Return the index of the first non-zero byte; if all bytes are zero, bytesCount is returned
 */
static int
findNonZeroIndex (uint8_t *bytes, size_t bytesCount) {
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
swapBytesIfLittleEndian (uint8_t *target, uint8_t *source, size_t count) {
    assert (target != source);  // common overlap case, but wholely insufficient.
    for (int i = 0; i < count; i++) {
#if BYTE_ORDER == LITTLE_ENDIAN
        target[i] = source[count - 1 - i];
#else
        target[i] = source[i]
#endif
    }
}

/**
 * Fill `length` bytes formatted as big-endian into `target` from `source`.  Set `targetIndex`
 * as the `target` index of the first non-zero value; set `targetCount` is the number of bytes
 * after `targetIndex`.
 */
static void
convertToBigEndianAndNormalize (uint8_t *target, uint8_t *source, size_t length,
                                size_t *targetIndex, size_t *targetCount) {
    assert (length <= CODER_NUMBER_BYTES_LIMIT);
    
    swapBytesIfLittleEndian (target, source, length);
    
    *targetIndex = findNonZeroIndex(target, length);
    *targetCount = length - *targetIndex;
    
    if (0 == *targetCount) {
        *targetCount = 1;
        *targetIndex = 0;
    }
}

/**
 * Fill `targetCount` bytes into `target` using the big-endian formatted `bytesCount` at `bytes`.
 */
static void
convertFromBigEndian (uint8_t *target, size_t targetCount, uint8_t *bytes, size_t bytesCount) {
    // Bytes represents a number in big-endian              : 04 00
    // Fill out the number with prefix zeros                : 00 00 00 00 00 00 04 00
    // Copy the bytes into target, swap if little endian    : 00 04 00 00 00 00 00 00
    uint8_t value[targetCount];
    memset (value, 0, targetCount);
    memcpy (&value[targetCount - bytesCount], bytes, bytesCount);

    swapBytesIfLittleEndian(target, value, targetCount);
}

#define RLP_PREFIX_BYTES  (0x80)
#define RLP_PREFIX_LIST   (0xc0)
#define RLP_PREFIX_LENGTH_LIMIT  (55)

static void
encodeLengthIntoBytes (uint64_t length, uint8_t baseline,
                       uint8_t *bytes9, uint8_t *bytes9Count) {

    // If the length is small, simply encode a single byte as (baseline + length)
    if (length <= RLP_PREFIX_LENGTH_LIMIT) {
        bytes9[0] = baseline + length;
        *bytes9Count = 1;
        return;
    }
    // Otherwise, encode the length as bytes.
    else {
        size_t lengthSize = sizeof (uint64_t);

        //uint8_t bytes [lengthSize]; // big_endian representation of the bytes in 'length'
        size_t bytesIndex;          // Index of the first non-zero byte
        size_t bytesCount;          // The number of bytes to encode (beyond index)

        convertToBigEndianAndNormalize (bytes9, (uint8_t *) &length, lengthSize, &bytesIndex, &bytesCount);

        // The encoding - a header byte with the bytesCount and then the big_endian bytes themselves.
        uint8_t encoding [1 + bytesCount];
        encoding[0] = baseline + RLP_PREFIX_LENGTH_LIMIT + bytesCount;
        memcpy (&encoding[1], &bytes9[bytesIndex], bytesCount);

        // Copy back to bytes
        memcpy (bytes9, encoding, 1 + bytesCount);
        *bytes9Count = 1 + bytesCount;
    }
}

#if 0
static BRRlpItem
coderEncodeLength (BRRlpCoder coder, uint64_t length, uint8_t baseline) {
    uint8_t bytesCount, bytes[9];
    coderEncodeLengthIntoBytes(coder, length, baseline, bytes, &bytesCount);
    return itemCreate (coder, bytes, bytesCount, 0);
}
#endif

/**
 * The value of `length` is used throughout for memcpy() and releated functions; it must
 * be a valid `size_t` type.  Ethereum RLP defines a length as a maximum of 8 bytes which
 * for some (32-bit) architures will be larger then size_t.  On such an architecute,
 * which is becoming ever rarer, we'll uncerimoniously fatal if `length` is too big.
 *
 * SIZE_MAX is 4294967295UL on a 32bit arch - no way this ever fails...
 *
 *@param length
 *@return length as size_t
 */
static size_t
confirmLength (uint64_t length) {
    assert (length <= (uint64_t) SIZE_MAX);   // never return
    return (size_t) length;
}

static size_t
decodeLength (uint8_t *bytes, uint8_t baseline, uint8_t *offset) {
    uint8_t prefix = bytes[0];

    *offset = 0;
    if (prefix < baseline) return 1;

    else if ((prefix - baseline) <= RLP_PREFIX_LENGTH_LIMIT) {
        *offset = 1; // just prefix
        return (prefix - baseline);
    }
    else {
        // Number of bytes encoding the length
        size_t lengthByteCount = (prefix - baseline) - RLP_PREFIX_LENGTH_LIMIT;
        *offset = 1 + lengthByteCount; // prefix + length bytes

        // Result
        uint64_t length = 0;
        size_t lengthSize = sizeof (uint64_t);
        assert (lengthByteCount <= lengthSize);

        convertFromBigEndian((uint8_t*)&length, lengthSize, &bytes[1], lengthByteCount);
        return confirmLength(length);
    }
}

// Includes RLP length encoding
static void
decodeNumber (uint8_t *target, size_t targetCount, uint8_t *bytes, size_t bytesCount) {
    uint8_t offset = 0;
    size_t length = decodeLength(bytes, RLP_PREFIX_BYTES, &offset);

    convertFromBigEndian(target, targetCount, &bytes[offset], length);
}

extern UInt256
rlpDataDecodeUInt256 (BRRlpData data) {
    UInt256 result;
    convertFromBigEndian(result.u8, sizeof(result), data.bytes, data.bytesCount);
    return result;
}

extern uint64_t
rlpDataDecodeUInt64 (BRRlpData data) {
    uint64_t result;
    convertFromBigEndian((uint8_t*) &result, sizeof(result), data.bytes, data.bytesCount);
    return result;
}

static BRRlpItem
coderEncodeBytes(BRRlpCoder coder, uint8_t *bytes, size_t bytesCount) {
    BRRlpItem item = itemCreateEmpty(coder, CODER_ITEM);

    // Encode a single byte directly
    if (1 == bytesCount && bytes[0] < RLP_PREFIX_BYTES) {
        uint8_t *encodedBytes = itemEnsureBytes(coder, item, 1);
        encodedBytes[0] = bytes[0];
    }
    
    // otherwise, encode the length and then the bytes themselves
    else {
        uint8_t bytes9Count, bytes9[9];
        encodeLengthIntoBytes(bytesCount, RLP_PREFIX_BYTES, bytes9, &bytes9Count);

        uint8_t *encodedBytes = itemEnsureBytes(coder, item, bytes9Count + bytesCount);
        memcpy(encodedBytes, bytes9, bytes9Count);
        memcpy(&encodedBytes[bytes9Count], bytes, bytesCount);
    }
    return item;
}

//
// Number
//
static BRRlpItem
coderEncodeNumber (BRRlpCoder coder, uint8_t *source, size_t sourceCount) {
    // Encode a number by converting the number to a big_endian representation and then simply
    // encoding those bytes.
    uint8_t bytes [sourceCount]; // big_endian representation of the bytes in 'source'
    size_t bytesIndex;           // Index of the first non-zero byte
    size_t bytesCount;           // The number of bytes to encode
    
    convertToBigEndianAndNormalize (bytes, source, sourceCount, &bytesIndex, &bytesCount);
    
    return coderEncodeBytes(coder, &bytes[bytesIndex], bytesCount);
}

static void
coderDecodeNumber (BRRlpCoder coder, uint8_t *target, size_t targetCount, uint8_t *bytes, size_t bytesCount) {
    decodeNumber(target, targetCount, bytes, bytesCount);
}

//
// UInt64
//
static BRRlpItem
coderEncodeUInt64 (BRRlpCoder coder, uint64_t value) {
    return coderEncodeNumber(coder, (uint8_t *) &value, sizeof(value));
}

static uint64_t
coderDecodeUInt64 (BRRlpCoder coder, BRRlpItem context) {
    assert (itemIsValid(coder, context));
    uint64_t value = 0;
    coderDecodeNumber (coder, (uint8_t*)&value, sizeof(uint64_t), context->bytes, context->bytesCount);
    return value;
}

//
// UInt256
//
static BRRlpItem
coderEncodeUInt256 (BRRlpCoder coder, UInt256 value) {
    return coderEncodeNumber(coder, (uint8_t *) &value, sizeof(value));
}

static UInt256
coderDecodeUInt256 (BRRlpCoder coder, BRRlpItem context) {
    assert (itemIsValid(coder, context));
    UInt256 value = UINT256_ZERO;
    coderDecodeNumber (coder, (uint8_t*)&value, sizeof (UInt256), context->bytes, context->bytesCount);
    return value;
}

//
// List
//
static BRRlpItem
coderEncodeList (BRRlpCoder coder, BRRlpItem *items, size_t itemsCount) {
    // Validate the items
    for (int i = 0; i < itemsCount; i++) {
        assert (itemIsValid(coder, items[i]));
    }

    // Acquire an item
    BRRlpItem item = rlpCoderAcquireItem(coder);

    // Eventually fill these by concatentating bytes from each of `items`
    size_t bytesCount = 0;

    // Determine the number of concatenated bytes...
    for (int i = 0; i < itemsCount; i++)
        bytesCount += items[i]->bytesCount;

    // ... given that, determine the length encoding
    uint8_t bytes9Count, bytes9[9];
    encodeLengthIntoBytes (bytesCount, RLP_PREFIX_LIST, bytes9, &bytes9Count);

    // ... now allocate the memory needed as length-encoding-prefix + bytes
    uint8_t *bytes = itemEnsureBytes (coder, item, bytes9Count + bytesCount);

    // ... now fill in the length encoding
    memcpy (bytes, bytes9, bytes9Count);

    // ... and concatenate the bytes from items
    for (size_t bytesIndex = bytes9Count, i = 0; i < itemsCount; i++) {
        BRRlpItem itemContext = items[i];
        memcpy (&bytes[bytesIndex], itemContext->bytes, itemContext->bytesCount);
        bytesIndex += itemContext->bytesCount;
    }

    itemFillList(coder, item, items, itemsCount);
    return item;
}

//
// Public Interface
//

//
// UInt64
//
extern BRRlpItem
rlpEncodeUInt64(BRRlpCoder coder, uint64_t value, int zeroAsEmptyString) {
    return (1 == zeroAsEmptyString && 0 == value
            ? rlpEncodeString(coder, "")
            : coderEncodeUInt64(coder, value));
}

extern uint64_t
rlpDecodeUInt64(BRRlpCoder coder, BRRlpItem item, int zeroAsEmptyString) {
    return (1 == zeroAsEmptyString &&  rlpDecodeStringEmptyCheck (coder, item)
            ? 0
            : coderDecodeUInt64(coder, item));
}

//
// UInt256
//
extern BRRlpItem
rlpEncodeUInt256(BRRlpCoder coder, UInt256 value, int zeroAsEmptyString) {
    return (1 == zeroAsEmptyString && 0 == compareUInt256 (value, UINT256_ZERO)
            ? rlpEncodeString(coder, "")
            : coderEncodeUInt256(coder, value));
}

extern UInt256
rlpDecodeUInt256(BRRlpCoder coder, BRRlpItem item, int zeroAsEmptyString) {
    return (1 == zeroAsEmptyString &&  rlpDecodeStringEmptyCheck (coder, item)
            ? UINT256_ZERO
            : coderDecodeUInt256(coder, item));
}

//
// Bytes
//
extern BRRlpItem
rlpEncodeBytes(BRRlpCoder coder, uint8_t *bytes, size_t bytesCount) {
    return coderEncodeBytes(coder, bytes, bytesCount);
}

extern BRRlpItem
rlpEncodeBytesPurgeLeadingZeros (BRRlpCoder coder, uint8_t *bytes, size_t bytesCount) {
    size_t offset = 0;
    for (; offset < bytesCount; offset++)
        if (0 != bytes[offset]) break;
    return rlpEncodeBytes (coder, &bytes[offset], bytesCount - offset);
}

extern BRRlpData
rlpDecodeBytes (BRRlpCoder coder, BRRlpItem item) {
    assert (itemIsValid(coder, item));

    uint8_t offset = 0;
    size_t length = decodeLength(item->bytes, RLP_PREFIX_BYTES, &offset);

    BRRlpData result;
    result.bytesCount = length;
    result.bytes = malloc (length);
    memcpy (result.bytes, &item->bytes[offset], length);

    return result;
}

static BRRlpData
rlpDecodeBytesSharedDontReleaseBaseline (BRRlpCoder coder, BRRlpItem item, uint8_t baseline) {
    assert (itemIsValid (coder, item));

    uint8_t offset = 0;
    size_t length = decodeLength(item->bytes, baseline, &offset);

    BRRlpData result;
    result.bytesCount = length;
    result.bytes = &item->bytes[offset];

    return result;

}

extern BRRlpData
rlpDecodeBytesSharedDontRelease (BRRlpCoder coder, BRRlpItem item) {
    return rlpDecodeBytesSharedDontReleaseBaseline(coder, item, RLP_PREFIX_BYTES);
}

extern BRRlpData
rlpDecodeListSharedDontRelease (BRRlpCoder coder, BRRlpItem item) {
    return rlpDecodeBytesSharedDontReleaseBaseline(coder, item, RLP_PREFIX_LIST);
}

//
// String
//
extern BRRlpItem
rlpEncodeString (BRRlpCoder coder, char *string) {
    if (NULL == string) string = "";
    return rlpEncodeBytes(coder, (uint8_t *) string, strlen (string));
}

extern char *
rlpDecodeString (BRRlpCoder coder, BRRlpItem item) {
    assert (itemIsValid(coder, item));

    uint8_t offset = 0;
    size_t length = decodeLength(item->bytes, RLP_PREFIX_BYTES, &offset);

    char *result = malloc (length + 1);
    memcpy (result, &item->bytes[offset], length);
    result[length] = '\0';

    return result;
}

extern int
rlpDecodeStringCheck (BRRlpCoder coder, BRRlpItem item) {
    assert (itemIsValid(coder, item));
    return (CODER_ITEM == item->type
            && 0 != item->bytesCount
            && RLP_PREFIX_BYTES <= item->bytes[0]
            && item->bytes[0] <  RLP_PREFIX_LIST);
}

static int
rlpDecodeStringEmptyCheck (BRRlpCoder coder, BRRlpItem item) {
    assert (itemIsValid(coder, item));
    return (CODER_ITEM == item->type
            && 1 == item->bytesCount
            && RLP_PREFIX_BYTES <= item->bytes[0]);
}

//
// Hex String
//
extern BRRlpItem
rlpEncodeHexString (BRRlpCoder coder, char *string) {
    if (NULL == string || string[0] == '\0')
        return rlpEncodeString(coder, "");
    
    // Strip off "0x" if it exists
    if (0 == strncmp (string, "0x", 2))
        string = &string[2];

    size_t stringLen = strlen(string);
    assert (0 == stringLen % 2);

    // Decode Hex into BYTES; then RLP encode those bytes.  If string is sufficiently short
    // (under 16k) then avoid some memory allocation by using the stack.

    if (0 == stringLen)
        return rlpEncodeString(coder, string);
    else if (stringLen < (16 * 1024)) {
        size_t bytesCount = stringLen / 2;
        uint8_t bytes[bytesCount];
        decodeHex(bytes, bytesCount, string, stringLen);
        return rlpEncodeBytes(coder, bytes, bytesCount);
    }
    else {
        size_t bytesCount = 0;
        uint8_t *bytes = decodeHexCreate(&bytesCount, string, strlen(string));
        BRRlpItem item = rlpEncodeBytes(coder, bytes, bytesCount);
        free (bytes);
        return item;
    }
}

extern char *
rlpDecodeHexString (BRRlpCoder coder, BRRlpItem item, const char *prefix) {
    BRRlpData data = rlpDecodeBytes(coder, item);
    if (NULL == prefix) prefix = "";

    char *result = malloc (strlen(prefix) + 2 * data.bytesCount + 1);
    strcpy (result, prefix);
    encodeHex(&result[strlen(prefix)], 2 * data.bytesCount + 1, data.bytes, data.bytesCount);

    rlpDataRelease (data);
    return result;
}

//
// List
//
extern BRRlpItem
rlpEncodeList1 (BRRlpCoder coder, BRRlpItem item) {
    assert (itemIsValid(coder, item));
    BRRlpItem items[1];
    
    items[0] = item;
    
    return coderEncodeList(coder, items, 1);
}

extern BRRlpItem
rlpEncodeList2 (BRRlpCoder coder, BRRlpItem item1, BRRlpItem item2) {
    assert (itemIsValid(coder, item1));
    assert (itemIsValid(coder, item1));
    
    BRRlpItem items[2];
    
    items[0] = item1;
    items[1] = item2;
    
    return coderEncodeList(coder, items, 2);
}

extern BRRlpItem
rlpEncodeList (BRRlpCoder coder, size_t count, ...) {
    BRRlpItem items[count];
    
    va_list args;
    va_start (args, count);
    for (int i = 0; i < count; i++)
        items[i] = va_arg (args, BRRlpItem);
    va_end(args);
    
    return coderEncodeList(coder, items, count);
}

extern BRRlpItem
rlpEncodeListItems (BRRlpCoder coder, BRRlpItem *items, size_t itemsCount) {
    return coderEncodeList(coder, items, itemsCount);
}

extern const BRRlpItem *
rlpDecodeList (BRRlpCoder coder, BRRlpItem item, size_t *itemsCount) {
    assert (itemIsValid(coder, item));

    switch (item->type) {
        case CODER_ITEM:
            *itemsCount = 0;
            return NULL;
        case CODER_LIST:
            *itemsCount = item->itemsCount;
            return item->items;
    }
}

//
// RLP Data
//
extern BRRlpData
rlpDataCopy (BRRlpData data) {
    uint8_t *bytesCopy = NULL;
    if (NULL != data.bytes) {
        bytesCopy = malloc (data.bytesCount);
        memcpy (bytesCopy, data.bytes, data.bytesCount);
    }

    return (BRRlpData) { data.bytesCount, bytesCopy };
}

extern void
rlpDataRelease (BRRlpData data) {
    if (NULL != data.bytes) free (data.bytes);
    data.bytesCount = 0;
    data.bytes = NULL;
}

extern void
rlpDataExtract (BRRlpCoder coder, BRRlpItem item, uint8_t **bytes, size_t *bytesCount) {
    assert (itemIsValid(coder, item));
    assert (NULL != bytes && NULL != bytesCount);
    
    *bytesCount = item->bytesCount;
    *bytes = malloc (*bytesCount);
    memcpy (*bytes, item->bytes, item->bytesCount);
}

extern BRRlpData
rlpGetData (BRRlpCoder coder, BRRlpItem item) {
    BRRlpData data;
    rlpDataExtract(coder, item, &data.bytes, &data.bytesCount);
    return data;
}

extern BRRlpData
rlpGetDataSharedDontRelease (BRRlpCoder coder, BRRlpItem item) {
    assert (itemIsValid(coder, item));
    BRRlpData result = { item->bytesCount, item->bytes };
    return result;
}

/**
 * Return `data` with `bytes` and bytesCount derived from the bytes[0] and associated length.
 */
static BRRlpData
rlpGetItem_FillData (BRRlpCoder coder, uint8_t *bytes) {
    BRRlpData data;
    data.bytes = bytes;
    data.bytesCount = 1;

    uint8_t prefix = bytes[0];
    if (prefix >= RLP_PREFIX_BYTES) {
        uint8_t offset;
        data.bytesCount = decodeLength(bytes,
                                            (prefix < RLP_PREFIX_LIST ? RLP_PREFIX_BYTES : RLP_PREFIX_LIST),
                                            &offset);
        data.bytesCount += offset;
    }
    return data;
}

#define DEFAULT_ITEM_INCREMENT 20

/**
 * Convet the bytes in `data` into an `item`.  If `data` represents a RLP list, then `item` will
 * represent a list.
 */
extern BRRlpItem
rlpGetItem (BRRlpCoder coder, BRRlpData data) {
    assert (0 != data.bytesCount);

    BRRlpItem result = rlpCoderAcquireItem (coder);
    uint8_t *encodedBytes = itemEnsureBytes (coder, result, data.bytesCount);
    memcpy (encodedBytes, data.bytes, data.bytesCount);

    uint8_t prefix = data.bytes[0];

    // If not a list, then we are done; just return an `item` with `data`
    if (prefix < RLP_PREFIX_LIST) {
        return result;
    }

    // If a list, then we'll consume `data` with sub-items.
    else {
        // We can have an arbitrary number of sub-times.  Assume we have DEFAULT_ITEM_INCREMENT
        // but be willing to increase the number if needed.
        BRRlpItem itemsArray[DEFAULT_ITEM_INCREMENT];
        size_t itemsIndex = 0;
        size_t itemsCount = DEFAULT_ITEM_INCREMENT;

        // We'll use this to accumulate subitems.
        BRRlpItem *items = itemsArray;

        // The upper limit on bytes to consume.
        uint8_t *bytesLimit = data.bytes + data.bytesCount;
        uint8_t *bytes = data.bytes;

        // Start of `data` encodes a list with a number of bytes.  We'll start extracting
        // sub-items after the list's length.
        uint8_t bytesOffset = 0;
        size_t bytesCount = decodeLength(data.bytes, RLP_PREFIX_LIST, &bytesOffset);
        assert (data.bytesCount == bytesCount + bytesOffset);

        // Start of the first sub-item
        bytes += bytesOffset;
        
        while (bytes < bytesLimit) {
            // Get the `data` for this sub-item and then recurse
            BRRlpData d = rlpGetItem_FillData(coder, bytes);
            items[itemsIndex++] = rlpGetItem (coder, d);

            // Move to the next sub-item
            bytes += d.bytesCount;

            // Extend `items` is we've used the allocated number.
            if (itemsIndex == itemsCount) {
                itemsCount += DEFAULT_ITEM_INCREMENT;
                if (items == itemsArray) {
                    // Move 'off' the stack allocated array.
                    items = malloc(itemsCount * sizeof(BRRlpItem));
                    memcpy (items, itemsArray, itemsIndex * sizeof(BRRlpItem));
                }
                else
                    items = realloc(items, itemsCount * sizeof (BRRlpItem));
            }
        }
        itemFillList(coder, result, items, itemsIndex);

        if (items != itemsArray) free(items);
    }
    return result;
}

//
// Show
//
#define RLP_SHOW_INDENT_INCREMENT  2

static void
rlpShowItemInternal (BRRlpCoder coder, BRRlpItem context, const char *topic, int indent) {
    if (indent > 256) indent = 256;
    char spaces [257];
    memset (spaces, ' ', indent);
    spaces[indent] = '\0';

    switch (context->type) {
        case CODER_LIST:
            if (0 == context->itemsCount)
                eth_log(topic, "%sL  0: []", spaces);
            else {
                eth_log(topic, "%sL%3zu: [", spaces, context->itemsCount);
                for (int i = 0; i < context->itemsCount; i++)
                    rlpShowItemInternal(coder,
                                        context->items[i],
                                        topic,
                                        indent + RLP_SHOW_INDENT_INCREMENT);
                eth_log(topic, "%s]", spaces);
            }
            break;
        case CODER_ITEM: {
            // We'll display this as hex-encoded bytes; we could use rlpDecodeItemBytes() but
            // that allocates memory, which we don't need so critically herein.
            uint8_t offset = 0;
            size_t length = decodeLength(context->bytes, RLP_PREFIX_BYTES, &offset);

            // We'll limit the display to a string of 1024 characters.
            size_t bytesCount = length > 512 ? 512 : length;
            char string[1024 + 1];
            encodeHex(string, 2 * bytesCount + 1, &context->bytes[offset], bytesCount);

            eth_log(topic, "%sI%3zu: 0x%s%s", spaces, length, string,
                    (bytesCount == length ? "" : "..."));
            break;
        }
    }
}

extern void
rlpReleaseItem (BRRlpCoder coder, BRRlpItem item) {
    assert (itemIsValid(coder, item));
    for (size_t index = 0; index < item->itemsCount; index++)
        rlpReleaseItem(coder, item->items[index]);
    itemRelease(coder, item);
}

extern void
rlpShowItem (BRRlpCoder coder, BRRlpItem item, const char *topic) {
    rlpShowItemInternal(coder, item, topic, 0);
}

extern void
rlpShow (BRRlpData data, const char *topic) {
    BRRlpCoder coder = rlpCoderCreate();
    BRRlpItem item = rlpGetItem(coder, data);
    rlpShowItem (coder, item, topic);
    rlpReleaseItem(coder, item);
}

/*
 def rlp_decode(input):
 if len(input) == 0:
 return
 output = ''
 (offset, dataLen, type) = decode_length(input)
 if type is str:
 output = instantiate_str(substr(input, offset, dataLen))
 elif type is list:
 output = instantiate_list(substr(input, offset, dataLen))

 output + rlp_decode(substr(input, offset + dataLen))
 return output

 def decode_length(input):
 length = len(input)
 if length == 0:
 raise Exception("input is null")
 prefix = ord(input[0])
 if prefix <= 0x7f:
 return (0, 1, str)
 elif prefix <= 0xb7 and length > prefix - 0x80:
 strLen = prefix - 0x80
 return (1, strLen, str)
 elif prefix <= 0xbf and length > prefix - 0xb7 and length > prefix - 0xb7 + to_integer(substr(input, 1, prefix - 0xb7)):
 lenOfStrLen = prefix - 0xb7
 strLen = to_integer(substr(input, 1, lenOfStrLen))
 return (1 + lenOfStrLen, strLen, str)
 elif prefix <= 0xf7 and length > prefix - 0xc0:
 listLen = prefix - 0xc0;
 return (1, listLen, list)
 elif prefix <= 0xff and length > prefix - 0xf7 and length > prefix - 0xf7 + to_integer(substr(input, 1, prefix - 0xf7)):
 lenOfListLen = prefix - 0xf7
 listLen = to_integer(substr(input, 1, lenOfListLen))
 return (1 + lenOfListLen, listLen, list)
 else:
 raise Exception("input don't conform RLP encoding form")

 def to_integer(b)
 length = len(b)
 if length == 0:
 raise Exception("input is null")
 elif length == 1:
 return ord(b[0])
 else:
 return ord(substr(b, -1)) + to_integer(substr(b, 0, -1)) * 256
 */
