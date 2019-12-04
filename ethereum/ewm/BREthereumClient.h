//
//  BREthereumClient.h
//  BRCore
//
//  Created by Ed Gamble on 11/20/18.
//  Copyright © 2018-2019 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_Ethereum_Client_H
#define BR_Ethereum_Client_H

#include "BREthereumBase.h"
#include "BRCryptoSync.h"

#ifdef __cplusplus
extern "C" {
#endif

    // Cookies are used as markers to match up an asynchronous operation
    // request with its corresponding event.
    typedef void *BREthereumCookie;

    //
    // BREthereumClient
    //
    // Type definitions for client functions.  When configuring a EWM these functions must be
    // provided.  A EWM has limited cababilities; these callbacks provide data back into the
    // EWM (such as with the 'gas price' or with the BRD-indexed logs for a given address) or they
    // request certain data be saved to reestablish EWM state on start or they announce events
    // signifying changes in EWM state.
    //
    typedef void *BREthereumClientContext;

    /// MARK: - Balance

    typedef void
    (*BREthereumClientHandlerGetBalance) (BREthereumClientContext context,
                                          BREthereumEWM ewm,
                                          BREthereumWallet wid,
                                          const char *address,
                                          int rid);

    extern BREthereumStatus
    ewmAnnounceWalletBalance (BREthereumEWM ewm,
                              BREthereumWallet wid,
                              const char *balance,
                              int rid);

    /// MARK: - Gas Price

    typedef void
    (*BREthereumClientHandlerGetGasPrice) (BREthereumClientContext context,
                                           BREthereumEWM ewm,
                                           BREthereumWallet wid,
                                           int rid);

    extern BREthereumStatus
    ewmAnnounceGasPrice(BREthereumEWM ewm,
                        BREthereumWallet wid,
                        const char *gasPrice,
                        int rid);

    extern void
    ewmUpdateGasPrice (BREthereumEWM ewm,
                       BREthereumWallet wid);

    /// MARK: - Gas Estimate

    typedef void
    (*BREthereumClientHandlerEstimateGas) (BREthereumClientContext context,
                                           BREthereumEWM ewm,
                                           BREthereumWallet wid,
                                           BREthereumCookie cookie,
                                           const char *from,
                                           const char *to,
                                           const char *amount,
                                           const char *gasPrice,
                                           const char *data,
                                           int rid);

    extern BREthereumStatus
    ewmAnnounceGasEstimateSuccess (BREthereumEWM ewm,
                                   BREthereumWallet wallet,
                                   BREthereumCookie cookie,
                                   const char *gasEstimate,
                                   const char *gasPrice,
                                   int rid);

    extern BREthereumStatus
    ewmAnnounceGasEstimateFailure (BREthereumEWM ewm,
                                   BREthereumWallet wallet,
                                   BREthereumCookie cookie,
                                   BREthereumStatus status,
                                   int rid);

    extern void
    ewmGetGasEstimate (BREthereumEWM ewm,
                       BREthereumWallet wallet,
                       BREthereumTransfer transfer,
                       BREthereumCookie cookie);

    /// MARK: - Submit Transfer

    /**
     * Client handler for submitting a transaction.  Makes a JSON_RPC call with `transaction` and
     * then invokes `ewmAnnounceSubmitTransfer()` with the result.
     */
    typedef void
    (*BREthereumClientHandlerSubmitTransaction) (BREthereumClientContext context,
                                                 BREthereumEWM ewm,
                                                 BREthereumWallet wid,
                                                 BREthereumTransfer tid,
                                                 const char *transaction,
                                                 int rid);

    /**
     * Announce the result of a submitted transaction.  This is only called from the client
     * in the implementation of BREthereumClientHandlerSubmitTransaction (after the JSON_RPC
     * call is made to actually submit the transfer)
     *
     * @param ewm the ewm
     * @param wid the wallet
     * @param tid the transfer
     * @param hash the hash result of the submit, or NULL
     * @param errorCode the error code result of a failed submit, or -1
     * @param errorMessage the error message of a failed submit, or NULL
     * @param rid the rid
     *
     * @return 
     */
    extern BREthereumStatus
    ewmAnnounceSubmitTransfer (BREthereumEWM ewm,
                               BREthereumWallet wid,
                               BREthereumTransfer tid,
                               const char *hash,
                               int errorCode,
                               const char *errorMessage,
                               int rid);

    /// MARK: - Get Transactions

    typedef void
    (*BREthereumClientHandlerGetTransactions) (BREthereumClientContext context,
                                               BREthereumEWM ewm,
                                               const char *address,
                                               uint64_t begBlockNumber,
                                               uint64_t endBlockNumber,
                                               int rid);
    
    extern BREthereumStatus
    ewmAnnounceTransaction (BREthereumEWM ewm,
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
    ewmAnnounceTransactionComplete (BREthereumEWM ewm,
                                    int id,
                                    BREthereumBoolean success);

    /// MARK: - Get Logs

    typedef void
    (*BREthereumClientHandlerGetLogs) (BREthereumClientContext context,
                                       BREthereumEWM ewm,
                                       const char *contract,
                                       const char *address,
                                       const char *event,
                                       uint64_t begBlockNumber,
                                       uint64_t endBlockNumber,
                                       int rid);

    extern BREthereumStatus
    ewmAnnounceLog (BREthereumEWM ewm,
                    int id,
                    const char *strHash,
                    const char *strContract,
                    int topicCount,
                    const char **arrayTopics,
                    const char *strData,
                    const char *strGasPrice,
                    const char *strGasUsed,
                    const char *strLogIndex,
                    const char *strBlockNumber,
                    const char *strBlockTransactionIndex,
                    const char *strBlockTimestamp);

    extern void
    ewmAnnounceLogComplete (BREthereumEWM ewm,
                            int id,
                            BREthereumBoolean success);

    /// MARK: - Get Tokens

    typedef void
    (*BREthereumClientHandlerGetTokens) (BREthereumClientContext context,
                                         BREthereumEWM ewm,
                                         int rid);

    extern void
    ewmAnnounceToken(BREthereumEWM ewm,
                     int rid,
                     const char *address,
                     const char *symbol,
                     const char *name,
                     const char *description,
                     unsigned int decimals,
                     const char *strDefaultGasLimit,
                     const char *strDefaultGasPrice);

    extern void
    ewmAnnounceTokenComplete (BREthereumEWM ewm,
                              int rid,
                              BREthereumBoolean success);
    
    extern void
    ewmUpdateTokens (BREthereumEWM ewm);

    /// MARK: - BlockNumber

    typedef void
    (*BREthereumClientHandlerGetBlockNumber) (BREthereumClientContext context,
                                              BREthereumEWM ewm,
                                              int rid);

    extern BREthereumStatus
    ewmAnnounceBlockNumber (BREthereumEWM ewm,
                            const char *blockNumber,
                            int rid);

    /// MARK: - Nonce

    typedef void
    (*BREthereumClientHandlerGetNonce) (BREthereumClientContext context,
                                        BREthereumEWM ewm,
                                        const char *address,
                                        int rid);

    extern BREthereumStatus
    ewmAnnounceNonce (BREthereumEWM ewm,
                      const char *strAddress,
                      const char *strNonce,
                      int rid);

    /// MARK: - Blocks

    typedef void
    (*BREthereumClientHandlerGetBlocks) (BREthereumClientContext context,
                                         BREthereumEWM ewm,
                                         const char *address, // disappears immediately
                                         BREthereumSyncInterestSet interests,
                                         uint64_t blockNumberStart,
                                         uint64_t blockNumberStop,
                                         int rid);

    extern BREthereumStatus
    ewmAnnounceBlocks (BREthereumEWM ewm,
                       int id,
                       // const char *strBlockHash,
                       int blockNumbersCount,
                       uint64_t *blockNumbers);

    /// MARK: - Events

    typedef enum {
        WALLET_EVENT_CREATED = 0,
        WALLET_EVENT_BALANCE_UPDATED,
        WALLET_EVENT_DEFAULT_GAS_LIMIT_UPDATED,
        WALLET_EVENT_DEFAULT_GAS_PRICE_UPDATED,
        WALLET_EVENT_FEE_ESTIMATED,
        WALLET_EVENT_DELETED,
    } BREthereumWalletEventType;

    typedef struct {
        BREthereumWalletEventType type;
        BREthereumStatus status;
        union {
            struct {
                BREthereumCookie cookie;
                BREthereumGas gasEstimate;
                BREthereumGasPrice gasPrice;
            } feeEstimate;
        } u;
        char errorDescription[16];
    } BREthereumWalletEvent;

#define WALLET_NUMBER_OF_EVENT_TYPES  (1 + WALLET_EVENT_DELETED)

    extern BREthereumWalletEvent
    walletEventCreateError (BREthereumWalletEventType type,
                            BREthereumStatus status,
                            const char *errorDescription);
    
    typedef void (*BREthereumClientHandlerWalletEvent) (BREthereumClientContext context,
                                                        BREthereumEWM ewm,
                                                        BREthereumWallet wid,
                                                        BREthereumWalletEvent event);

#if defined (NEVER_DEFINED)
    typedef enum {
        BLOCK_EVENT_CREATED = 0,

        BLOCK_EVENT_CHAINED,
        BLOCK_EVENT_ORPHANED,

        BLOCK_EVENT_DELETED
    } BREthereumBlockEventType;

#define BLOCK_NUMBER_OF_EVENT_TYPES (1 + BLOCK_EVENT_DELETED)

    typedef struct {
        BREthereumBlockEventType type;
        BREthereumStatus status;
        char errorDescription[16];
    } BREthereumBlockEvent;

    typedef void (*BREthereumClientHandlerBlockEvent) (BREthereumClientContext context,
                                                       BREthereumEWM ewm,
                                                       BREthereumBlock bid,
                                                       BREthereumBlockEvent event);
#endif

    typedef enum {
        // Added/Removed from Wallet
        TRANSFER_EVENT_CREATED = 0,

        // Transfer State
        TRANSFER_EVENT_SIGNED,
        TRANSFER_EVENT_SUBMITTED,
        TRANSFER_EVENT_INCLUDED,  // aka confirmed
        TRANSFER_EVENT_ERRORED,

        TRANSFER_EVENT_GAS_ESTIMATE_UPDATED,

        TRANSFER_EVENT_DELETED
    } BREthereumTransferEventType;

#define TRANSACTION_NUMBER_OF_EVENT_TYPES (1 + TRANSACTION_EVENT_DELETED)

    typedef struct {
        BREthereumTransferEventType type;
        BREthereumStatus status;
        // uniont {} u;
        char errorDescription[16];
    } BREthereumTransferEvent;

    extern BREthereumTransferEvent
    transferEventCreateError (BREthereumTransferEventType type,
                              BREthereumStatus status,
                              const char *errorDescription);

    typedef void (*BREthereumClientHandlerTransferEvent) (BREthereumClientContext context,
                                                          BREthereumEWM ewm,
                                                          BREthereumWallet wid,
                                                          BREthereumTransfer tid,
                                                          BREthereumTransferEvent event);

    typedef enum {
        PEER_EVENT_CREATED = 0,
        PEER_EVENT_DELETED
        // added/removed/updated
    } BREthereumPeerEventType;

#define PEER_NUMBER_OF_EVENT_TYPES   (1 + PEER_EVENT_DELETED)
    typedef struct {
        BREthereumPeerEventType type;
        BREthereumStatus status;
        // union { } u;
        char errorDescription[16];
    } BREthereumPeerEvent;

    typedef void (*BREthereumClientHandlerPeerEvent) (BREthereumClientContext context,
                                                      BREthereumEWM ewm,
                                                      BREthereumPeerEvent event);

    typedef enum {
        TOKEN_EVENT_CREATED = 0,
        TOKEN_EVENT_DELETED
    } BREthereumTokenEventType;

#define TOKEN_NUMBER_OF_EVENT_TYPES  (1 + TOKEN_EVENT_DELETED)

    typedef struct {
        BREthereumTokenEventType type;
        BREthereumStatus status;
        // union {} u;
        char errorDescription[16];
    } BREthereumTokenEvent;
    
    typedef void (*BREthereumClientHandlerTokenEvent) (BREthereumClientContext context,
                                                       BREthereumEWM ewm,
                                                       BREthereumToken token,
                                                       BREthereumTokenEvent event);

    typedef enum {
        EWM_STATE_CREATED,
        EWM_STATE_CONNECTED,
        EWM_STATE_SYNCING,
        EWM_STATE_DISCONNECTED,
        EWM_STATE_DELETED
    } BREthereumEWMState;

    typedef enum {
        EWM_EVENT_CREATED = 0,
        EWM_EVENT_CHANGED,

        EWM_EVENT_SYNC_PROGRESS,
        EWM_EVENT_BLOCK_HEIGHT_UPDATED,
        EWM_EVENT_NETWORK_UNAVAILABLE,

        EWM_EVENT_DELETED,
    } BREthereumEWMEventType;

#define EWM_NUMBER_OF_EVENT_TYPES   (1 + EWM_EVENT_DELETED)

    typedef struct {
        BREthereumEWMEventType type;
        BREthereumStatus status;
        union {
            struct {
                BREthereumEWMState oldState;
                BREthereumEWMState newState;
            } changed;
            struct {
                BRCryptoSyncTimestamp timestamp;
                BRCryptoSyncPercentComplete percentComplete;
            } syncProgress;
            struct {
                uint64_t value;
            } blockHeight;
        } u;
        char errorDescription[16];
    } BREthereumEWMEvent;

    extern BREthereumEWMEvent
    ewmEventCreateError (BREthereumEWMEventType type,
                         BREthereumStatus status,
                         const char *errorDescription);

    typedef void (*BREthereumClientHandlerEWMEvent) (BREthereumClientContext context,
                                                     BREthereumEWM ewm,
                                                     // BREthereumWallet wid,
                                                     // BREthereumTransaction tid,
                                                     BREthereumEWMEvent event);

    //
    // EWM Configuration
    //
    // Used to configure a EWM appropriately for BRD or LES.  Starts with a
    // BREthereumNetwork (one of ethereum{Mainnet,Testnet,Rinkeby} and then specifics for the
    // type.
    //
    typedef struct {
        BREthereumClientContext context;

        // Backend Server Support - typically implemented with HTTP requests for JSON_RPC or DB
        // queries of BRD endpoints.  All of these functions *must* callback to the EWM with a
        // corresponding 'announce' function.
        BREthereumClientHandlerGetBalance funcGetBalance;
        BREthereumClientHandlerGetGasPrice funcGetGasPrice;
        BREthereumClientHandlerEstimateGas funcEstimateGas;
        BREthereumClientHandlerSubmitTransaction funcSubmitTransaction;
        BREthereumClientHandlerGetTransactions funcGetTransactions; // announce one-by-one
        BREthereumClientHandlerGetLogs funcGetLogs; // announce one-by-one
        BREthereumClientHandlerGetBlocks funcGetBlocks;
        BREthereumClientHandlerGetTokens funcGetTokens; // announce one-by-one
        BREthereumClientHandlerGetBlockNumber funcGetBlockNumber;
        BREthereumClientHandlerGetNonce funcGetNonce;

        // Events - Announce changes to entities that normally impact the UI.
        BREthereumClientHandlerEWMEvent funcEWMEvent;
        BREthereumClientHandlerPeerEvent funcPeerEvent;
        BREthereumClientHandlerWalletEvent funcWalletEvent;
        BREthereumClientHandlerTokenEvent funcTokenEvent;
        //       BREthereumClientHandlerBlockEvent funcBlockEvent;
        BREthereumClientHandlerTransferEvent funcTransferEvent;

    } BREthereumClient;

#ifdef __cplusplus
}
#endif

#endif //BR_Ethereum_Client_H

