//
//  BREthereumLES.h
//  breadwallet-core Ethereum
//
//  Created by Lamont Samuels on 5/01/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_Ethereum_LES_h
#define BR_Ethereum_LES_h

#include <inttypes.h>
#include "support/BRArray.h"
#include "support/BRKey.h"
#include "ethereum/base/BREthereumBase.h"
#include "ethereum/blockchain/BREthereumBlockChain.h"
#include "BREthereumProvision.h"

#ifdef __cplusplus
extern "C" {
#endif

/// MARK: LES Node Config

/**
 * A LES Node Config is used to save a node for use when reestablishing LES.  See the typedef
 * BREthereumLESCallbackSaveNodes.  (NO: Note: these interfaces are implemented in BREthereumNode.c)
 */
typedef struct BREthereumNodeConfigRecord *BREthereumNodeConfig;

extern void
nodeConfigRelease (BREthereumNodeConfig config);

extern BREthereumHash
nodeConfigGetHash (BREthereumNodeConfig config);

extern BRRlpItem
nodeConfigEncode (BREthereumNodeConfig config,
                  BRRlpCoder coder);

extern BREthereumNodeConfig
nodeConfigDecode (BRRlpItem item,
                  BRRlpCoder coder);

// Support BRSet
extern size_t
nodeConfigHashValue (const void *h);

// Support BRSet
extern int
nodeConfigHashEqual (const void *h1, const void *h2);


/*!
 * @typedef BREthereumLES
 *
 * @abstract
 * An instance to handle LES functionalty.
 */
typedef struct BREthereumLESRecord *BREthereumLES;

/**
 * An opaque type for a Node.  Only used in the announce callback to identify what node produced
 * the result.  This NodeReference is actually a pointer but we override that with some specific
 * values, see NODE_REFERENCE_* below, to represent some special behaviors.
 */
typedef void *BREthereumNodeReference;

/** Reserved reference range */
#define NODE_REFERENCE_BASE   ((BREthereumNodeReference)  0)
#define NODE_REFERENCE_LIMIT  ((BREthereumNodeReference) 20)

/** References to select a specific node index */
#define NODE_REFERENCE_0      ((BREthereumNodeReference) 0)
#define NODE_REFERENCE_1      ((BREthereumNodeReference) 1)
#define NODE_REFERENCE_2      ((BREthereumNodeReference) 2)
#define NODE_REFERENCE_3      ((BREthereumNodeReference) 3)
#define NODE_REFERENCE_4      ((BREthereumNodeReference) 4)

#if 5 < LES_ACTIVE_NODE_COUNT  // 5 <= 1 + NODE_REFERENCE_4
#error Not enough NODE_REFERENCE declarations
#endif

/** References to select an arbitrary index */
#define NODE_REFERENCE_NIL    ((BREthereumNodeReference) 10)
#define NODE_REFERENCE_ANY    ((BREthereumNodeReference) 11)
#define NODE_REFERENCE_ALL    ((BREthereumNodeReference) 12)

/** Predicate to check if `n` is a 'generic' reference */
#define NODE_REFERENCE_IS_GENERIC(n)                        \
  (NODE_REFERENCE_BASE <= ((BREthereumNodeReference) n) &&  \
   ((BREthereumNodeReference) n) < NODE_REFERENCE_LIMIT)

/** Predicate to check if `n` is an 'arbitrary' node */
#define NODE_REFERENCE_IS_ARBITRARY(n)                       \
    (NODE_REFERENCE_NIL <= ((BREthereumNodeReference) n) &&  \
    ((BREthereumNodeReference) n) <= NODE_REFERENCE_ALL)

/*!
 *@typedef BREthereumLESStatus
 *
 * @abstract
 * An enumeration for deteremining the error that occured when submitting messages using LES
 */
typedef enum {
    LES_SUCCESS                   = 0x00,  // No error was generated after submtting a message using LES
    LES_ERROR_NETWORK_UNREACHABLE = 0x01,  // Error is thrown when the LES context can not connect to the ethereum network
    LES_ERROR_UNKNOWN             = 0x02   // Error is thrown but it's unknown what caused it.
                                           // NOT_CONNECTED?
} BREthereumLESStatus;

/*!
 * @typedef BREthereumLESCallbackContext
 *
 * @abstract
 * The context to use for handling LES callbacks, such as 'announce' and 'status'
 */
typedef void* BREthereumLESCallbackContext;

/*!
 * @typedef BREthereumLESCallbackAnnounce
 *
 * @abstract
 * The callback to use for handling a LES 'Announce' message.
 */
typedef void
(*BREthereumLESCallbackAnnounce) (BREthereumLESCallbackContext context,
                                  BREthereumNodeReference node,
                                  BREthereumHash headHash,
                                  uint64_t headNumber,
                                  UInt256 headTotalDifficulty,
                                  uint64_t reorgDepth);

/**
 * The callback to use for Node/Peer status message - announces the headHash and headNumber for
 * the peer.
 *
 * TODO: Might need a `peer ID` or let the NodeManager evaluate different peers.
 *
 */
typedef void
(*BREthereumLESCallbackStatus) (BREthereumLESCallbackContext context,
                                BREthereumNodeReference node,
                                BREthereumHash headHash,
                                uint64_t headNumber);

/**
 * The callback to use for saving nodes
 */
typedef void
(*BREthereumLESCallbackSaveNodes) (BREthereumLESCallbackContext context,
                                   BRArrayOf(BREthereumNodeConfig) nodes);

/*!
 * @function lesCreate
 *
 * @abstract
 * Create an instance to handle the LES interface.  Will connect to the provided `network` and
 * begin getting block headers.
 *
 * @param network
 * The network to connect with.
 *
 * @param announceContext
 * The context to use when handling a LES 'Announce' message
 *
 * @param announceCallback
 * The callback to use when handling a LES 'Announce' message
 *
 * ...
 *
 * @result
 * A new LES interface handler.
 */
extern BREthereumLES
lesCreate (BREthereumNetwork network,
           BREthereumLESCallbackContext callbackContext,
           BREthereumLESCallbackAnnounce callbackAnnounce,
           BREthereumLESCallbackStatus callbackStatus,
           BREthereumLESCallbackSaveNodes callbackSaveNodes,
           BREthereumHash headHash,
           uint64_t headNumber,
           UInt256 headTotalDifficulty,
           BREthereumHash genesisHash,
           BRSetOf(BREthereumNodeConfig) configs,
           BREthereumBoolean discoverNodes,
           BREthereumBoolean handleSync);

/*!
 * @function lesRelease
 *
 * @abstract
 * Releases the les context
 */
extern void
lesRelease(BREthereumLES les);

extern void
lesStart (BREthereumLES les);

extern void
lesStop (BREthereumLES les);

extern void
lesClean (BREthereumLES les);

extern void
lesUpdateBlockHead (BREthereumLES les,
                    BREthereumHash headHash,
                    uint64_t headNumber,
                    UInt256 headTotalDifficulty);

extern void
lesSetNodePrefer (BREthereumLES les,
               BREthereumNodeReference nodeReference);

extern BREthereumNodeReference
lesGetNodePrefer (BREthereumLES les);

extern const char *
lesGetNodeHostname (BREthereumLES les,
                    BREthereumNodeReference node);

/// MARK: LES Provision Callbacks

typedef void *BREthereumLESProvisionContext;

typedef void
(*BREthereumLESProvisionCallback) (BREthereumLESProvisionContext context,
                                   BREthereumLES les,
                                   BREthereumNodeReference node,
                                   BREthereumProvisionResult result);

/*!
 * @function lesProvideBlockHeaders
 *
 * @abstract
 * Make a LES Block Headers requests.
 *
 * @param context
 * The context to use for the callback.
 *
 * @param callback
 * The callback to use when handling each BREthereumBlockHeader.
 *
 * @param blockNumber
 * The starting blockNumber
 *
 * @param maxBlockCount
 * The maximum blocks to return.
 *
 * @param skip
 * The number of blocks to skip.  Should be '0' to get every block between `blockNumber` and
 * `blockNumber + maxBlockCount`.
 *
 * @param reverse
 * If ETHEREUM_BOOLEAN_TRUE the returned headers will be in reverse order; otherwise the
 * first header will have `blockNumber`.
 */
extern void
lesProvideBlockHeaders (BREthereumLES les,
                        BREthereumNodeReference node,
                        BREthereumLESProvisionContext context,
                        BREthereumLESProvisionCallback callback,
                        uint64_t blockNumber,
                        uint32_t maxBlockCount,
                        uint64_t skip,
                        BREthereumBoolean reverse);

/**
 * @function lesProvideBlockProofs
 *
 * @param les
 * @param context
 * @param callback
 * @param uint64_t
 */
extern void
lesProvideBlockProofs (BREthereumLES les,
                       BREthereumNodeReference node,
                       BREthereumLESProvisionContext context,
                       BREthereumLESProvisionCallback callback,
                       OwnershipGiven BRArrayOf(uint64_t) blockNumbers);

extern void
lesProvideBlockProofsOne (BREthereumLES les,
                          BREthereumNodeReference node,
                          BREthereumLESProvisionContext context,
                          BREthereumLESProvisionCallback callback,
                          uint64_t blockNumber);

/*!
 * @function lesProvdeBlockBodies
 *
 * @abstract
 * Make a LES Block Bodies requests.
 *
 * @param les
 * @param context
 * @param callback
 * @param blockHashes - OwnershipGiven
 */
extern void
lesProvideBlockBodies (BREthereumLES les,
                       BREthereumNodeReference node,
                       BREthereumLESProvisionContext context,
                       BREthereumLESProvisionCallback callback,
                       OwnershipGiven BRArrayOf(BREthereumHash) blockHashes);

extern void
lesProvideBlockBodiesOne (BREthereumLES les,
                          BREthereumNodeReference node,
                          BREthereumLESProvisionContext context,
                          BREthereumLESProvisionCallback callback,
                          BREthereumHash blockHash);

/**
 * @function lesProvideReceipts
 *
 * @param les
 * @param context
 * @param callback
 * @param BREthereumHash
 */
extern void
lesProvideReceipts (BREthereumLES les,
                    BREthereumNodeReference node,
                    BREthereumLESProvisionContext context,
                    BREthereumLESProvisionCallback callback,
                    OwnershipGiven BRArrayOf(BREthereumHash) blockHashes);
extern void
lesProvideReceiptsOne (BREthereumLES les,
                       BREthereumNodeReference node,
                       BREthereumLESProvisionContext context,
                       BREthereumLESProvisionCallback callback,
                       BREthereumHash blockHash);

/**
 * @function lesProvideAccountStates
 *
 * @param les
 * @param context
 * @param callback
 * @param address
 * @param BREthereumHash
 */
extern void
lesProvideAccountStates (BREthereumLES les,
                         BREthereumNodeReference node,
                         BREthereumLESProvisionContext context,
                         BREthereumLESProvisionCallback callback,
                         BREthereumAddress address,
                         OwnershipGiven BRArrayOf(BREthereumHash) blockHashes);

extern void
lesProvideAccountStatesOne (BREthereumLES les,
                            BREthereumNodeReference node,
                            BREthereumLESProvisionContext context,
                            BREthereumLESProvisionCallback callback,
                            BREthereumAddress address,
                            BREthereumHash blockHash);

/**
 * @function lesProvideTransactionStauts
 *
 * @param les
 * @param context
 * @param callback
 * @param BREthereumHash
 */
extern void
lesProvideTransactionStatus (BREthereumLES les,
                             BREthereumNodeReference node,
                             BREthereumLESProvisionContext context,
                             BREthereumLESProvisionCallback callback,
                             OwnershipGiven BRArrayOf(BREthereumHash) transactionHashes);

extern void
lesProvideTransactionStatusOne (BREthereumLES les,
                                BREthereumNodeReference node,
                                BREthereumLESProvisionContext context,
                                BREthereumLESProvisionCallback callback,
                                BREthereumHash transactionHash);

/**
 * @function lesSubmitTransaction
 *
 * @param les
 * @param context
 * @param callback
 * @param transaction
 */
extern void
lesSubmitTransaction (BREthereumLES les,
                      BREthereumNodeReference node,
                      BREthereumLESProvisionContext context,
                      BREthereumLESProvisionCallback callback,
                      OwnershipGiven BREthereumTransaction transaction);


/**
 * Retry a provision
 *
 * @param les les
 * @param node node
 * @param context context
 * @param callback callback
 * @param provision provision
 */
extern void
lesRetryProvision (BREthereumLES les,
                   BREthereumNodeReference node,
                   BREthereumLESProvisionContext context,
                   BREthereumLESProvisionCallback callback,
                   OwnershipGiven BREthereumProvision *provision);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_LES_h */
