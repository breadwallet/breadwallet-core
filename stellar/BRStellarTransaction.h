//
//  BRStellarTransaction.h
//  Core
//
//  Created by Carl Cherry on 5/21/2019
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#ifndef BRStellar_transaction_h
#define BRStellar_transaction_h

#include "BRStellarBase.h"
#include "BRKey.h"

typedef struct BRStellarTransactionRecord *BRStellarTransaction;
typedef struct BRStellarSerializedTransactionRecord *BRStellarSerializedTransaction;

/**
 * Create a Stellar transaction
 *
 * @param  sourceAddress  Stellar address of owner account
 * @param  targetAddress  Stellar address of recieving account
 *
 * @return transaction    a stellar transaction
 */
extern BRStellarTransaction /* caller must free - stellarTransactionFree */
stellarTransactionCreate(BRStellarAddress sourceAddress,
                        BRStellarAddress targetAddress
                        /* other stuff */);

/**
 * Create a Stellar transaction
 *
 * @param  bytes    serialized bytes from the server
 * @param  length   length of above bytes
 *
 * @return transaction    a stellar transaction
 */
extern BRStellarTransaction /* caller must free - stellarTransactionFree */
stellarTransactionCreateFromBytes(uint8_t *bytes, int length);

/**
 * Clean up any memory for this transaction
 *
 * @param transaction  BRStellarTransaction
 */
extern void stellarTransactionFree(BRStellarTransaction transaction);

/**
 * Get the size of a serialized transaction
 *
 * @param  s     serialized transaction
 * @return size
 */
extern uint32_t getSerializedSize(BRStellarSerializedTransaction s);

/**
 * Get the raw bytes of a serialized transaction
 *
 * @param  s     serialized transaction
 * @return bytes uint8_t
 */
extern uint8_t* /* DO NOT FREE - owned by the transaction object */
getSerializedBytes(BRStellarSerializedTransaction s);

/**
 * Get the hash of a stellar transaction
 *
 * @param  transaction   a valid stellar transaction
 * @return hash          a BRStellarTransactionHash object
 */
extern BRStellarTransactionHash stellarTransactionGetHash(BRStellarTransaction transaction);

/**
 * Get the Account Txn ID hash (if applicable)
 *
 * @param  transaction   a valid stellar transaction
 * @return hash          a BRStellarTransactionHash object
 */
extern BRStellarTransactionHash stellarTransactionGetAccountTxnId(BRStellarTransaction transaction);

/**
 * Various getter methods for the transaction
 */

#endif
