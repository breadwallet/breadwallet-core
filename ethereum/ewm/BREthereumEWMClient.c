//
//  BREthereumEWMClient.c
//  BRCore
//
//  Created by Ed Gamble on 5/7/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "support/BRArray.h"
#include "BREthereumEWMPrivate.h"

// ==============================================================================================
//
// Wallet Balance
//
/**
 * Handle the Client Announcement for the balance of `wallet`.  This will be the BRD Backend's
 * result (not necessarily a JSON_RPC result) for the balance which will be ether ETH or TOK.
 *
 * @param ewm
 * @param wallet
 * @param value
 * @param rid
 */
extern void
ewmHandleAnnounceBalance (BREthereumEWM ewm,
                                BREthereumWallet wallet,
                                UInt256 value,
                                int rid) {
    BREthereumAmount amount =
    (AMOUNT_ETHER == walletGetAmountType(wallet)
     ? amountCreateEther(etherCreate(value))
     : amountCreateToken(createTokenQuantity(walletGetToken(wallet), value)));

    ewmSignalBalance(ewm, amount);
}

extern BREthereumStatus
ewmAnnounceWalletBalance (BREthereumEWM ewm,
                               BREthereumWallet wallet,
                               const char *balance,
                               int rid) {
    if (NULL == wallet) { return ERROR_UNKNOWN_WALLET; }

    // Passed in `balance` can be base 10 or 16.  Let UInt256Prase decide.
    BRCoreParseStatus parseStatus;
    UInt256 value = createUInt256Parse(balance, 0, &parseStatus);
    if (CORE_PARSE_OK != parseStatus) { return ERROR_NUMERIC_PARSE; }

    ewmSignalAnnounceBalance(ewm, wallet, value, rid);
    return SUCCESS;
}

// ==============================================================================================
//
// Default Wallet Gas Price
//
extern void
ewmUpdateGasPrice (BREthereumEWM ewm,
                                BREthereumWallet wallet) {

    if (NULL == wallet) {
        ewmSignalWalletEvent(ewm, wallet, WALLET_EVENT_DEFAULT_GAS_PRICE_UPDATED,
                                     ERROR_UNKNOWN_WALLET,
                                     NULL);
        
    } else if (ETHEREUM_BOOLEAN_IS_FALSE(ewmIsConnected(ewm))) {
        ewmSignalWalletEvent(ewm, wallet, WALLET_EVENT_DEFAULT_GAS_PRICE_UPDATED,
                                     ERROR_NODE_NOT_CONNECTED,
                                     NULL);
    } else {
        switch (ewm->mode) {
            case BRD_ONLY:
            case BRD_WITH_P2P_SEND: {
                ewm->client.funcGetGasPrice (ewm->client.context,
                                             ewm,
                                             wallet,
                                             ++ewm->requestId);
                break;
            }

            case P2P_WITH_BRD_SYNC:
            case P2P_ONLY:
                // TODO: LES Update Wallet Balance
                break;
        }
    }
}

extern void
ewmHandleAnnounceGasPrice (BREthereumEWM ewm,
                                 BREthereumWallet wallet,
                                 UInt256 amount,
                                 int rid) {
    ewmSignalGasPrice(ewm, wallet, gasPriceCreate(etherCreate(amount)));
}

extern BREthereumStatus
ewmAnnounceGasPrice(BREthereumEWM ewm,
                               BREthereumWallet wallet,
                               const char *gasPrice,
                               int rid) {
    if (NULL == wallet) { return ERROR_UNKNOWN_WALLET; }

    BRCoreParseStatus parseStatus;
    UInt256 amount = createUInt256Parse(gasPrice, 0, &parseStatus);
    if (CORE_PARSE_OK != parseStatus) { return ERROR_NUMERIC_PARSE; }

    ewmSignalAnnounceGasPrice(ewm, wallet, amount, rid);
    return SUCCESS;
}

// ==============================================================================================
//
// Transaction Gas Estimate
//

extern void
ewmUpdateGasEstimate (BREthereumEWM ewm,
                      BREthereumWallet wallet,
                      BREthereumTransfer transfer) {
    if (NULL == transfer) {
        ewmSignalTransferEvent(ewm, wallet, transfer,
                               TRANSFER_EVENT_GAS_ESTIMATE_UPDATED,
                               ERROR_UNKNOWN_WALLET,
                               NULL);
        
    } else if (ETHEREUM_BOOLEAN_IS_FALSE(ewmIsConnected(ewm))) {
        ewmSignalTransferEvent(ewm, wallet, transfer,
                               TRANSFER_EVENT_GAS_ESTIMATE_UPDATED,
                               ERROR_NODE_NOT_CONNECTED,
                               NULL);
    } else {
        switch (ewm->mode) {
            case BRD_ONLY:
            case BRD_WITH_P2P_SEND: {
                // This will be ZERO if transaction amount is in TOKEN.
                BREthereumEther amountInEther = transferGetEffectiveAmountInEther(transfer);
                BREthereumAddress fromAddress = transferGetSourceAddress(transfer);

                BREthereumTransaction transaction = transferGetOriginatingTransaction(transfer);

                char *from = addressGetEncodedString(fromAddress, 1);
                char *to = (char *) addressGetEncodedString(transactionGetTargetAddress(transaction), 0);
                char *amount = coerceStringPrefaced(amountInEther.valueInWEI, 16, "0x");

                ewm->client.funcEstimateGas (ewm->client.context,
                                             ewm,
                                             wallet,
                                             transfer,
                                             from,
                                             to,
                                             amount,
                                             transactionGetData(transaction),
                                             ++ewm->requestId);

                free (from);
                free(to);
                free(amount);
                break;
            }

            case P2P_WITH_BRD_SYNC:
            case P2P_ONLY:
                // TODO: LES Update Wallet Balance
                break;
        }
    }
}

extern void
ewmHandleAnnounceGasEstimate (BREthereumEWM ewm,
                                    BREthereumWallet wallet,
                                    BREthereumTransfer transfer,
                                    UInt256 value,
                                    int rid) {
    ewmSignalGasEstimate(ewm, wallet, transfer, gasCreate(value.u64[0]));
}

extern BREthereumStatus
ewmAnnounceGasEstimate (BREthereumEWM ewm,
                                   BREthereumWallet wallet,
                                   BREthereumTransfer transfer,
                                   const char *gasEstimate,
                                   int rid) {
    if (NULL == wallet) { return ERROR_UNKNOWN_WALLET; }
    if (NULL == transfer) { return ERROR_UNKNOWN_TRANSACTION; }

    BRCoreParseStatus parseStatus;
    UInt256 gas = createUInt256Parse(gasEstimate, 0, &parseStatus);

    if (CORE_PARSE_OK != parseStatus ||
        0 != gas.u64[1] || 0 != gas.u64[2] || 0 != gas.u64[3]) { return ERROR_NUMERIC_PARSE; }


    ewmSignalAnnounceGasEstimate(ewm, wallet, transfer, gas, rid);
    return SUCCESS;
}

// ==============================================================================================
//
// Get Block Number
//
/**
 * Handle a Client Announcement of `blockNumber`.  This will be BRD Backend's JSON_RPC result for
 * the most recent Ethereum blockNumber
 *
 * @param ewm
 * @param blockNumber
 * @param rid
 */
extern void
ewmHandleAnnounceBlockNumber (BREthereumEWM ewm,
                                    uint64_t blockNumber,
                                    int rid) {
    ewmUpdateBlockHeight(ewm, blockNumber);
}

extern BREthereumStatus
ewmAnnounceBlockNumber (BREthereumEWM ewm,
                        const char *strBlockNumber,
                        int rid) {
    uint64_t blockNumber = strtoull(strBlockNumber, NULL, 0);
    ewmSignalAnnounceBlockNumber (ewm, blockNumber, rid);
    return SUCCESS;
}

// ==============================================================================================
//
// Get Nonce
//
extern BREthereumStatus
ewmAnnounceNonce (BREthereumEWM ewm,
                  const char *strAddress,
                  const char *strNonce,
                  int rid) {
    BREthereumAddress address = addressCreate(strAddress);
    uint64_t nonce = strtoull (strNonce, NULL, 0);
    ewmSignalAnnounceNonce(ewm, address, nonce, rid);
    return SUCCESS;
}

/**
 * Handle a Client Announcement of `nonce`.  This will be the BRD Backend's JSON_RPC result for
 * the the address' nonce.
 *
 * @param ewm
 * @param address
 * @param nonce
 * @param rid
 */
extern void
ewmHandleAnnounceNonce (BREthereumEWM ewm,
                              BREthereumAddress address,
                              uint64_t nonce,
                              int rid) {
    //    BREthereumEncodedAddress address = accountGetPrimaryAddress (ewmGetAccount(ewm));
    //    assert (ETHEREUM_BOOLEAN_IS_TRUE (addressHasString(address, strAddress)));
    accountSetAddressNonce(ewm->account, accountGetPrimaryAddress(ewm->account), nonce, ETHEREUM_BOOLEAN_FALSE);
}

// ==============================================================================================
//
// Get Transactions
//


extern void
ewmHandleAnnounceTransaction(BREthereumEWM ewm,
                                   BREthereumEWMClientAnnounceTransactionBundle *bundle,
                                   int id) {
    switch (ewm->mode) {
        case BRD_ONLY:
        case BRD_WITH_P2P_SEND: {
            //
            // This 'announce' call is coming from the guaranteed BRD endpoint; thus we don't need to
            // worry about the validity of the transaction - it is surely confirmed.  Is that true
            // if newly submitted?

            // TODO: Confirm we are not repeatedly creating transactions
            BREthereumTransaction transaction = transactionCreate (bundle->from,
                                                                   bundle->to,
                                                                   etherCreate(bundle->amount),
                                                                   gasPriceCreate(etherCreate(bundle->gasPrice)),
                                                                   gasCreate(bundle->gasLimit),
                                                                   bundle->data,
                                                                   bundle->nonce);

            // We set the transaction's hash based on the value providedin the bundle.  However,
            // and importantly, if we attempted to compute the hash - as we normally do for a
            // signed transaction - the computed hash would be utterly wrong.  The transaction
            // just created does not have: network nor signature.
            //
            // TODO: Confirm that BRPersistData does not overwrite the transaction's hash
            transactionSetHash (transaction, bundle->hash);

            BREthereumTransactionStatus status = transactionStatusCreateIncluded (bundle->blockHash,
                                                                                  bundle->blockNumber,
                                                                                  bundle->blockTransactionIndex,
                                                                                  bundle->blockTimestamp,
                                                                                  gasCreate(bundle->gasUsed));
            transactionSetStatus (transaction, status);

            // If we had a `bcs` we might think about `bcsSignalTransaction(ewm->bcs, transaction);`
            ewmSignalTransaction(ewm, BCS_CALLBACK_TRANSACTION_UPDATED, transaction);

            break;
        }

        case P2P_WITH_BRD_SYNC:
        case P2P_ONLY:
            bcsSendTransactionRequest(ewm->bcs,
                                      bundle->hash,
                                      bundle->blockNumber,
                                      bundle->blockTransactionIndex);
            break;
    }
    ewmClientAnnounceTransactionBundleRelease(bundle);
}

extern BREthereumStatus
ewmAnnounceTransaction(BREthereumEWM ewm,
                       int id,
                       const char *hashString,
                       const char *from,
                       const char *to,
                       const char *contract,
                       const char *strAmount, // value
                       const char *strGasLimit,
                       const char *strGasPrice,
                       const char *data,
                       const char *strNonce,
                       const char *strGasUsed,
                       const char *strBlockNumber,
                       const char *strBlockHash,
                       const char *strBlockConfirmations,
                       const char *strBlockTransactionIndex,
                       const char *strBlockTimestamp,
                       const char *isError) {
    BRCoreParseStatus parseStatus;
    BREthereumEWMClientAnnounceTransactionBundle *bundle = malloc(sizeof (BREthereumEWMClientAnnounceTransactionBundle));

    bundle->hash = hashCreate(hashString);

    bundle->from = addressCreate(from);
    bundle->to = addressCreate(to);
    bundle->contract = (NULL == contract || '\0' == contract[0]
                        ? EMPTY_ADDRESS_INIT
                        : addressCreate(contract));

    bundle->amount = createUInt256Parse(strAmount, 0, &parseStatus);

    bundle->gasLimit = strtoull(strGasLimit, NULL, 0);
    bundle->gasPrice = createUInt256Parse(strGasPrice, 0, &parseStatus);
    bundle->data = strdup(data);

    bundle->nonce = strtoull(strNonce, NULL, 0); // TODO: Assumes `nonce` is uint64_t; which it is for now
    bundle->gasUsed = strtoull(strGasUsed, NULL, 0);

    bundle->blockNumber = strtoull(strBlockNumber, NULL, 0);
    bundle->blockHash = hashCreate (strBlockHash);
    bundle->blockConfirmations = strtoull(strBlockConfirmations, NULL, 0);
    bundle->blockTransactionIndex = (unsigned int) strtoul(strBlockTransactionIndex, NULL, 0);
    bundle->blockTimestamp = strtoull(strBlockTimestamp, NULL, 0);

    bundle->isError = AS_ETHEREUM_BOOLEAN(0 != strcmp (isError, "0"));

    ewmSignalAnnounceTransaction(ewm, bundle, id);
    return SUCCESS;
}

extern void
ewmAnnounceTransactionComplete (BREthereumEWM ewm,
                                int id,
                                BREthereumBoolean success) {
    ewmSignalAnnounceComplete (ewm, ETHEREUM_BOOLEAN_TRUE, success, id);
}

// ==============================================================================================
//
// Get Logs
//
extern void
ewmHandleAnnounceLog (BREthereumEWM ewm,
                      BREthereumEWMClientAnnounceLogBundle *bundle,
                      int id) {
    switch (ewm->mode) {
        case BRD_ONLY:
        case BRD_WITH_P2P_SEND: {
            // This 'announce' call is coming from the guaranteed BRD endpoint; thus we don't need to
            // worry about the validity of the transaction - it is surely confirmed.

            BREthereumLogTopic topics [bundle->topicCount];
            for (size_t index = 0; index < bundle->topicCount; index++)
                topics[index] = logTopicCreateFromString(bundle->arrayTopics[index]);

            // In general, log->data is arbitrary data.  In the case of an ERC20 token, log->data
            // is a numeric value - for the transfer amount.  When parsing in logRlpDecode(),
            // log->data is assigned with rlpDecodeBytes(coder, items[2]); we'll need the same
            // thing, somehow

            BRCoreParseStatus parseStatus = CORE_PARSE_OK;
            UInt256 value = createUInt256Parse(bundle->data, 0, &parseStatus);
            assert (CORE_PARSE_OK == parseStatus);

            BRRlpItem  item  = rlpEncodeUInt256 (ewm->coder, value, 1);

            BREthereumLog log = logCreate(bundle->contract,
                                          bundle->topicCount,
                                          topics,
                                          rlpGetDataSharedDontRelease(ewm->coder, item));
            rlpReleaseItem (ewm->coder, item);

            // Given {hash,logIndex}, initialize the log's identifier
            assert (bundle->logIndex <= (uint64_t) SIZE_MAX);
            logInitializeIdentifier(log, bundle->hash, (size_t) bundle->logIndex);

            BREthereumTransactionStatus status =
            transactionStatusCreateIncluded (hashCreateEmpty(),
                                             bundle->blockNumber,
                                             bundle->blockTransactionIndex,
                                             bundle->blockTimestamp,
                                             gasCreate(0));
            logSetStatus(log, status);

            // If we had a `bcs` we might think about `bcsSignalLog(ewm->bcs, log);`
            ewmSignalLog(ewm, BCS_CALLBACK_LOG_UPDATED, log);

            // Do we need a transaction here?  No, if another originated this Log, then we can't ever
            // care and if we originated this Log, then we'll get the transaction (as part of the BRD
            // 'getTransactions' endpoint).
            //
            // Of course, see the comment in bcsHandleLog asking how to tell the client about a Log...
            break;
        }

        case P2P_WITH_BRD_SYNC:
        case P2P_ONLY:
            bcsSendLogRequest(ewm->bcs,
                              bundle->hash,
                              bundle->blockNumber,
                              bundle->blockTransactionIndex);
            break;
    }
    ewmClientAnnounceLogBundleRelease(bundle);
}

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
                const char *strBlockTimestamp) {

    BRCoreParseStatus parseStatus;
    BREthereumEWMClientAnnounceLogBundle *bundle = malloc(sizeof (BREthereumEWMClientAnnounceLogBundle));

    bundle->hash = hashCreate(strHash);
    bundle->contract = addressCreate(strContract);
    bundle->topicCount = topicCount;
    bundle->arrayTopics = calloc (topicCount, sizeof (char *));
    for (int i = 0; i < topicCount; i++)
        bundle->arrayTopics[i] = strdup (arrayTopics[i]);
    bundle->data = strdup (strData);
    bundle->gasPrice = createUInt256Parse(strGasPrice, 0, &parseStatus);
    bundle->gasUsed = strtoull(strGasUsed, NULL, 0);
    bundle->logIndex = strtoull(strLogIndex, NULL, 0);
    bundle->blockNumber = strtoull(strBlockNumber, NULL, 0);
    bundle->blockTransactionIndex = strtoull(strBlockTransactionIndex, NULL, 0);
    bundle->blockTimestamp = strtoull(strBlockTimestamp, NULL, 0);

    ewmSignalAnnounceLog(ewm, bundle, id);
    return SUCCESS;
}

extern void
ewmAnnounceLogComplete (BREthereumEWM ewm,
                        int id,
                        BREthereumBoolean success) {
    ewmSignalAnnounceComplete (ewm, ETHEREUM_BOOLEAN_FALSE, success, id);
}

// ==============================================================================================
//
// Announce {Transactions, Logs} Complete
//
extern void
ewmHandleAnnounceComplete (BREthereumEWM ewm,
                           BREthereumBoolean isTransaction,
                           BREthereumBoolean success,
                           int rid) {
    if (ETHEREUM_BOOLEAN_IS_TRUE(isTransaction)) {
        if (rid == ewm->brdSync.ridTransaction)
            ewm->brdSync.completedTransaction = ETHEREUM_BOOLEAN_IS_TRUE(success);
    }
    else {
        if (rid == ewm->brdSync.ridLog)
            ewm->brdSync.completedLog = ETHEREUM_BOOLEAN_IS_TRUE(success);
    }
}

// ==============================================================================================
//
// Blocks
//

extern BREthereumStatus
ewmAnnounceBlocks (BREthereumEWM ewm,
                              int id,
                              // const char *strBlockHash,
                              int blockNumbersCount,
                              uint64_t *blockNumbers) {  // BRArrayOf(const char *) strBlockNumbers ??
    assert (P2P_ONLY == ewm->mode || P2P_WITH_BRD_SYNC == ewm->mode);

    // into bcs...
    BRArrayOf(uint64_t) numbers;
    array_new (numbers, blockNumbersCount);
    array_add_array(numbers, blockNumbers, blockNumbersCount);
    bcsReportInterestingBlocks (ewm->bcs, numbers);

    return SUCCESS;
}


// ==============================================================================================
//
// Submit Transaction
//


/**
 * Submit the specfied transfer.  The transfer may represent the exchange of ETH or of TOK (an
 * ERC20 token).  In the former the transfer's basis is a transaction; in the later a log.
 *
 * @param ewm ewm
 * @param wid wid
 & @param tid tid
 */
extern void // status, error
ewmWalletSubmitTransfer(BREthereumEWM ewm,
                        BREthereumWallet wallet,
                        BREthereumTransfer transfer) {
    // assert: wallet-has-transfer
    // assert: signed
    // assert: originatingTransaction

    BREthereumTransaction transaction = transferGetOriginatingTransaction(transfer);
    BREthereumBoolean isSigned = transactionIsSigned (transaction);

    // NOTE: If the transfer is for TOK, we (likely) don't have a log.  Therefore we can't tell BCS
    // to pend on a Log.  As perhaps a function `bcsSendLog(bcs, log)` might do.  Instead on
    // transaction callbacks we'll need to find the transfer with the log for transaction and then
    // update the transfer's status.

    switch (ewm->mode) {
        case BRD_ONLY: {
            char *rawTransaction = transactionGetRlpHexEncoded (transaction,
                                                                ewm->network,
                                                                (ETHEREUM_BOOLEAN_IS_TRUE (isSigned)
                                                                 ? RLP_TYPE_TRANSACTION_SIGNED
                                                                 : RLP_TYPE_TRANSACTION_UNSIGNED),
                                                                "0x");

            ewm->client.funcSubmitTransaction (ewm->client.context,
                                               ewm,
                                               wallet,
                                               transfer,
                                               rawTransaction,
                                               ++ewm->requestId);

            free(rawTransaction);
            break;
        }

        case BRD_WITH_P2P_SEND:
        case P2P_WITH_BRD_SYNC:
        case P2P_ONLY:
            bcsSendTransaction(ewm->bcs, transaction);
            break;
    }
}

extern void
ewmHandleAnnounceSubmitTransfer (BREthereumEWM ewm,
                                 BREthereumWallet wallet,
                                 BREthereumTransfer transfer,
                                 int errorCode,
                                 const char *errorMessage,
                                 int rid) {
    BREthereumTransaction transaction = transferGetOriginatingTransaction (transfer);
    assert (NULL != transaction);

    BREthereumTransactionStatus status = transactionStatusCreate(TRANSACTION_STATUS_PENDING);

    switch (ewm->mode) {
        case BRD_ONLY:
            // NODE: For BRD_ONLY the BRD endpoint is a GETH node.  Hence lesTransactionErrorPreface,
            if (NULL != errorMessage) {
                BREthereumTransactionErrorType type = lookupTransactionErrorType (lesTransactionErrorPreface, errorMessage);
                status = transactionStatusCreateErrored (type, errorMessage);
            }
            else if (-1 != errorCode) // -32000
                status = transactionStatusCreateErrored (TRANSACTION_ERROR_UNKNOWN, transactionGetErrorName(TRANSACTION_ERROR_UNKNOWN));
            break;

        case BRD_WITH_P2P_SEND:
        case P2P_WITH_BRD_SYNC:
        case P2P_ONLY:
            // TODO: Is this anything besides PENDING?
            // Is this even called outside of BRD_ONLY?  If so, why did BRD_ONLY have assert(0)?
            break;
    }

    transactionSetStatus (transaction, status);

    // If we had a `bcs` we might think about `bcsSignalTransaction(ewm->bcs, transaction);`
    ewmSignalTransaction (ewm, BCS_CALLBACK_TRANSACTION_ADDED, transaction);

}

extern BREthereumStatus
ewmAnnounceSubmitTransfer(BREthereumEWM ewm,
                          BREthereumWallet wallet,
                          BREthereumTransfer transfer,
                          const char *strHash,
                          int errorCode,
                          const char *errorMessage,
                          int id) {
    if (NULL == wallet) { return ERROR_UNKNOWN_WALLET; }
    if (NULL == transfer) { return ERROR_UNKNOWN_TRANSACTION; }

    if (NULL != strHash) {
        BREthereumHash hash = hashCreate(strHash);
        // We announce a submitted transfer => there is an originating transaction.
        if (ETHEREUM_BOOLEAN_IS_TRUE (hashEqual (hash, EMPTY_HASH_INIT))
            || ETHEREUM_BOOLEAN_IS_FALSE (hashEqual (hash, transferGetOriginatingTransactionHash (transfer))))
            return ERROR_TRANSACTION_HASH_MISMATCH;
    }

    ewmSignalAnnounceSubmitTransfer (ewm, wallet, transfer, errorCode, errorMessage, id);
    return SUCCESS;
}


// ==============================================================================================
//
// Update Tokens
//
extern void
ewmUpdateTokens (BREthereumEWM ewm) {
    ewm->client.funcGetTokens
    (ewm->client.context,
     ewm,
     ++ewm->requestId);
}

extern void
ewmHandleAnnounceToken (BREthereumEWM ewm,
                              BREthereumEWMClientAnnounceTokenBundle *bundle,
                              int id) {
    tokenInstall (bundle->address,
                  bundle->symbol,
                  bundle->name,
                  bundle->description,
                  bundle->decimals,
                  bundle->gasLimit,
                  bundle->gasPrice);

    BREthereumToken token = tokenLookup(bundle->address);
    assert (NULL != token);

    ewm->client.funcTokenEvent (ewm->client.context,
                                ewm,
                                token,
                                TOKEN_EVENT_CREATED);

    ewmClientAnnounceTokenBundleRelease(bundle);
}

#include <errno.h>

extern void
ewmAnnounceToken(BREthereumEWM ewm,
                 const char *address,
                 const char *symbol,
                 const char *name,
                 const char *description,
                 unsigned int decimals,
                 const char *strDefaultGasLimit,
                 const char *strDefaultGasPrice,
                 int rid) {
    char *strEndPointer = NULL;

    // Parse strDefaultGasLimit - as a decimal, hex or even octal string.  If the parse fails
    // quietly fall back to the default gas limit of TOKEN_BRD_DEFAULT_GAS_LIMIT.  The parse can
    // fail if: strDefaultGasLimit is not fully consumed (e.g. "123abc"); the parsed value is zero,
    // the parsed value is out of range, the parse fails.
    errno = 0;
    unsigned long long gasLimitValue = 0;
    if (NULL != strDefaultGasLimit)
        gasLimitValue = strtoull(strDefaultGasLimit, &strEndPointer, 0);
    if (gasLimitValue == 0 ||
        errno == EINVAL || errno == ERANGE ||
        (strEndPointer != NULL && *strEndPointer != '\0'))
        gasLimitValue = TOKEN_BRD_DEFAULT_GAS_LIMIT;

    // Parse strDefaultGasPrice - as a decimal, hex, etc - into a UInt256 representing the gasPrice
    // in WEI.  If the parse fails, quietly fall back to using the default of
    // TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64.
    BRCoreParseStatus status = CORE_PARSE_STRANGE_DIGITS;
    UInt256 gasPriceValue = UINT256_ZERO;
    if (NULL != strDefaultGasPrice)
        gasPriceValue = createUInt256Parse(strDefaultGasPrice, 0, &status);
    if (status != CORE_PARSE_OK)
        gasPriceValue = createUInt256(TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64);

    BREthereumEWMClientAnnounceTokenBundle *bundle = malloc(sizeof (BREthereumEWMClientAnnounceTokenBundle));

    bundle->address     = strdup (address);
    bundle->symbol      = strdup (symbol);
    bundle->name        = strdup (name);
    bundle->description = strdup (description);
    bundle->decimals    = decimals;
    bundle->gasLimit    = gasCreate(gasLimitValue);
    bundle->gasPrice    = gasPriceCreate(etherCreate(gasPriceValue));

    ewmSignalAnnounceToken (ewm, bundle, rid);
}

extern void
ewmHandleAnnounceTokenComplete (BREthereumEWM ewm,
                                BREthereumBoolean success,
                                int rid) {
    if (ETHEREUM_BOOLEAN_IS_TRUE (success))
        ewmSync (ewm);
}

extern void
ewmAnnounceTokenComplete (BREthereumEWM ewm,
                          BREthereumBoolean success,
                          int rid) {
    ewmSignalAnnounceTokenComplete (ewm, success, rid);
}

// ==============================================================================================
//
// Client
//

extern void
ewmHandleWalletEvent(BREthereumEWM ewm,
                           BREthereumWallet wid,
                           BREthereumWalletEvent event,
                           BREthereumStatus status,
                           const char *errorDescription) {
    ewm->client.funcWalletEvent
    (ewm->client.context,
     ewm,
     wid,
     event,
     status,
     errorDescription);
}

#if defined (NEVER_DEFINED)
extern void
ewmHandleBlockEvent(BREthereumEWM ewm,
                          BREthereumBlock block,
                          BREthereumBlockEvent event,
                          BREthereumStatus status,
                          const char *errorDescription) {
    ewm->client.funcBlockEvent (ewm->client.context,
                                ewm,
                                block,
                                event,
                                status,
                                errorDescription);
}
#endif

static int
ewmNeedTransferSave (BREthereumEWM ewm,
                     BREthereumTransferEvent event) {
    return (TRANSFER_EVENT_GAS_ESTIMATE_UPDATED != event &&
            TRANSFER_EVENT_BLOCK_CONFIRMATIONS_UPDATED != event);
}

extern void
ewmHandleTransferEvent (BREthereumEWM ewm,
                        BREthereumWallet wallet,
                        BREthereumTransfer transfer,
                        BREthereumTransferEvent event,
                        BREthereumStatus status,
                        const char *errorDescription) {

    // If `transfer` represents a token transfer that we've created/submitted, then we won't have
    // the actual `log` until the corresponding originating transaction is included.  We won't
    // have a hash (the log hash - based on the transaction hash + transaction index) and
    // we won't have data (the RLP encoding of the log).  Therefore there is literally nothing
    // to callback with `funcChangeLog`.
    //
    // Either we must a) create a log from the transaction (perhaps being willing to replace it
    // once the included log exists or b) avoid announcing `funcChangeLog:CREATED` until the
    // log is actually created....

    if (ewmNeedTransferSave (ewm, event)) {
        BREthereumTransaction transaction = transferGetBasisTransaction (transfer);
        BREthereumLog         log         = transferGetBasisLog(transfer);

        // If we have a hash, then we've got something to save.
        BREthereumHash hash = transferGetIdentifier(transfer);
        if (ETHEREUM_BOOLEAN_IS_FALSE (hashCompare (hash, EMPTY_HASH_INIT))) {

            // One of `transaction` or `log` will always be null
            assert (NULL == transaction || NULL == log);

            // We know that only on SIGNED do we have a transaction hash.  Only on
            // included do we have a log hash.  Thus we might see CHANGE_UPD w/o a
            // CHANGE_ADD - and that is NOT a problem.
            BREthereumClientChangeType type = ((event == TRANSFER_EVENT_CREATED ||
                                                event == TRANSFER_EVENT_SIGNED)
                                               ? CLIENT_CHANGE_ADD
                                               : (event == TRANSFER_EVENT_DELETED
                                                  ? CLIENT_CHANGE_REM
                                                  : CLIENT_CHANGE_UPD));

            if (NULL != transaction)
                ewmHandleSaveTransaction(ewm, transaction, type);

            if (NULL != log)
                ewmHandleSaveLog(ewm, log, type);
        }
    }

    // Announce the transfer
    ewm->client.funcTransferEvent (ewm->client.context,
                                   ewm,
                                   wallet,
                                   transfer,
                                   event,
                                   status,
                                   errorDescription);
}

extern void
ewmHandlePeerEvent(BREthereumEWM ewm,
                         // BREthereumWallet wid,
                         // BREthereumTransaction tid,
                         BREthereumPeerEvent event,
                         BREthereumStatus status,
                         const char *errorDescription) {
    ewm->client.funcPeerEvent (ewm->client.context,
                               ewm,
                               // event->wid,
                               // event->tid,
                               event,
                               status,
                               errorDescription);
}

extern void
ewmHandleEWMEvent(BREthereumEWM ewm,
                        // BREthereumWallet wid,
                        // BREthereumTransaction tid,
                        BREthereumEWMEvent event,
                        BREthereumStatus status,
                        const char *errorDescription) {
    ewm->client.funcEWMEvent (ewm->client.context,
                              ewm,
                              //event->wid,
                              // event->tid,
                              event,
                              status,
                              errorDescription);
}



#if 0 // Transaction
BREthereumTransactionId tid = -1;
BREthereumAddress primaryAddress = accountGetPrimaryAddress(ewm->account);

assert (ETHEREUM_BOOLEAN_IS_TRUE(addressEqual(primaryAddress, bundle->from))
        || ETHEREUM_BOOLEAN_IS_TRUE(addressEqual(primaryAddress, bundle->to)));

// primaryAddress is either the transaction's `source` or `target`.
BREthereumBoolean isSource = addressEqual(primaryAddress, bundle->from);

BREthereumWalletId wid = ewmGetWallet(ewm);
BREthereumWallet wallet = ewmLookupWallet(ewm, wid);

BREthereumBlock block = ewmLookupBlockByHash(ewm, bundle->blockHash);
block = blockCreateMinimal(bundle->blockHash, bundle->blockNumber, bundle->blockTimestamp);
ewmSignalBlockEvent(ewm, ewmInsertBlock(ewm, block),
                          BLOCK_EVENT_CREATED,
                          SUCCESS, NULL);

// Look for a pre-existing transaction
BREthereumTransaction transaction = walletGetTransactionByHash(wallet, bundle->hash);

// If we did not have a transaction for 'hash' it might be (might likely be) a newly submitted
// transaction that we are holding but that doesn't have a hash yet.  This will *only* apply
// if we are the source.
if (NULL == transaction && ETHEREUM_BOOLEAN_IS_TRUE(isSource))
transaction = walletGetTransactionByNonce(wallet, primaryAddress, bundle->nonce);

// If we still don't have a transaction (with 'hash' or 'nonce'); then create one.
if (NULL == transaction) {
    BREthereumAddress sourceAddr = (ETHEREUM_BOOLEAN_IS_TRUE(isSource) ? primaryAddress : bundle->from);
    BREthereumAddress targetAddr = (ETHEREUM_BOOLEAN_IS_TRUE(isSource) ? bundle->to : primaryAddress);

    // Get the amount; this will be '0' if this is a token transfer
    BREthereumAmount amount = amountCreateEther(etherCreate(bundle->amount));

    // Easily extract the gasPrice and gasLimit.
    BREthereumGasPrice gasPrice = gasPriceCreate(etherCreate(bundle->gasPrice));

    BREthereumGas gasLimit = gasCreate(bundle->gasLimit);

    // Finally, get ourselves a transaction.
    transaction = transactionCreate(sourceAddr,
                                    targetAddr,
                                    amount,
                                    gasPrice,
                                    gasLimit,
                                    bundle->nonce);
    // With a new transaction:
    //
    //   a) add to the ewm
    tid = ewmInsertTransaction(ewm, transaction);
    //
    //  b) add to the wallet
    walletHandleTransaction(wallet, transaction);
    //
    //  c) announce the wallet update
    ewmSignalTransactionEvent(ewm, wid, tid,
                                    TRANSACTION_EVENT_CREATED,
                                    SUCCESS, NULL);
    //
    //  d) announce as submitted (=> there is a hash, submitted by 'us' or 'them')
    walletTransactionSubmitted(wallet, transaction, bundle->hash);

}

if (-1 == tid)
tid = ewmLookupTransactionId(ewm, transaction);

BREthereumGas gasUsed = gasCreate(bundle->gasUsed);
// TODO: Process 'state' properly - errors?

// Get the current status.
BREthereumTransactionStatus status = transactionGetStatus(transaction);

// Update the status as blocked
if (TRANSACTION_STATUS_INCLUDED != status.type)
walletTransactionIncluded(wallet, transaction, gasUsed,
                          blockGetHash(block),
                          blockGetNumber(block),
                          bundle->blockTransactionIndex);

// Announce a transaction event.  If already 'BLOCKED', then update CONFIRMATIONS.
ewmSignalTransactionEvent(ewm, wid, tid,
                                (TRANSACTION_STATUS_INCLUDED == status.type
                                 ? TRANSACTION_EVENT_BLOCK_CONFIRMATIONS_UPDATED
                                 : TRANSACTION_EVENT_BLOCKED),
                                SUCCESS,
                                NULL);
#endif


#if 0 // Log
BREthereumTransactionId tid = -1;

pthread_mutex_lock(&ewm->lock);

// Token of interest
BREthereumToken token = tokenLookupByAddress(bundle->contract);
if (NULL == token) { pthread_mutex_unlock(&ewm->lock); return; } // uninteresting token

// Event of interest
BREthereumContractEvent event = contractLookupEventForTopic (contractERC20, bundle->arrayTopics[0]);
if (NULL == event || event != eventERC20Transfer) { pthread_mutex_unlock(&ewm->lock); return; }; // uninteresting event

BREthereumBlock block = NULL;
//    BREthereumBlock block = ewmLookupBlockByHash(ewm, bundle->blockHash);
//    block = blockCreateMinimal(bundle->blockHash, bundle->blockNumber, bundle->blockTimestamp);
//    ewmSignalBlockEvent(ewm, ewmInsertBlock(ewm, block),
//                                BLOCK_EVENT_CREATED,
//                                SUCCESS, NULL);

// Wallet for token
BREthereumWalletId wid = (NULL == token
                          ? ewmGetWallet(ewm)
                          : ewmGetWalletHoldingToken(ewm, token));
BREthereumWallet wallet = ewmLookupWallet(ewm, wid);

// Existing transaction
BREthereumTransaction transaction = walletGetTransactionByHash(wallet, bundle->hash);

BREthereumGasPrice gasPrice = gasPriceCreate(etherCreate(bundle->gasPrice));
BREthereumGas gasUsed = gasCreate(bundle->gasUsed);


// Create a token transaction
if (NULL == transaction) {

    // Parse the topic data - we fake it becasue we 'know' topics indices
    BREthereumAddress sourceAddr =
    addressCreate(eventERC20TransferDecodeAddress(event, bundle->arrayTopics[1]));

    BREthereumAddress targetAddr =
    addressCreate(eventERC20TransferDecodeAddress(event, bundle->arrayTopics[2]));

    BRCoreParseStatus status = CORE_PARSE_OK;

    BREthereumAmount amount =
    amountCreateToken(createTokenQuantity(token, eventERC20TransferDecodeUInt256(event,
                                                                                 bundle->data,
                                                                                 &status)));

    transaction = transactionCreate(sourceAddr, targetAddr, amount, gasPrice, gasUsed, 0);

    // With a new transaction:
    //
    //   a) add to the ewm
    tid = ewmInsertTransaction(ewm, transaction);
    //
    //  b) add to the wallet
    walletHandleTransaction(wallet, transaction);
    //
    //  c) announce the wallet update
    ewmSignalTransactionEvent(ewm, wid, tid, TRANSACTION_EVENT_CREATED, SUCCESS, NULL);

    //
    //  d) announce as submitted.
    walletTransactionSubmitted(wallet, transaction, bundle->hash);

}

if (-1 == tid)
tid = ewmLookupTransactionId(ewm, transaction);

// TODO: Process 'state' properly - errors?

// Get the current status.
BREthereumTransactionStatus status = transactionGetStatus(transaction);

// Update the status as blocked
if (TRANSACTION_STATUS_INCLUDED != status.type)
walletTransactionIncluded(wallet, transaction, gasUsed,
                          blockGetHash(block),
                          blockGetNumber(block),
                          bundle->blockTransactionIndex);

// Announce a transaction event.  If already 'BLOCKED', then update CONFIRMATIONS.
ewmSignalTransactionEvent(ewm, wid, tid,
                                (TRANSACTION_STATUS_INCLUDED == status.type
                                 ? TRANSACTION_EVENT_BLOCK_CONFIRMATIONS_UPDATED
                                 : TRANSACTION_EVENT_BLOCKED),
                                SUCCESS,
                                NULL);

// Hmmmm...
pthread_mutex_unlock(&ewm->lock);
#endif


#if 0 // token
static int
ewmDataIsEmpty (BREthereumEWM ewm, const char *data) {
    return NULL == data || 0 == strcmp ("", data) || 0 == strcmp ("0x", data);
}

// If `data` is anything besides "0x", then we have a contract function call.  At that point
// it seems we need to process `data` to extract the 'function + args' and then, if the
// function is 'transfer() token' we can then and only then conclude that we have a token

if (ewmDataIsEmpty(ewm, data)) return NULL;

// There is contract data; see if it is a ERC20 function.
BREthereumContractFunction function = contractLookupFunctionForEncoding(contractERC20, data);

// Not an ERC20 token
if (NULL == function) return NULL;

// See if we have an existing token.
BREthereumToken token = tokenLookup(target);
if (NULL == token) token = tokenLookup(contract);

// We found a token...
if (NULL != token) return token;

// ... we didn't find a token - we should create is dynamically.
fprintf (stderr, "Ignoring transaction for unknown ERC20 token at '%s'", target);
return NULL;
#endif



