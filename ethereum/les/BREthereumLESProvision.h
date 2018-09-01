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
} BREthereumNodeProvisionStatus;

typedef enum {
    PROVISION_ERROR_X,
    PROVISION_ERROR_Y
} BREthereumNodeProvisionErrorReason;

/**
 * A Node provides four types of results, based on a corresponding request: Block Headers,
 * Block Bodies, Transaction Receipts and Accounts.  (More to come, likely).
 */
typedef enum {
    PROVISION_BLOCK_HEADERS,
    PROVISION_BLOCK_BODIES,
    PROVISION_TRANSACTION_RECEIPTS,
    PROVISION_ACCOUNTS
    // TRANSACTION_STATUS
    // SUBMIT_TRANSACTION
} BREthereumNodeProvisionType;

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
} BREthereumNodeProvisionHeaders;

/**
 * Bodies
 */
typedef struct {
    // Request
    BRArrayOf(BREthereumHash) hashes;
    // Response
    BRArrayOf(BREthereumBlockBodyPair) pairs;
} BREthereumNodeProvisionBodies;

/**
 * Receipts
 */
typedef struct {
    // Request
    BRArrayOf(BREthereumHash) hashes;
    // Response
    BRArrayOf(BRArrayOf(BREthereumTransactionReceipt)) receipts;
} BREthereumNodeProvisionReceipts;

/**
 * Accounts
 */
typedef struct {
    // Request
    BREthereumAddress address;
    BRArrayOf(BREthereumHash) hashes;
    // Response
    BRArrayOf(BREthereumAccountState) accounts;
} BREthereumNodeProvisionAccounts;

/**
 * Provision
 */
typedef struct {
    BREthereumNodeProvisionType type;
    union {
        BREthereumNodeProvisionHeaders headers;
        BREthereumNodeProvisionBodies bodies;
        BREthereumNodeProvisionReceipts receipts;
        BREthereumNodeProvisionAccounts accounts;
    } u;
} BREthereumNodeProvision;

/**
 * Provision Result
 */
typedef struct {
    BREthereumNodeProvisionStatus status;
    BREthereumNodeProvisionType type;
    union {
        // success - the provision
        struct {
            BREthereumNodeProvision provision;
        } success;

        // error - the error reason
        struct {
            BREthereumNodeProvisionErrorReason reason;
        } error;
    } u;
} BREthereumNodeProvisionResult;

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_LES_Provision_H */
