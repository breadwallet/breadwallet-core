//
//  BREthereumLightNodeUpdate.c
//  Core
//
//  Created by Ed Gamble on 5/22/18.
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

#include <stdlib.h>
#include <assert.h>
#include "BREthereumPrivate.h"
#include "BREthereumLightNodePrivate.h"

//
// Updates
//
#if defined(SUPPORT_JSON_RPC)

/**
 *
 * @param node
 */
extern void
lightNodeUpdateTransactions (BREthereumLightNode node) {
    if (LIGHT_NODE_CONNECTED != node->state) {
        // Nothing to announce
        return;
    }
    switch (node->type) {
        case NODE_TYPE_LES:
            // TODO: Fall-through on error, perhaps

        case NODE_TYPE_JSON_RPC: {
            char *address = addressAsString(accountGetPrimaryAddress(node->account));

            node->client.funcGetTransactions
            (node->client.funcContext,
             node,
             address,
             ++node->requestId);

            free (address);
            break;
        }

        case NODE_TYPE_NONE:
            break;
    }
}

//
// Logs
//

/**
 *
 * @param node
 */
static const char *
lightNodeGetWalletContractAddress (BREthereumLightNode node, BREthereumWalletId wid) {
    BREthereumWallet wallet = lightNodeLookupWallet(node, wid);
    if (NULL == wallet) return NULL;

    BREthereumToken token = walletGetToken(wallet);
    return (NULL == token ? NULL : tokenGetAddress(token));
}

extern void
lightNodeUpdateLogs (BREthereumLightNode node,
                     BREthereumWalletId wid,
                     BREthereumContractEvent event) {
    if (LIGHT_NODE_CONNECTED != node->state) {
        // Nothing to announce
        return;
    }
    switch (node->type) {
        case NODE_TYPE_LES:
            // TODO: Fall-through on error, perhaps

        case NODE_TYPE_JSON_RPC: {
            char *address = addressAsString(accountGetPrimaryAddress(node->account));
            char *encodedAddress =
            eventERC20TransferEncodeAddress (event, address);
            const char *contract =lightNodeGetWalletContractAddress(node, wid);

            node->client.funcGetLogs
            (node->client.funcContext,
             node,
             contract,
             encodedAddress,
             eventGetSelector(event),
             ++node->requestId);

            free (encodedAddress);
            free (address);
            break;
        }

        case NODE_TYPE_NONE:
            break;
    }
}

extern void
lightNodeUpdateWalletBalance(BREthereumLightNode node,
                             BREthereumWalletId wid) {
    BREthereumWallet wallet = lightNodeLookupWallet(node, wid);

    if (NULL == wallet) {
        lightNodeListenerAnnounceWalletEvent(node, wid, WALLET_EVENT_BALANCE_UPDATED,
                                             ERROR_UNKNOWN_WALLET,
                                             NULL);

    } else if (LIGHT_NODE_CONNECTED != node->state) {
        lightNodeListenerAnnounceWalletEvent(node, wid, WALLET_EVENT_BALANCE_UPDATED,
                                             ERROR_NODE_NOT_CONNECTED,
                                             NULL);
    } else {
        switch (node->type) {
            case NODE_TYPE_LES:
            case NODE_TYPE_JSON_RPC: {
                char *address = addressAsString(walletGetAddress(wallet));

                node->client.funcGetBalance
                (node->client.funcContext,
                 node,
                 wid,
                 address,
                 ++node->requestId);

                free(address);
                break;
            }

            case NODE_TYPE_NONE:
                break;
        }
    }
}

extern void
lightNodeUpdateTransactionGasEstimate (BREthereumLightNode node,
                                       BREthereumWalletId wid,
                                       BREthereumTransactionId tid) {
    BREthereumTransaction transaction = lightNodeLookupTransaction(node, tid);

    if (NULL == transaction) {
        lightNodeListenerAnnounceTransactionEvent(node, wid, tid,
                                                  TRANSACTION_EVENT_GAS_ESTIMATE_UPDATED,
                                                  ERROR_UNKNOWN_WALLET,
                                                  NULL);

    } else if (LIGHT_NODE_CONNECTED != node->state) {
        lightNodeListenerAnnounceTransactionEvent(node, wid, tid,
                                                  TRANSACTION_EVENT_GAS_ESTIMATE_UPDATED,
                                                  ERROR_NODE_NOT_CONNECTED,
                                                  NULL);
    } else {
        switch (node->type) {
            case NODE_TYPE_LES:
            case NODE_TYPE_JSON_RPC: {
                // This will be ZERO if transaction amount is in TOKEN.
                BREthereumEther amountInEther = transactionGetEffectiveAmountInEther(transaction);
                char *to = (char *) addressAsString(transactionGetTargetAddress(transaction));
                char *amount = coerceString(amountInEther.valueInWEI, 16);
                char *data = (char *) transactionGetData(transaction);

                node->client.funcEstimateGas
                (node->client.funcContext,
                 node,
                 wid,
                 tid,
                 to,
                 amount,
                 data,
                 ++node->requestId);

                free(to);
                free(amount);

                if (NULL != data && '\0' != data[0])
                    free(data);

                break;
            }
                assert (0);

            case NODE_TYPE_NONE:
                break;
        }
    }
}

extern void
lightNodeUpdateWalletDefaultGasPrice (BREthereumLightNode node,
                                      BREthereumWalletId wid) {
    BREthereumWallet wallet = lightNodeLookupWallet(node, wid);

    if (NULL == wallet) {
        lightNodeListenerAnnounceWalletEvent(node, wid, WALLET_EVENT_DEFAULT_GAS_PRICE_UPDATED,
                                             ERROR_UNKNOWN_WALLET,
                                             NULL);

    } else if (LIGHT_NODE_CONNECTED != node->state) {
        lightNodeListenerAnnounceWalletEvent(node, wid, WALLET_EVENT_DEFAULT_GAS_PRICE_UPDATED,
                                             ERROR_NODE_NOT_CONNECTED,
                                             NULL);
    } else {
        switch (node->type) {
            case NODE_TYPE_LES:
            case NODE_TYPE_JSON_RPC: {
                node->client.funcGetGasPrice
                (node->client.funcContext,
                 node,
                 wid,
                 ++node->requestId);
                break;
            }

            case NODE_TYPE_NONE:
                break;
        }
    }
}
#endif // ETHEREUM_LIGHT_NODE_USE_JSON_RPC

