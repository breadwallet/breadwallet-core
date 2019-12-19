//
//  BREthereumBase
//  Core Ethereum
//
//  Created by Ed Gamble on 3/22/18.
//  Copyright © 2018-2019 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_Ethereum_Base_H
#define BR_Ethereum_Base_H

#include "support/BRArray.h"
#include "support/BRBase.h"
#include "support/BRSet.h"
#include "ethereum/util/BRUtil.h"
#include "ethereum/rlp/BRRlp.h"

// All 'base'
#include "BREthereumLogic.h"
#include "BREthereumEther.h"
#include "BREthereumGas.h"
#include "BREthereumFeeBasis.h"
#include "BREthereumHash.h"
#include "BREthereumData.h"
#include "BREthereumAddress.h"
#include "BREthereumSignature.h"

#ifdef __cplusplus
extern "C" {
#endif

#define REPEAT(index, count) \
for (size_t index = 0, __indexLimit = (size_t) (count); index < __indexLimit; index++)

typedef uint64_t BREthereumTimestamp;  // A Unix time
#define ETHEREUM_TIMESTAMP_UNKNOWN    ((uint64_t) 0)

/**
 * An Ethereum RLP type is an enumeration of the RLP encoding types.
 *
 * Four types are defined: the 'network' type produces an encoding consistent with the Etherum
 * specification.  The 'archive' types produces a network encoding w/ some added values needed for
 * archival (aka persistent) storage.  The 'unsigned' type refers to a non-network transaction
 * encoding suitable for signing.  The 'signed' type is an alias for 'network' and is used to
 * contrast with the 'unsigned' type.
 */
typedef enum {
    RLP_TYPE_NETWORK,
    RLP_TYPE_ARCHIVE,
    RLP_TYPE_TRANSACTION_UNSIGNED,
    RLP_TYPE_TRANSACTION_SIGNED = RLP_TYPE_NETWORK,
} BREthereumRlpType;

/**
 * An Ehtereum Sync Interest specifies what a 'BRD Service Assisted' mode sync shoul provide data
 * for.  When query BRD services we can be interested in blocks where the User's account/address
 * appears in {transactions, logs} x {source, target}.
 */
typedef enum {
    CLIENT_GET_BLOCKS_NODE = 0,
    CLIENT_GET_BLOCKS_TRANSACTIONS_AS_SOURCE = (1 << 0),
    CLIENT_GET_BLOCKS_TRANSACTIONS_AS_TARGET = (1 << 1),
    CLIENT_GET_BLOCKS_LOGS_AS_SOURCE = (1 << 2),
    CLIENT_GET_BLOCKS_LOGS_AS_TARGET = (1 << 3)
} BREthereumSyncInterest;

/**
 * An Ethereum Sync Interest Set is an 'option set' of sync interests.
 */
typedef unsigned int BREthereumSyncInterestSet;

static inline int // 1 if match; 0 if not
syncInterestMatch(BREthereumSyncInterestSet interests,
                  BREthereumSyncInterest interest) {
    return interests & interest;
}

/**
 * An Ethereum Transfer Status is an enumeration of a transfer's states/status.
 *
 * A transfer status extends the 'Ethereum Transaction Status' (which is defined in the Ethereum
 * specfication) to include more 'life cycle' events such as 'created', 'cancelled, 'replaced' and
 * 'deleted'.  Thus a transfer status is IOS/Android application focused - not P2P network focused.
 */
typedef enum {
    /**
     * Created: transfer created in local memory
     */
    TRANSFER_STATUS_CREATED,

    /**
     * Submitted: transfer submitted
     */
    TRANSFER_STATUS_SUBMITTED,

    /**
     * Included: transfer is already included in the canonical chain. data contains an
     * RLP-encoded [blockHash: B_32, blockNumber: P, txIndex: P] structure.
     */
    TRANSFER_STATUS_INCLUDED,

    /**
     * Error: transfer sending failed. data contains a text error message
     */
    TRANSFER_STATUS_ERRORED,

    /**
     * Cancelled
     */
    TRANSFER_STATUS_CANCELLED,

    /**
     * Replaced
     */
    TRANSFER_STATUS_REPLACED,

    /**
     * Deleted
     */
    TRANSFER_STATUS_DELETED

} BREthereumTransferStatus;


#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Base_H */
