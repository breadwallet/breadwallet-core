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
#include "BRArray.h"
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <float.h>
#include <pthread.h>

#define DEFAULT_FEE_PER_KB ((TX_FEE_PER_KB*1000 + 190)/191) // default fee-per-kb to match standard fee on 191 byte tx

struct BRWalletStruct {
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
    void *callbackInfo;
    void (*balanceChanged)(void *info, uint64_t balance);
    void (*txAdded)(void *info, BRTransaction *tx);
    void (*txUpdated)(void *info, const UInt256 txHashes[], size_t count, uint32_t blockHeight, uint32_t timestamp);
    void (*txDeleted)(void *info, UInt256 txHash);
    pthread_mutex_t lock;
};

inline static uint64_t _txFee(uint64_t feePerKb, size_t size)
{
    uint64_t standardFee = ((size + 999)/1000)*TX_FEE_PER_KB, // standard fee based on tx size rounded up to nearest kb
             fee = (((size*feePerKb/1000) + 99)/100)*100; // fee using feePerKb, rounded up to 100 satoshi
    
    return (fee > standardFee) ? fee : standardFee;
}

// chain position of first tx output address that appears in chain
inline static size_t _txChainIdx(const BRTransaction *tx, const BRAddress *chain)
{
    for (size_t i = array_count(chain); i > 0; i--) {
        for (size_t j = 0; j < tx->outCount; j++) {
            if (BRAddressEq(tx->outputs[j].address, &chain[i - 1])) return i - 1;
        }
    }
    
    return SIZE_MAX;
}

inline static int _BRWalletTxIsAscending(BRWallet *wallet, const BRTransaction *tx1, const BRTransaction *tx2)
{
    if (! tx1 || ! tx2) return 0;
    if (tx1->blockHeight > tx2->blockHeight) return 1;
    if (tx1->blockHeight < tx2->blockHeight) return 0;
    
    for (size_t i = 0; i < tx1->inCount; i++) {
        if (UInt256Eq(tx1->inputs[i].txHash, tx2->txHash)) return 1;
    }
    
    for (size_t i = 0; i < tx2->inCount; i++) {
        if (UInt256Eq(tx2->inputs[i].txHash, tx1->txHash)) return 0;
    }

    for (size_t i = 0; i < tx1->inCount; i++) {
        if (_BRWalletTxIsAscending(wallet, BRSetGet(wallet->allTx, &(tx1->inputs[i].txHash)), tx2)) return 1;
    }

    return 0;
}

inline static int _BRWalletTxCompare(BRWallet *wallet, const BRTransaction *tx1, const BRTransaction *tx2)
{
    size_t i, j;

    if (_BRWalletTxIsAscending(wallet, tx1, tx2)) return 1;
    if (_BRWalletTxIsAscending(wallet, tx2, tx1)) return -1;
    i = _txChainIdx(tx1, wallet->internalChain);
    j = _txChainIdx(tx2, (i == SIZE_MAX) ? wallet->externalChain : wallet->internalChain);
    if (i == SIZE_MAX && j != SIZE_MAX) i = _txChainIdx((BRTransaction *)tx1, wallet->externalChain);
    if (i != SIZE_MAX && j != SIZE_MAX && i != j) return (i > j) ? 1 : -1;
    return 0;
}

// inserts tx into wallet->transactions, keeping wallet->transactions sorted by date, oldest first (insertion sort)
inline static void _BRWalletInsertTx(BRWallet *wallet, BRTransaction *tx)
{
    size_t i = array_count(wallet->transactions);
    
    array_set_count(wallet->transactions, i + 1);
    
    while (i > 0 && _BRWalletTxCompare(wallet, wallet->transactions[i - 1], tx) > 0) {
        wallet->transactions[i] = wallet->transactions[i - 1];
        i--;
    }
    
    wallet->transactions[i] = tx;
}

// non-threadsafe version of BRWalletContainsTransaction()
static int _BRWalletContainsTx(BRWallet *wallet, const BRTransaction *tx)
{
    int r = 0;
    
    for (size_t i = 0; ! r && i < tx->outCount; i++) {
        if (BRSetContains(wallet->allAddrs, tx->outputs[i].address)) r = 1;
    }
    
    for (size_t i = 0; ! r && i < tx->inCount; i++) {
        BRTransaction *t = BRSetGet(wallet->allTx, &tx->inputs[i].txHash);
        uint32_t n = tx->inputs[i].index;
        
        if (t && n < t->outCount && BRSetContains(wallet->allAddrs, t->outputs[n].address)) r = 1;
    }
    
    return r;
}

static int _BRWalletTxIsSend(BRWallet *wallet, BRTransaction *tx)
{
    int r = 0;
    
    for (size_t i = 0; ! r && i < tx->inCount; i++) {
        if (BRSetContains(wallet->allAddrs, tx->inputs[i].address)) r = 1;
    }
    
    return r;
}

static void _BRWalletUpdateBalance(BRWallet *wallet)
{
    uint64_t balance = 0, prevBalance = 0;
    BRTransaction *tx, *t;
    
    array_clear(wallet->utxos);
    array_clear(wallet->balanceHist);
    BRSetClear(wallet->spentOutputs);
    BRSetClear(wallet->invalidTx);
    wallet->totalSent = 0;
    wallet->totalReceived = 0;

    for (size_t i = 0; i < array_count(wallet->transactions); i++) {
        tx = wallet->transactions[i];

        // check if any inputs are invalid or already spent
        if (tx->blockHeight == TX_UNCONFIRMED) {
            for (size_t j = 0; j < tx->inCount; j++) {
                if (BRSetContains(wallet->spentOutputs, &tx->inputs[j]) ||
                    BRSetContains(wallet->invalidTx, &tx->inputs[j].txHash)) {
                    BRSetAdd(wallet->invalidTx, tx);
                    array_add(wallet->balanceHist, balance);
                    break;
                }
            }
            
            if (BRSetContains(wallet->invalidTx, tx)) continue;
        }

        // add inputs to spent output set
        for (size_t j = 0; j < tx->inCount; j++) {
            BRSetAdd(wallet->spentOutputs, &tx->inputs[j]);
        }

        // add outputs to UTXO set
        // TODO: don't add outputs below TX_MIN_OUTPUT_AMOUNT
        // TODO: don't add coin generation outputs < 100 blocks deep, or non-final lockTime > 1 block/10min in future
        // NOTE: balance/UTXOs will then need to be recalculated when last block changes
        for (size_t j = 0; j < tx->outCount; j++) {
            BRSetAdd(wallet->usedAddrs, tx->outputs[j].address);
            
            if (BRSetContains(wallet->allAddrs, tx->outputs[j].address)) {
                array_add(wallet->utxos, ((BRUTXO) { tx->txHash, (uint32_t)j }));
                balance += tx->outputs[j].amount;
            }
        }

        // transaction ordering is not guaranteed, so check the entire UTXO set against the entire spent output set
        for (size_t j = array_count(wallet->utxos); j > 0; j--) {
            if (! BRSetContains(wallet->spentOutputs, &wallet->utxos[j - 1])) continue;
            t = BRSetGet(wallet->allTx, &wallet->utxos[j - 1].hash);
            balance -= t->outputs[wallet->utxos[j - 1].n].amount;
            array_rm(wallet->utxos, j - 1);
        }
        
        if (prevBalance < balance) wallet->totalReceived += balance - prevBalance;
        if (balance < prevBalance) wallet->totalSent += prevBalance - balance;
        array_add(wallet->balanceHist, balance);
        prevBalance = balance;
    }

    wallet->balance = balance;
}

// allocates and populates a BRWallet struct which must be freed by calling BRWalletFree()
BRWallet *BRWalletNew(BRTransaction *transactions[], size_t txCount, BRMasterPubKey mpk)
{
    BRWallet *wallet = NULL;
    BRTransaction *tx;

    wallet = calloc(1, sizeof(BRWallet));
    array_new(wallet->utxos, 100);
    array_new(wallet->transactions, txCount + 100);
    wallet->feePerKb = DEFAULT_FEE_PER_KB;
    wallet->masterPubKey = mpk;
    array_new(wallet->internalChain, txCount/2 + 100);
    array_new(wallet->externalChain, txCount/2 + 100);
    array_new(wallet->balanceHist, txCount + 100);
    wallet->allTx = BRSetNew(BRTransactionHash, BRTransactionEq, txCount + 100);
    wallet->invalidTx = BRSetNew(BRTransactionHash, BRTransactionEq, 10);
    wallet->spentOutputs = BRSetNew(BRUTXOHash, BRUTXOEq, txCount + 100);
    wallet->usedAddrs = BRSetNew(BRAddressHash, BRAddressEq, txCount*4 + 100);
    wallet->allAddrs = BRSetNew(BRAddressHash, BRAddressEq, txCount + 200 + 100);
    pthread_mutex_init(&wallet->lock, NULL);

    for (size_t i = 0; i < txCount; i++) {
        tx = transactions[i];
        if (! BRTransactionIsSigned(tx) || BRSetContains(wallet->allTx, tx)) continue;
        BRSetAdd(wallet->allTx, tx);
        _BRWalletInsertTx(wallet, tx);
        for (size_t j = 0; j < tx->outCount; j++) BRSetAdd(wallet->usedAddrs, tx->outputs[j].address);
    }
    
    BRWalletUnusedAddrs(wallet, NULL, SEQUENCE_GAP_LIMIT_EXTERNAL, 0);
    BRWalletUnusedAddrs(wallet, NULL, SEQUENCE_GAP_LIMIT_INTERNAL, 1);
    _BRWalletUpdateBalance(wallet);
    if (wallet->balanceChanged) wallet->balanceChanged(wallet->callbackInfo, wallet->balance);
    return wallet;
}

// not thread-safe, set callbacks once after BRWalletNew(), before calling other BRWallet functions
// info is a void pointer that will be passed along with each callback call
// void balanceChanged(void *, uint64_t) - called when the wallet balance changes
// void txAdded(void *, BRTransaction *) - called when transaction is added to the wallet
// void txUpdated(void *, const UInt256[], size_t, uint32_t, uint32_t)
//   - called when the blockHeight or timestamp of previously added transactions are updated
// void txDeleted(void *, UInt256) - called when a previously added transaction is removed from the wallet
void BRWalletSetCallbacks(BRWallet *wallet, void *info,
                          void (*balanceChanged)(void *info, uint64_t balance),
                          void (*txAdded)(void *info, BRTransaction *tx),
                          void (*txUpdated)(void *info, const UInt256 txHash[], size_t count, uint32_t blockHeight,
                                            uint32_t timestamp),
                          void (*txDeleted)(void *info, UInt256 txHash))
{
    wallet->callbackInfo = info;
    wallet->balanceChanged = balanceChanged;
    wallet->txAdded = txAdded;
    wallet->txUpdated = txUpdated;
    wallet->txDeleted = txDeleted;
}

// wallets are composed of chains of addresses
// each chain is traversed until a gap of a number of addresses is found that haven't been used in any transactions
// this function writes to addrs an array of <gapLimit> unused addresses following the last used address in the chain
// the internal chain is used for change addresses and the external chain for receive addresses
// addrs may be NULL to only generate addresses for BRWalletContainsAddress()
void BRWalletUnusedAddrs(BRWallet *wallet, BRAddress addrs[], uint32_t gapLimit, int internal)
{
    BRAddress *chain;
    size_t i, j, count, startCount;

    pthread_mutex_lock(&wallet->lock);
    chain = (internal) ? wallet->internalChain : wallet->externalChain;
    i = count = startCount = array_count(chain);
    
    // keep only the trailing contiguous block of addresses with no transactions
    while (i > 0 && ! BRSetContains(wallet->usedAddrs, &chain[i - 1])) i--;
    
    while (i + gapLimit > count) { // generate new addresses up to gapLimit
        BRKey key;
        BRAddress address = BR_ADDRESS_NONE;
        uint8_t pubKey[BRBIP32PubKey(NULL, 0, wallet->masterPubKey, internal, count)];
        size_t len = BRBIP32PubKey(pubKey, sizeof(pubKey), wallet->masterPubKey, internal, (uint32_t)count);
        
        BRKeySetPubKey(&key, pubKey, len);
        if (! BRKeyAddress(&key, address.s, sizeof(address)) || BRAddressEq(&address, &BR_ADDRESS_NONE)) break;
        array_add(chain, address);
        count++;
    }

    if (addrs && i + gapLimit <= count) {
        for (j = 0; j < gapLimit; j++) {
            addrs[j] = chain[i + j];
        }
    }
    
    if (chain == (internal ? wallet->internalChain : wallet->externalChain)) { // was chain moved to a new mem location?
        for (i = startCount; i < count; i++) {
            BRSetAdd(wallet->allAddrs, &chain[i]);
        }
    }
    else {
        if (internal) wallet->internalChain = chain;
        if (! internal) wallet->externalChain = chain;
        BRSetClear(wallet->allAddrs); // clear and rebuild allAddrs

        for (i = array_count(wallet->internalChain); i > 0; i--) {
            BRSetAdd(wallet->allAddrs, &wallet->internalChain[i - 1]);
        }
        
        for (i = array_count(wallet->externalChain); i > 0; i--) {
            BRSetAdd(wallet->allAddrs, &wallet->externalChain[i - 1]);
        }
    }

    pthread_mutex_unlock(&wallet->lock);
}

// current wallet balance, not including transactions known to be invalid
uint64_t BRWalletBalance(BRWallet *wallet)
{
    uint64_t balance;

    pthread_mutex_lock(&wallet->lock);
    balance = wallet->balance;
    pthread_mutex_unlock(&wallet->lock);
    return balance;
}

// writes unspent outputs to utxos and returns the number of outputs written, or total number available if utxos is NULL
size_t BRWalletUTXOs(BRWallet *wallet, BRUTXO *utxos, size_t count)
{
    pthread_mutex_lock(&wallet->lock);
    if (! utxos || array_count(wallet->utxos) < count) count = array_count(wallet->utxos);

    for (size_t i = 0; utxos && i < count; i++) {
        utxos[i] = wallet->utxos[i];
    }

    pthread_mutex_unlock(&wallet->lock);
    return count;
}

// writes transactions registered in the wallet, sorted by date, oldest first, to the given transactions array
// returns the number of transactions written, or total number available if transactions is NULL
size_t BRWalletTransactions(BRWallet *wallet, BRTransaction *transactions[], size_t count)
{
    pthread_mutex_lock(&wallet->lock);
    if (! transactions || array_count(wallet->transactions) < count) count = array_count(wallet->transactions);

    for (size_t i = 0; transactions && i < count; i++) {
        transactions[i] = wallet->transactions[i];
    }
    
    pthread_mutex_unlock(&wallet->lock);
    return count;
}

// writes transactions registered in the wallet but not yet confirmed in a block, to the given transactions array
// returns the number of transactions written, or total number available if transactions is NULL
size_t BRWalletUnconfirmedTx(BRWallet *wallet, BRTransaction *transactions[], size_t count)
{
    size_t total, n = 0;
    
    pthread_mutex_lock(&wallet->lock);
    total = array_count(wallet->transactions);
    while (n < total && wallet->transactions[(total - n) - 1]->blockHeight == TX_UNCONFIRMED) n++;
    if (! transactions || n < count) count = n;

    for (size_t i = 0; transactions && i < count; i++) {
        transactions[i] = wallet->transactions[(total - n) + i];
    }

    pthread_mutex_unlock(&wallet->lock);
    return count;
}

// total amount spent from the wallet (exluding change)
uint64_t BRWalletTotalSent(BRWallet *wallet)
{
    uint64_t totalSent;
    
    pthread_mutex_lock(&wallet->lock);
    totalSent = wallet->totalSent;
    pthread_mutex_unlock(&wallet->lock);
    return totalSent;
}

// total amount received by the wallet (exluding change)
uint64_t BRWalletTotalReceived(BRWallet *wallet)
{
    uint64_t totalReceived;
    
    pthread_mutex_lock(&wallet->lock);
    totalReceived = wallet->totalReceived;
    pthread_mutex_unlock(&wallet->lock);
    return totalReceived;
}

// fee-per-kb of transaction size to use when creating a transaction
void BRWalletSetFeePerKb(BRWallet *wallet, uint64_t feePerKb)
{
    pthread_mutex_lock(&wallet->lock);
    wallet->feePerKb = feePerKb;
    pthread_mutex_unlock(&wallet->lock);
}

// returns the first unused external address
BRAddress BRWalletReceiveAddress(BRWallet *wallet)
{
    BRAddress addr = BR_ADDRESS_NONE;
    
    BRWalletUnusedAddrs(wallet, &addr, 1, 0);
    return addr;
}

// returns the first unused internal address
BRAddress BRWalletChangeAddress(BRWallet *wallet)
{
    BRAddress addr = BR_ADDRESS_NONE;
    
    BRWalletUnusedAddrs(wallet, &addr, 1, 1);
    return addr;
}

// writes all addresses previously genereated with BRWalletUnusedAddrs() to addrs
// returns the number addresses written, or total number available if addrs is NULL
size_t BRWalletAllAddrs(BRWallet *wallet, BRAddress addrs[], size_t count)
{
    size_t i, internalCount = 0, externalCount = 0;
    
    pthread_mutex_lock(&wallet->lock);
    internalCount = (! addrs || array_count(wallet->internalChain) < count) ?
                    array_count(wallet->internalChain) : count;

    for (i = 0; addrs && i < internalCount; i++) {
        addrs[i] = wallet->internalChain[i];
    }

    externalCount = (! addrs || array_count(wallet->externalChain) < count - internalCount) ?
                    array_count(wallet->externalChain) : count - internalCount;

    for (i = 0; addrs && i < externalCount; i++) {
        addrs[internalCount + i] = wallet->externalChain[i];
    }

    pthread_mutex_unlock(&wallet->lock);
    return internalCount + externalCount;
}

// true if the address was previously generated by BRWalletUnusedAddrs() (even if it's now used)
int BRWalletContainsAddress(BRWallet *wallet, const char *addr)
{
    int r;

    pthread_mutex_lock(&wallet->lock);
    r = BRSetContains(wallet->allAddrs, addr);
    pthread_mutex_unlock(&wallet->lock);
    return r;
}

// true if the address was previously used as an output in any wallet transaction
int BRWalletAddressIsUsed(BRWallet *wallet, const char *addr)
{
    int r;
    
    pthread_mutex_lock(&wallet->lock);
    r = BRSetContains(wallet->usedAddrs, addr);
    pthread_mutex_unlock(&wallet->lock);
    return r;
}

// returns an unsigned transaction that sends the specified amount from the wallet to the given address
// result must be freed using BRTransactionFree()
BRTransaction *BRWalletCreateTransaction(BRWallet *wallet, uint64_t amount, const char *addr)
{
    BRTxOutput o = BR_TX_OUTPUT_NONE;
    
    o.amount = amount;
    BRTxOutputSetAddress(&o, addr);
    return BRWalletCreateTxForOutputs(wallet, &o, 1);
}

// returns an unsigned transaction that satisifes the given transaction outputs
// result must be freed using BRTransactionFree()
BRTransaction *BRWalletCreateTxForOutputs(BRWallet *wallet, const BRTxOutput outputs[], size_t count)
{
    BRTransaction *tx, *transaction = BRTransactionNew();
    uint64_t feeAmount, amount = 0, balance = 0;
    size_t i, j, cpfpSize = 0;
    BRUTXO *o;
    
    for (i = 0; i < count; i++) {
        BRTransactionAddOutput(transaction, outputs[i].amount, outputs[i].script, outputs[i].scriptLen);
        amount += outputs[i].amount;
    }
    
    pthread_mutex_lock(&wallet->lock);
    feeAmount = _txFee(wallet->feePerKb, BRTransactionSize(transaction) + TX_OUTPUT_SIZE);
    
    // TODO: use up all UTXOs for all used addresses to avoid leaving funds in addresses whose public key is revealed
    // TODO: avoid combining addresses in a single transaction when possible to reduce information leakage
    // TODO: use up UTXOs received from any of the output scripts that this transaction sends funds to, to mitigate an
    //       attacker double spending and requesting a refund
    for (i = 0; i < array_count(wallet->utxos); i++) {
        o = &wallet->utxos[i];
        tx = BRSetGet(wallet->allTx, o);
        if (! tx) continue;
        BRTransactionAddInput(transaction, tx->txHash, o->n, tx->outputs[o->n].script, tx->outputs[o->n].scriptLen,
                              NULL, 0, TXIN_SEQUENCE);
        
        if (BRTransactionSize(transaction) + TX_OUTPUT_SIZE > TX_MAX_SIZE) { // transaction size-in-bytes too large
            BRTransactionFree(transaction);
            transaction = NULL;
        
            // check for sufficient total funds before building a smaller transaction
            if (wallet->balance < amount + _txFee(wallet->feePerKb, 10 + array_count(wallet->utxos)*TX_INPUT_SIZE +
                                                  (count + 1)*TX_OUTPUT_SIZE + cpfpSize)) break;
            pthread_mutex_unlock(&wallet->lock);

            if (outputs[count - 1].amount > amount + feeAmount + TX_MIN_OUTPUT_AMOUNT - balance) {
                BRTxOutput newOutputs[count];
                
                for (j = 0; j < count; j++) {
                    newOutputs[j] = outputs[j];
                }
                
                newOutputs[count - 1].amount -= amount + feeAmount - balance; // reduce last output amount
                transaction = BRWalletCreateTxForOutputs(wallet, newOutputs, count);
            }
            else transaction = BRWalletCreateTxForOutputs(wallet, outputs, count - 1); // remove last output

            balance = amount = feeAmount = 0;
            pthread_mutex_lock(&wallet->lock);
            break;
        }
        
        balance += tx->outputs[o->n].amount;
        
        // size of unconfirmed, non-change inputs for child-pays-for-parent fee
        if (tx->blockHeight == TX_UNCONFIRMED && ! _BRWalletTxIsSend(wallet, tx)) cpfpSize += BRTransactionSize(tx);

        // fee amount after adding a change output
        feeAmount = _txFee(wallet->feePerKb, BRTransactionSize(transaction) + TX_OUTPUT_SIZE + cpfpSize);

        // increase fee to round off remaining wallet balance to nearest 100 satoshi
        if (wallet->balance > amount + feeAmount) feeAmount += (wallet->balance - (amount + feeAmount)) % 100;
        
        if (balance == amount + feeAmount || balance >= amount + feeAmount + TX_MIN_OUTPUT_AMOUNT) break;
    }
    
    pthread_mutex_unlock(&wallet->lock);
    
    if (transaction && (count < 1 || balance < amount + feeAmount)) { // no outputs/insufficient funds
        BRTransactionFree(transaction);
        transaction = NULL;
    }
    else if (transaction && balance - (amount + feeAmount) >= TX_MIN_OUTPUT_AMOUNT) { // add change output
        BRAddress addr = BRWalletChangeAddress(wallet);
        uint8_t script[BRAddressScriptPubKey(NULL, 0, addr.s)];
        size_t scriptLen = BRAddressScriptPubKey(script, sizeof(script), addr.s);
    
        BRTransactionAddOutput(transaction, balance - (amount + feeAmount), script, scriptLen);
        BRTransactionShuffleOutputs(transaction);
    }
    
    return transaction;
}

// signs any inputs in tx that can be signed using private keys from the wallet
// seed is the master private key (wallet seed) corresponding to the master public key given when the wallet was created
// returns true if all inputs were signed, or false if there was an error or not all inputs were able to be signed
int BRWalletSignTransaction(BRWallet *wallet, BRTransaction *tx, const void *seed, size_t seedLen)
{
    uint32_t j, internalIdx[tx->inCount], externalIdx[tx->inCount];
    size_t i, internalCount = 0, externalCount = 0;
    int r = 0;
    
    pthread_mutex_lock(&wallet->lock);
    
    for (i = 0; i < tx->inCount; i++) {
        for (j = (uint32_t)array_count(wallet->internalChain); j > 0; j--) {
            if (BRAddressEq(tx->inputs[i].address, &wallet->internalChain[j - 1])) internalIdx[internalCount++] = j - 1;
        }

        for (j = (uint32_t)array_count(wallet->externalChain); j > 0; j--) {
            if (BRAddressEq(tx->inputs[i].address, &wallet->externalChain[j - 1])) externalIdx[externalCount++] = j - 1;
        }
    }

    pthread_mutex_unlock(&wallet->lock);

    BRKey keys[internalCount + externalCount];

    if (seed) {
        BRBIP32PrivKeyList(keys, internalCount, seed, seedLen, 1, internalIdx);
        BRBIP32PrivKeyList(&keys[internalCount], externalCount, seed, seedLen, 0, externalIdx);
        // TODO: XXX wipe seed callback
        seed = NULL;
        r = BRTransactionSign(tx, keys, internalCount + externalCount);
        for (i = 0; i < internalCount + externalCount; i++) BRKeyClean(&keys[i]);
    }
    else r = -1; // user canceled authentication
    
    return r;
}

// true if the given transaction is associated with the wallet (even if it hasn't been registered)
int BRWalletContainsTransaction(BRWallet *wallet, const BRTransaction *tx)
{
    int r;
    
    pthread_mutex_lock(&wallet->lock);
    r = _BRWalletContainsTx(wallet, tx);
    pthread_mutex_unlock(&wallet->lock);
    return r;
}

// adds a transaction to the wallet, or returns false if it isn't associated with the wallet
int BRWalletRegisterTransaction(BRWallet *wallet, BRTransaction *tx)
{
    int wasAdded = 0, r = 1;
    
    if (BRTransactionIsSigned(tx)) {
        pthread_mutex_lock(&wallet->lock);

        if (! BRSetContains(wallet->allTx, tx)) {
            if (_BRWalletContainsTx(wallet, tx)) {
                // TODO: verify signatures when possible
                // TODO: handle tx replacement with input sequence numbers
                //       (for now, replacements appear invalid until confirmation)
                BRSetAdd(wallet->allTx, tx);
                _BRWalletInsertTx(wallet, tx);
                _BRWalletUpdateBalance(wallet);
                wasAdded = 1;
            }
            else { // keep track of unconfirmed non-wallet tx for invalid tx checks and child-pays-for-parent fees
                if (tx->blockHeight == TX_UNCONFIRMED) BRSetAdd(wallet->allTx, tx);
                r = 0;
            }
        }
    
        pthread_mutex_unlock(&wallet->lock);
    }
    else r = 0;

    if (wasAdded) {
        // when a wallet address is used in a transaction, generate a new address to replace it
        BRWalletUnusedAddrs(wallet, NULL, SEQUENCE_GAP_LIMIT_EXTERNAL, 0);
        BRWalletUnusedAddrs(wallet, NULL, SEQUENCE_GAP_LIMIT_INTERNAL, 1);
        if (wallet->balanceChanged) wallet->balanceChanged(wallet->callbackInfo, wallet->balance);
        if (wallet->txAdded) wallet->txAdded(wallet->callbackInfo, tx);
    }

    return r;
}

// removes a tx from the wallet and calls BRTransactionFree() on it, along with any tx that depend on its outputs
void BRWalletRemoveTransaction(BRWallet *wallet, UInt256 txHash)
{
    BRTransaction *tx, *t;
    UInt256 *hashes = NULL;

    pthread_mutex_lock(&wallet->lock);
    tx = BRSetGet(wallet->allTx, &txHash);

    if (tx) {
        array_new(hashes, 0);

        for (size_t i = array_count(wallet->transactions); i > 0; i--) { // find depedent transactions
            t = wallet->transactions[i - 1];
            if (t->blockHeight < tx->blockHeight) break;
            if (BRTransactionEq(tx, t)) continue;
            
            for (size_t j = 0; j < t->inCount; j++) {
                if (! UInt256Eq(t->inputs[j].txHash, txHash)) continue;
                array_add(hashes, t->txHash);
                break;
            }
        }
        
        if (array_count(hashes) > 0) {
            pthread_mutex_unlock(&wallet->lock);
            
            for (size_t i = array_count(hashes); i > 0; i--) {
                BRWalletRemoveTransaction(wallet, hashes[i - 1]);
            }
            
            BRWalletRemoveTransaction(wallet, txHash);
        }
        else {
            BRSetRemove(wallet->allTx, tx);
            BRSetRemove(wallet->invalidTx, tx);
            for (size_t i = 0; i < tx->outCount; i++) BRSetRemove(wallet->usedAddrs, tx->outputs[i].address);
            
            for (size_t i = array_count(wallet->transactions); i > 0; i--) {
                if (! BRTransactionEq(wallet->transactions[i - 1], tx)) continue;
                array_rm(wallet->transactions, i - 1);
                break;
            }
            
            _BRWalletUpdateBalance(wallet);
            pthread_mutex_unlock(&wallet->lock);
            BRTransactionFree(tx);
            if (wallet->balanceChanged) wallet->balanceChanged(wallet->callbackInfo, wallet->balance);
            if (wallet->txDeleted) wallet->txDeleted(wallet->callbackInfo, txHash);
        }
        
        array_free(hashes);
    }
    else pthread_mutex_unlock(&wallet->lock);
}

// returns the transaction with the given hash if it's been registered in the wallet
BRTransaction *BRWalletTransactionForHash(BRWallet *wallet, UInt256 txHash)
{
    BRTransaction *tx;
    
    pthread_mutex_lock(&wallet->lock);
    tx = BRSetGet(wallet->allTx, &txHash);
    pthread_mutex_unlock(&wallet->lock);
    return tx;
}

// true if no previous wallet transaction spends any of the given transaction's inputs, and no input tx is invalid
int BRWalletTransactionIsValid(BRWallet *wallet, const BRTransaction *tx)
{
    BRTransaction *t;
    int r = 1;
    
    // TODO: XXX attempted double spends should cause conflicted tx to remain unverified until they're confirmed
    // TODO: XXX conflicted tx with the same wallet outputs should be presented as the same tx to the user

    if (tx->blockHeight == TX_UNCONFIRMED) { // only unconfirmed transactions can be invalid
        pthread_mutex_lock(&wallet->lock);

        if (! BRSetContains(wallet->allTx, tx)) {
            for (size_t i = 0; r && i < tx->inCount; i++) {
                if (BRSetContains(wallet->spentOutputs, &tx->inputs[i])) r = 0;
            }
        }
        else if (BRSetContains(wallet->invalidTx, tx)) r = 0;

        pthread_mutex_unlock(&wallet->lock);

        for (size_t i = 0; r && i < tx->inCount; i++) {
            t = BRWalletTransactionForHash(wallet, tx->inputs[i].txHash);
            if (t && ! BRWalletTransactionIsValid(wallet, t)) r = 0;
        }
    }
    
    return r;
}

// returns true if all sequence numbers are final (otherwise transaction can be replaced-by-fee), if no outputs are
// dust, transaction size is not over TX_MAX_SIZE, timestamp is greater than 0, and no inputs are known to be unverfied
int BRWalletTransactionIsVerified(BRWallet *wallet, const BRTransaction *tx)
{
    BRTransaction *t;
    int r = 1;

    if (tx->blockHeight == TX_UNCONFIRMED) { // only unconfirmed transactions can be unverified
        if (tx->timestamp == 0) r = 0; // a timestamp of 0 indicates transaction is to remain unverified
        if (r && BRTransactionSize(tx) > TX_MAX_SIZE) r = 0; // check transaction size is under TX_MAX_SIZE
        
        for (size_t i = 0; r && i < tx->inCount; i++) { // check that all sequence numbers are final
            if (tx->inputs[i].sequence != TXIN_SEQUENCE) r = 0;
        }
        
        for (size_t i = 0; r && i < tx->outCount; i++) { // check that no outputs are dust
            if (tx->outputs[i].amount < TX_MIN_OUTPUT_AMOUNT) r = 0;
        }
        
        if (r) { // check if any inputs are known to be unverified
            for (size_t i = 0; r && i < tx->inCount; i++) {
                t = BRWalletTransactionForHash(wallet, tx->inputs[i].txHash);
                if (t && ! BRWalletTransactionIsVerified(wallet, t)) r = 0;
            }
        }
    }
    
    return r;
}

// returns true if transaction won't be valid by blockHeight + 1 or within the next 10 minutes
int BRWalletTransactionIsPostdated(BRWallet *wallet, const BRTransaction *tx, uint32_t blockHeight)
{
    BRTransaction *t;
    int r = 0;

    if (tx->blockHeight == TX_UNCONFIRMED) { // only unconfirmed transactions can be postdated
        for (size_t i = 0; ! r && i < tx->inCount; i++) { // check if any inputs are known to be postdated
            t = BRWalletTransactionForHash(wallet, tx->inputs[i].txHash);
            if (t && BRWalletTransactionIsPostdated(wallet, t, blockHeight)) r = 1;
        }
    
        if ((tx->lockTime > blockHeight + 1 && tx->lockTime < TX_MAX_LOCK_HEIGHT) ||
            (tx->lockTime >= TX_MAX_LOCK_HEIGHT && tx->lockTime >= time(NULL) + 10*60)) {
            for (size_t i = 0; i < tx->inCount; i++) { // lockTime is ignored if all sequence numbers are final
                if (tx->inputs[i].sequence != TXIN_SEQUENCE) r = 1;
            }
        }
    }
    
    return r;
}

// set the block heights and timestamps for the given transactions
// use height TX_UNCONFIRMED and timestamp 0 to indicate a tx should remain marked as unverified (not 0-conf safe)
void BRWalletUpdateTransactions(BRWallet *wallet, const UInt256 txHashes[], size_t count, uint32_t blockHeight,
                                uint32_t timestamp)
{
    BRTransaction *tx;
    UInt256 hashes[count];
    size_t i, j;
    
    pthread_mutex_lock(&wallet->lock);
    
    for (i = 0, j = 0; i < count; i++) {
        tx = BRSetGet(wallet->allTx, &txHashes[i]);
        if (! tx || (tx->blockHeight == blockHeight && tx->timestamp == timestamp)) continue;
        tx->timestamp = timestamp;
        tx->blockHeight = blockHeight;
        
        if (_BRWalletContainsTx(wallet, tx)) {
            hashes[j++] = txHashes[i];
        }
        else if (blockHeight != TX_UNCONFIRMED) { // remove and free confirmed non-wallet tx
            BRSetRemove(wallet->allTx, tx);
            BRTransactionFree(tx);
        }
    }
        
    pthread_mutex_unlock(&wallet->lock);
    if (j > 0 && wallet->txUpdated) wallet->txUpdated(wallet->callbackInfo, hashes, j, blockHeight, timestamp);
}

// marks all transactions confirmed after blockHeight as unconfirmed (useful for chain re-orgs)
void BRWalletSetTxUnconfirmedAfter(BRWallet *wallet, uint32_t blockHeight)
{
    size_t i, j, count;
    
    pthread_mutex_lock(&wallet->lock);
    count = i = array_count(wallet->transactions);
    while (i > 0 && wallet->transactions[i - 1]->blockHeight > blockHeight) i--;
    count -= i;

    UInt256 hashes[count];

    for (j = 0; j < count; j++) {
        wallet->transactions[i + j]->blockHeight = TX_UNCONFIRMED;
        hashes[j] = wallet->transactions[i + j]->txHash;
    }

    pthread_mutex_unlock(&wallet->lock);
    if (count > 0 && wallet->txUpdated) wallet->txUpdated(wallet->callbackInfo, hashes, count, TX_UNCONFIRMED, 0);
}

// returns the amount received by the wallet from the transaction (total outputs to change and/or receive addresses)
uint64_t BRWalletAmountReceivedFromTx(BRWallet *wallet, const BRTransaction *tx)
{
    uint64_t amount = 0;
    
    pthread_mutex_lock(&wallet->lock);
    
    // TODO: don't include outputs below TX_MIN_OUTPUT_AMOUNT
    for (size_t i = 0; i < tx->outCount; i++) {
        if (BRSetContains(wallet->allAddrs, tx->outputs[i].address)) amount += tx->outputs[i].amount;
    }
    
    pthread_mutex_unlock(&wallet->lock);
    return amount;
}

// returns the amount sent from the wallet by the trasaction (total wallet outputs consumed, change and fee included)
uint64_t BRWalletAmountSentByTx(BRWallet *wallet, const BRTransaction *tx)
{
    uint64_t amount = 0;
    
    pthread_mutex_lock(&wallet->lock);
    
    for (size_t i = 0; i < tx->inCount; i++) {
        BRTransaction *t = BRSetGet(wallet->allTx, &tx->inputs[i].txHash);
        uint32_t n = tx->inputs[i].index;
        
        if (t && n < t->outCount && BRSetContains(wallet->allAddrs, t->outputs[n].address)) {
            amount += t->outputs[n].amount;
        }
    }
    
    pthread_mutex_unlock(&wallet->lock);
    return amount;
}

// returns the fee for the given transaction if all its inputs are from wallet transactions, UINT64_MAX otherwise
uint64_t BRWalletFeeForTx(BRWallet *wallet, const BRTransaction *tx)
{
    uint64_t amount = 0;
    
    pthread_mutex_lock(&wallet->lock);
    
    for (size_t i = 0; i < tx->inCount && amount != UINT64_MAX; i++) {
        BRTransaction *t = BRSetGet(wallet->allTx, &tx->inputs[i].txHash);
        uint32_t n = tx->inputs[i].index;
        
        if (t && n < t->outCount) {
            amount += t->outputs[n].amount;
        }
        else amount = UINT64_MAX;
    }
    
    pthread_mutex_unlock(&wallet->lock);
    
    for (size_t i = 0; i < tx->outCount && amount != UINT64_MAX; i++) {
        amount -= tx->outputs[i].amount;
    }
    
    return amount;
}

// historical wallet balance after the given transaction, or current balance if transaction is not registered in wallet
uint64_t BRWalletBalanceAfterTx(BRWallet *wallet, const BRTransaction *tx)
{
    uint64_t balance;
    
    pthread_mutex_lock(&wallet->lock);
    balance = wallet->balance;
    
    for (size_t i = array_count(wallet->transactions); i > 0; i--) {
        if (! BRTransactionEq(tx, wallet->transactions[i - 1])) continue;
        balance = wallet->balanceHist[i - 1];
        break;
    }

    pthread_mutex_unlock(&wallet->lock);
    return balance;
}

// fee that will be added for a transaction of the given size in bytes
uint64_t BRWalletFeeForTxSize(BRWallet *wallet, size_t size)
{
    uint64_t fee;
    
    pthread_mutex_lock(&wallet->lock);
    fee = _txFee(wallet->feePerKb, size);
    pthread_mutex_unlock(&wallet->lock);
    return fee;
}

// outputs below this amount are uneconomical due to fees (TX_MIN_OUTPUT_AMOUNT is the absolute minimum output amount)
uint64_t BRWalletMinOutputAmount(BRWallet *wallet)
{
    uint64_t amount;
    
    pthread_mutex_lock(&wallet->lock);
    amount = wallet->feePerKb*3*(34 + 148)/1000;
    pthread_mutex_unlock(&wallet->lock);
    return amount;
}

// maximum amount that can be sent from the wallet to a single address after fees
uint64_t BRWalletMaxOutputAmount(BRWallet *wallet)
{
    BRTransaction *tx;
    BRUTXO *o;
    uint64_t fee, amount = 0;
    size_t i, txSize, cpfpSize = 0, inCount = 0;

    pthread_mutex_lock(&wallet->lock);

    for (i = array_count(wallet->utxos); i > 0; i--) {
        o = &wallet->utxos[i - 1];
        tx = BRSetGet(wallet->allTx, &o->hash);
        if (! tx || o->n >= tx->outCount) continue;
        inCount++;
        amount += tx->outputs[o->n].amount;
        
        // size of unconfirmed, non-change inputs for child-pays-for-parent fee
        if (tx->blockHeight == TX_UNCONFIRMED && ! _BRWalletTxIsSend(wallet, tx)) cpfpSize += BRTransactionSize(tx);
    }

    txSize = 8 + BRVarIntSize(inCount) + TX_INPUT_SIZE*inCount + BRVarIntSize(2) + TX_OUTPUT_SIZE*2;
    fee = _txFee(wallet->feePerKb, txSize + cpfpSize);
    pthread_mutex_unlock(&wallet->lock);
    
    return (amount > fee) ? amount - fee : 0;
}

// frees memory allocated for wallet, and calls BRTransactionFree() for all registered transactions
void BRWalletFree(BRWallet *wallet)
{
    pthread_mutex_lock(&wallet->lock);
    BRSetFree(wallet->allAddrs);
    BRSetFree(wallet->usedAddrs);
    BRSetFree(wallet->allTx);
    array_free(wallet->balanceHist);

    for (size_t i = array_count(wallet->transactions); i > 0; i--) {
        BRTransactionFree(wallet->transactions[i - 1]);
    }

    array_free(wallet->transactions);
    array_free(wallet->utxos);
    pthread_mutex_unlock(&wallet->lock);
    pthread_mutex_destroy(&wallet->lock);
    free(wallet);
}

// returns the given amount (in satoshis) in local currency units (i.e. pennies, pence)
// price is local currency units per bitcoin
int64_t BRLocalAmount(int64_t amount, double price)
{
    int64_t localAmount = llabs(amount)*(price/SATOSHIS);
    
    // if amount is not 0, but is too small to be represented in local currency, return minimum non-zero localAmount
    if (localAmount == 0 && amount != 0) localAmount = 1;
    return (amount < 0) ? -localAmount : localAmount;
}

// returns the given local currency amount in satoshis
// price is local currency units (i.e. pennies, pence) per bitcoin
int64_t BRBitcoinAmount(int64_t localAmount, double price)
{
    int overflowbits = 0;
    int64_t p = 10, min, max, amount = 0, lamt = llabs(localAmount);

    if (lamt != 0 && price > 0) {
        while (lamt + 1 > INT64_MAX/SATOSHIS) lamt /= 2, overflowbits++; // make sure we won't overflow an int64_t
        min = lamt*SATOSHIS/price; // minimum amount that safely matches localAmount
        max = (lamt + 1)*SATOSHIS/price - 1; // maximum amount that safely matches localAmount
        amount = (min + max)/2; // average min and max
        while (overflowbits > 0) lamt *= 2, min *= 2, max *= 2, amount *= 2, overflowbits--;
        
        if (amount >= MAX_MONEY) return (localAmount < 0) ? -MAX_MONEY : MAX_MONEY;
        while ((amount/p)*p >= min && p <= INT64_MAX/10) p *= 10; // lowest decimal precision matching localAmount
        p /= 10;
        amount = (amount/p)*p;
    }
    
    return (localAmount < 0) ? -amount : amount;
}
