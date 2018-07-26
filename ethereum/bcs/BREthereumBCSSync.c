//
//  BREthereumBCSSync.c
//  Core
//
//  Created by Ed Gamble on 7/25/18.
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
#include "BREthereumBCSPrivate.h"
#include "../les/BREthereumLES.h"

#define BCS_SYNC_DEFAULT_BLOCK_ARRAY_CAPACITY   100

// Do we report block headers or blocks themselves?  The LINEAR_SYNC must/will fill in blocks -
// requesting bodies and receipts as appropriate.  Thus the SYNC_BINARY must as well.
//
// If blocks, then they will be fully constituted with {transactions, ommers} and a status of
// 'COMPLETE' including the interesting transactions and logs and, perhaps, the account state.
// When chained the transactions and logs get


struct BREthereumBCSSyncStruct {
    BREthereumBCSSyncType type;

    BREthereumBCSSyncContext context;
    BREthereumBCSSyncReportBlocks callback;

    uint64_t blockNumberStart;
    uint64_t blockNumberEnd;

    BRArrayOf(BREthereumBlock) reportedblocks;
    BRArrayOf(BREthereumBlock) pendingBlocks;

    union {
        struct {
            uint64_t blockNumberCurrent;
        } linear;

        struct {
        } binary;
    } u;

    BREthereumLES les;
};


static BREthereumBCSSync
bcsSyncCreate (BREthereumBCSSyncType type,
               BREthereumBCSSyncContext context,
               BREthereumBCSSyncReportBlocks callback,
               BREthereumLES les) {
    BREthereumBCSSync sync = malloc (sizeof(struct BREthereumBCSSyncStruct));
    sync->type = type;
    sync->context = context;
    sync->callback = callback;

    array_new (sync->reportedblocks, BCS_SYNC_DEFAULT_BLOCK_ARRAY_CAPACITY);
    array_new (sync->pendingBlocks, BCS_SYNC_DEFAULT_BLOCK_ARRAY_CAPACITY);

    sync->les = les;
    return sync;
}
extern BREthereumBCSSync
bcsSyncCreateLinear (BREthereumBCSSyncContext context,
                     BREthereumBCSSyncReportBlocks callback,
                     BREthereumLES les) {
    BREthereumBCSSync sync = bcsSyncCreate (SYNC_LINEAR, context, callback, les);
    // ...
    return sync;
}

extern BREthereumBCSSync
bcsSyncCreateBinary (BREthereumBCSSyncContext context,
                     BREthereumBCSSyncReportBlocks callback,
                     BREthereumLES les) {
    BREthereumBCSSync sync = bcsSyncCreate (SYNC_BINARY, context, callback, les);
    // ...
    return  sync;
}

extern void
bcsSyncRelease (BREthereumBCSSync sync) {
    memset (sync, 0, sizeof (struct BREthereumBCSSyncStruct));
    free (sync);
}

extern void
bcsSyncStart (BREthereumBCSSync sync) {
    // Use LES
    // Accumulate header
    // report headers/blocks with ETH/TOK transfers
}

#if 0
eth_log("BCS", "Block Sync {%llu, %llu}",
        blockStart,
        blockStart + blockCount);

lesGetBlockHeaders(bcs->les,
                   (BREthereumLESBlockHeadersContext) bcs,
                   (BREthereumLESBlockHeadersCallback) bcsSignalBlockHeader,
                   blockStart,
                   blockCount,
                   0,
                   ETHEREUM_BOOLEAN_FALSE);
#endif
