//
//  BREthereumBCSSync.c
//  Core
//
//  Created by Ed Gamble on 7/25/18.
//  Copyright © 2018-2019 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <stdlib.h>
#include "ethereum/les/BREthereumLES.h"
#include "BREthereumBCSPrivate.h"

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
 * The BCS Sync Type represents the types of nodes in an N-ary tree.  We sync Ethereum blocks based
 * on a N-ary search for regions of blocks where the account state of the desired address changed.
 *
 * When a block region is large (> SYNC_LINEAR_LIMIT), we'll split the region up into above 150
 * sub-regions (actual number is between SYNC_N_ARY_REQUEST_MINIMUM and
 * SYNC_N_ARY_REQUEST_MAXIMUM) and then request account state for the sub-region boundary headers.
 * If the account state did not change over the subregion, then the sync is done for that subregion;
 * but, if the account state did change, when we recursively sync that subregion - which creates
 * a tree structure.  Such nodes are N_ARY
 *
 * Eventually the subregions get small enough that it is expediant to simplly download the headers
 * directly - this is a LINEAR_SMALL node.
 *
 * In some cases, we desire a large number of linear nodes - which serves to ensure that the
 * block chain is validated (based on the relationships between headers) - this is a
 * LINEAR_LARGE node.
 *
 * Sometimes, actually often, we need a mixture of the above three node types.  Usually we require
 * a N_ARY node to be followed by a LINEAR_SMALL node so as ensure that no header is lost - this
 * is a MIXED node.
 */
typedef enum {
    SYNC_LINEAR_SMALL,      // leaf
    SYNC_LINEAR_LARGE,      // children: SMALL+
    SYNC_N_ARY,            // children: ANY+ + SMALL
    SYNC_MIXED              // children: ANY+
} BREthereumBCSSyncType;

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

/// MARK: - Sync Range

/**
 * The Context for the Sync Range Callback
 */
typedef void* BREthereumBCSSyncRangeContext;

/**
 * The Sync Range Callback to announce each result (block headers, at least).
 */
typedef void
(*BREthereumBCSSyncRangeCallback) (BREthereumBCSSyncRangeContext context,
                                   BREthereumBCSSyncRange range,
                                   BREthereumBlockHeader header,
                                   uint64_t headerNumber);

/**
 * A Sync Range defines the range of block header numbers overwhich a sync is performed.  The
 * type of sync over the range varies (see BREthereumBCSSyncType).
 */
struct BREthereumBCSSyncRangeRecord {

    /** Addres of interest */
    BREthereumAddress address;

    /** LES for Node interactions */
    BREthereumLES les;

    /** LES Node we sync to */
    BREthereumNodeReference node;

    /** Event query handling our events */
    BREventHandler handler;

    /** Callback */
    BREthereumBCSSyncRangeContext context;
    BREthereumBCSSyncRangeCallback callback;

    /** Parameters defining this Range */
    BREthereumBCSSyncType type;

    /** The oldest blockNumber */
    uint64_t tail;

    /** Then newest blocknumber */
    uint64_t head;

    /**
     * The block numbers between consecutive headers.  For a LINEAR sync this is always `1`; for
     * a N_ARY sync this is derived as: (head - tail) / count - more or less.  The
     * `lesGetBlockHeaders()` `skip` parameters is `step - 1` */
    uint64_t step;

    /**
     * The number of headers (minus `1` in a lesGetBlockHeaders() call.  For a N_ARY sync, `count`
     * is the number of subranges - since the lesGetBlockHeaders() call includes both boundariees,
     * the call's `maxBlockCount` is `count + 1`
     */
    uint64_t count;

    /**
     * The result headers.  Once we get a set of headers, we make/will ask for the account state
     * at each header.  If the account state changed between two headers, we'll need to make
     * a recusive request for headers - for that request we'll need the header block number.
     */
    BRArrayOf(BREthereumBlockHeader) headers;

    /** The parent of this node.  If this is NULL, then `this` is the root node. */
    BREthereumBCSSyncRange parent;

    /** The children of this node.  If this is NULL, then `this` is a leaf node */
    BRArrayOf(BREthereumBCSSyncRange) children;
};

/**
 * The depth of `range`.  If 0, then `range` is a root node.
 */
static int
syncRangeGetDepth (BREthereumBCSSyncRange range) {
    return NULL == range->parent ? 0 : (1 + syncRangeGetDepth (range->parent));
}

/**
 * Report SYNC results using eth_log()
 */
static void
syncRangeReport (BREthereumBCSSyncRange range,
                 const char *action) {
    int depth = syncRangeGetDepth(range);

    char spaces[2 * depth + 1];
    memset(spaces, ' ', 2 * depth);
    spaces[2 * depth] = '\0';

    assert (range->head > range->tail);

    eth_log ("BCS", "Sync: %s: (T:C:R:D) = ( %d : %4" PRIu64 " : {%7" PRIu64 ", %7" PRIu64 "} : %2d ) *** %s%p -> %p",
             action,
             range->type, range->count, range->tail, range->head, depth,
             spaces, range, range->parent);
}

/**
 * Create a Sync Range will all the paremeters provided
 */
static BREthereumBCSSyncRange
syncRangeCreateDetailed (BREthereumAddress address,
                         BREthereumLES les,
                         BREthereumNodeReference node,
                         BREventHandler handler,
                         BREthereumBCSSyncRangeContext context,
                         BREthereumBCSSyncRangeCallback callback,
                         uint64_t tail,
                         uint64_t head,
                         uint64_t step,
                         uint64_t count,
                         BREthereumBCSSyncType type) {
    assert (head > tail);

    BREthereumBCSSyncRange range = calloc (1, sizeof (struct BREthereumBCSSyncRangeRecord));

    range->address = address;
    range->les  = les;
    range->node = node;
    range->handler = handler;

    range->context = context;
    range->callback = callback;

    range->tail = tail;
    range->head = head;
    range->step = step;
    range->count = (type == SYNC_MIXED ? 0 : count);  // count is unused for SYNC_MIXED
    range->type  = type;

    range->headers = NULL;

    range->parent = NULL;
    range->children = NULL;

    // syncRangeReport(range, "Create  ");
    return range;
}

extern void
syncRangeRelease (BREthereumBCSSyncRange range) {
    if (NULL != range->children) {
        for (size_t index = 0; index < array_count(range->children); index++) {
            range->children[index]->parent = NULL;
            syncRangeRelease (range->children[index]);
        }
        array_free(range->children);
    }

    // result[].header and result[].account should be gone already.

    free (range);
}

/**
 * Return the Event Handler - we use this in BREthereumBCSEvent.c and thereby avoid needing to
 * expose the BREthereumBCSSyncRange and BREthereumBCSSync abstractions.
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
 * of the parent's propertyes (address, les, handler).  This method calls
 * `syncRangeCreateDetailed()` - which isn't always the required way to create a SyncRange.
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
                                                        parent->node,
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
 * `step` and `count` for a 'N_ARY sync') will be optimized.
 */
static BREthereumBCSSyncRange
syncRangeCreate (BREthereumAddress address,
                 BREthereumLES les,
                 BREthereumNodeReference node,
                 BREventHandler handler,
                 BREthereumBCSSyncRangeContext context,
                 BREthereumBCSSyncRangeContext callback,
                 uint64_t tail,
                 uint64_t head,
                 uint64_t linearLargeLimit) {

    uint64_t total = head - tail;

    uint64_t step = 1;
    uint64_t count = total;

    BREthereumBCSSyncType type;

    // Determine the type
    if (total <= SYNC_LINEAR_REQUEST_MAXIMUM) type = SYNC_LINEAR_SMALL;
    else if (total <= linearLargeLimit) type = SYNC_LINEAR_LARGE;
    else {
        computeOptimalStep(total, &step, &count);
        type = (total == step * count
                ? SYNC_N_ARY         // An exact fit for N_ARY
                : SYNC_MIXED);       // Not exact, add a LINEAR_SMALL node
    }

    BREthereumBCSSyncRange root = syncRangeCreateDetailed (address, les, node, handler,
                                                           context, callback,
                                                           tail,
                                                           head,
                                                           step,
                                                           count,
                                                           type);

    switch (type) {
        case SYNC_LINEAR_SMALL:
        case SYNC_N_ARY:
            break;

        case SYNC_LINEAR_LARGE:
            // Split a large sync into small syncs w/ each one being of size SYNC_LINEAR_SMALL -
            // except for the last 'cleanup sync' which is just right to sync through `head`
            //
            // Adding all these child syncs makes sense when SYNC_LINEAR_LIMIT is a smallish
            // multiple of SYNC_LINEAR_REQUEST_MAXIMUM.  If we are doing a linear sync of
            // 6,000,000 with a maximum of 200 - we'd be adding 30,000 children.  Actually,
            // not that bad...
            //
            // TODO: Decide if a large-ish SYNC_LINEAR_LIMIT is a problem.
            while (tail < head) {
                uint64_t next = (tail + SYNC_LINEAR_REQUEST_MAXIMUM <= head
                                 ? tail + SYNC_LINEAR_REQUEST_MAXIMUM
                                 : head);
                syncRangeCreateAndAddChild (root,
                                            tail,
                                            next,
                                            1,
                                            next - tail,
                                            SYNC_LINEAR_SMALL);
                tail = next;
            }
            break;

        case SYNC_MIXED:
            // Split a mixed sync into a N_ARY sync and a small 'cleanup' sync.  By design,
            // computeOptimalStep() left the 'cleanup' sync is a small linear sync.
            syncRangeCreateAndAddChild (root,
                                        tail,
                                        tail + step * count,
                                        step,
                                        count,
                                        SYNC_N_ARY);

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
    if (NULL == range->parent)
        eth_log ("BCS", "Sync: Start%s", "");

    syncRangeReport(range, "Dispatch");

    if (NULL == range->parent)
        // Callback to announce sync start
        range->callback (range->context, range, NULL, range->tail);

    switch (range->type) {
        case SYNC_LINEAR_SMALL:
        case SYNC_N_ARY:
            lesProvideBlockHeaders (range->les, range->node,
                                    (BREthereumLESProvisionContext) range,
                                    (BREthereumLESProvisionCallback) bcsSyncSignalProvision,
                                    range->tail,
                                    (uint32_t) (range->count + 1),  // both endpoints
                                    range->step - 1,   // skip
                                    ETHEREUM_BOOLEAN_FALSE);
            break;

        case SYNC_MIXED:
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
 * Get the root for `range`.
 */
static BREthereumBCSSyncRange
syncRangeGetRoot (BREthereumBCSSyncRange range) {
    return (NULL == range->parent
            ? range
            : syncRangeGetRoot(range->parent));
}

/**
 * Complete a Sync Range by a) completing the sync overall if `child` is the root, b) dispatching
 * on any remaining children of parent.
 */
static void
syncRangeComplete (BREthereumBCSSyncRange child) {
    BREthereumBCSSyncRange parent = child->parent;

    syncRangeReport (child, "Complete");

    // If `child` does not have a `parent`, then we are at the top-level and completely complete.
    if (NULL == parent) {
        eth_log ("BCS", "Sync: Done%s", "");
        child->callback (child->context, child, NULL, child->head);
        return;
    }

    // If this is a LINEAR_SMALL sync, then report (incremental) progress
    if (SYNC_LINEAR_SMALL == child->type) {
        BREthereumBCSSyncRange root = syncRangeGetRoot (child);
        root->callback (root->context, child, NULL, child->head);
    }

    // Remove the child from parent.
    syncRangeRemChild(child);

    // Release child.
    assert (NULL == child->children || array_count(child->children) == 0);
    syncRangeRelease(child);

    // If we have children remaining, then dispatch the first one; otherwise ...
    if (array_count(parent->children) > 0)
        syncRangeDispatch(parent->children[0]);

    // ... parent is complete.
    else
        syncRangeComplete (parent);
}

/// MARK: - Sync

/**
 * A BCS Sync handles ongoing sync reqeusts.  A call to bcsSyncContinue() will start a sync as
 * needed.
 */
struct BREthereumBCSSyncStruct {
    /** Addres of interest */
    BREthereumAddress address;

    /** LES for Node interactions */
    BREthereumLES les;

    /** Event handler for our events */
    BREventHandler handler;

    /** Callback */
    BREthereumBCSSyncContext context;
    BREthereumBCSSyncReportBlocks callbackBlocks;
    BREthereumBCSSyncReportProgress callbackProgress;

    /** The root `range`, if a sync is in progress */
    BREthereumBCSSyncRange root;

    /** Accumulated sync results.  Will be periodically reported with the callback. */
    BRArrayOf(BREthereumBCSSyncResult) results;
};

/**
 * Create a BCS sync.
 */
extern BREthereumBCSSync
bcsSyncCreate (BREthereumBCSSyncContext context,
               BREthereumBCSSyncReportBlocks callbackBlocks,
               BREthereumBCSSyncReportProgress callbackProgress,
               BREthereumAddress address,
               BREthereumLES les,
               BREventHandler handler) {
    BREthereumBCSSync sync = malloc (sizeof(struct BREthereumBCSSyncStruct));

    sync->address = address;
    sync->les = les;
    sync->handler = handler;

    sync->context = context;
    sync->callbackBlocks = callbackBlocks;
    sync->callbackProgress = callbackProgress;

    // No sync in progress.
    sync->root = NULL;

    // Allocate `result` with at most BCS_SYNC_RESULT_PERIOD results.
    array_new (sync->results, BCS_SYNC_RESULT_PERIOD);
    return sync;
}

/**
 * Release `sync`
 */
extern void
bcsSyncRelease (BREthereumBCSSync sync) {
    // TODO: Recursively release `root`; ensure that pending LES callbacks don't crash.
    if (NULL != sync->root) syncRangeRelease(sync->root);

    if (NULL != sync->results) {
        for (size_t index = 0; index < array_count(sync->results); index++)
            blockHeaderRelease(sync->results[index].header);
        array_free(sync->results);
    }

    memset (sync, 0, sizeof (struct BREthereumBCSSyncStruct));
    free (sync);
}

/**
 * Return `true` if active; `false` otherwise
 */
extern BREthereumBoolean
bcsSyncIsActive (BREthereumBCSSync sync) {
    return AS_ETHEREUM_BOOLEAN(NULL != sync->root);
}

/**
 * The callback for sync ranges.  If `header` is NULL, then we are simply announcing progress.  If
 * `header` is not NULL then add `header` to `results` and periodically invoke the sync callback
 * (to BCS, generally).
 */
static void
bcsSyncRangeCallback (BREthereumBCSSync sync,
                      BREthereumBCSSyncRange range,
                      BREthereumBlockHeader header,
                      uint64_t headerNumber) {

    // If `header` is provided (not NULL), then we are reporting a block; we'll invoke
    // `callbackBlocks` and then skip out

    // If `header` is provided (not NULL), then add it to sync->results
    if (NULL != header) {
        BREthereumBCSSyncResult result = { header };
        array_add (sync->results, result);
    }

    // If sync->results is now full or if range is done, then report the results
    if (array_count (sync->results) > 0 &&
        (BCS_SYNC_RESULT_PERIOD == array_count (sync->results) ||
         headerNumber == range->head)) {
            sync->callbackBlocks (sync->context,
                                  sync,
                                  range->node,
                                  sync->results);
            array_new (sync->results, BCS_SYNC_RESULT_PERIOD);
        }

    // Skip out now if header was provided
    if (NULL != header) return;

    // We are reporting progress.  The tricky part is that we don't want to report 'end' twice.
    // It can be reported twice when the last child of `root` completes and then `root`
    // itself completes.

    // If we are not at the end, just report it and skip out.
    if (headerNumber != range->head) {
        sync->callbackProgress (sync->context,
                                sync,
                                range->node,
                                sync->root->tail,
                                headerNumber,
                                sync->root->head);
        return;
    }

    // We are at the end... then report if `range` is sync->root or at an 'intermediate' range
    if (range == sync->root || range->head != sync->root->head)
        sync->callbackProgress (sync->context,
                                sync,
                                range->node,
                                sync->root->tail,
                                headerNumber,
                                sync->root->head);

    // If we are at the end, and `range` is root, then the sync is complete.
    if (range == sync->root) {
        syncRangeRelease(sync->root);
        sync->root = NULL;
    }
}

/**
 * Continue a sync for blocks from `chainBlockNumber` to `needBlockNumber`.
 */
extern void
bcsSyncStart (BREthereumBCSSync sync,
              BREthereumNodeReference node,
              uint64_t chainBlockNumber,
              uint64_t needBlockNumber) {
    // Skip out if already syncing.
    if (NULL != sync->root) return;

    // Only do something if we need something.
    if (needBlockNumber <= chainBlockNumber) return;
    uint64_t total = needBlockNumber - chainBlockNumber;

    // If `node` is generic, we need to find LES's preferred node.
    if (NODE_REFERENCE_IS_GENERIC (node)) {
        node = lesGetNodePrefer (sync->les);
        // If still 'generic' (specifically NIL), skip this sync.
        if (NODE_REFERENCE_NIL == node) {
            eth_log ("BCS", "Sync: Start Skipped: No suitable nodes%s", "");
            return;
        }
    }

    // We MUST have the last N headers be from a linear sync.  This is required to 'fill the
    // BCS chain' and allows `needBlockNumber` to be the head (bcs->chain) which then allows
    // orphans to be chained.

    // If total is small enough, then syncRangeCreate will produce a LINEAR sync...
    if (total < SYNC_LINEAR_LIMIT)
        sync->root = syncRangeCreate (sync->address,
                                      sync->les,
                                      node,
                                      sync->handler,
                                      (BREthereumBCSSyncRangeContext) sync,
                                      (BREthereumBCSSyncRangeCallback) bcsSyncRangeCallback,
                                      chainBlockNumber /* + 1 */,
                                      needBlockNumber,
                                      SYNC_LINEAR_LIMIT);

    // ... but if total is too large, then build a MIXED sync with two children.  The first sync
    // will generally be a N_ARY sync, which may itself end with a LINEAR_SMALL sync.  But herein
    // we want a suitable number of headers - so as to build up trust in the blockchain.
    else {
        sync->root = syncRangeCreateDetailed (sync->address,
                                              sync->les,
                                              node,
                                              sync->handler,
                                              (BREthereumBCSSyncRangeContext) sync,
                                              (BREthereumBCSSyncRangeCallback) bcsSyncRangeCallback,
                                              chainBlockNumber /* + 1 */,
                                              needBlockNumber,
                                              1,
                                              total,
                                              SYNC_MIXED);

        // Split the two children at a suitable offset from `needBlockNumber`
        uint64_t linearStartBlockNumber = needBlockNumber - SYNC_LINEAR_REQUEST_MAXIMUM; //  SYNC_LINEAR_LIMIT;

        // Add the first child; it will generally be a N_ARY sync
        syncRangeAddChild (sync->root,
                           syncRangeCreate (sync->address,
                                            sync->les,
                                            node,
                                            sync->handler,
                                            NULL,
                                            NULL,
                                            chainBlockNumber,
                                            linearStartBlockNumber,
                                            SYNC_LINEAR_LIMIT));

        // Add the second child; we've orchastrated this is be a LINEAR sync.
        syncRangeAddChild (sync->root,
                           syncRangeCreate (sync->address,
                                            sync->les,
                                            node,
                                            sync->handler,
                                            NULL,
                                            NULL,
                                            linearStartBlockNumber,
                                            needBlockNumber,
                                            SYNC_LINEAR_LIMIT));
    }

    // Kick off the new sync.
    syncRangeDispatch (sync->root);
}

extern void
bcsSyncStopInternal (BREthereumBCSSync sync,
                     const char *reason) {
    assert (NULL != sync->root);

    eth_log ("BCS", "Sync: Stopped%s%s",
             (NULL != reason ? ": " : ""),
             (NULL != reason ? reason : ""));

    // Callback as if all is good.
    sync->callbackProgress (sync->context,
                            sync,
                            sync->root->node,
                            sync->root->tail,
                            sync->root->head,
                            sync->root->head);

    syncRangeRelease(sync->root);
    sync->root = NULL;
}

extern void
bcsSyncStop (BREthereumBCSSync sync) {
    if (NULL != sync->root)
        bcsSyncStopInternal (sync, NULL);
}

/// MARK: - Sync Handle Block Header / Account State

/**
 * Given all block headers then: a) for a N_ARY range, request the account states; or b) for a
 * LINEAR_SMALL range, invoke the callback to report the header results.
 */
static void
bcsSyncHandleBlockHeaders (BREthereumBCSSyncRange range,
                           BREthereumNodeReference node,
                           OwnershipGiven BRArrayOf(BREthereumBlockHeader) headers) {
    assert (1 + range->count == array_count(headers));
    size_t count = array_count(headers);

    switch (range->type) {
        case SYNC_MIXED:
        case SYNC_LINEAR_LARGE:
            assert (0);
            break;

        case SYNC_N_ARY: {
            // Save the headers... for use with account states.
            range->headers = headers;

            // Setup a call to lesProvideAccountStates()
            BRArrayOf(BREthereumHash) hashes;

            array_new (hashes, count);

            for (size_t index = 0; index < count; index++)
                array_add (hashes,  blockHeaderGetHash (headers[index]));

            lesProvideAccountStates (range->les, node,
                                     (BREthereumLESProvisionContext) range,
                                     (BREthereumLESProvisionCallback) bcsSyncSignalProvision,
                                     range->address,
                                     hashes);
            break;
        }

        case SYNC_LINEAR_SMALL: {
            BREthereumBCSSyncRange root = syncRangeGetRoot(range);

            // TODO: Don't make `count` callback invocations; make one with `count` headers.
            for (size_t index = 0; index < count; index++)
                // `header` is now owned by `root->context`.
                root->callback (root->context, range, headers[index], 0);

            array_free (headers);
            syncRangeComplete (range);
            break;
        }
    }
}

/**
 * Given all the accoun states, compare each pair of consecutive accounts and if different create a
 * new subrange as a child to range.  Once all accounts have been compared then dispatch on the
 * first child (if any exist).
 */
static void
bcsSyncHandleAccountStates (BREthereumBCSSyncRange range,
                            BREthereumNodeReference node,
                            BREthereumAddress address,
                            OwnershipGiven BRArrayOf(BREthereumHash) hashes,
                            OwnershipGiven BRArrayOf(BREthereumAccountState) states) {
    size_t count = array_count(states);

    assert (1 + range->count == count);
    assert (array_count(hashes) == count);

    assert (SYNC_N_ARY == range->type);

    for (size_t index = 1; index < count; index++) {
        BREthereumAccountState oldState = states[index - 1];
        BREthereumAccountState newState = states[index];

        // If we found an AcountState change...
        if (ETHEREUM_BOOLEAN_IS_FALSE(accountStateEqual(oldState, newState))) {
            BREthereumBlockHeader oldHeader = range->headers[index - 1];
            BREthereumBlockHeader newHeader = range->headers[index];

            uint64_t oldNumber = blockHeaderGetNumber(oldHeader);
            uint64_t newNumber = blockHeaderGetNumber(newHeader);

            assert (newNumber > oldNumber);

            // ... then we need to explore this header range, recursively.
            syncRangeAddChild (range,
                               syncRangeCreate (range->address,
                                                range->les,
                                                range->node,
                                                range->handler,
                                                NULL,
                                                NULL,
                                                oldNumber,
                                                newNumber,
                                                SYNC_LINEAR_LIMIT_IF_N_ARY));
        }
    }
    // TODO: Move this to syncRangeComplete?  Switch on type for N_ARY only?

    array_free (hashes);
    array_free (states);

    // Release range->result.
    blockHeadersRelease(range->headers);
    range->headers = NULL;

    // If we now have children, dispatch on the first one.  As each one completes, we'll
    // dispatch on the subsequent ones until this N_ARY range itself completes.
    if (NULL != range->children && array_count(range->children) > 0)
        syncRangeDispatch(range->children[0]);

    // Otherwise nothing left, completely complete here and now.
    else
        syncRangeComplete(range);
}

/**
 * Compute the optimal `step` and `count` for a N_ARY sync over `numberOfBlocks`.
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
    for (int count = SYNC_N_ARY_REQUEST_MINIMUM; count < SYNC_N_ARY_REQUEST_MAXIMUM; count++) {
        uint64_t remainder = numberOfBlocks % count;
        if (remainder <= optimalRemainder) {
            optimalRemainder = remainder;
            *optimalCount = count;
        }
    }
    *optimalStep  = (numberOfBlocks / (*optimalCount));
}

#if 0
static void
computeOptimalStep (uint64_t numberOfBlocks,
                    uint64_t *optimalStep,
                    uint64_t *optimalCount) {
    *optimalCount = 0;
    uint64_t optimalRemainder = UINT64_MAX;
    for (int count = (int) minimum (numberOfBlocks / 3, SYNC_N_ARY_REQUEST_MINIMUM);
         count < SYNC_N_ARY_REQUEST_MAXIMUM;
         count++) {
        uint64_t remainder = numberOfBlocks % count;
        uint64_t quotient  = numberOfBlocks / count;
        if (quotient >= 3 && remainder <= optimalRemainder) {
            optimalRemainder = remainder;
            *optimalCount = count;
        }
    }
    *optimalStep  = (numberOfBlocks / (*optimalCount));
}
#endif

uint64_t optimalStep;
uint64_t optimalCount;
extern void optimal (uint64_t number) { computeOptimalStep (number, &optimalStep, &optimalCount); }



extern void
bcsSyncHandleProvision (BREthereumBCSSyncRange range,
                        BREthereumLES les,
                        BREthereumNodeReference node,
                        OwnershipGiven BREthereumProvisionResult result) {
    assert (range->les == les);

    BREthereumProvision *provision = &result.provision;
    switch (result.status) {
        case PROVISION_ERROR: {
            BREthereumBCSSyncRange root = syncRangeGetRoot (range);
            bcsSyncStopInternal((BREthereumBCSSync) root->context, "provision failed");
            break;
        }
        case PROVISION_SUCCESS: {
            assert (result.type == provision->type);
            switch (result.type) {
                case PROVISION_BLOCK_HEADERS: {
                    BRArrayOf(BREthereumBlockHeader) headers;
                    provisionHeadersConsume (&provision->u.headers, &headers);
                    bcsSyncHandleBlockHeaders (range, node, headers);
                    break;
                }

                case PROVISION_BLOCK_PROOFS:
                    assert (0);
                    
                case PROVISION_BLOCK_BODIES:
                    assert (0);

                case PROVISION_TRANSACTION_RECEIPTS:
                    assert (0);

                case PROVISION_ACCOUNTS: {
                    BRArrayOf(BREthereumHash) hashes;
                    BRArrayOf(BREthereumAccountState) accounts;
                    provisionAccountsConsume (&provision->u.accounts, &hashes, &accounts);
                    bcsSyncHandleAccountStates (range, node,
                                                provision->u.accounts.address,
                                                hashes,
                                                accounts);
                    break;
                }

                case PROVISION_TRANSACTION_STATUSES:
                    assert (0);

                case PROVISION_SUBMIT_TRANSACTION:
                    assert (0);
            }
            break;
        }
    }
    provisionResultRelease (&result);
}

