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
#include "support/BRKey.h"
#include "support/BRArray.h"

#ifdef __cplusplus
extern "C" {
#endif

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
stellarTransactionCreate(BRStellarAccountID *accountID,
                        BRStellarFee fee,
                        BRStellarTimeBounds *timeBounds,
                        int numTimeBounds,
                        BRStellarMemo *memo,
                        BRArrayOf(BRStellarOperation) operations);

/**
 * Create a Stellar transaction
 *
 * @param  bytes    serialized bytes from the server
 * @param  length   length of above bytes
 *
 * @return transaction    a stellar transaction
 */
extern BRStellarTransaction /* caller must free - stellarTransactionFree */
stellarTransactionCreateFromBytes(uint8_t *tx_bytes, size_t tx_length);

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
extern size_t stellarGetSerializedSize(BRStellarSerializedTransaction s);

/**
 * Get the raw bytes of a serialized transaction
 *
 * @param  s     serialized transaction
 * @return bytes uint8_t
 */
extern uint8_t* /* DO NOT FREE - owned by the transaction object */
stellarGetSerializedBytes(BRStellarSerializedTransaction s);

/**
 * Get the hash of a stellar transaction
 *
 * @param  transaction   a valid stellar transaction
 * @return hash          a BRStellarTransactionHash object
 */
extern BRStellarTransactionHash stellarTransactionGetHash(BRStellarTransaction transaction);

/**
 * Get the result code of the transaction
 *
 * This may change - but the thinking here is that the caller should have both the
 * transaction XDR and the result XDR.  So, first they create a transaction from bytes
 * then send it in here with the result XDR string and we can parse the results
 *
 * @param  transaction   a valid stellar transaction
 * @param  result_xdr    a base64 string result that was returned from the server
 * @return result        BRStellarTXResultCode value
 */
extern BRStellarTransactionResult
stellarTransactionGetResult(BRStellarTransaction transaction, const char* result_xdr);

/**
 * Various getter methods for the transaction - useful after we deserialize a transaction
 * from the server (or for testing)
 */
extern BRStellarAccountID stellarTransactionGetAccountID(BRStellarTransaction transaction);
extern size_t stellarTransactionGetOperationCount(BRStellarTransaction transaction);
extern uint32_t stellarTransactionGetSignatureCount(BRStellarTransaction transaction);

extern BRStellarOperation * /* DO NOT FREE - owned by the transaction object */
stellarTransactionGetOperation(BRStellarTransaction transaction, uint32_t operationIndex);

extern BRStellarMemo * /* DO NOT FREE - owned by the transaction object */
stellarTransactionGetMemo(BRStellarTransaction transaction);


#ifdef __cplusplus
}
#endif

#endif // BRStellar_transaction_h
