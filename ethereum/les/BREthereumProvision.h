//
//  BREthereumLESProvision.h
//  BRCore
//
//  Created by Ed Gamble on 9/3/18.
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
} BREthereumProvisionErrorReason;

/// MARK: Provision

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
typedef uint64_t BREthereumProvisionIdentifier;

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

extern BREthereumMessage
provisionCreateMessage (BREthereumProvision *provision,
                        BREthereumMessageIdentifier type,
                        size_t messageContentLimit,
                        uint64_t messageIdBase,
                        size_t index);

extern void
provisionHandleMessage (BREthereumProvision *provision,
                        BREthereumMessage message,
                        size_t messageContentLimit,
                        uint64_t messageIdBase);

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
