//
//  BBREthereumTransaction.h
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 2/21/2018.
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

#ifndef BR_Ethereum_Transaction_H
#define BR_Ethereum_Transaction_H

#include "BREthereumNetwork.h"
#include "BREthereumAccount.h"
#include "BREthereumEther.h"
#include "BREthereumGas.h"
#include "BREthereumAmount.h"
#include "rlp/BRRlp.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// Transaction
//
typedef struct BREthereumTransactionRecord *BREthereumTransaction;

extern BREthereumTransaction
transactionCreate(BREthereumAddress sourceAddress,
                  BREthereumAddress targetAddress,
                  BREthereumAmount amount,
                  BREthereumGasPrice gasPrice,
                  BREthereumGas gasLimit,
                  uint64_t nonce);

extern BREthereumAddress
transactionGetSourceAddress(BREthereumTransaction transaction);

extern BREthereumAddress
transactionGetTargetAddress(BREthereumTransaction transaction);

extern BREthereumAmount
transactionGetAmount(BREthereumTransaction transaction);

extern BREthereumGasPrice
transactionGetGasPrice (BREthereumTransaction transaction);

extern void
transactionSetGasPrice (BREthereumTransaction transaction,
                        BREthereumGasPrice gasPrice);

extern BREthereumGas
transactionGetGasLimit (BREthereumTransaction transaction);

extern void
transactionSetGasLimit (BREthereumTransaction transaction,
                        BREthereumGas gasLimit);

extern BREthereumGas
transactionGetGasEstimate (BREthereumTransaction transaction);

extern void
transactionSetGasEstimate (BREthereumTransaction transaction,
                          BREthereumGas gasEstimate);

extern uint64_t
transactionGetNonce (BREthereumTransaction transaction);

extern const BREthereumHash
transactionGetHash (BREthereumTransaction transaction);

/**
 * A token or null
 */
extern BREthereumToken
transactionGetToken (BREthereumTransaction transaction);

/**
 * Do not modify returned value
 */
extern const char *
transactionGetData (BREthereumTransaction transaction);

extern void
transactionSign(BREthereumTransaction transaction,
                BREthereumAccount account,
                BREthereumSignature signature);

extern BREthereumAccount
transactionGetSigner (BREthereumTransaction transaction);

extern BREthereumBoolean
transactionIsSigned (BREthereumTransaction transaction);

//
// RLP Encoding
//
typedef enum {
    TRANSACTION_RLP_SIGNED,
    TRANSACTION_RLP_UNSIGNED
} BREthereumTransactionRLPType;

/**
 * RLP encode transaction for the provided network with the specified type.  Different networks
 * have different RLP encodings - notably the network's chainId is part of the encoding.
 */
extern BRRlpData
transactionEncodeRLP (BREthereumTransaction transaction,
                      BREthereumNetwork network,
                      BREthereumTransactionRLPType type);

extern BREthereumTransaction
createTransactionDecodeRLP (BRRlpData data,
                            BREthereumTransactionRLPType type);

//
// Transaction Comparison
//

/**
 * Compares two transactions.
 */
extern BREthereumComparison
transactionCompare (BREthereumTransaction t1,
                    BREthereumTransaction t2);

//
// Transaction Status
//
typedef enum {
  TRANSACTION_CREATED,
  TRANSACTION_SIGNED,
  TRANSACTION_SUBMITTED,  // more than just 'sent'; in one 'mempool'; has hash
  TRANSACTION_BLOCKED,
  TRANSACTION_DROPPED     // not in any 'mempool'
} BREthereumTransactionStatus;

extern BREthereumTransactionStatus
transactionGetStatus (BREthereumTransaction transaction);

extern void
transactionAnnounceBlocked (BREthereumTransaction transaction,
                            BREthereumHash blockHash,
                            unsigned int blockIndex);

extern void
transactionAnnounceDropped (BREthereumTransaction transaction,
                            int foo); // dropped info

extern void
transactionAnnounceSubmitted (BREthereumTransaction transaction,
                              BREthereumHash hash); // submitted info

//
// Transaction Result
//
typedef struct BREthereumTransactionResultRecord *BREthereumTransactionResult;

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Transaction_H */
