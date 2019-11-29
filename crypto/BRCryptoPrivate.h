//
//  BRCryptoP.h
//  BRCore
//
//  Created by Ed Gamble on 3/20/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRCryptoP_h
#define BRCryptoP_h

#include "BRCryptoStatus.h"
#include "BRCryptoAmount.h"
#include "BRCryptoKey.h"
#include "BRCryptoAccount.h"
#include "BRCryptoNetwork.h"
#include "BRCryptoTransfer.h"
#include "BRCryptoPayment.h"

#include "bitcoin/BRChainParams.h"
#include "bcash/BRBCashParams.h"
#include "ethereum/BREthereum.h"

#include "support/BRInt.h"
#include "support/BRArray.h"
#include "ethereum/base/BREthereumHash.h"
#include "generic/BRGeneric.h"

#ifdef __cplusplus
extern "C" {
#endif

/// MARK: - Hash

private_extern BRCryptoHash
cryptoHashCreateAsBTC (UInt256 btc);

private_extern BRCryptoHash
cryptoHashCreateAsETH (BREthereumHash eth);

private_extern BRCryptoHash
cryptoHashCreateAsGEN (BRGenericHash gen);

/// MARK: - Key

private_extern BRCryptoKey
cryptoKeyCreateFromKey (BRKey *key);

private_extern BRKey *
cryptoKeyGetCore (BRCryptoKey key);

/// MARK: - Currency

/// MARK: - Unit

private_extern BRArrayOf(BRCryptoUnit)
cryptoUnitTakeAll (BRArrayOf(BRCryptoUnit) units);

private_extern BRArrayOf(BRCryptoUnit)
cryptoUnitGiveAll (BRArrayOf(BRCryptoUnit) units);


/// MARK: - Amount

private_extern BRCryptoAmount
cryptoAmountCreate (BRCryptoUnit unit,
                    BRCryptoBoolean isNegative,
                    UInt256 value);

private_extern BRCryptoAmount
cryptoAmountCreateInternal (BRCryptoUnit unit,
                            BRCryptoBoolean isNegative,
                            UInt256 value,
                            int takeCurrency);

private_extern UInt256
cryptoAmountGetValue (BRCryptoAmount amount);

/// MARK: - Address

/// MARK: - Account

private_extern const char *
cryptoAccountAddressAsETH (BRCryptoAccount account);

/// MARK: FeeBasis

/// MARK: Transfer

/// MARK: - Network Fee

/// MARK: - Network

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
                          const char *name,
                          uint8_t isMainnet);

/// MARK: - Payment

private_extern BRArrayOf(BRTxOutput)
cryptoPaymentProtocolRequestGetOutputsAsBTC (BRCryptoPaymentProtocolRequest request);

/// MARK: - Status

private_extern BRCryptoStatus
cryptoStatusFromETH (BREthereumStatus status);

private_extern BREthereumStatus
cryptoStatusAsETH (BRCryptoStatus status);

/// MARK: - Wallet

/// MARK: - WalletManager

/// MARK: - WalletSweeper

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoP_h */
