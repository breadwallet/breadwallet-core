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

/**
 * An Ethereum Light Node
 *
 */
typedef struct BREthereumLightNodeRecord *BREthereumLightNode;

// Opaque Pointers
typedef int32_t BREthereumLightNodeTransactionId;
typedef int32_t BREthereumLightNodeAccountId;
typedef int32_t BREthereumLightNodeWalletId;


//
// JSON RPC Support
//

//
// Type defintions for callback functions.  When configuring a LightNode to use JSON_RPC these
// functions must be provided.  The functions will use the provided arguments to create a JSON_RPC
// Ethereum call; block until the call returns, unpack the response and then provide the result
// (as a newly allocated string - the Ethereum Core will own it and free() it.)
//
typedef void *JsonRpcContext;
typedef void (*JsonRpcGetBalance) (JsonRpcContext context,
                                   BREthereumLightNode node,
                                   BREthereumLightNodeWalletId wid,
                                   const char *address,
                                   int rid);

typedef void (*JsonRpcGetGasPrice) (JsonRpcContext context,
                                    BREthereumLightNode node,
                                    BREthereumLightNodeWalletId wid,
                                    int rid);

typedef void (*JsonRpcEstimateGas) (JsonRpcContext context,
                                    BREthereumLightNode node,
                                    BREthereumLightNodeWalletId wid,
                                    BREthereumLightNodeTransactionId tid,
                                    const char *to,
                                    const char *amount,
                                    const char *data,
                                    int rid);

typedef void (*JsonRpcSubmitTransaction) (JsonRpcContext context,
                                          BREthereumLightNode node,
                                          BREthereumLightNodeWalletId wid,
                                          BREthereumLightNodeTransactionId tid,
                                          const char *transaction,
                                          int rid);

typedef void (*JsonRpcGetTransactions) (JsonRpcContext context,
                                        BREthereumLightNode node,
                                        const char *address,
                                        int rid);


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


/**
 * Create a LightNode managing the account associated with the paperKey.  (The `paperKey` must
 * use words from the defaul wordList (Use installSharedWordList).  The `paperKey` is used for
 * BIP-32 generation of keys; the same paper key must be used when signing transactions for
 * this node's account.
 */
extern BREthereumLightNode
createLightNode (BREthereumLightNodeConfiguration configuration,
                 const char *paperKey);

/**
 * Create an Ethereum Account using `paperKey` for BIP-32 generation of keys.  The same paper key
 * must be used when signing transactions for this account.
 */
extern BREthereumLightNodeAccountId
lightNodeGetAccount (BREthereumLightNode node);

/**
 * Get the primary address for `account`.  This is the '0x'-prefixed, 40-char, hex encoded
 * string.  The returned char* is newly allocated, on each call - you MUST free() it.
 */
extern char *
lightNodeGetAccountPrimaryAddress (BREthereumLightNode node);

/**
 * Connect to the Ethereum Network;
 *
 * @param node
 * @return
 */
extern BREthereumBoolean
lightNodeConnect (BREthereumLightNode node);

extern BREthereumBoolean
lightNodeDisconnect (BREthereumLightNode node);

/**
 * Get the wallet for `account` holding ETHER.  This wallet is created, along with the account,
 * when a light node itself is created.
 *
 * @param node
 * @return
 */
extern BREthereumLightNodeWalletId
lightNodeGetWallet (BREthereumLightNode node);

/**
 * Get the wallet holding `token`.  If none exists, returns NULL.

 * @param node
 * @param token
 * @return
 */
extern BREthereumLightNodeWalletId
lightNodeGetWalletHoldingToken (BREthereumLightNode node,
                                BREthereumToken token);

/**
 * Create a wallet for `token` if one doesn't aleady exist; otherwise return the existing one.
 *
 * @param node
 * @param account
 * @param network
 * @param token
 */
extern BREthereumLightNodeWalletId
lightNodeCreateWalletHoldingToken (BREthereumLightNode node,
                                   BREthereumToken token);

/**
 * Token can be NULL => holds Ether
 *
 * @param node
 * @param wid
 * @param token
 * @return
 */
extern BREthereumBoolean
lightNodeWalletHoldsToken (BREthereumLightNode node,
                           BREthereumLightNodeWalletId wid,
                           BREthereumToken token);

extern BREthereumToken
lightNodeWalletGetToken (BREthereumLightNode node,
                         BREthereumLightNodeWalletId wid);

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
// Wallet Defaults
//
extern uint64_t
lightNodeWalletGetDefaultGasLimit (BREthereumLightNode node,
                                   BREthereumLightNodeWalletId wallet);

extern void
lightNodeWalletSetDefaultGasLimit (BREthereumLightNode node,
                                   BREthereumLightNodeWalletId wallet,
                                   uint64_t gasLimit);
  
extern uint64_t
lightNodeWalletGetGasEstimate (BREthereumLightNode node,
                               BREthereumLightNodeWalletId wallet,
                               BREthereumLightNodeTransactionId transaction);

extern void
lightNodeWalletSetDefaultGasPrice (BREthereumLightNode node,
                                   BREthereumLightNodeWalletId wallet,
                                   BREthereumEtherUnit unit,
                                   uint64_t value);

// Returns the ETH/GAS price in WEI.  IF the value is too large to express in WEI as a uint64_t
// then ZERO is returned.  Caution warranted.
extern uint64_t
lightNodeWalletGetDefaultGasPrice (BREthereumLightNode node,
                                   BREthereumLightNodeWalletId wallet);

extern BREthereumAmount
lightNodeWalletGetBalance (BREthereumLightNode node,
                           BREthereumLightNodeWalletId wallet);

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
                                BREthereumLightNodeWalletId wid,
                                BREthereumLightNodeTransactionId tid,
                                const char *paperKey);

extern void // status, error
lightNodeWalletSubmitTransaction (BREthereumLightNode node,
                                  BREthereumLightNodeWalletId wid,
                                  BREthereumLightNodeTransactionId tid);

/**
 * Returns a -1 terminated array of transaction identifiers.
 */
extern BREthereumLightNodeTransactionId *
lightNodeWalletGetTransactions (BREthereumLightNode node,
                                BREthereumLightNodeWalletId wallet);

/**
 * Returns -1 on invalid wid
 */
extern int // TODO: What in invalid wid?
lightNodeWalletGetTransactionCount (BREthereumLightNode node,
                                    BREthereumLightNodeWalletId wid);

/**
 * Update the transactions for the node's account.  A JSON_RPC light node will call out to
 * JsonRpcGetTransactions which is expected to query all transactions associated with the
 * accounts address and then the call out is to call back the 'announce transaction' callback.
 */
extern void
lightNodeUpdateTransactions (BREthereumLightNode node);

#if ETHEREUM_LIGHT_NODE_USE_JSON_RPC
//
// Wallet Updates
//
extern void
lightNodeUpdateWalletBalance (BREthereumLightNode node,
                              BREthereumLightNodeWalletId wid);

extern void
lightNodeUpdateTransactionGasEstimate (BREthereumLightNode node,
                                       BREthereumLightNodeWalletId wid,
                                       BREthereumLightNodeTransactionId tid);

extern void
lightNodeUpdateWalletDefaultGasPrice (BREthereumLightNode node,
                                      BREthereumLightNodeWalletId wid);


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

// Some JSON_RPC call will occur to get all transactions associated with an account.  We'll
// process these transactions into the LightNode (associated with a wallet).  Thereafter
// a 'light node client' can get the announced transactions using non-JSON_RPC interfaces.
extern void
lightNodeAnnounceTransaction(BREthereumLightNode node,
                             int id,
                             const char *hash,
                             const char *from,
                             const char *to,
                             const char *contract,
                             const char *amount, // value
                             const char *gasLimit,
                             const char *gasPrice,
                             const char *data,
                             const char *nonce,
                             const char *gasUsed,
                             const char *blockNumber,
                             const char *blockHash,
                             const char *blockConfirmations,
                             const char *blockTransactionIndex,
                             const char *blockTimestamp,
                             // cumulative gas used,
                             // confirmations
                             // txreceipt_status
                             const char *isError);

extern void
lightNodeAnnounceBalance (BREthereumLightNode node,
                          BREthereumLightNodeWalletId wid,
                          const char *balance,
                          int rid);

extern void
lightNodeAnnounceGasPrice(BREthereumLightNode node,
                          BREthereumLightNodeWalletId wid,
                          const char *gasEstimate,
                          int id);

extern void
lightNodeAnnounceGasEstimate (BREthereumLightNode node,
                              BREthereumLightNodeTransactionId tid,
                              const char *gasEstimate,
                              int rid);

extern void
lightNodeAnnounceSubmitTransaction (BREthereumLightNode node,
                                    BREthereumLightNodeTransactionId tid,
                                    const char *hash,
                                    int rid);

#endif // ETHEREUM_LIGHT_NODE_USE_JSON_RPC

extern char * // receiver, target
lightNodeTransactionGetRecvAddress (BREthereumLightNode node,
                                    BREthereumLightNodeTransactionId transaction);

extern char * // sender, source
lightNodeTransactionGetSendAddress (BREthereumLightNode node,
                                    BREthereumLightNodeTransactionId transaction);

extern char *
lightNodeTransactionGetAmountEther(BREthereumLightNode node,
                                   BREthereumLightNodeTransactionId transaction,
                                   BREthereumEtherUnit unit);

extern char *
lightNodeTransactionGetAmountTokenQuantity(BREthereumLightNode node,
                                           BREthereumLightNodeTransactionId transaction,
                                           BREthereumTokenQuantityUnit unit);

extern char *
lightNodeTransactionGetGasPrice (BREthereumLightNode node,
                                 BREthereumLightNodeTransactionId transaction,
                                 BREthereumEtherUnit unit);

extern uint64_t
lightNodeTransactionGetGasLimit (BREthereumLightNode node,
                                 BREthereumLightNodeTransactionId transaction);

extern uint64_t
lightNodeTransactionGetGasUsed (BREthereumLightNode node,
                                BREthereumLightNodeTransactionId transaction);

extern uint64_t
lightNodeTransactionGetNonce (BREthereumLightNode node,
                              BREthereumLightNodeTransactionId transaction);

extern uint64_t
lightNodeTransactionGetBlockNumber (BREthereumLightNode node,
                                    BREthereumLightNodeTransactionId transaction);

extern uint64_t
lightNodeTransactionGetBlockTimestamp (BREthereumLightNode node,
                                       BREthereumLightNodeTransactionId transaction);

extern BREthereumBoolean
lightNodeTransactionIsConfirmed (BREthereumLightNode node,
                                 BREthereumLightNodeTransactionId transaction);

extern BREthereumBoolean
lightNodeTransactionHoldsToken (BREthereumLightNode node,
                           BREthereumLightNodeTransactionId tid,
                           BREthereumToken token);

extern BREthereumToken
lightNodeTransactionGetToken (BREthereumLightNode node,
                         BREthereumLightNodeTransactionId tid);


#ifdef __cplusplus
}
#endif

#endif //BR_Ethereum_Light_Node_H
