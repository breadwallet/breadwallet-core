//
//  BREthereumLightNode
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 3/5/18.
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

#ifndef BR_Ethereum_Light_Node_H
#define BR_Ethereum_Light_Node_H

#define ETHEREUM_LIGHT_NODE_USE_JSON_RPC  1

#include <stdint.h>
#include "BREthereumEther.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int callback_1;
    int callback_2;
    char *url; //json_rpc??
} BREthereumLightNodeConfiguration;

// Errors
typedef enum {
    NODE_ERROR_X,
    NODE_ERROR_Y
} BREthereumLightNodeError;

typedef enum {
    NODE_TRANSACTION_STATUS_Unknown  = 0,  // (0): transaction is unknown
    NODE_TRANSACTION_STATUS_Queued   = 1,  // (1): transaction is queued (not processable yet)
    NODE_TRANSACTION_STATUS_Pending  = 2,  // (2): transaction is pending (processable)
    NODE_TRANSACTION_STATUS_Included = 3,  // (3): transaction is already included in the canonical chain. data contains an RLP-encoded [blockHash: B_32, blockNumber: P, txIndex: P] structure
    NODE_TRANSACTION_STATUS_Error    = 4   // (4): transaction sending failed. data contains a text error message
} BREthereumLightNodeTransactionStatus;

// Opaque Pointers
typedef void *BREthereumLightNodeTransactionId;
typedef void *BREthereumLightNodeAccountId;
typedef void *BREthereumLightNodeWalletId;

/**
 * An Ethereum Light Node
 *
 */
typedef struct BREthereumLightNodeRecord *BREthereumLightNode;

extern BREthereumLightNode
createLightNode (BREthereumLightNodeConfiguration configuration);

extern BREthereumLightNodeAccountId
lightNodeCreateAccount (BREthereumLightNode node,
                        const char *paperKey);

extern const char *
lightNodeGetAccountPrimaryAddress (BREthereumLightNode node,
                                   BREthereumLightNodeAccountId account);

/**
 * Create an wallet for `account` holding ETHER.
 *
 * @param node
 * @param accountId
 * @return
 */
extern BREthereumLightNodeWalletId
lightNodeCreateWallet (BREthereumLightNode node,
                       BREthereumLightNodeAccountId account);

// Token

//
// Holding / Ether
//

/**
 * Create Ether from a string representing a number.  The string can *only* contain digits and
 * a single decimal point.  No '-', no '+' no 'e' (exponents).
 *
 * @param node
 * @param number
 * @param unit
 * @param status This MUST NOT BE NULL. If assigned anything but OK, the return Ether is 0.
 */
extern BREthereumEther
lightNodeCreateEtherAmountString (BREthereumLightNode node,
                                  const char *number,
                                  BREthereumEtherUnit unit,
                                  BREthereumEtherParseStatus *status);

/**
 * Create Ether from a 'smallish' number an a unit
 *
 */
extern BREthereumEther
lightNodeCreateEtherAmountUnit (BREthereumLightNode node,
                                uint64_t amountInUnit,
                                BREthereumEtherUnit unit);

//
// Wallet Defaults
//
extern uint64_t
lightNodeGetWalletGasLimit (BREthereumLightNode node,
                            BREthereumLightNodeWalletId wallet);
extern void
lightNodeSetWalletGasLimit (BREthereumLightNode node,
                            BREthereumLightNodeWalletId wallet,
                            uint64_t gasLimit);

extern void
lightNodeSetWalletGasPrice (BREthereumLightNode node,
                            BREthereumLightNodeWalletId wallet,
                            BREthereumEtherUnit unit,
                            uint64_t value);

// ...

//
// Wallet Transactions
//

/**
 *
 * @param node
 * @param wallet
 * @param recvAddress A '0x' prefixed, strlen 42 Ethereum address.
 * @param unit
 * @param amountInUnit
 * @return
 */
extern BREthereumLightNodeTransactionId
lightNodeWalletCreateTransaction(BREthereumLightNode node,
                                 BREthereumLightNodeWalletId wallet,
                                 const char *recvAddress,
                                 BREthereumEther amount);

extern void // status, error
lightNodeWalletSignTransaction (BREthereumLightNode node,
                                BREthereumLightNodeWalletId wallet,
                                BREthereumLightNodeTransactionId transaction,
                                const char *paperKey);

#if !ETHEREUM_LIGHT_NODE_USE_JSON_RPC
extern void // status, error
lightNodeWalletSubmitTransaction (BREthereumLightNode node,
                                  BREthereumLightNodeWalletId wallet,
                                  BREthereumLightNodeTransactionId transaction);
#endif // !ETHEREUM_LIGHT_NODE_USE_JSON_RPC

//
// Transactions
//

/**
 * Return the serialized raw data for `transaction`.  The value `*bytesPtr` points to a byte array;
 * the callee OWNs that byte array (and thus must call free).  The value `*bytesCountPtr` hold
 * the size of the byte array.
 *
 * @param node
 * @param transaction
 * @param bytesPtr
 * @param bytesCountPtr
 */
#if ETHEREUM_LIGHT_NODE_USE_JSON_RPC
extern void
lightNodeFillTransactionRawData(BREthereumLightNode node,
                                BREthereumLightNodeWalletId wallet,
                                BREthereumLightNodeTransactionId transaction,
                                uint8_t **bytesPtr, size_t *bytesCountPtr);

extern const char *
lightNodeGetTransactionRawDataHexEncoded(BREthereumLightNode node,
                                         BREthereumLightNodeWalletId walletId,
                                         BREthereumLightNodeTransactionId transactionId,
                                         const char *prefix);
#endif // ETHEREUM_LIGHT_NODE_USE_JSON_RPC

extern const char *
lightNodeGetTransactionRecvAddress (BREthereumLightNode node,
                                    BREthereumLightNodeTransactionId transaction);

extern BREthereumEther
lightNodeGetTransactionAmount (BREthereumLightNode node,
                               BREthereumLightNodeTransactionId transaction);

#ifdef __cplusplus
}
#endif

#endif //BR_Ethereum_Light_Node_H
