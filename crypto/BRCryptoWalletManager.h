//
//  BRCryptoWalletManager.h
//  BRCore
//
//  Created by Ed Gamble on 3/19/19.
//  Copyright © 2019 breadwallet. All rights reserved.
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

#ifndef BRCryptoWalletManager_h
#define BRCryptoWalletManager_h

#include "BRCryptoNetwork.h"
#include "BRCryptoAccount.h"
#include "BRCryptoTransfer.h"
#include "BRCryptoWallet.h"

#include "ethereum/BREthereum.h"
#include "bitcoin/BRWalletManager.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct BRCryptoWalletManagerRecord *BRCryptoWalletManager;

    /// MARK: Listener

    typedef void *BRCryptoCWMListenerContext;

    // Add events

    typedef enum {
        CRYPTO_WALLET_MANAGER_EVENT_FOO
    } BRCryptoWalletManagerEventType;

    typedef struct {
        BRCryptoWalletManagerEventType type;
    } BRCryptoWalletManagerEvent;

    typedef void (*BRCryptoCWMListenerWalletManagerEvent) (BRCryptoCWMListenerContext context,
                                                           BRCryptoWalletManager manager,
                                                           BRCryptoWalletManagerEvent event);

    typedef enum {
        CRYPTO_WALLET_EVENT_FOO
    } BRCryptoWalletEventType;

    typedef struct {
        BRCryptoWalletEventType type;
    } BRCryptoWalletEvent;

    typedef void (*BRCryptoCWMListenerWalletEvent) (BRCryptoCWMListenerContext context,
                                                    BRCryptoWalletManager manager,
                                                    BRCryptoWallet wallet,
                                                    BRCryptoWalletEvent event);

    typedef enum {
        CRYPTO_TRANSFER_EVENT_FOO
    } BRCryptoTransferEventType;

    typedef struct {
        BRCryptoTransferEventType type;
    } BRCryptoTransferEvent;

    typedef void (*BRCryptoCWMListenerTransferEvent) (BRCryptoCWMListenerContext context,
                                                      BRCryptoWalletManager manager,
                                                      BRCryptoWallet wallet,
                                                      BRCryptoTransfer transfer,
                                                      BRCryptoTransferEvent event);

    typedef struct {
        BRCryptoCWMListenerContext context;
        BRCryptoCWMListenerWalletManagerEvent walletManagerEventCallback;
        BRCryptoCWMListenerWalletEvent walletEventCallback;
        BRCryptoCWMListenerTransferEvent transferEventCallback;
    } BRCryptoCWMListener;

    // MARK: Client

    typedef void *BRCryptoCWMClientContext;

    typedef struct {
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
    } BRCryptoCWMClientETH;

    typedef struct {
        BRGetBlockNumberCallback  funcGetBlockNumber;
        BRGetTransactionsCallback funcGetTransactions;
        BRSubmitTransactionCallback funcSubmitTransaction;
    } BRCryptoCWMClientBTC;

    typedef struct {
    } BRCryptoCWMClientGEN;

    typedef struct {
        BRCryptoCWMClientContext context;
        BRCryptoCWMClientBTC btc;
        BRCryptoCWMClientETH eth;
        BRCryptoCWMClientGEN gen;
    } BRCryptoCWMClient;


    extern BRCryptoWalletManager
    cryptoWalletManagerCreate (BRCryptoCWMListener listener,
                               BRCryptoCWMClient client,
                               BRCryptoAccount account,
                               BRCryptoNetwork network,
                               BRSyncMode mode,
                               const char *path);

    extern BRCryptoNetwork
    cryptoWalletManagerGetNetwork (BRCryptoWalletManager cwm);

    extern BRCryptoAccount
    cryptoWalletManagerGetAccount (BRCryptoWalletManager cwm);

    extern BRSyncMode
    cryptoWalletManagerGetMode (BRCryptoWalletManager cwm);

    extern const char *
    cryptoWalletManagerGetPath (BRCryptoWalletManager cwm);
    
    extern BRCryptoWallet
    cryptoWalletManagerGetWallet (BRCryptoWalletManager cwm);

    extern size_t
    cryptoWalletManagerGetWalletsCount (BRCryptoWalletManager cwm);

    extern BRCryptoWallet
    cryptoWalletManagerGetWalletAtIndex (BRCryptoWalletManager cwm,
                                            size_t index);

    extern BRCryptoWallet
    cryptoWalletManagerGetWalletForCurrency (BRCryptoWalletManager cwm,
                                               BRCryptoCurrency currency);

    extern void
    cryptoWalletManagerConnect (BRCryptoWalletManager cwm);

    extern void
    cryptoWalletManagerDisconnect (BRCryptoWalletManager cwm);

    extern void
    cryptoWalletManagerSync (BRCryptoWalletManager cwm);

    extern void
    cryptoWalletManagerSubmit (BRCryptoWalletManager cwm,
                               BRCryptoWallet wid,
                               BRCryptoTransfer tid,
                               const char *paperKey);
    

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoWalletManager_h */
