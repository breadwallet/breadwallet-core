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

/* Forward Declarations */
static void
syncRangeComplete (BREthereumBCSSyncRange child);

static void
syncRangeDispatch (BREthereumBCSSyncRange range);

static void
computeOptimalStep (uint64_t numberOfBlocks,
                    uint64_t *optimalStep,
                    uint64_t *optimalCount);
/**
 *
 */
typedef enum {
    SYNC_LINEAR_SMALL,      // leaf
    SYNC_LINEAR_LARGE,      // children: SMALL + [dynamic SMALL]
    SYNC_BINARY,            // children: [dynamic]
    SYNC_BINARY_MIXED       // children: BINARY + SMALL
} BREthereumBCSSyncType;


/**
 * For a 'binary sync' we'll split the range (of needed blockNumbers) into sub-ranges.  We'll find
 * the optimum number of subranges (such that the subranges exactly span the parent range). We'll
 * limit the number of subranges to between MINIMUM and MAXIMUM (below).  The maximum is determined
 * by the maximum in LES GetBlockHeaders; the minimum is arbitrary.
 */
#define SYNC_BINARY_REQUEST_MINIMUM     (100)
#define SYNC_BINARY_REQUEST_MAXIMUM     (192)     // maximum number of headers in a LES request.

/**
 * For a 'linear sync' we'll request at most MAXIMUM headers.  The maximum is determined by the
 * maximum in LES GetBlockHeaders.
 *
 * We'll favor a 'linear sync' over a 'binary sync' if the range (of needed blockNumbers) is less
 * than LIMIT (below)
 */
#define SYNC_LINEAR_REQUEST_MAXIMUM     (192)
#define SYNC_LINEAR_LIMIT               (10 * SYNC_LINEAR_REQUEST_MAXIMUM)

/**
 * As the sync find results (block headers, at least) we'll report them every PERIOD results.
 */
#define BCS_SYNC_RESULT_PERIOD  250

/**
 * The Sync Result State identifies the current state of the sync processing.  Depending on the
 * Sync Type we may require a LES request for headers and account state.  We mark our progess
 * as NONE, HEADER, and ACCOUNT.
 */
typedef enum {
    SYNC_RESULT_NONE,
    SYNC_RESULT_HEADER,
    SYNC_RESULT_ACCOUNT
} BREthereumBCSSyncResultState;

/**
 * The Context for the Sync Range Callback
 */
typedef void* BREthereumBCSSyncRangeContext;

/**
 * The Sync Range Callback to announce each result (block headers, at least).
 */
typedef void
(*BREthereumBCSSyncRangeCallback) (BREthereumBCSSyncRangeContext context,
                                     BREthereumBlockHeader header);

/**
 * A Sync Range defines the range of block header numbers overwhich a sync is performed.  The
 * type of sync over the range varies (see BREthereumBCSSyncType).
 */
struct BREthereumBCSSyncRangeRecord {

    /** Addres of interest */
    BREthereumAddress address;

    /** LES for Node interactions */
    BREthereumLES les;

    /** Event query handling our events */
    BREventHandler handler;

    /** Callback */
    BREthereumBCSSyncRangeContext context;
    BREthereumBCSSyncRangeCallback callback;

    /** Parameters defining this Range */
    BREthereumBCSSyncType type;
    uint64_t tail;
    uint64_t head;
    uint64_t step;
    uint64_t count;

    /** Accumlated results */
    uint64_t resultCount;
    struct {
        BREthereumBCSSyncResultState state;
        BREthereumBlockHeader header;
        BREthereumAccountState account;
    } result[SYNC_BINARY_REQUEST_MAXIMUM];

    /** Tree structure */
    BREthereumBCSSyncRange parent;
    BRArrayOf(BREthereumBCSSyncRange) children;
};


/**
 * Create a Sync Range will all the paremeters provided
 */
static BREthereumBCSSyncRange
syncRangeCreateDetailed (BREthereumAddress address,
                           BREthereumLES les,
                           BREventHandler handler,
                           BREthereumBCSSyncRangeContext context,
                           BREthereumBCSSyncRangeCallback callback,
                           uint64_t tail,
                           uint64_t head,
                           uint64_t step,
                           uint64_t count,
                           BREthereumBCSSyncType type) {
    BREthereumBCSSyncRange range = calloc (1, sizeof (struct BREthereumBCSSyncRangeRecord));

    range->address = address;
    range->les = les;
    range->handler = handler;

    range->context = context;
    range->callback = callback;

    range->tail = tail;
    range->head = head;
    range->step = step;
    range->count = count;
    range->type  = type;

    range->resultCount = 0;

    range->parent = NULL;
    range->children = NULL;

    //    eth_log ("BCS", "Sync: Create: Type: %d; Count: %llu, Range: {%llu, %llu)",
    //             range->type, range->count, range->tail, range->head);

    return range;
}

/**
 * Return the Event Handler
 */
extern BREventHandler
bcsSyncRangeGetHandler (BREthereumBCSSyncRange range) {
    return range->handler;
}

/**
 * Find `child` among the children of `parent`.  If found, return the index; otherwise -1.
 */
static int
syncRangeLookupChild (BREthereumBCSSyncRange parent,
                        BREthereumBCSSyncRange child) {
    if (NULL != parent->children)
        for (size_t index = 0; index < array_count(parent->children); index++)
            if (child == parent->children[index])
                return (int) index;
    return -1;
}

/**
 * Add `child` to the children of `parent`.  Added as the last child; will fatal if `child`
 * is already among children.
 */
static void
syncRangeAddChild (BREthereumBCSSyncRange parent,
                     BREthereumBCSSyncRange child) {
    assert (-1 == syncRangeLookupChild(parent, child));

    if (NULL == parent->children)
        array_new (parent->children, 5);

    array_add (parent->children, child);
    child->parent = parent;
}

/**
 * Remove `child` from its parent.  Will fatal if child is parent-less.
 */
static void
syncRangeRemChild (BREthereumBCSSyncRange child) {
    assert (NULL != child->parent);
    assert (NULL != child->parent->children);

    BREthereumBCSSyncRange parent = child->parent;

    int index = syncRangeLookupChild(parent, child);
    if (-1 != index) {
        array_rm (parent->children, index);
        child->parent = NULL;
    }
}

/**
 * Create a Sync Range as a child of `parent`.  This is a convenience method to 'inherit' many
 * of the parent's propertyes (address, les, handler).
 */
static void
syncRangeCreateAndAddChild (BREthereumBCSSyncRange parent,
                              uint64_t tail,
                              uint64_t head,
                              uint64_t step,
                              uint64_t count,
                              BREthereumBCSSyncType type) {
    syncRangeAddChild (parent, syncRangeCreateDetailed (parent->address,
                                                            parent->les,
                                                            parent->handler,
                                                            NULL,
                                                            NULL,
                                                            tail,
                                                            head,
                                                            step,
                                                            count,
                                                            type));
}


/**
 * Create a Sync Range based on block numbers for `tail` and `head`.  The range's type will be
 * determined from the total number of needed blocks; given the range, sync parameters (notably
 * `step` and `count` for a 'binary sync') will be optimized.
 */
static BREthereumBCSSyncRange
syncRangeCreate (BREthereumAddress address,
                   BREthereumLES les,
                   BREventHandler handler,
                   BREthereumBCSSyncRangeContext context,
                   BREthereumBCSSyncRangeContext callback,
                   uint64_t tail,
                   uint64_t head) { // binaryThreshold

    uint64_t total = head - tail;

    uint64_t step = 1;
    uint64_t count = total;

    BREthereumBCSSyncType type;

    // Determine the type
    if (total < SYNC_LINEAR_REQUEST_MAXIMUM) type = SYNC_LINEAR_SMALL;
    else if (total < SYNC_LINEAR_LIMIT) type = SYNC_LINEAR_LARGE;
    else {
        computeOptimalStep(total, &step, &count);
        type = (total == step * count
                ? SYNC_BINARY
                : SYNC_BINARY_MIXED);
    }

    BREthereumBCSSyncRange root = syncRangeCreateDetailed (address, les, handler,
                                                               context, callback,
                                                               tail,
                                                               head,
                                                               step,
                                                               count,
                                                               type);

    switch (type) {
        case SYNC_LINEAR_SMALL:
        case SYNC_BINARY:
            break;

        case SYNC_LINEAR_LARGE:
            syncRangeCreateAndAddChild (root,
                                          tail,
                                          head + SYNC_LINEAR_REQUEST_MAXIMUM - 1,
                                          1,
                                          SYNC_LINEAR_REQUEST_MAXIMUM,
                                          SYNC_LINEAR_SMALL);
            break;

        case SYNC_BINARY_MIXED:
            syncRangeCreateAndAddChild (root,
                                          tail,
                                          tail + step * count,
                                          step,
                                          count,
                                          SYNC_BINARY);

            syncRangeCreateAndAddChild (root,
                                          tail + step * count,
                                          head,
                                          1,
                                          head - (tail + step * count),
                                          SYNC_LINEAR_SMALL);
            break;
    }

    return root;
}

/**
 * Dispatch a Sync Range by a) issuing a LES request and/or b) dispatching any children.
 */
static void
syncRangeDispatch (BREthereumBCSSyncRange range) {
    eth_log ("BCS", "Sync: Dispatch: Type: %d; Count: %3llu; Range: {%llu, %llu)",
             range->type, range->count, range->tail, range->head);

    switch (range->type) {
        case SYNC_LINEAR_SMALL:
        case SYNC_BINARY:
            lesGetBlockHeaders (range->les,
                                (BREthereumLESBlockHeadersContext) range,
                                (BREthereumLESBlockHeadersCallback) bcsSyncSignalBlockHeader,
                                range->tail,
                                range->count + 1,  // both endpoints
                                range->step - 1,   // skip
                                ETHEREUM_BOOLEAN_FALSE);
            break;

        case SYNC_BINARY_MIXED:
        case SYNC_LINEAR_LARGE:
            assert (NULL != range->children);
            if (0 == array_count(range->children))
                syncRangeComplete(range);
            else
                syncRangeDispatch(range->children[0]);
            break;
    }
}

/**
 * Complete a Sync Range by a) completing the sync overall if `child` is the root, b) dispatching
 * on any remaining children of parent.
 */
static void
syncRangeComplete (BREthereumBCSSyncRange child) {
    BREthereumBCSSyncRange parent = child->parent;

    // If `child` does not have a `parent`, then we are at the top-level and completely complete.
    if (NULL == parent) {
        eth_log ("BCS", "Sync: Done%s", "");
        return;
    }

    eth_log ("BCS", "Sync: Complete: Type: %d; Count: %3llu; Range: {%llu, %llu)",
             child->type, child->count, child->tail, child->head);

    // If `child` does have a `parent`, then `parent` must have children, including `child`.
    assert (NULL != parent->children);

    // Remove the child
    syncRangeRemChild(child);

    // If we have children remaining, then dispatch the first one; otherwise ...
    if (array_count(parent->children) > 0)
        syncRangeDispatch(parent->children[0]);

    // ... parent is complete.
    else
        syncRangeComplete (parent);
}

/**
 * Get the root for `range`.
 */
static BREthereumBCSSyncRange
syncRangeGetRoot (BREthereumBCSSyncRange range) {
    return (NULL == range->parent
            ? range
            : syncRangeGetRoot(range->parent));
}

/**
 * Add a single `header` result to `range`.  If all results have been provided then: a) for a
 * BINARY range, request the account states; or b) for a LINEAR_SMALL range, invoke the callback
 * to report the header results.
 */
static void
syncRangeAddResultHeader (BREthereumBCSSyncRange range,
                            BREthereumBlockHeader header) {
    range->result[range->resultCount].state = SYNC_RESULT_HEADER;
    range->result[range->resultCount].header = header;
    range->resultCount += 1;

//    eth_log ("BCS", "Sync: Header: Type: %d; Count: %llu; Range: {%llu, %llu); Results: %llu",
//             range->type, range->count, range->tail, range->head, range->resultCount);

    // If we have all requested headers, then move on.
    if (range->resultCount == 1 + range->count) {
        switch (range->type) {
            case SYNC_BINARY_MIXED:
            case SYNC_LINEAR_LARGE:
                assert (0);
                break;

            case SYNC_BINARY: {
                uint64_t count = range->resultCount;
                range->resultCount = 0;
                for (size_t index = 0; index < count; index++) {
                    BREthereumBlockHeader header = range->result[index].header;
                    lesGetAccountState(range->les,
                                       (BREthereumLESAccountStateContext) range,
                                       (BREthereumLESAccountStateCallback) bcsSyncHandleAccountState,
                                       blockHeaderGetNumber(header),
                                       blockHeaderGetHash (header),
                                       range->address);
                }
                break;
            }

            case SYNC_LINEAR_SMALL: {
                BREthereumBCSSyncRange root = syncRangeGetRoot(range);
                for (size_t index = 0; index < range->resultCount; index++)
                    root->callback (root->context, range->result[index].header);

                syncRangeComplete (range);
                break;
            }
        }
    }
}

/**
 * Add a single `account state` to `range`.  This only applies to a BINARY sync.  If all results
 * have been provided then compare each pair of consecutive accounts and if different create a
 * new subrange as a child to range.  Once all accounts have been compared then dispatch on the
 * first child (if any exist).
 */
static void
syncRangeAddResultAccountState (BREthereumBCSSyncRange range,
                                  BREthereumAccountState accountState) {
    range->result[range->resultCount].state = SYNC_RESULT_ACCOUNT;
    range->result[range->resultCount].account = accountState;
    range->resultCount += 1;
    
    assert (SYNC_BINARY == range->type);
    
    if (range->resultCount == 1 + range->count) {
        assert (range->resultCount > 1);
        for (size_t index = 1; index < range->resultCount; index++) {
            BREthereumAccountState oldState = range->result[index - 1].account;
            BREthereumAccountState newState = range->result[index].account;

            // If we found an AcountState change...
            if (ETHEREUM_BOOLEAN_IS_FALSE(accountStateEqual(oldState, newState))) {
                BREthereumBlockHeader oldHeader = range->result[index - 1].header;
                BREthereumBlockHeader newHeader = range->result[index].header;
                
                uint64_t oldNumber = blockHeaderGetNumber(oldHeader);
                uint64_t newNumber = blockHeaderGetNumber(newHeader);
                
                assert (newNumber > oldNumber);

                // ... hen we need to explore this header range, recursively.
                syncRangeAddChild (range,
                                     syncRangeCreate(range->address,
                                                       range->les,
                                                       range->handler,
                                                       NULL,
                                                       NULL,
                                                       oldNumber,
                                                       newNumber));
            }
        }
        
        // If we now have children, dispatch on the first one.  As each one completes, we'll
        // dispatch on the subsequent ones until this BINARY range itself completes. Otherwise ...
        if (NULL != range->children && array_count(range->children) > 0)
            syncRangeDispatch(range->children[0]);
        
        // ... nothing left, completely complete here and now.
        else
            syncRangeComplete(range);
    }
}

///
/// MARK: BCS Sync
///

/**
 * A BCS Sync handles ongoing sync reqeusts.  A call to bcsSyncContinue() will start a sync as
 * needed.
 */
struct BREthereumBCSSyncStruct {
    /** Addres of interest */
    BREthereumAddress address;

    /** LES for Node interactions */
    BREthereumLES les;

    /** Event query handling our events */
    BREventHandler handler;

    /** Callback */
    BREthereumBCSSyncContext context;
    BREthereumBCSSyncReportBlocks callback;

    /** The root `range`, if a sync is in progress */
    BREthereumBCSSyncRange range;

    /** Accumulated sync results.  Will be periodically reported with the callback. */
    BRArrayOf(BREthereumBCSSyncResult) results;
};

/**
 * Create a BCS sync.
 */
extern BREthereumBCSSync
bcsSyncCreate (BREthereumBCSSyncContext context,
               BREthereumBCSSyncReportBlocks callback,
               BREthereumAddress address,
               BREthereumLES les,
               BREventHandler handler) {
    BREthereumBCSSync sync = malloc (sizeof(struct BREthereumBCSSyncStruct));
    sync->address = address;
    sync->les = les;
    sync->handler = handler;

    sync->context = context;
    sync->callback = callback;

    sync->range = NULL;

    array_new (sync->results, BCS_SYNC_RESULT_PERIOD);
    return sync;
}

/**
 * Release `sync`
 */
extern void
bcsSyncRelease (BREthereumBCSSync sync) {
    // TODO: Recursively release `root`; ensure that pending LES callbacks don't crash.
    memset (sync, 0, sizeof (struct BREthereumBCSSyncStruct));
    free (sync);
}

/**
 * Return the event handler for `sync`.
 */
extern BREventHandler
bcsSyncHandler (BREthereumBCSSync sync) {
    return sync->handler;
}

/**
 * Return `true` if active; `false` otherwise
 */
extern BREthereumBoolean
bcsSyncIsActive (BREthereumBCSSync sync) {
    return AS_ETHEREUM_BOOLEAN(NULL != sync->range);
}

extern void
bcsSyncStart (BREthereumBCSSync sync) {
    // Use LES
    // Accumulate header
    // report headers/blocks with ETH/TOK transfers
}

extern void
bcsSyncStop (BREthereumBCSSync sync) {

}

/**
 * The callback for sync ranges.  Will add `header` to `results` and periodically invoke the
 * sync callback (to BCS, generally).
 */
static void
bcsSyncRangeCallback (BREthereumBCSSync sync,
                        BREthereumBlockHeader header) {
    BREthereumBCSSyncResult result = {
        header
    };
    array_add (sync->results, result);
    if (BCS_SYNC_RESULT_PERIOD == array_count(sync->results)) {
        sync->callback (sync->context,
                        sync,
                        sync->results,
                        0.0);
        array_new (sync->results, BCS_SYNC_RESULT_PERIOD);
    }
}

/**
 * Continue a sync for blocks from `chainBlockNumber` to `needBlockNumber`.
 */
extern void
bcsSyncContinue (BREthereumBCSSync sync,
                 uint64_t chainBlockNumber,
                 uint64_t needBlockNumber) {
    // Skip out if already syncing.
    if (NULL != sync->range) return;

    sync->range = syncRangeCreate (sync->address,
                                       sync->les,
                                       sync->handler,
                                       (BREthereumBCSSyncRangeContext) sync,
                                       (BREthereumBCSSyncRangeCallback) bcsSyncRangeCallback,
                                       chainBlockNumber /* + 1 */,
                                       needBlockNumber);
    syncRangeDispatch(sync->range);

    ////        bcs->listener.syncCallback (bcs->listener.context,
    ////                                    BCS_CALLBACK_SYNC_STOPPED,
    ////                                    bcs->syncTail,
    ////                                    bcs->syncHead,
    ////                                    bcs->syncHead);
}

///
/// MARK: - Sync
///


//
extern double
bcsSyncPercentComplete (BREthereumBCSSync sync) {
    if (NULL == sync->range) return 100.00;

    uint64_t needCount = sync->range->head - sync->range->tail;
    return 0.0;
//    return (100.0 * sync->doneCount) / needCount;
}

/** Handle a LES callback for BlockHeader */
extern void
bcsSyncHandleBlockHeader (BREthereumBCSSyncRange range,
                          BREthereumBlockHeader header) {
    // Fill the range.
    syncRangeAddResultHeader(range, header);
}

/** Handle a LES callback for Account State */
extern void
bcsSyncHandleAccountState (BREthereumBCSSyncRange range,
                           BREthereumLESAccountStateResult result) {
    // Out-of-order arrival - in result, match with hash
    syncRangeAddResultAccountState(range, result.u.success.accountState);
}

/**
 * Compute the optimal `step` and `count` for a binary sync over `numberOfBlocks`.
 *
 * @param numberOfBlocks
 * @param optimalStep
 * @param optimalCount
 */
static void
computeOptimalStep (uint64_t numberOfBlocks,
                    uint64_t *optimalStep,
                    uint64_t *optimalCount) {
    *optimalCount = 0;
    uint64_t optimalRemainder = UINT64_MAX;
    for (int count = SYNC_BINARY_REQUEST_MINIMUM; count < SYNC_BINARY_REQUEST_MAXIMUM; count++) {
        uint64_t remainder = numberOfBlocks % count;
        if (remainder <= optimalRemainder) {
            optimalRemainder = remainder;
            *optimalCount = count;
        }
    }
    *optimalStep  = (numberOfBlocks / (*optimalCount));
}



uint64_t optimalStep;
uint64_t optimalCount;
extern void optimal (uint64_t number) { computeOptimalStep (number, &optimalStep, &optimalCount); }

