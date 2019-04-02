//
//  rlp
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 2/25/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_RLP_Coder_H
#define BR_RLP_Coder_H

#include <stddef.h>
#include <stdint.h>
#include "support/BRInt.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// RLP Coder
//
typedef struct BRRlpCoderRecord *BRRlpCoder;

extern BRRlpCoder
rlpCoderCreate (void);

extern void
rlpCoderRelease (BRRlpCoder coder);

/**
 * Reclaim coder memory. A coder can hold memory to avoid repeated free/malloc calls.  If
 * desired one can reclaim coder memory that is unused.
 */
extern void
rlpCoderReclaim (BRRlpCoder coder);

extern void
rlpCoderSetFailed (BRRlpCoder coder);

extern void
rlpCoderClrFailed (BRRlpCoder coder);

extern int
rlpCoderHasFailed (BRRlpCoder coder);

//
// RLP Data
//
typedef struct {
    size_t bytesCount;
    uint8_t *bytes;
} BRRlpData;

extern BRRlpData
rlpDataCopy (BRRlpData data);

extern void
rlpDataRelease (BRRlpData data);

//
// RLP Item
//
typedef struct BRRlpItemRecord *BRRlpItem;

extern void
rlpReleaseItem (BRRlpCoder coder, BRRlpItem item);

/**
 * Convet the bytes in `data` into an `item`.  If `data` represents a RLP list, then `item` will
 * represent a list (thus `data` is 'walked' to identify subitems, including sublists).
 */
extern BRRlpItem
rlpGetItem (BRRlpCoder coder, BRRlpData data);

/**
 * Return the RLP data associated with `item`.  You own this data and must call
 * rlpDataRelese().
 */
extern BRRlpData
rlpGetData (BRRlpCoder coder, BRRlpItem item);

/**
 * Return the RLP data associated with `item`.  You DO NOT own this data; you must not
 * modify the data nor release the data nor hold the data.  [The returned data is a direct
 * pointer into the `coder` memory and will become invalid on coder release.
 */
extern BRRlpData
rlpGetDataSharedDontRelease (BRRlpCoder coder, BRRlpItem item);

/**
 * Extract the `bytes` and `bytesCount` for `item`.  The returns `bytes` will be the complete
 * RLP encoding for `item` which includes the RLP encoding of length.  Contrast this with
 * rlpDecodeItemBytes() which only returns the RLP encoding data w/o the length.
 * Extract the `item` data as a copy, filling `bytes` and `bytesCount`.
 *
 * TODO: ?? Hold onto BRRlpItem 'forever'... then try to use... will fail because 'coder'
 * TODO: will not have 'context' ??
 */
//extern void
//rlpDataExtract (BRRlpItem item, uint8_t **bytes, size_t *bytesCount);

//
// UInt64
//
extern BRRlpItem
rlpEncodeUInt64(BRRlpCoder coder, uint64_t value, int zeroAsEmptyString);

extern uint64_t
rlpDecodeUInt64(BRRlpCoder coder, BRRlpItem item, int zeroAsEmptyString);

//
// UInt256
//
extern BRRlpItem
rlpEncodeUInt256(BRRlpCoder coder, UInt256 value, int zeroAsEmptyString);

extern UInt256
rlpDecodeUInt256(BRRlpCoder coder, BRRlpItem item, int zeroAsEmptyString);

//
// Bytes
//
extern BRRlpItem
rlpEncodeBytes (BRRlpCoder coder, uint8_t *bytes, size_t bytesCount);

extern BRRlpItem
rlpEncodeBytesPurgeLeadingZeros (BRRlpCoder coder, uint8_t *bytes, size_t bytesCount);

/**
 * Extract the `data` for `item` as the item's bytes w/o the RLP encoding of length.  Thus, if
 * you used rlpEncodeItemBytes() to encode `bytes` and `bytesCount`, then the result of
 * rlpDecodeItemBytes() will be `data` with exactly the same data as was encoded.
 */
extern BRRlpData
rlpDecodeBytes (BRRlpCoder coder, BRRlpItem item);

extern BRRlpData
rlpDecodeBytesSharedDontRelease (BRRlpCoder coder, BRRlpItem item);

extern BRRlpData
rlpDecodeListSharedDontRelease (BRRlpCoder coder, BRRlpItem item);

//
// String
//
extern BRRlpItem
rlpEncodeString (BRRlpCoder coder, char *string);

extern char *
rlpDecodeString (BRRlpCoder coder, BRRlpItem item);

extern int
rlpDecodeStringCheck (BRRlpCoder coder, BRRlpItem item);

//
// Hex String
//
extern BRRlpItem
rlpEncodeHexString (BRRlpCoder coder, char *string);

extern char *
rlpDecodeHexString (BRRlpCoder coder, BRRlpItem item, const char *prefix);

//
// List
//
extern BRRlpItem
rlpEncodeList1 (BRRlpCoder coder, BRRlpItem item1);

extern BRRlpItem
rlpEncodeList2 (BRRlpCoder coder, BRRlpItem item1, BRRlpItem item2);

extern BRRlpItem
rlpEncodeList (BRRlpCoder coder, size_t count, ...);

extern BRRlpItem
rlpEncodeListItems (BRRlpCoder coder, BRRlpItem *items, size_t itemsCount);

extern const BRRlpItem *
rlpDecodeList (BRRlpCoder coder, BRRlpItem item, size_t *itemsCount);
    
//
// Show
//
extern void
rlpShowItem (BRRlpCoder coder, BRRlpItem item, const char *topic);

extern void
rlpShow (BRRlpData data, const char *topic);

//
// Decode RLPData directly to numbers
//   (Used for logGetData -> UInt256
//
extern UInt256
rlpDataDecodeUInt256 (BRRlpData data);

extern uint64_t
rlpDataDecodeUInt64 (BRRlpData data);

#ifdef __cplusplus
}
#endif

#endif //BR_RLP_Coder_H
