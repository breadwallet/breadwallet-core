//
//  BRCryptoPrivate.h
//  BRCore
//
//  Created by Ed Gamble on 3/20/19.
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

#ifndef BRCryptoPrivate_h
#define BRCryptoPrivate_h

/// A 'private header' - to define interfaces that are implementation dependent and that require
/// including implementation specific headers.

#include <inttypes.h>

#include "BRCryptoHash.h"
#include "BRCryptoAccount.h"
#include "BRCryptoAmount.h"
#include "BRCryptoAddress.h"
#include "BRCryptoFeeBasis.h"
#include "BRCryptoTransfer.h"
#include "BRCryptoNetwork.h"
#include "BRCryptoWallet.h"
#include "BRCryptoWalletManager.h"

///
#include "support/BRAddress.h"
#include "support/BRBIP32Sequence.h"
#include "bitcoin/BRChainParams.h"
#include "bitcoin/BRTransaction.h"
#include "bitcoin/BRWallet.h"
#include "bitcoin/BRWalletManager.h"
#include "bcash/BRBCashParams.h"
#include "ethereum/BREthereum.h"


#ifdef __cplusplus
extern "C" {
#endif

    /// MARK: - Hash

    private_extern BRCryptoHash
    cryptoHashCreateAsBTC (UInt256 btc);

    private_extern BRCryptoHash
    cryptoHashCreateAsETH (BREthereumHash eth);

    /// MARK: - Currency

    /**
     * Create a currency
     *
     * @param uids the 'unique identifier string'.  This will be globally unique
     * @param name the name, such as "The Breadwallet Token"
     * @param code the code, such as "BRD"
     * @param type the type, such a 'erc20'
     * @param issuer the issuer or NULL.  For currency derived from an ERC20 token, the issue must
     *    be a 'hex string' (starts with '0x') representing the Smart Contract Address.
     *
     * @return a currency
     */
    private_extern BRCryptoCurrency
    cryptoCurrencyCreate (const char *uids,
                          const char *name,
                          const char *code,
                          const char *type,
                          const char *issuer);


    /// MARK: - Unit

    private_extern BRCryptoUnit
    cryptoUnitCreateAsBase (BRCryptoCurrency currency,
                            const char *uids,
                            const char *name,
                            const char *symbol);

    private_extern BRCryptoUnit
    cryptoUnitCreate (BRCryptoCurrency currency,
                      const char *uids,
                      const char *name,
                      const char *symbol,
                      BRCryptoUnit baseUnit,
                      uint8_t powerOffset);

    private_extern BRArrayOf(BRCryptoUnit)
    cryptoUnitTakeAll (BRArrayOf(BRCryptoUnit) units);

    private_extern BRArrayOf(BRCryptoUnit)
    cryptoUnitGiveAll (BRArrayOf(BRCryptoUnit) units);

    /// MARK: - Amount

    private_extern BRCryptoAmount
    cryptoAmountCreate (BRCryptoCurrency currency,
                        BRCryptoBoolean isNegative,
                        UInt256 value);

    private_extern BRCryptoAmount
    cryptoAmountCreateInternal (BRCryptoCurrency currency,
                                BRCryptoBoolean isNegative,
                                UInt256 value,
                                int takeCurrency);
    
    /// MARK: - Address

    private_extern BRCryptoAddress
    cryptoAddressCreateAsETH (BREthereumAddress eth);

    private_extern BRCryptoAddress
    cryptoAddressCreateAsBTC (BRAddress btc);

    /// MARK: - Account

    private_extern BREthereumAccount
    cryptoAccountAsETH (BRCryptoAccount account);

    private_extern const char *
    cryptoAccountAddressAsETH (BRCryptoAccount account);

    private_extern BRMasterPubKey
    cryptoAccountAsBTC (BRCryptoAccount account);

    /// MARK: FeeBasis
    
    private_extern uint64_t
    cryptoFeeBasisAsBTC (BRCryptoFeeBasis feeBasis);

    private_extern BREthereumFeeBasis
    cryptoFeeBasisAsETH (BRCryptoFeeBasis feeBasis);

    private_extern BRCryptoFeeBasis
    cryptoFeeBasisCreateAsBTC (uint64_t feePerKB);

    private_extern BRCryptoFeeBasis
    cryptoFeeBasisCreateAsETH (BREthereumGas gas,
                               BREthereumGasPrice gasPrice);
    
    /// MARK: Transfer

    private_extern void
    cryptoTransferSetState (BRCryptoTransfer transfer,
                            BRCryptoTransferState state);
    
    private_extern BRCryptoTransfer
    cryptoTransferCreateAsBTC (BRCryptoCurrency currency,
                               BRWallet *wid,
                               BRTransaction *tid);

    private_extern BRCryptoTransfer
    cryptoTransferCreateAsETH (BRCryptoCurrency currency,
                               BREthereumEWM ewm,
                               BREthereumTransfer tid);

    private_extern BRTransaction *
    cryptoTransferAsBTC (BRCryptoTransfer transfer);

    private_extern BREthereumTransfer
    cryptoTransferAsETH (BRCryptoTransfer transfer);

    private_extern BRCryptoBoolean
    cryptoTransferHasBTC (BRCryptoTransfer transfer,
                          BRTransaction *btc);

    private_extern BRCryptoBoolean
    cryptoTransferHasETH (BRCryptoTransfer transfer,
                          BREthereumTransfer eth);

    /// MARK: - Network

    private_extern void
    cryptoNetworkAnnounce (BRCryptoNetwork network);

    private_extern void
    cryptoNetworkSetHeight (BRCryptoNetwork network,
                            BRCryptoBlockChainHeight height);

    private_extern void
    cryptoNetworkSetCurrency (BRCryptoNetwork network,
                              BRCryptoCurrency currency);

    private_extern void
    cryptoNetworkAddCurrency (BRCryptoNetwork network,
                              BRCryptoCurrency currency,
                              BRCryptoUnit baseUnit,
                              BRCryptoUnit defaultUnit);

    private_extern void
    cryptoNetworkAddCurrencyUnit (BRCryptoNetwork network,
                                  BRCryptoCurrency currency,
                                  BRCryptoUnit unit);

    private_extern BREthereumNetwork
    cryptoNetworkAsETH (BRCryptoNetwork network);

    private_extern const BRChainParams *
    cryptoNetworkAsBTC (BRCryptoNetwork network);

    private_extern BRCryptoNetwork
    cryptoNetworkCreateAsBTC (const char *uids,
                              const char *name,
                              uint8_t forkId,
                              const BRChainParams *params);

    private_extern BRCryptoNetwork
    cryptoNetworkCreateAsETH (const char *uids,
                              const char *name,
                              uint32_t chainId,
                              BREthereumNetwork net);

    private_extern BRCryptoNetwork
    cryptoNetworkCreateAsGEN (const char *uids,
                              const char *name);
    
    private_extern BRCryptoBlockChainType
    cryptoNetworkGetBlockChainType (BRCryptoNetwork network);

    private_extern BRCryptoCurrency
    cryptoNetworkGetCurrencyforTokenETH (BRCryptoNetwork network,
                                         BREthereumToken token);

    /// MARK: - Wallet

    private_extern BRCryptoBlockChainType
    cryptoWalletGetType (BRCryptoWallet wallet);

    private_extern void
    cryptoWalletSetState (BRCryptoWallet wallet,
                          BRCryptoWalletState state);

    private_extern BRWallet *
    cryptoWalletAsBTC (BRCryptoWallet wallet);

    private_extern BREthereumWallet
    cryptoWalletAsETH (BRCryptoWallet wallet);

    private_extern BRCryptoWallet
    cryptoWalletCreateAsBTC (BRCryptoUnit unit,
                             BRCryptoUnit unitForFee,
                             BRWalletManager bwm,
                             BRWallet *wid);

    private_extern BRCryptoWallet
    cryptoWalletCreateAsETH (BRCryptoUnit unit,
                             BRCryptoUnit unitForFee,
                             BREthereumEWM ewm,
                             BREthereumWallet wid);

    private_extern BRCryptoTransfer
    cryptoWalletFindTransferAsBTC (BRCryptoWallet wallet,
                                   BRTransaction *btc);

    private_extern BRCryptoTransfer
    cryptoWalletFindTransferAsETH (BRCryptoWallet wallet,
                                   BREthereumTransfer eth);

    private_extern void
    cryptoWalletAddTransfer (BRCryptoWallet wallet, BRCryptoTransfer transfer);

    private_extern void
    cryptoWalletRemTransfer (BRCryptoWallet wallet, BRCryptoTransfer transfer);
    
    /// MARK: - WalletManager

    private_extern void
    cryptoWalletManagerSetState (BRCryptoWalletManager cwm,
                                 BRCryptoWalletManagerState state);
    
    private_extern BRCryptoBoolean
    cryptoWalletManagerHasETH (BRCryptoWalletManager manager,
                               BREthereumEWM ewm);

    private_extern BRCryptoBoolean
    cryptoWalletManagerHasBTC (BRCryptoWalletManager manager,
                               BRWalletManager bwm);

    private_extern BRCryptoWallet
    cryptoWalletManagerFindWalletAsBTC (BRCryptoWalletManager manager,
                                        BRWallet *btc);

    private_extern BRCryptoWallet
    cryptoWalletManagerFindWalletAsETH (BRCryptoWalletManager manager,
                                        BREthereumWallet eth);

    private_extern void
    cryptoWalletManagerAddWallet (BRCryptoWalletManager cwm,
                                  BRCryptoWallet wallet);

    private_extern void
    cryptoWalletManagerRemWallet (BRCryptoWalletManager cwm,
                                  BRCryptoWallet wallet);
    
#ifdef __cplusplus
}
#endif


#endif /* BRCryptoPrivate_h */
