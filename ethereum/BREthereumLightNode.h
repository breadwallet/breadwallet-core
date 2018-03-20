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
#include "BREthereumGas.h"
#include "BREthereumAmount.h"
#include "BREthereumNetwork.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// JSON RPC Support
//

// Some JSON_RPC call will occur to get all transactions associated with an account.  We'll
// process these transactions into the LightNode (associated with a wallet).  Thereafter
// a 'light node client' can get the announced transactions using non-JSON_RPC interfaces.
typedef void *JsonRpcAnnounceTransactionContext;
typedef void (*JsonRpcAnnounceTransaction) (JsonRpcAnnounceTransactionContext context,
                                            const char *from,
                                            const char *to,
                                            const char *amount, // value
                                            const char *gasLimit,
                                            const char *gasPrice,
                                            const char *data,
                                            const char *nonce);
//  {
//    "blockNumber":"1627184",
//    "timeStamp":"1516477482",
//    "hash":"0x4f992a47727f5753a9272abba36512c01e748f586f6aef7aed07ae37e737d220",
//    "nonce":"118",
//    "blockHash":"0x0ef0110d68ee3af220e0d7c10d644fea98252180dbfc8a94cab9f0ea8b1036af",
//    "transactionIndex":"3",
//    "from":"0x0ea166deef4d04aaefd0697982e6f7aa325ab69c",
//    "to":"0xde0b295669a9fd93d5f28d9ec85e40f4cb697bae",
//    "value":"11113000000000",
//    "gas":"21000",
//    "gasPrice":"21000000000",
//    "isError":"0",
//    "txreceipt_status":"1",
//    "input":"0x",
//    "contractAddress":"",
//    "cumulativeGasUsed":"106535",
//    "gasUsed":"21000",
//    "confirmations":"339050"}

//
// Type defintions for callback functions.  When configuring a LightNode to use JSON_RPC these
// functions must be provided.  The functions will use the provided arguments to create a JSON_RPC
// Ethereum call; block until the call returns, unpack the response and then provide the result
// (as a newly allocated string - the Ethereum Core will own it and free() it.)
//
typedef void *JsonRpcContext;
typedef const char* (*JsonRpcGetBalance) (JsonRpcContext context, int id, const char *account);
typedef const char* (*JsonRpcGetGasPrice) (JsonRpcContext context, int id);
typedef const char* (*JsonRpcEstimateGas) (JsonRpcContext context, int id, const char *to, const char *amount, const char *data);
typedef const char* (*JsonRpcSubmitTransaction) (JsonRpcContext context, int id, const char *transaction);
typedef void (*JsonRpcGetTransactions) (JsonRpcContext context, int id, const char *account,
                                        JsonRpcAnnounceTransaction announceTransaction,
                                        JsonRpcAnnounceTransactionContext announceTransactionContext);


//
// Two types of LightNode - JSON_RPC or LES (Light Ethereum Subprotocol).
//
typedef enum {
  NODE_TYPE_JSON_RPC,
  NODE_TYPE_LES
} BREthereumLightNodeType;

//
// Light Node Configuration
//
// Used to configure a light node appropriately for JSON_RPC or LES.  Starts with a
// BREthereumNetwork (one of ethereum{Mainnet,Testnet,Rinkeby} and then specifics for the
// type.
//
typedef struct {
  BREthereumNetwork network;
  BREthereumLightNodeType type;
  union {
    //
    struct {
      JsonRpcContext funcContext;
      JsonRpcGetBalance funcGetBalance;
      JsonRpcGetGasPrice functGetGasPrice;
      JsonRpcEstimateGas funcEstimateGas;
      JsonRpcSubmitTransaction funcSubmitTransaction;
      JsonRpcGetTransactions funcGetTransactions;
    } json_rpc;

    //
    struct {
      int foo;
    } les;
  } u;
} BREthereumLightNodeConfiguration;

/**
 * Create a LES configuration
 */
extern BREthereumLightNodeConfiguration
lightNodeConfigurationCreateLES (BREthereumNetwork network,
                                int foo);

/**
 * Create a JSON_RPC configuration w/ the set of functions needed to perform JSON RPC calls
 * and to process the result.
 */
extern BREthereumLightNodeConfiguration
lightNodeConfigurationCreateJSON_RPC(BREthereumNetwork network,
                                     JsonRpcContext context,
                                     JsonRpcGetBalance funcGetBalance,
                                     JsonRpcGetGasPrice functGetGasPrice,
                                     JsonRpcEstimateGas funcEstimateGas,
                                     JsonRpcSubmitTransaction funcSubmitTransaction,
                                     JsonRpcGetTransactions funcGetTransactions);

// Errors
typedef enum {
    NODE_ERROR_X,
    NODE_ERROR_Y
} BREthereumLightNodeError;

/**
 * Light Node Transaction Status - these are Ethereum defined.
 */
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

/**
 * Create an Ethereum Account using `paperKey` for BIP-32 generation of keys.  The same paper key
 * must be used when signing transactions for this account.
 */
extern BREthereumLightNodeAccountId
lightNodeCreateAccount (BREthereumLightNode node,
                        const char *paperKey);

/**
 * Get the primary address for `account`.  This is the '0x'-prefixed, 40-char, hex encoded
 * string.  The returned char* is newly allocated, on each call - you MUST free() it.
 */
extern const char *
lightNodeGetAccountPrimaryAddress (BREthereumLightNode node,
                                   BREthereumLightNodeAccountId account);

/**
 * Create a wallet for `account` holding ETHER.
 *
 * @param node
 * @param accountId
 * @return
 */
extern BREthereumLightNodeWalletId
lightNodeCreateWallet (BREthereumLightNode node,
                       BREthereumLightNodeAccountId account,
                       BREthereumNetwork network);

/**
 * Create a wallet for `account` holding `token`.
 *
 * @param node
 * @param account
 * @param network
 * @param token
 */
extern BREthereumLightNodeWalletId
lightNodeCreateWalletHoldingToken (BREthereumLightNode node,
                                   BREthereumLightNodeAccountId account,
                                   BREthereumNetwork network,
                                   BREthereumToken token);

//
// Holding / Ether
//

/**
 * Create Ether from a string representing a number.  The string can *only* contain digits and
 * a single decimal point.  No '-', no '+' no 'e' (exponents).
 *
 * @param node
 * @param number the amount as a decimal (base10) number.
 * @param unit the number's unit - typically ETH, GWEI or WEI.
 * @param status This MUST NOT BE NULL. If assigned anything but OK, the return Ether is 0.  In
 *        practice you must reference `status` otherwise you'll have unknown errors with 0 ETH.
 */
extern BREthereumAmount
lightNodeCreateEtherAmountString (BREthereumLightNode node,
                                  const char *number,
                                  BREthereumEtherUnit unit,
                                  BRCoreParseStatus *status);

/**
 * Create Ether from a 'smallish' number and a unit
 *
 */
extern BREthereumAmount
lightNodeCreateEtherAmountUnit (BREthereumLightNode node,
                                uint64_t amountInUnit,
                                BREthereumEtherUnit unit);

/**
 * Convert `ether` to a char* in `unit`.  Caller owns the result and must call free()
 */
extern char *
lightNodeCoerceEtherAmountToString (BREthereumLightNode node,
                                    BREthereumEther ether,
                                    BREthereumEtherUnit unit);

extern BREthereumAmount
lightNodeCreateTokenAmountString (BREthereumLightNode node,
                                  BREthereumToken token,
                                  const char *number,
                                  BREthereumTokenQuantityUnit unit,
                                  BRCoreParseStatus *status);

//
// Wallet Updates
//
extern BREthereumEther
lightNodeUpdateWalletBalance (BREthereumLightNode node,
                              BREthereumLightNodeWalletId wallet,
                              BRCoreParseStatus *status);

extern BREthereumGas
lightNodeUpdateWalletEstimatedGas (BREthereumLightNode node,
                                   BREthereumLightNodeWalletId wallet,
                                   BREthereumLightNodeTransactionId transaction,
                                   BRCoreParseStatus *status);

extern BREthereumGasPrice
lightNodeUpdateWalletEstimatedGasPrice (BREthereumLightNode node,
                                        BREthereumLightNodeWalletId wallet,
                                        BRCoreParseStatus *status);

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
 * Create a transaction to transfer `amount` from `wallet` to `recvAddrss`.
 *
 * @param node
 * @param wallet the wallet
 * @param recvAddress A '0x' prefixed, strlen 42 Ethereum address.
 * @param amount to transfer
 * @return
 */
extern BREthereumLightNodeTransactionId
lightNodeWalletCreateTransaction(BREthereumLightNode node,
                                 BREthereumLightNodeWalletId wallet,
                                 const char *recvAddress,
                                 BREthereumAmount amount);

  /**
   * Sign the transaction using the wallet's account (for the sender's address).  The paperKey
   * is used to 'lookup' the private key.
   *
   * @param node
   * @param wallet
   * @param transaction
   * @param paperKey
   */
extern void // status, error
lightNodeWalletSignTransaction (BREthereumLightNode node,
                                BREthereumLightNodeWalletId wallet,
                                BREthereumLightNodeTransactionId transaction,
                                const char *paperKey);

extern BREthereumBoolean // status, error
lightNodeWalletSubmitTransaction (BREthereumLightNode node,
                                  BREthereumLightNodeWalletId wallet,
                                  BREthereumLightNodeTransactionId transaction);

/**
 * Update the transactions for `wallet`.  For a JSON_RPC light node will call out to
 * JsonRpcGetTransactions which is expected to query all transactions associated with the
 * wallet's address and then the call out is to call back the 'account transaction' callback.
 */
extern void
lightNodeWalletUpdateTransactions (BREthereumLightNode node,
                                   BREthereumLightNodeWalletId walletId);
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
