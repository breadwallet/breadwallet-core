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
        uint8_t pubKey[BRBIP32PubKey(NULL, 0, wallet->masterPubKey, internal, count)];
        size_t len = BRBIP32PubKey(pubKey, sizeof(pubKey), wallet->masterPubKey, internal, count);

        BRKeySetPubKey(&key, pubKey, len);
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
    // not used in already signed transactions, we hijack the first script pointer for our own nafarious purposes
    return (tx->inputs[0].scriptLen == 0) ? tx->inputs[0].script : NULL;
}

inline static void BRWalletTxSetContext(BRTransaction *tx, void *info)
{
    if (tx->inputs[0].scriptLen > 0) array_free(tx->inputs[0].script);
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
    uint64_t balance = 0, prevBalance = 0;
    BRSet *spent = BRSetNew(BRUTXOHash, BRUTXOEq, array_count(wallet->utxos));
    
    array_clear(wallet->utxos);
    array_clear(wallet->balanceHist);
    BRSetClear(wallet->spentOutputs);
    BRSetClear(wallet->invalidTx);
    wallet->totalSent = 0;
    wallet->totalReceived = 0;

    for (size_t i = 0; i < array_count(wallet->transactions); i++) {
        BRSetClear(spent);
        
        for (size_t j = 0; j < wallet->transactions[i]->inCount; j++) {
            BRSetAdd(spent, &wallet->transactions[i]->inputs[j]);
        }

        // check if any inputs are invalid or already spent
        if (wallet->transactions[i]->blockHeight == TX_UNCONFIRMED &&
            (BRSetIntersects(spent, wallet->spentOutputs) || BRSetIntersects(spent, wallet->invalidTx))) {
            BRSetAdd(wallet->invalidTx, wallet->transactions[i]);
            continue;
        }
        
        BRSetUnion(wallet->spentOutputs, spent);

        //TODO: don't add outputs below TX_MIN_OUTPUT_AMOUNT
        //TODO: don't add coin generation outputs < 100 blocks deep, or non-final lockTime > 1 block/10min in future
        //NOTE: balance/UTXOs will then need to be recalculated when last block changes
        for (size_t j = 0; j < wallet->transactions[i]->outCount; j++) {
            if (BRSetContains(wallet->allAddrs, wallet->transactions[i]->outputs[j].address)) {
                array_add(wallet->utxos, ((BRUTXO) { wallet->transactions[i]->txHash, (uint32_t)j }));
                balance += wallet->transactions[i]->outputs[j].amount;
            }
        }

        // transaction ordering is not guaranteed, so check the entire UTXO set against the entire spent output set
        BRSetClear(spent);
        for (size_t j = 0; j < array_count(wallet->utxos); j++) BRSetAdd(spent, &wallet->utxos[j]);
        BRSetIntersect(spent, wallet->spentOutputs);

        for (size_t j = 0; j < array_count(wallet->utxos);) { // remove any spent outputs from UTXO set
            if (BRSetContains(spent, &wallet->utxos[j])) {
                BRTransaction *tx = BRSetGet(wallet->allTx, &wallet->utxos[j].hash);

                balance -= tx->outputs[wallet->utxos[j].n].amount;
                array_rm(wallet->utxos, j);
            }
            else j++;
        }

        if (prevBalance < balance) wallet->totalReceived += balance - prevBalance;
        if (balance < prevBalance) wallet->totalSent += prevBalance - balance;
        array_add(wallet->balanceHist, balance);
        prevBalance = balance;
    }

    if (balance != wallet->balance) {
        wallet->balance = balance;
        if (wallet->balanceChanged) wallet->balanceChanged(wallet, balance, wallet->info);
    }
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
        BRTransaction *tx = transactions[i];

        BRWalletTxSetContext(tx, wallet);
        BRSetAdd(wallet->allTx, tx);
        for (size_t j = 0; j < tx->inCount; j++) BRSetAdd(wallet->usedAddrs, tx->inputs[j].address);
        for (size_t j = 0; j < tx->outCount; j++) BRSetAdd(wallet->usedAddrs, tx->outputs[j].address);
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
    return BRWalletUnusedAddrs(wallet, 1, 0)->s;
}

// returns the first unused internal address
const char *BRWalletChangeAddress(BRWallet *wallet)
{
    return BRWalletUnusedAddrs(wallet, 1, 1)->s;
}

// true if the given txHash is registered in the wallet
int BRWalletContainsTxHash(BRWallet *wallet, UInt256 txHash)
{
    return BRSetContains(wallet->allTx, &txHash);
}

// true if the address is controlled by the wallet
int BRWalletContainsAddress(BRWallet *wallet, const char *addr)
{
    return BRSetContains(wallet->allAddrs, addr);
}

// true if the address was previously used as an input or output in any wallet transaction
int BRWalletAddressIsUsed(BRWallet *wallet, const char *addr)
{
    return BRSetContains(wallet->usedAddrs, addr);
}

// returns an unsigned transaction that sends the specified amount from the wallet to the given address, result must be
// freed using BRTransactionFree()
BRTransaction *BRWalletCreateTransaction(BRWallet *wallet, uint64_t amount, const char *addr)
{
    BRTxOutput o;
    
    o.amount = amount;
    BRTxOutputSetAddress(&o, addr);
    return BRWalletCreateTxForOutputs(wallet, &o, 1);
}

// returns an unsigned transaction that satisifes the given transaction outputs, result must be freed using
// BRTransactionFree()
BRTransaction *BRWalletCreateTxForOutputs(BRWallet *wallet, BRTxOutput *outputs, size_t count)
{
    uint64_t amount = 0, balance = 0, feeAmount = 0;
    BRTransaction *tx, *transaction = BRTransactionNew();
    size_t i, cpfpSize = 0;
    BRUTXO *o;
    
    for (i = 0; i < count; i++) {
        BRTransactionAddOutput(transaction, outputs[i].amount, outputs[i].script, outputs[i].scriptLen);
        amount += outputs[i].amount;
    }
    
    //TODO: make sure transaction is less than TX_MAX_SIZE
    //TODO: use up all UTXOs for all used addresses to avoid leaving funds in addresses whose public key is revealed
    //TODO: avoid combining addresses in a single transaction when possible to reduce information leakage
    //TODO: use any UTXOs received from output addresses to mitigate an attacker double spending and requesting a refund
    for (i = 0; i < array_count(wallet->utxos); i++) {
        o = &wallet->utxos[i];
        tx = BRSetGet(wallet->allTx, o);
        if (! tx) continue;

        BRTransactionAddInput(transaction, tx->txHash, o->n, tx->outputs[o->n].script, tx->outputs[o->n].scriptLen,
                              NULL, 0, TXIN_SEQUENCE);
        balance += tx->outputs[o->n].amount;
        
        if (tx->blockHeight == TX_UNCONFIRMED && BRWalletAmountSentByTx(wallet, tx) == 0) {
            cpfpSize += BRTransactionSize(tx); // size of unconfirmed, non-change inputs for child-pays-for-parent fee
        }

        feeAmount = BRWalletFeeForTxSize(wallet, BRTransactionSize(transaction) + 34 + cpfpSize); // add a change output
        if (balance == amount + feeAmount || balance >= amount + feeAmount + TX_MIN_OUTPUT_AMOUNT) break;
    }
    
    if (balance < amount + feeAmount) { // insufficient funds
        BRTransactionFree(transaction);
        transaction = NULL;
    }
    else if (balance - (amount + feeAmount) >= TX_MIN_OUTPUT_AMOUNT) {
        const char *address = BRWalletChangeAddress(wallet);
        uint8_t script[BRAddressScriptPubKey(NULL, 0, address)];
        size_t scriptLen = BRAddressScriptPubKey(script, sizeof(script), address);
    
        BRTransactionAddOutput(transaction, balance - (amount + feeAmount), script, scriptLen);
        BRTransactionShuffleOutputs(transaction);
    }
    
    return transaction;
}

// sign any inputs in the given transaction that can be signed using private keys from the wallet
int BRWalletSignTransaction(BRWallet *wallet, BRTransaction *tx, const char *authPrompt)
{
    int64_t amount = BRWalletAmountSentByTx(wallet, tx) - BRWalletAmountReceivedFromTx(wallet, tx);
    unsigned internalIdx[tx->inCount], externalIdx[tx->inCount];
    size_t internalCount = 0, externalCount = 0, seedLen = 0;
    int r = 0;
    
    for (size_t i = 0; i < tx->inCount; i++) {
        for (unsigned j = 0; j < array_count(wallet->internalChain); j++) {
            if (BRAddressEq(tx->inputs[i].address, &wallet->internalChain[j])) internalIdx[internalCount++] = j;
        }

        for (unsigned j = 0; j < array_count(wallet->externalChain); j++) {
            if (BRAddressEq(tx->inputs[i].address, &wallet->externalChain[j])) externalIdx[externalCount++] = j;
        }
    }

    BRKey keys[internalCount + externalCount];
    void *seed = wallet->seed(authPrompt, (amount > 0) ? amount : 0, &seedLen);
    
    if (seed) {
        BRBIP32PrivKeyList(keys, internalCount, seed, seedLen, 1, internalIdx);
        BRBIP32PrivKeyList(&keys[internalCount], externalCount, seed, seedLen, 0, externalIdx);
        seed = NULL;
        r = BRTransactionSign(tx, keys, internalCount + externalCount);
        for (size_t i = 0; i < internalCount + externalCount; i++) BRKeyClean(&keys[i]);
    }
    else r = ! 0; // user canceled authentication
    
    return r;
}

// true if the given transaction is associated with the wallet (even if it hasn't been registered)
int BRWalletContainsTransaction(BRWallet *wallet, BRTransaction *tx)
{
    for (size_t i = 0; i < tx->outCount; i++) {
        if (BRSetContains(wallet->allAddrs, tx->outputs[i].address)) return ! 0;
    }

    for (size_t i = 0; i < tx->inCount; i++) {
        BRTransaction *t = BRSetGet(wallet->allTx, &tx->inputs[i].txHash);
        uint32_t n = tx->inputs[i].index;
        
        if (t && n < t->outCount && BRSetContains(wallet->allAddrs, t->outputs[n].address)) return ! 0;
    }
    
    return 0;
}

// adds a transaction to the wallet, or returns false if it isn't associated with the wallet
int BRWalletRegisterTransaction(BRWallet *wallet, BRTransaction *tx)
{
    if (BRSetContains(wallet->allTx, tx)) return ! 0;
    if (! BRWalletContainsTransaction(wallet, tx)) return 0;
    
    //TODO: verify signatures when possible
    //TODO: handle tx replacement with input sequence numbers (now replacements appear invalid until confirmation)
    
    BRWalletTxSetContext(tx, wallet);
    BRSetAdd(wallet->allTx, tx);
    array_add(wallet->transactions, tx);
    for (size_t i = 0; i < tx->inCount; i++) BRSetAdd(wallet->usedAddrs, tx->inputs[i].address);
    for (size_t i = 0; i < tx->outCount; i++) BRSetAdd(wallet->usedAddrs, tx->outputs[i].address);
    BRWalletUpdateBalance(wallet);
    
    // when a wallet address is used in a transaction, generate a new address to replace it
    BRWalletUnusedAddrs(wallet, SEQUENCE_GAP_LIMIT_EXTERNAL, 0);
    BRWalletUnusedAddrs(wallet, SEQUENCE_GAP_LIMIT_INTERNAL, 1);
    
    if (wallet->txAdded) wallet->txAdded(wallet, tx, wallet->info);
    return ! 0;
}

// removes a transaction from the wallet along with any transactions that depend on its outputs
void BRWalletRemoveTransaction(BRWallet *wallet, UInt256 txHash)
{
    BRTransaction *tx = BRSetGet(wallet->allTx, &txHash);
    UInt256 *hashes = NULL;

    if (! tx) return;
    
    array_new(hashes, 0);
    
    for (size_t i = array_count(wallet->transactions); i > 0; i--) { // find depedent transactions
        BRTransaction *t = wallet->transactions[i - 1];

        if (t->blockHeight < tx->blockHeight) break;
        if (BRTransactionEq(tx, t)) continue;

        for (size_t j = 0; j < tx->inCount; j++) {
            if (! UInt256Eq(tx->inputs[i].txHash, t->txHash)) continue;
            array_add(hashes, t->txHash);
            break;
        }
    }

    for (size_t i = 0; i < array_count(hashes); i++) {
        BRWalletRemoveTransaction(wallet, hashes[i]);
    }
    
    BRSetRemove(wallet->allTx, tx);
    
    for (size_t i = array_count(wallet->transactions); i > 0; i--) {
        if (! BRTransactionEq(wallet->transactions[i - 1], tx)) continue;
        array_rm(wallet->transactions, i - 1);
        break;
    }
    
    BRWalletUpdateBalance(wallet);
    if (wallet->txDeleted) wallet->txDeleted(wallet, txHash, wallet->info);
}

// returns the transaction with the given hash if it's been registered in the wallet
const BRTransaction *BRWalletTransactionForHash(BRWallet *wallet, UInt256 txHash)
{
    return BRSetGet(wallet->allTx, &txHash);
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
    uint64_t amount = 0;
    
    //TODO: don't include outputs below TX_MIN_OUTPUT_AMOUNT
    for (size_t i = 0; i < tx->outCount; i++) {
        if (BRSetContains(wallet->allAddrs, tx->outputs[i].address)) amount += tx->outputs[i].amount;
    }
    
    return amount;
}

// retuns the amount sent from the wallet by the trasaction (total wallet outputs consumed, change and fee included)
uint64_t BRWalletAmountSentByTx(BRWallet *wallet, BRTransaction *tx)
{
    uint64_t amount = 0;
    
    for (size_t i = 0; i < tx->inCount; i++) {
        BRTransaction *t = BRSetGet(wallet->allTx, &tx->inputs[i].txHash);
        uint32_t n = tx->inputs[i].index;
        
        if (t && n < t->outCount && BRSetContains(wallet->allAddrs, t->outputs[n].address)) {
            amount += t->outputs[n].amount;
        }
    }
    
    return amount;
}

// returns the fee for the given transaction if all its inputs are from wallet transactions, UINT64_MAX otherwise
uint64_t BRWalletFeeForTx(BRWallet *wallet, BRTransaction *tx)
{
    uint64_t amount = 0;
    
    for (size_t i = 0; i < tx->inCount; i++) {
        BRTransaction *t = BRSetGet(wallet->allTx, &tx->inputs[i].txHash);
        uint32_t n = tx->inputs[i].index;
        
        if (! t || n > t->outCount) return UINT64_MAX;
        amount += t->outputs[n].amount;
    }
    
    for (size_t i = 0; i < tx->outCount; i++) {
        amount -= tx->outputs[i].amount;
    }
    
    return amount;
}

// historical wallet balance after the given transaction, or current balance if transaction is not registered in wallet
uint64_t BRWalletBalanceAfterTx(BRWallet *wallet, BRTransaction *tx)
{
    for (size_t i = 0; i < array_count(wallet->transactions); i++) {
        if (BRTransactionEq(tx, wallet->transactions[i])) return wallet->balanceHist[i];
    }

    return wallet->balance;
}

// fee that will be added for a transaction of the given size in bytes
uint64_t BRWalletFeeForTxSize(BRWallet *wallet, size_t size)
{
    uint64_t standardFee = ((size + 999)/1000)*TX_FEE_PER_KB, // standard fee based on tx size rounded up to nearest kb
             fee = (((size*wallet->feePerKb/1000) + 99)/100)*100; // fee using feePerKb, rounded up to 100 satoshi
    
    return (fee > standardFee) ? fee : standardFee;
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
