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
#include <pthread.h>

struct _BRWallet {
    uint64_t balance; // current wallet balance excluding transactions known to be invalid
    BRUTXO *utxos; // unspent outputs
    BRTransaction **transactions; // transactions sorted by date, oldest first
    uint64_t totalSent; // total amount spent from the wallet (excluding change)
    uint64_t totalReceived; // total amount received by the wallet (excluding change)
    uint64_t feePerKb; // fee-per-kb of transaction size to use when creating a transaction
    BRMasterPubKey masterPubKey;
    BRAddress *internalChain;
    BRAddress *externalChain;
    uint64_t *balanceHist;
    BRSet *allTx;
    BRSet *invalidTx;
    BRSet *spentOutputs;
    BRSet *usedAddrs;
    BRSet *allAddrs;
    void *(*seed)(const char *authPrompt, uint64_t amount, size_t *seedLen); // called during tx signing
    void (*balanceChanged)(BRWallet *wallet, uint64_t balance, void *info);
    void (*txAdded)(BRWallet *wallet, BRTransaction *tx, void *info);
    void (*txUpdated)(BRWallet *wallet, UInt256 txHash, uint32_t blockHeight, uint32_t timestamp, void *info);
    void (*txDeleted)(BRWallet *wallet, UInt256 txHash, void *info);
    void *info;
    pthread_mutex_t lock;
};

// Wallets are composed of chains of addresses. Each chain is traversed until a gap of a certain number of addresses is
// found that haven't been used in any transactions. This function returns an array of <gapLimit> unused addresses
// following the last used address in the chain. The internal chain is used for change addresses and the external chain
// for receive addresses.
inline static BRAddress *BRWalletUnusedAddrs(BRWallet *wallet, uint32_t gapLimit, int internal)
{
    BRAddress *chain = (internal) ? wallet->internalChain : wallet->externalChain;
    uint32_t count = (uint32_t)array_count(chain), i = count;
    
    // keep only the trailing contiguous block of addresses with no transactions
    while (i > 0 && ! BRSetContains(wallet->usedAddrs, &chain[i - 1])) i--;
    if (count - i >= gapLimit) return &chain[i];
    
    if (i == 0 && gapLimit > 1) { // get receive address and change address first to avoid blocking
        BRWalletReceiveAddress(wallet);
        BRWalletChangeAddress(wallet);
    }
    
    pthread_mutex_lock(&wallet->lock);

    chain = (internal) ? wallet->internalChain : wallet->externalChain;
    i = count = (uint32_t)array_count(chain);

    // keep only the trailing contiguous block of addresses with no transactions
    while (i > 0 && ! BRSetContains(wallet->usedAddrs, &chain[i - 1])) i--;

    while (count - i < gapLimit) { // generate new addresses up to gapLimit
        BRKey key;
        BRAddress addr = BR_ADDRESS_NONE;

        BRKeySetPubKey(&key, BRBIP32PubKey(wallet->masterPubKey, internal, count));
        if (BRKeyAddress(&key, addr.s, sizeof(addr)) == 0) break;
        if (BRAddressEq(&addr, &BR_ADDRESS_NONE)) break;
        array_add(chain, addr);
        BRSetAdd(wallet->allAddrs, &chain[count]);
        count++;
    }
    
    pthread_mutex_unlock(&wallet->lock);
    return (count - i >= gapLimit) ? &chain[i] : NULL;
}

inline static void *BRWalletTxContext(BRTransaction *tx)
{
    // dirty dirty hack to sneak context data into qsort() and bsearch() comparator callbacks, since input scripts are
    // not used in already signed transactions, we can hijack the first script pointer for our own nafarious purposes
    return (tx->inputs[0].scriptLen == 0) ? tx->inputs[0].script : NULL;
}

inline static void BRWalletTxSetContext(BRTransaction *tx, void *info)
{
    if (tx->inputs[0].script && tx->inputs[0].script != info) free(tx->inputs[0].script);
    tx->inputs[0].script = info;
    tx->inputs[0].scriptLen = 0;
}

// chain position of first tx output address that appears in chain
inline static size_t BRWalletTxChainIdx(BRTransaction *tx, BRAddress *chain) {
    for (size_t i = 0; i < tx->outCount; i++) {
        for (size_t j = 0; j < array_count(chain); j++) {
            if (BRAddressEq(tx->outputs[i].address, &chain[j])) return j;
        }
    }
    
    return SIZE_MAX;
}

inline static int BRWalletTxIsAscending(BRWallet *wallet, BRTransaction *tx1, BRTransaction *tx2)
{
    if (! tx1 || ! tx2) return 0;
    if (tx1->blockHeight > tx2->blockHeight) return ! 0;
    if (tx1->blockHeight < tx2->blockHeight) return 0;
    
    for (size_t i = 0; i < tx1->inCount; i++) {
        if (UInt256Eq(tx1->inputs[i].txHash, tx2->txHash)) return ! 0;
    }
    
    for (size_t i = 0; i < tx2->inCount; i++) {
        if (UInt256Eq(tx2->inputs[i].txHash, tx1->txHash)) return 0;
    }

    for (size_t i = 0; i < tx1->inCount; i++) {
        if (BRWalletTxIsAscending(wallet, BRSetGet(wallet->allTx, &(tx1->inputs[i].txHash)), tx2)) return ! 0;
    }

    return 0;
}

inline static int BRWalletTxCompare(const void *tx1, const void *tx2)
{
    BRWallet *wallet = BRWalletTxContext(*(BRTransaction **)tx1);

    if (BRWalletTxIsAscending(wallet, *(BRTransaction **)tx1, *(BRTransaction **)tx2)) return 1;
    if (BRWalletTxIsAscending(wallet, *(BRTransaction **)tx2, *(BRTransaction **)tx1)) return -1;
    
    size_t i = BRWalletTxChainIdx(*(BRTransaction **)tx1, wallet->internalChain);
    size_t j = BRWalletTxChainIdx(*(BRTransaction **)tx2,
                                  (i == SIZE_MAX) ? wallet->externalChain : wallet->internalChain);

    if (i == SIZE_MAX && j != SIZE_MAX) i = BRWalletTxChainIdx(*(BRTransaction **)tx1, wallet->externalChain);
    if (i == SIZE_MAX || j == SIZE_MAX || i == j) return 0;
    return (i > j) ? 1 : -1;
}

inline static void BRWalletSortTransactions(BRWallet *wallet)
{
    qsort(wallet->transactions, array_count(wallet->transactions), sizeof(*(wallet->transactions)), BRWalletTxCompare);
}

static void BRWalletUpdateBalance(BRWallet *wallet)
{
}

// allocate and populate a wallet
BRWallet *BRWalletNew(BRTransaction *transactions[], size_t txCount, BRMasterPubKey mpk,
                      void *(*seed)(const char *, uint64_t, size_t *))
{
    BRWallet *wallet = calloc(1, sizeof(BRWallet));

    array_new(wallet->utxos, 100);
    array_new(wallet->transactions, txCount + 100);
    array_add_array(wallet->transactions, transactions, txCount);
    wallet->masterPubKey = mpk;
    array_new(wallet->balanceHist, txCount + 100);
    wallet->allTx = BRSetNew(BRTransactionHash, BRTransactionEq, txCount + 100);
    wallet->usedAddrs = BRSetNew(BRAddressHash, BRAddressEq, txCount*4 + 100);
    wallet->allAddrs = BRSetNew(BRAddressHash, BRAddressEq, txCount + 200 + 100);
    wallet->seed = seed;

    for (size_t i = 0; i < txCount; i++) {
        BRWalletTxSetContext(transactions[i], wallet);
        BRSetAdd(wallet->allTx, transactions[i]);
        
        for (size_t j = 0; j < transactions[i]->inCount; j++) {
            BRSetAdd(wallet->usedAddrs, transactions[i]->inputs[j].address);
        }
        
        for (size_t j = 0; j < transactions[i]->outCount; j++) {
            BRSetAdd(wallet->usedAddrs, transactions[i]->outputs[j].address);
        }
    }
    
    BRWalletSortTransactions(wallet);
    wallet->balance = UINT64_MAX; // trigger balanceChanged callback even if balance is zero
    BRWalletUpdateBalance(wallet);
    return wallet;
}

void BRWalletSetCallbacks(BRWallet *wallet,
                          void (*balanceChanged)(BRWallet *wallet, uint64_t balance, void *info),
                          void (*txAdded)(BRWallet *wallet, BRTransaction *tx, void *info),
                          void (*txUpdated)(BRWallet *wallet, UInt256 txHash, uint32_t blockHeight, uint32_t timestamp,
                                            void *info),
                          void (*txDeleted)(BRWallet *wallet, UInt256 txHash, void *info),
                          void *info)
{
    wallet->balanceChanged = balanceChanged;
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
inline BRUTXO *BRWalletUTXOs(BRWallet *wallet, size_t *count)
{
    *count = array_count(wallet->utxos);
    return wallet->utxos;
}

// all transactions registered in the wallet, sorted by date, most recent first
inline BRTransaction **BRWalletTransactions(BRWallet *wallet, size_t *count)
{
    *count = array_count(wallet->transactions);
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
    BRSetFree(wallet->allAddrs);
    BRSetFree(wallet->usedAddrs);
    BRSetFree(wallet->allTx);
    array_free(wallet->balanceHist);

    for (size_t i = 0; i < array_count(wallet->transactions); i++) {
        BRWalletTxSetContext(wallet->transactions[i], NULL);
        BRTransactionFree(wallet->transactions[i]);
    }

    array_free(wallet->transactions);
    array_free(wallet->utxos);
    free(wallet);
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
