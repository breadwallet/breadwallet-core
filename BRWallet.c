//
//  BRWallet.c
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

#include "BRWallet.h"
#include "BRSet.h"
#include "BRAddress.h"
#include <stdlib.h>

struct _BRWallet {
    uint64_t balance; // current wallet balance excluding transactions known to be invalid
    BRUTXO *utxos; // unspent outputs
    size_t utxoCount;
    const BRTransaction **transactions; // transactions sorted by date, most recent first
    size_t txCount;
    uint64_t totalSent; // total amount spent from the wallet (excluding change)
    uint64_t totalReceived; // total amount received by the wallet (excluding change)
    uint64_t feePerKb; // fee-per-kb of transaction size to use when creating a transaction
    BRMasterPubKey masterPubKey;
    uint64_t *balanceHistory;
    BRSet *allTx;
    BRSet *usedAddresses;
    BRSet *allAddresses;
    void *(*seed)(const char *authPrompt, uint64_t amount, size_t *seedLen); // called during tx signing
    void (*txAdded)(BRWallet *wallet, BRTransaction *tx, void *info);
    void (*txUpdated)(BRWallet *wallet, UInt256 txHash, uint32_t blockHeight, uint32_t timestamp, void *info);
    void (*txDeleted)(BRWallet *wallet, UInt256 txHash, void *info);
    void *info;
};

// allocate and populate a wallet
BRWallet *BRWalletNew(BRTransaction *transactions[], size_t txCount, BRMasterPubKey mpk,
                      void *(*seed)(const char *, uint64_t, size_t *))
{
    BRWallet *wallet = calloc(1, sizeof(BRWallet));
    uint8_t *arr = malloc(sizeof(size_t) + sizeof(BRTransaction *)*(txCount + 100));
    
    *(size_t *)arr = sizeof(size_t) + sizeof(BRTransaction *)*(txCount + 100);
    memcpy(arr + sizeof(size_t), transactions, sizeof(*transactions)*txCount);
    wallet->transactions = (const BRTransaction **)(arr + sizeof(size_t));
    wallet->txCount = txCount;
    wallet->masterPubKey = mpk;
    wallet->seed = seed;
    wallet->allTx = BRSetNew(BRTransactionHash, BRTransactionEq, txCount + 100);
    wallet->usedAddresses = BRSetNew(BRAddressHash, BRAddressEq, txCount*4 + 100);
    wallet->allAddresses = BRSetNew(BRAddressHash, BRAddressEq, txCount + 300);

    for (size_t i = 0; i < txCount; i++) {
        BRSetAdd(wallet->allTx, transactions[i]);
        
        for (size_t j = 0; j < transactions[i]->inCount; j++) {
            BRSetAdd(wallet->usedAddresses, transactions[i]->inputs[j].address);
        }
        
        for (size_t j = 0; j < transactions[i]->outCount; j++) {
            BRSetAdd(wallet->usedAddresses, transactions[i]->outputs[j].address);
        }
    }
    
    return wallet;
}

void BRWalletSetCallbacks(BRWallet *wallet,
                          void (*txAdded)(BRWallet *wallet, BRTransaction *tx, void *info),
                          void (*txUpdated)(BRWallet *wallet, UInt256 txHash, uint32_t blockHeight, uint32_t timestamp,
                                            void *info),
                          void (*txDeleted)(BRWallet *wallet, UInt256 txHash, void *info),
                          void *info)
{
    wallet->txAdded = txAdded;
    wallet->txUpdated = txUpdated;
    wallet->txDeleted = txDeleted;
    wallet->info = info;
}

// current wallet balance, not including transactions known to be invalid
inline uint64_t BRWalletBalance(BRWallet *wallet)
{
    return wallet->balance;
}

// list of all unspent outputs
inline const BRUTXO *BRWalletUTXOs(BRWallet *wallet, size_t *count)
{
    *count = wallet->utxoCount;
    return wallet->utxos;
}

// all transactions registered in the wallet, sorted by date, most recent first
inline const BRTransaction **BRWalletTransactions(BRWallet *wallet, size_t *count)
{
    *count = wallet->txCount;
    return wallet->transactions;
}

// total amount spent from the wallet (exluding change)
inline uint64_t BRWalletTotalSent(BRWallet *wallet)
{
    return wallet->totalSent;
}

// total amount received by the wallet (exluding change)
inline uint64_t BRWalletTotalReceived(BRWallet *wallet)
{
    return wallet->totalReceived;
}

// fee-per-kb of transaction size to use when creating a transaction
void BRWalletSetFeePerKb(BRWallet *wallet, uint64_t feePerKb)
{
    wallet->feePerKb = feePerKb;
}

// returns the first unused external address
const char *BRWalletReceiveAddress(BRWallet *wallet)
{
    return NULL;
}

// returns the first unused internal address
const char *BRWalletChangeAddress(BRWallet *wallet)
{
    return NULL;
}

// true if the given txHash is registered in the wallet
int BRWalletContainsTxHash(BRWallet *wallet, UInt256 txHash)
{
    return 0;
}

// true if the address is controlled by the wallet
int BRWalletContainsAddress(BRWallet *wallet, const char *addr)
{
    return 0;
}

// true if the address was previously used as an input or output in any wallet transaction
int BRWalletAddressIsUsed(BRWallet *wallet, const char *addr)
{
    return 0;
}

// returns an unsigned transaction that sends the specified amount from the wallet to the given address, result must be
// freed using BRTransactionFree()
BRTransaction *BRWalletCreateTransaction(BRWallet *wallet, uint64_t amount, const char *addr)
{
    return NULL;
}

// sign any inputs in the given transaction that can be signed using private keys from the wallet
int BRWalletSignTransaction(BRWallet *wallet, BRTransaction *tx, const char *authPrompt)
{
    return 0;
}

// true if the given transaction is associated with the wallet (even if it hasn't been registered)
int BRWalletContainsTransaction(BRWallet *wallet, BRTransaction *tx)
{
    return 0;
}

// adds a transaction to the wallet, or returns false if it isn't associated with the wallet
int BRWalletRegisterTransaction(BRWallet *wallet, BRTransaction *tx)
{
    return 0;
}

// removes a transaction from the wallet along with any transactions that depend on its outputs
void BRWalletRemoveTransaction(BRWallet *wallet, UInt256 txHash)
{
}

// returns the transaction with the given hash if it's been registered in the wallet
const BRTransaction *BRWalletTransactionForHash(BRWallet *wallet, UInt256 txHash)
{
    return NULL;
}

// true if no previous wallet transaction spends any of the given transaction's inputs, and no input tx is invalid
int BRWalletTransactionIsValid(BRWallet *wallet, BRTransaction *tx)
{
    return 0;
}

// returns true if transaction won't be valid by blockHeight + 1 or within the next 10 minutes
int BRWalletTransactionIsPostdated(BRWallet *wallet, BRTransaction *tx, uint32_t blockHeight)
{
    return 0;
}

// set the block height and timestamp for the given transaction
void BRWalletUpdateTransaction(BRWallet *wallet, UInt256 txHash, uint32_t blockHeight, uint32_t timestamp)
{
}

// returns the amount received by the wallet from the transaction (total outputs to change and/or receive addresses)
uint64_t BRWalletAmountReceivedFromTx(BRWallet *wallet, BRTransaction *tx)
{
    return 0;
}

// retuns the amount sent from the wallet by the trasaction (total wallet outputs consumed, change and fee included)
uint64_t BRWalletAmountSentByTx(BRWallet *wallet, BRTransaction *tx)
{
    return 0;
}

// returns the fee for the given transaction if all its inputs are from wallet transactions, ULLONG_MAX otherwise
uint64_t BRWalletFeeForTx(BRWallet *wallet, BRTransaction *tx)
{
    return 0;
}

// historical wallet balance after the given transaction, or current balance if transaction is not registered in wallet
uint64_t BRWalletBalanceAfterTx(BRWallet *wallet, BRTransaction *tx)
{
    return 0;
}

// fee that will be added for a transaction of the given size in bytes
uint64_t BRWalletFeeForTxSize(BRWallet *wallet, size_t size)
{
    return 0;
}

// frees memory allocated for wallet, also calls BRTransactionFree() for all registered transactions
void BRWalletFree(BRWallet *wallet)
{
}

// returns the given amount in local currency units, price is local currency units per bitcoin
uint64_t BRLocalAmount(uint64_t amount, double price)
{
    return 0;
}

// returns the given local currency amount in satoshis, price is local currency units per bitcoin
uint64_t BRBitcoinAmount(uint64_t localAmount, double price)
{
    return 0;
}
