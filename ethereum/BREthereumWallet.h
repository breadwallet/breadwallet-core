//
//  BBREthereumWallet.h
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

#ifndef BR_Ethereum_Wallet_H
#define BR_Ethereum_Wallet_H

#include "BREthereumEther.h"
#include "BREthereumGas.h"
#include "BREthereumAccount.h"
#include "BREthereumTransaction.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * An EthereumWallet holds ETH or ERC20 Tokens.
 *
 * GetEtherPerHolding
 * SetEtherPerHolding
 * GetHoldingValueInEther
 *
 * GetLocalCurrentyPerHolding
 * SetLocalCurrencyPerHolding
 * GetHoldingValueInLocalCurrency
 */
typedef struct BREthereumWalletRecord *BREthereumWallet;

/**
 *
 * @param account
 * @return
 */
extern BREthereumWallet
walletCreate(BREthereumAccount account);

/**
 *
 * @param account
 * @param address
 * @return
 */
extern BREthereumWallet
walletCreateWithAddress(BREthereumAccount account,
                        BREthereumAddress address);

/**
 * Create a Wallet holding Token.
 *
 * @param account
 * @param token
 * @return
 */
extern BREthereumWallet
walletCreateHoldingToken(BREthereumAccount account,
                         BREthereumAddress address,
                         BREthereumToken token);

/**
 *
 * @param wallet
 * @param recvAddress
 * @param amount
 * @return
 */
extern BREthereumTransaction
walletCreateTransaction(BREthereumWallet wallet,
                        BREthereumAddress recvAddress,
                        BREthereumEther amount);

/**
 *
 * @param wallet
 * @param recvAddress
 * @param amount
 * @param gasPrice
 * @param gasLimit
 * @param nonce
 * @return
 */
extern BREthereumTransaction
walletCreateTransactionDetailed(BREthereumWallet wallet,
                                BREthereumAddress recvAddress,
                                BREthereumEther amount,
                                BREthereumGasPrice gasPrice,
                                BREthereumGas gasLimit,
                                int nonce);

extern void
walletSignTransaction(BREthereumWallet wallet,
                      BREthereumTransaction transaction,
                      const char *paperKey);

/**
 * For `transaction`, get the 'signed transaction data' suitable for use in the RPC-JSON Ethereum
 * method `eth_sendRawTransaction`.
 *
 * @param wallet
 * @param transaction
 * @return
 */
extern BRRlpData  // uint8_t, EthereumByteArray
walletGetRawTransaction(BREthereumWallet wallet,
                        BREthereumTransaction transaction);

/**
 * Get the wallet's default Gas Limit.
 *
 * @param wallet
 * @return
 */
extern BREthereumGas
walletGetDefaultGasLimit(BREthereumWallet wallet);

/**
 * Set the wallet's default Gas Limit.  When creating a transaction, unless otherwise specified,
 * this GasLimit is used.  The default value depends on the wallet type: ETH, ERC20 Token or
 * Contract.
 *
 * @param wallet
 * @param gasLimit
 */
extern void
walletSetDefaultGasLimit(BREthereumWallet wallet, BREthereumGas gasLimit);

/**
 * Gets the wallet's default Gas Price.
 *
 * @param wallet
 * @return
 */
extern BREthereumGasPrice
walletGetDefaultGasPrice(BREthereumWallet wallet);

/**
 * Sets the wallets' default Gas Price.
 *
 * @param wallet
 * @param gasPrice
 */
extern void
walletSetDefaultGasPrice(BREthereumWallet wallet, BREthereumGasPrice gasPrice);

/**
 *
 * [Used for walletCreateTransactionDetailed()]
 *
 * @param wallet
 * @return
 */
extern int
walletGetNonce(BREthereumWallet wallet);

/**
 *
 * [Used for walletCreateTransactionDetailed()]
 *
 * @param wallet
 * @return
 */
extern int
walletIncrementNonce(BREthereumWallet wallet);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Wallet_H */
