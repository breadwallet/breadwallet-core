//
//  BBREthereumWallet.h
//  Core Ethereum
//
//  Created by Ed Gamble on 2/21/2018.
//  Copyright © 2018-2019 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_Ethereum_Wallet_H
#define BR_Ethereum_Wallet_H

#include "ethereum/blockchain/BREthereumBlockChain.h"
#include "BREthereumBase.h"
#include "BREthereumAccount.h"
#include "BREthereumTransfer.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create a wallet holding ETH; will use the account's primary address.
 *
 * @param account
 * @return
 */
extern BREthereumWallet
walletCreate(BREthereumAccount account,
             BREthereumNetwork network);

/**
 * Create a Wallet holding Token; will use the account's primary address.
 *
 * @param account
 * @param token
 * @return
 */
extern BREthereumWallet
walletCreateHoldingToken(BREthereumAccount account,
                         BREthereumNetwork network,
                         BREthereumToken token);

extern void
walletRelease (BREthereumWallet wallet);

extern void
walletsRelease (OwnershipGiven BRArrayOf(BREthereumWallet) wallets);

/**
 * Estimate the transfer fee (in Ether) for transferring amount.  The estimate uses
 * the wallet's default gasPrice and the amount's default gas.
 */
extern BREthereumEther
walletEstimateTransferFee (BREthereumWallet wallet,
                              BREthereumAmount amount,
                              int *overflow);

/**
 * Estimate the transfer fee (in Ether) for transferring amount.
 */
extern BREthereumEther
walletEstimateTransferFeeDetailed (BREthereumWallet wallet,
                                      BREthereumAmount amount,
                                      BREthereumGasPrice price,
                                      BREthereumGas gas,
                                      int *overflow);

/**
 *
 * @param wallet
 * @param recvAddress
 * @param amount
 * @return
 */
extern BREthereumTransfer
walletCreateTransfer(BREthereumWallet wallet,
                        BREthereumAddress recvAddress,
                        BREthereumAmount amount);

extern BREthereumTransfer
walletCreateTransferWithFeeBasis (BREthereumWallet wallet,
                                  BREthereumAddress recvAddress,
                                  BREthereumAmount amount,
                                  BREthereumFeeBasis feeBasis);

extern BREthereumTransfer
walletCreateTransferGeneric(BREthereumWallet wallet,
                            BREthereumAddress recvAddress,
                            BREthereumEther amount,
                            BREthereumGasPrice gasPrice,
                            BREthereumGas gasLimit,
                            const char *data);

extern void
walletSignTransfer(BREthereumWallet wallet,
                      BREthereumTransfer transfer,
                      const char *paperKey);

extern void
walletSignTransferWithPrivateKey(BREthereumWallet wallet,
                                    BREthereumTransfer transfer,
                                    BRKey privateKey);

#ifdef WALLET_NEED_RAW
/**
 * For `transfer`, get the 'signed transfer data' suitable for use in the RPC-JSON Ethereum
 * method `eth_sendRawTransfer` (once hex-encoded).
 *
 * @param wallet
 * @param transfer
 * @return
 */
extern BRRlpData  // uint8_t, EthereumByteArray
walletGetRawTransfer(BREthereumWallet wallet,
                        BREthereumTransfer transfer);

/**
 * For `transfer`, get the `signed transation data`, encoded in hex with an optional prefix
 * (typically "0x"), suitable for use in the RPC-JSON Ethereum method 'eth_sendRawTransfer"
 *
 */
extern char *
walletGetRawTransferHexEncoded (BREthereumWallet wallet,
                                   BREthereumTransfer transfer,
                                   const char *prefix);
#endif // WALLET_NEED_RAW
/**
 *
 */
extern BREthereumAddress
walletGetAddress(BREthereumWallet wallet);

/**
 * The wallet's amount type: ETHER or TOKEN
 */
extern BREthereumAmountType
walletGetAmountType (BREthereumWallet wallet);

/**
 * The wallet's token or NULL if the wallet holds ETHER.
 */
extern BREthereumToken
walletGetToken (BREthereumWallet wallet);

/**
 * The wallet's balance
 */
extern BREthereumAmount
walletGetBalance (BREthereumWallet wallet);

/**
 * Get the wallet's default Gas Limit.
 *
 * @param wallet
 * @return
 */
extern BREthereumGas
walletGetDefaultGasLimit(BREthereumWallet wallet);

/**
 * Set the wallet's default Gas Limit.  When creating a transfer, unless otherwise specified,
 * this GasLimit is used.  The default value depends on the wallet type: ETH, ERC20 Token or
 * Contract; therefore, set it carefully.
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

//
// Transfers
//
typedef int (*BREthereumTransferPredicate) (void *context, BREthereumTransfer transfer, unsigned int index);
typedef void (*BREthereumTransferWalker) (void *context, BREthereumTransfer transfer, unsigned int index);

extern int
transferPredicateAny (void *ignore,
                      BREthereumTransfer transfer,
                      unsigned int index);

extern int
transferPredicateStatus (BREthereumTransferStatus status,
                         BREthereumTransfer transfer,
                         unsigned int index);

extern void
walletWalkTransfers (BREthereumWallet wallet,
                     void *context,
                     BREthereumTransferPredicate predicate,
                     BREthereumTransferWalker walker);

extern BREthereumTransfer
walletGetTransferByIdentifier (BREthereumWallet wallet,
                               BREthereumHash hash);

extern BREthereumTransfer
walletGetTransferByOriginatingHash (BREthereumWallet wallet,
                                    BREthereumHash hash);

extern BREthereumTransfer
walletGetTransferByNonce(BREthereumWallet wallet,
                         BREthereumAddress sourceAddress,
                         uint64_t nonce);

extern BREthereumTransfer
walletGetTransferByIndex(BREthereumWallet wallet,
                         uint64_t index);

extern unsigned long
walletGetTransferCount (BREthereumWallet wallet);

//
// Private
// TODO: Make 'static'
//

private_extern void
walletSetBalance (BREthereumWallet wallet,
                  BREthereumAmount balance);

private_extern void
walletUpdateBalance (BREthereumWallet wallet);

private_extern void
walletTransferSubmitted (BREthereumWallet wallet,
                         BREthereumTransfer transaction,
                         const BREthereumHash hash); // ....

private_extern void
walletTransferIncluded(BREthereumWallet wallet,
                       BREthereumTransfer transaction,
                       BREthereumGas gasUsed,
                       BREthereumHash blockHash,
                       uint64_t blockNumber,
                       uint64_t blockTransactionIndex);

private_extern void
walletTransferErrored (BREthereumWallet wallet,
                       BREthereumTransfer transaction,
                       const char *reason);

private_extern void
walletHandleTransfer (BREthereumWallet wallet,
                      BREthereumTransfer transfer);

private_extern void
walletUnhandleTransfer (BREthereumWallet wallet,
                        BREthereumTransfer transaction);

private_extern int
walletHasTransfer (BREthereumWallet wallet,
                   BREthereumTransfer transaction);

/// MARK: - Persisted Wallet State;

typedef struct BREthereumWalletStateRecord *BREthereumWalletState;

extern BREthereumWalletState
walletStateCreate (const BREthereumWallet wallet);

extern void
walletStateRelease (BREthereumWalletState state);

/**
 * If WalletState holds Ether, then the address will be EMPTY_ADDRESS_INIT
 */
extern BREthereumAddress
walletStateGetAddress (const BREthereumWalletState walletState);

extern UInt256
walletStateGetAmount (const BREthereumWalletState walletState);

extern uint64_t
walletStateGetNonce (const BREthereumWalletState walletState);

extern void
walletStateSetNonce (BREthereumWalletState walletState,
                     uint64_t nonce);

extern BRRlpItem
walletStateEncode (const BREthereumWalletState walletState,
                   BRRlpCoder coder);

extern BREthereumWalletState
walletStateDecode (BRRlpItem item,
                   BRRlpCoder coder);

extern BREthereumHash
walletStateGetHash (const BREthereumWalletState walletState);

extern BRSetOf(BREthereumWalletState)
walletStateSetCreate (size_t capacity);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Wallet_H */
