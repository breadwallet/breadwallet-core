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

#include "BREthereumAccount.h"
#include "BREthereumEther.h"
#include "BREthereumGas.h"

#include "rlp/BRRlp.h"

#ifdef __cplusplus
extern "C" {
#endif

// Value type
typedef struct BREthereumTransactionRecord *BREthereumTransaction;

extern BREthereumTransaction
transactionCreate(BREthereumAddress sourceAddress,
                  BREthereumAddress targetAddress,
                  BREthereumEther amount,
                  BREthereumGasPrice gasPrice,
                  BREthereumGas gasLimit,
                  int nonce);

extern BREthereumAddress
transactionGetSourceAddress(BREthereumTransaction transaction);

extern BREthereumAddress
transactionGetTargetAddress(BREthereumTransaction transaction);

extern BREthereumEther
transactionGetAmount (BREthereumTransaction transaction);

extern BREthereumGasPrice
transactionGetGasPrice (BREthereumTransaction transaction);

extern BREthereumGas
transactionGetGasLimit (BREthereumTransaction transaction);

extern BREthereumAccount
transactionGetSigner (BREthereumTransaction transaction);

// Error if account does not hold sourceAddress ?
extern void
transactionSetSigner (BREthereumTransaction transaction, BREthereumAccount account);

extern BREthereumBoolean
transactionIsSigned (BREthereumTransaction transaction);

//
// RLP Encoding
//
typedef enum {
    TRANSACTION_RLP_SIGNED,
    TRANSACTION_RLP_UNSIGNED
} BREthereumTranactionRLPType;

extern BRRlpData
transactionEncodeRLP (BREthereumTransaction transaction,
                      BREthereumTranactionRLPType type);

extern BREthereumTransaction
createTransactionDecodeRLP (BRRlpData data,
                            BREthereumTranactionRLPType type);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Transaction_H */
