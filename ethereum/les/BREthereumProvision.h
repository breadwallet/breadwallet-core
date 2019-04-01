//
//  BREthereumLESProvision.h
//  BRCore
//
//  Created by Ed Gamble on 9/3/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#ifndef BR_Ethereum_LES_Provision_H
#define BR_Ethereum_LES_Provision_H

#include "BREthereumMessage.h"  // BRArrayOf

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PROVISION_SUCCESS,
    PROVISION_ERROR
} BREthereumProvisionStatus;

typedef enum {
    PROVISION_ERROR_NODE_INACTIVE,
    PROVISION_ERROR_NODE_DATA,
} BREthereumProvisionErrorReason;

extern const char *
provisionErrorGetReasonName (BREthereumProvisionErrorReason reason);

/// MARK: - Provision Support

/**
 * A Node provides four types of results, based on a corresponding request: Block Headers,
 * Block Bodies, Transaction Receipts and Accounts.  (More to come, likely).
 */
typedef enum {
    PROVISION_BLOCK_HEADERS,
    PROVISION_BLOCK_PROOFS,
    PROVISION_BLOCK_BODIES,
    PROVISION_TRANSACTION_RECEIPTS,
    PROVISION_ACCOUNTS,
    PROVISION_TRANSACTION_STATUSES,
    PROVISION_SUBMIT_TRANSACTION
} BREthereumProvisionType;

extern BREthereumLESMessageIdentifier
provisionGetMessageLESIdentifier (BREthereumProvisionType type);

extern BREthereumPIPRequestType
provisionGetMessagePIPIdentifier (BREthereumProvisionType type);

extern const char *
provisionGetTypeName (BREthereumProvisionType type);
    
/**
 * Headers
 */
typedef struct {
    // Request
    uint64_t start;
    uint64_t skip;
    uint32_t limit;
    BREthereumBoolean reverse;
    // Response
    BRArrayOf(BREthereumBlockHeader) headers;
} BREthereumProvisionHeaders;

extern void
provisionHeadersConsume (BREthereumProvisionHeaders *provision,
                         BRArrayOf(BREthereumBlockHeader) *headers);

/**
 * Proofs
 */
typedef struct {
    // Request
    BRArrayOf(uint64_t) numbers;
    // Response
    BRArrayOf(BREthereumBlockHeaderProof) proofs;
} BREthereumProvisionProofs;

extern void
provisionProofsConsume (BREthereumProvisionProofs *provision,
                        BRArrayOf(uint64_t) *numbers,
                        BRArrayOf(BREthereumBlockHeaderProof) *proofs);

/**
 * Bodies
 */
typedef struct {
    // Request
    BRArrayOf(BREthereumHash) hashes;
    // Response
    BRArrayOf(BREthereumBlockBodyPair) pairs;
} BREthereumProvisionBodies;

extern void
provisionBodiesConsume (BREthereumProvisionBodies *provision,
                        BRArrayOf(BREthereumHash) *hashes,
                        BRArrayOf(BREthereumBlockBodyPair) *pairs);

/**
 * Receipts
 */
typedef struct {
    // Request
    BRArrayOf(BREthereumHash) hashes;
    // Response
    BRArrayOf(BRArrayOf(BREthereumTransactionReceipt)) receipts;
} BREthereumProvisionReceipts;

extern void
provisionReceiptsConsume (BREthereumProvisionReceipts *provision,
                          BRArrayOf(BREthereumHash) *hashes,
                          BRArrayOf(BRArrayOf(BREthereumTransactionReceipt)) *receipts);

/**
 * Accounts
 */
typedef struct {
    // Request
    BREthereumAddress address;
    BRArrayOf(BREthereumHash) hashes;
    // Response
    BRArrayOf(BREthereumAccountState) accounts;
} BREthereumProvisionAccounts;

extern void
provisionAccountsConsume (BREthereumProvisionAccounts *provision,
                          BRArrayOf(BREthereumHash) *hashes,
                          BRArrayOf(BREthereumAccountState) *accounts);

/**
 * Transaction Statuses
 */
typedef struct {
    // Request
    BRArrayOf(BREthereumHash) hashes;
    // Response
    BRArrayOf(BREthereumTransactionStatus) statuses;
} BREthereumProvisionStatuses;

extern void
provisionStatusesConsume (BREthereumProvisionStatuses *provision,
                          BRArrayOf(BREthereumHash) *hashes,
                          BRArrayOf(BREthereumTransactionStatus) *statuses);

/**
 * Transaction Submission
 */
typedef struct {
    // Request
    BREthereumTransaction transaction;
    // Response
    BREthereumTransactionStatus status;
} BREthereumProvisionSubmission;

extern void
provisionSubmissionConsume (BREthereumProvisionSubmission *provision,
                            BREthereumTransaction *transaction,
                            BREthereumTransactionStatus *status);

/// MARK: - Provision

/**
 * A Provision Identifer.
 */
typedef size_t BREthereumProvisionIdentifier;

#define PROVISION_IDENTIFIER_UNDEFINED  ((BREthereumProvisionIdentifier) -1)

/**
 * Provision
 */
typedef struct {
    BREthereumProvisionIdentifier identifier;
    BREthereumProvisionType type;
    union {
        BREthereumProvisionHeaders headers;
        BREthereumProvisionProofs proofs;
        BREthereumProvisionBodies bodies;
        BREthereumProvisionReceipts receipts;
        BREthereumProvisionAccounts accounts;
        BREthereumProvisionStatuses statuses;
        BREthereumProvisionSubmission submission;
    } u;
} BREthereumProvision;

extern BREthereumProvision
provisionCopy (BREthereumProvision *provision,
               BREthereumBoolean copyResults);

extern void
provisionRelease (BREthereumProvision *provision,
                  BREthereumBoolean releaseResults);

/**
 * Release only the results portion of a provision.  If a provision fails, we'll release an
 * partial result and then expect to reschedule the provision.
 */
extern void
provisionReleaseResults (BREthereumProvision *provision);

extern BREthereumMessage
provisionCreateMessage (BREthereumProvision *provision,
                        BREthereumMessageIdentifier type,
                        size_t messageContentLimit,
                        size_t messageIdBase,
                        size_t index);

extern BREthereumProvisionStatus
provisionHandleMessage (BREthereumProvision *provision,
                        BREthereumMessage message,
                        size_t messageContentLimit,
                        size_t messageIdBase);

extern BREthereumBoolean
provisionMatches (BREthereumProvision *provision1,
                  BREthereumProvision *provision2);

/**
 * Provision Result
 */
typedef struct {
    BREthereumProvisionIdentifier identifier;
    BREthereumProvisionType type;
    BREthereumProvisionStatus status;
    BREthereumProvision provision;

    union {
        // success - the provision
        struct {
        } success;

        // error - the error reason
        struct {
            BREthereumProvisionErrorReason reason;
        } error;
    } u;
} BREthereumProvisionResult;

extern void
provisionResultRelease (BREthereumProvisionResult *result);

typedef void *BREthereumProvisionCallbackContext;

typedef void
(*BREthereumProvisionCallback) (BREthereumProvisionCallbackContext context,
                                BREthereumProvisionResult result);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_LES_Provision_H */
