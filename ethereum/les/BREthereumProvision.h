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

#include "BREthereumLESMessage.h"  // BRArrayOf

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PROVISION_SUCCESS,
    PROVISION_ERROR
} BREthereumProvisionStatus;

typedef enum {
    PROVISION_ERROR_X,
    PROVISION_ERROR_Y
} BREthereumProvisionErrorReason;

/// MARK: Provision

/**
 * A Node provides four types of results, based on a corresponding request: Block Headers,
 * Block Bodies, Transaction Receipts and Accounts.  (More to come, likely).
 */
typedef enum {
    PROVISION_BLOCK_HEADERS,
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

/**
 * Bodies
 */
typedef struct {
    // Request
    BRArrayOf(BREthereumHash) hashes;
    // Response
    BRArrayOf(BREthereumBlockBodyPair) pairs;
} BREthereumProvisionBodies;

/**
 * Receipts
 */
typedef struct {
    // Request
    BRArrayOf(BREthereumHash) hashes;
    // Response
    BRArrayOf(BRArrayOf(BREthereumTransactionReceipt)) receipts;
} BREthereumProvisionReceipts;

/**
 * Accounts
 */
typedef struct {
    // Request
    BREthereumAddress address;
    BRArrayOf(BREthereumHash) hashes;
    BRArrayOf(uint64_t) numbers;    // HACK
    // Response
    BRArrayOf(BREthereumAccountState) accounts;
} BREthereumProvisionAccounts;

/**
 *
 */
typedef struct {
    // Request
    BRArrayOf(BREthereumHash) hashes;
    // Response
    BRArrayOf(BREthereumTransactionStatus) statuses;
} BREthereumProvisionStatuses;

/**
 *
 */
typedef struct {
    // Request
    BREthereumTransaction transaction;
    // Response
    BREthereumTransactionStatus status;
} BREthereumProvisionSubmission;

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
        BREthereumProvisionBodies bodies;
        BREthereumProvisionReceipts receipts;
        BREthereumProvisionAccounts accounts;
        BREthereumProvisionStatuses statuses;
        BREthereumProvisionSubmission submission;
    } u;
} BREthereumProvision;

/**
 * Provision Result
 */
typedef struct {
    BREthereumProvisionIdentifier identifier;
    BREthereumProvisionType type;
    BREthereumProvisionStatus status;
    union {
        // success - the provision
        struct {
            BREthereumProvision provision;
        } success;

        // error - the error reason
        struct {
            BREthereumProvisionErrorReason reason;
        } error;
    } u;
} BREthereumProvisionResult;


typedef void *BREthereumProvisionCallbackContext;

typedef void
(*BREthereumProvisionCallback) (BREthereumProvisionCallbackContext context,
                                BREthereumProvisionResult result);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_LES_Provision_H */
