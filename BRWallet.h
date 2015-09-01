//
//  BRWallet.h
//  breadwallet-core
//
//  Created by Aaron Voisine on 9/1/15.
//  Copyright (c) 2015 breadwallet LLC
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

#ifndef BRWallet_h
#define BRWallet_h

#include "BRTypes.h"
#include "BRTransaction.h"
#include "BRBIP32Sequence.h"
#include <string.h>

typedef struct {
    char c[36];
} BRAddress;

static inline int BRAddressEq(BRAddress a, BRAddress b)
{
    return (strncmp(a.c, b.c, sizeof(BRAddress)) == 0);
}

typedef struct {
    UInt256 hash;
    unsigned n;
} BRUTXO;

static inline int BRUTXOEq(BRUTXO a, BRUTXO b)
{
    return (uint256_eq(a.hash, b.hash) && a.n == b.n);
}

typedef struct {
    unsigned long long balance; // current wallet balance excluding transactions known to be invalid
    BRUTXO *utxos; // unspent outputs
    unsigned long utxoCount;
    BRTransaction *transactions; // transactions sorted by date, most recent first
    unsigned long txCount;
    unsigned long long totalSent; // the total amount spent from the wallet (excluding change)
    unsigned long long totalReceived; // the total amount received by the wallet (excluding change)
    unsigned long long feePerKb; // fee per kb of transaction size to use when creating a transaction
    BRMasterPubKey masterPubKey;
    unsigned long long *balanceHistory;
    void *(*seed)(const char *authPrompt, unsigned long long amount, size_t *seedLen);
    void (*addTx)(BRTransaction *tx); // called when a transaction is registered to the wallet
    void (*updateTx)(UInt256 txHash, unsigned blockHeight, unsigned timestamp); // called when a transaction is updated
    void (*deleteTx)(UInt256 txHash); // called when a transaction is removed from the wallet
} BRWallet;

// XXX need to decide on allocation scheme

// returns the first unused external address
BRAddress BRWalletReceiveAddress(BRWallet *wallet);

// returns the first unused internal address
BRAddress BRWalletChangeAddress(BRWallet *wallet);

// true if the given txHash is registered in the wallet
int BRWalletContainsTxHash(BRWallet *wallet, UInt256 txHash);

// true if the address is controlled by the wallet
int BRWalletContainsAddress(BRWallet *wallet, BRAddress addr);

// true if the address was previously used as an input or output in any wallet transaction
int BRWalletAddressIsUsed(BRWallet *wallet, BRAddress addr);

// returns an unsigned transaction that sends the specified amount from the wallet to the given address
BRTransaction *BRWalletCreateTransaction(BRWallet *wallet, unsigned long long amount, BRAddress addr);

// sign any inputs in the given transaction that can be signed using private keys from the wallet
int BRWalletSignTransaction(BRWallet *wallet, BRTransaction *tx, const char *authPrompt);

// true if the given transaction is associated with the wallet (even if it hasn't been registered)
int BRWalletContainsTransaction(BRWallet *wallet, BRTransaction *tx);

// adds a transaction to the wallet, or returns false if it isn't associated with the wallet
int BRWalletRegisterTransaction(BRWallet *wallet, BRTransaction *tx);

// removes a transaction from the wallet along with any transactions that depend on its outputs
void BRWalletRemoveTransaction(BRWallet *wallet, UInt256 txHash);

// returns the transaction with the given hash if it's been registered in the wallet
BRTransaction *BRWalletTransactionForHash(BRWallet *wallet, UInt256 txHash);

// true if no previous wallet transaction spends any of the given transaction's inputs, and no input tx is invalid
int BRWalletTransactionIsValid(BRWallet *wallet, BRTransaction *tx);

// returns true if transaction won't be valid by blockHeight + 1 or within the next 10 minutes
int BRWalletTransactionIsPostdated(BRWallet *wallet, BRTransaction *tx, unsigned blockHeight);

// set the block height and timestamp for the given transaction
void BRWalletUpdateTransaction(BRWallet *wallet, UInt256 txHash, unsigned blockHeight, unsigned timestamp);

// returns the amount received by the wallet from the transaction (total outputs to change and/or receive addresses)
unsigned long long BRWalletAmountReceivedFromTx(BRWallet *wallet, BRTransaction *tx);

// retuns the amount sent from the wallet by the trasaction (total wallet outputs consumed, change and fee included)
unsigned long long BRWalletAmountSentByTx(BRWallet *wallet, BRTransaction *tx);

// returns the fee for the given transaction if all its inputs are from wallet transactions, ULLONG_MAX otherwise
unsigned long long BRWalletFeeForTx(BRWallet *wallet, BRTransaction *tx);

// historical wallet balance after the given transaction, or current balance if transaction is not registered in wallet
unsigned long long BRWalletBalanceAfterTx(BRWallet *wallet, BRTransaction *tx);

// fee that will be added for a transaction of the given size in bytes
unsigned long long BRWalletFeeForTxSize(BRWallet *wallet, size_t size);

#endif // BRWallet_h
