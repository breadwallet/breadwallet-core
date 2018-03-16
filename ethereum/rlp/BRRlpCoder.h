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


typedef struct {
  void *identifier;
  unsigned long indexer;
} BRRlpItem;

typedef struct BRRlpCoderRecord *BRRlpCoder;

extern BRRlpCoder
rlpCoderCreate (void);

extern void
rlpCoderRelease (BRRlpCoder coder);

extern BRRlpItem
rlpEncodeItemUInt64 (BRRlpCoder coder, uint64_t value);

extern BRRlpItem
rlpEncodeItemUInt256 (BRRlpCoder coder, UInt256 value);

extern BRRlpItem
rlpEncodeItemBytes (BRRlpCoder coder, uint8_t *bytes, size_t bytesCount);

extern BRRlpItem
rlpEncodeItemString (BRRlpCoder coder, char *string);

extern BRRlpItem
rlpEncodeItemHexString (BRRlpCoder coder, char *string);

extern BRRlpItem
rlpEncodeList1 (BRRlpCoder coder, BRRlpItem item1);

extern BRRlpItem
rlpEncodeList2 (BRRlpCoder coder, BRRlpItem item1, BRRlpItem item2);

extern BRRlpItem
rlpEncodeList (BRRlpCoder coder, size_t count, ...);

extern BRRlpItem
rlpEncodeListItems (BRRlpCoder coder, BRRlpItem *items, size_t itemsCount);

// Hold onto BRRlpItem 'forever'... then try to use... will fail because 'coder'
// will not have 'context'
extern void
rlpGetData (BRRlpCoder coder, BRRlpItem item, uint8_t **bytes, size_t *bytesCount);

typedef struct {
  size_t bytesCount;
  uint8_t *bytes;
} BRRlpData;

extern BRRlpData
createRlpDataEmpty (void);

extern void
rlpDataRelease (BRRlpData data);
  
#ifdef __cplusplus
}
#endif

#endif //BR_RLP_Coder_H
