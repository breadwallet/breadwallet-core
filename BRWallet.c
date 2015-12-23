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
#include "BRRWLock.h"
#include "BRArray.h"
#include <stdlib.h>

#define DEFAULT_FEE_PER_KB ((TX_FEE_PER_KB*1000 + 190)/191) // default fee-per-kb to match standard fee on 191 byte tx

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
    void *seedInfo;
    const void *(*seed)(void *info, const char *authPrompt, uint64_t amount, size_t *seedLen);
    void *callbackInfo;
    void (*balanceChanged)(void *info, uint64_t balance);
    void (*txAdded)(void *info, BRTransaction *tx);
    void (*txUpdated)(void *info, const UInt256 txHashes[], size_t count, uint32_t blockHeight, uint32_t timestamp);
    void (*txDeleted)(void *info, UInt256 txHash);
    BRRWLock lock;
};

// chain position of first tx output address that appears in chain
inline static size_t BRWalletTxChainIdx(BRTransaction *tx, BRAddress *chain) {
    for (size_t i = array_count(chain); i > 0; i--) {
        for (size_t j = 0; j < tx->outCount; j++) {
            if (BRAddressEq(tx->outputs[j].address, &chain[i - 1])) return i - 1;
        }
    }
    
    return SIZE_MAX;
}

inline static int BRWalletTxIsAscending(BRWallet *wallet, BRTransaction *tx1, BRTransaction *tx2)
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
        if (BRWalletTxIsAscending(wallet, BRSetGet(wallet->allTx, &(tx1->inputs[i].txHash)), tx2)) return 1;
    }

    return 0;
}

inline static int BRWalletTxCompare(void *info, const void *tx1, const void *tx2)
{
    BRWallet *wallet = info;
    size_t i, j;

    if (BRWalletTxIsAscending(wallet, (BRTransaction *)tx1, (BRTransaction *)tx2)) return 1;
    if (BRWalletTxIsAscending(wallet, (BRTransaction *)tx2, (BRTransaction *)tx1)) return -1;
    i = BRWalletTxChainIdx((BRTransaction *)tx1, wallet->internalChain);
    j = BRWalletTxChainIdx((BRTransaction *)tx2, (i == SIZE_MAX) ? wallet->externalChain : wallet->internalChain);
    if (i == SIZE_MAX && j != SIZE_MAX) i = BRWalletTxChainIdx(*(BRTransaction **)tx1, wallet->externalChain);
    if (i != SIZE_MAX && j != SIZE_MAX && i != j) return (i > j) ? 1 : -1;
    return 0;
}

// inserts a transaction into wallet->transactions, keeping wallet->transactions sorted by date, oldest first
inline static void BRWalletInsertTransaction(BRWallet *wallet, BRTransaction *tx)
{
    size_t i = array_count(wallet->transactions);
    
    array_set_count(wallet->transactions, i + 1);
    
    while (i > 0 && BRWalletTxCompare(wallet, wallet->transactions[i - 1], tx) > 0) {
        wallet->transactions[i] = wallet->transactions[i - 1];
        i--;
    }
    
    wallet->transactions[i] = tx;
}

static void BRWalletUpdateBalance(BRWallet *wallet)
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

    if (balance != wallet->balance) {
        wallet->balance = balance;
        if (wallet->balanceChanged) wallet->balanceChanged(wallet->callbackInfo, balance);
    }
}

// allocate and populate a wallet
BRWallet *BRWalletNew(BRTransaction *transactions[], size_t txCount, BRMasterPubKey mpk, void *info,
                      const void *(*seed)(void *info, const char *authPrompt, uint64_t amount, size_t *seedLen))
{
    BRWallet *wallet = calloc(1, sizeof(BRWallet));
    BRTransaction *tx;

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
    wallet->seedInfo = info;
    wallet->seed = seed;
    BRRWLockInit(&wallet->lock);

    for (size_t i = 0; i < txCount; i++) {
        tx = transactions[i];
        BRSetAdd(wallet->allTx, tx);
        BRWalletInsertTransaction(wallet, tx);
        for (size_t j = 0; j < tx->inCount; j++) BRSetAdd(wallet->usedAddrs, tx->inputs[j].address);
        for (size_t j = 0; j < tx->outCount; j++) BRSetAdd(wallet->usedAddrs, tx->outputs[j].address);
    }
    
    wallet->balance = UINT64_MAX; // this forces a balanceChanged callback even if balance is zero
    BRWalletUpdateBalance(wallet);
    return wallet;
}

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

// Wallets are composed of chains of addresses. Each chain is traversed until a gap of a certain number of addresses is
// found that haven't been used in any transactions. This function writes to addrs an array of <gapLimit> unused
// addresses following the last used address in the chain. The internal chain is used for change addresses and the
// external chain for receive addresses. addrs may be NULL to only generate addresses for BRWalletContainsAddress()
void BRWalletUnusedAddrs(BRWallet *wallet, BRAddress addrs[], uint32_t gapLimit, int internal)
{
    BRAddress **chain;
    uint32_t count, i;

    BRRWLockRead(&wallet->lock);
    chain = (internal) ? &wallet->internalChain : &wallet->externalChain;
    i = count = (uint32_t)array_count(*chain);
    
    // keep only the trailing contiguous block of addresses with no transactions
    while (i > 0 && ! BRSetContains(wallet->usedAddrs, &(*chain)[i - 1])) i--;
    
    if (count - i < gapLimit) {
        BRRWLockUnlock(&wallet->lock);
        BRRWLockWrite(&wallet->lock);
        chain = (internal) ? &wallet->internalChain : &wallet->externalChain;
        count = (uint32_t)array_count(*chain);
    }
    
    while (count - i < gapLimit) { // generate new addresses up to gapLimit
        BRKey key;
        BRAddress a = BR_ADDRESS_NONE;
        uint8_t pubKey[BRBIP32PubKey(NULL, 0, wallet->masterPubKey, internal, count)];
        size_t len = BRBIP32PubKey(pubKey, sizeof(pubKey), wallet->masterPubKey, internal, count);
        
        BRKeySetPubKey(&key, pubKey, len);
        if (BRKeyAddress(&key, a.s, sizeof(a)) == 0) break;
        if (BRAddressEq(&a, &BR_ADDRESS_NONE)) break;
        array_add(*chain, a);
        BRSetAdd(wallet->allAddrs, &(*chain)[count]);
        count++;
    }
    
    if (addrs && count - i >= gapLimit) memcpy(addrs, &(*chain)[i], gapLimit*sizeof(*addrs));
    BRRWLockUnlock(&wallet->lock);
}

// current wallet balance, not including transactions known to be invalid
uint64_t BRWalletBalance(BRWallet *wallet)
{
    uint64_t balance;

    BRRWLockRead(&wallet->lock);
    balance = wallet->balance;
    BRRWLockUnlock(&wallet->lock);
    return balance;
}

// writes unspent outputs to utxos, returns the number of outputs written, or total number available if utxos is NULL
size_t BRWalletUTXOs(BRWallet *wallet, BRUTXO *utxos, size_t count)
{
    BRRWLockRead(&wallet->lock);
    if (! utxos || array_count(wallet->utxos) < count) count = array_count(wallet->utxos);
    if (utxos) memcpy(utxos, wallet->utxos, count*sizeof(*utxos));
    BRRWLockUnlock(&wallet->lock);
    return count;
}

// writes transactions registered in the wallet, sorted by date, oldest first, to the given transactions array, returns
// the number of transactions written, or total number available if transactions is NULL
size_t BRWalletTransactions(BRWallet *wallet, BRTransaction *transactions[], size_t count)
{
    BRRWLockRead(&wallet->lock);
    if (! transactions || array_count(wallet->transactions) < count) count = array_count(wallet->transactions);
    if (transactions) memcpy(transactions, wallet->transactions, count*sizeof(*transactions));
    BRRWLockUnlock(&wallet->lock);
    return count;
}

// total amount spent from the wallet (exluding change)
uint64_t BRWalletTotalSent(BRWallet *wallet)
{
    uint64_t totalSent;
    
    BRRWLockRead(&wallet->lock);
    totalSent = wallet->totalSent;
    BRRWLockUnlock(&wallet->lock);
    return totalSent;
}

// total amount received by the wallet (exluding change)
uint64_t BRWalletTotalReceived(BRWallet *wallet)
{
    uint64_t totalReceived;
    
    BRRWLockRead(&wallet->lock);
    totalReceived = wallet->totalReceived;
    BRRWLockUnlock(&wallet->lock);
    return totalReceived;
}

// fee-per-kb of transaction size to use when creating a transaction
void BRWalletSetFeePerKb(BRWallet *wallet, uint64_t feePerKb)
{
    wallet->feePerKb = feePerKb;
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

// true if the given txHash is registered in the wallet
int BRWalletContainsTxHash(BRWallet *wallet, UInt256 txHash)
{
    int r;
    
    BRRWLockRead(&wallet->lock);
    r = BRSetContains(wallet->allTx, &txHash);
    BRRWLockUnlock(&wallet->lock);
    return r;
}

// true if the address is was previously generated by BRWalletUnusedAddrs() (either as used or unused)
int BRWalletContainsAddress(BRWallet *wallet, const char *addr)
{
    int r;

    BRRWLockRead(&wallet->lock);
    r = BRSetContains(wallet->allAddrs, addr);
    BRRWLockUnlock(&wallet->lock);
    return r;
}

// true if the address was previously used as an input or output in any wallet transaction
int BRWalletAddressIsUsed(BRWallet *wallet, const char *addr)
{
    int r;
    
    BRRWLockRead(&wallet->lock);
    r = BRSetContains(wallet->usedAddrs, addr);
    BRRWLockUnlock(&wallet->lock);
    return r;
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
BRTransaction *BRWalletCreateTxForOutputs(BRWallet *wallet, const BRTxOutput outputs[], size_t count)
{
    uint64_t amount = 0, balance = 0, feeAmount = 0;
    BRTransaction *tx, *transaction = BRTransactionNew();
    size_t i, cpfpSize = 0;
    BRUTXO *o;
    
    for (i = 0; i < count; i++) {
        BRTransactionAddOutput(transaction, outputs[i].amount, outputs[i].script, outputs[i].scriptLen);
        amount += outputs[i].amount;
    }
    
    BRRWLockRead(&wallet->lock);
    
    // TODO: use up all UTXOs for all used addresses to avoid leaving funds in addresses whose public key is revealed
    // TODO: avoid combining addresses in a single transaction when possible to reduce information leakage
    // TODO: use up UTXOs received from any of the output scripts that this transaction sends funds to, to mitigate an
    //       attacker double spending and requesting a refund
    for (i = 0; count > 0 && i < array_count(wallet->utxos); i++) {
        o = &wallet->utxos[i];
        tx = BRSetGet(wallet->allTx, o);
        if (! tx) continue;

        BRTransactionAddInput(transaction, tx->txHash, o->n, tx->outputs[o->n].script, tx->outputs[o->n].scriptLen,
                              NULL, 0, TXIN_SEQUENCE);
        
        if (BRTransactionSize(transaction) + TX_OUTPUT_SIZE > TX_MAX_SIZE) { // transaction size-in-bytes too large
            BRTransactionFree(transaction);
            transaction = NULL;
        
            // check for sufficient total funds before building a smaller transaction
            if (wallet->balance < amount + BRWalletFeeForTxSize(wallet, 10 + array_count(wallet->utxos)*TX_INPUT_SIZE +
                                                                (count + 1)*TX_OUTPUT_SIZE + cpfpSize)) break;
        
            if (outputs[count - 1].amount > amount + feeAmount + TX_MIN_OUTPUT_AMOUNT - balance) {
                BRTxOutput newOutputs[count];
                
                memcpy(newOutputs, outputs, sizeof(*outputs)*count);
                newOutputs[count - 1].amount -= amount + feeAmount - balance; // reduce last output amount
                transaction = BRWalletCreateTxForOutputs(wallet, newOutputs, count);
            }
            else transaction = BRWalletCreateTxForOutputs(wallet, outputs, count - 1); // remove last output

            balance = amount = feeAmount = 0;
            break;
        }
        
        balance += tx->outputs[o->n].amount;
        
        if (tx->blockHeight == TX_UNCONFIRMED && BRWalletAmountSentByTx(wallet, tx) == 0) {
            cpfpSize += BRTransactionSize(tx); // size of unconfirmed, non-change inputs for child-pays-for-parent fee
        }

        feeAmount = BRWalletFeeForTxSize(wallet, BRTransactionSize(transaction) + 34 + cpfpSize); // add a change output
        if (wallet->balance > amount) feeAmount += (wallet->balance - amount) % 100; // round off balance to 100 satoshi
        if (balance == amount + feeAmount || balance >= amount + feeAmount + TX_MIN_OUTPUT_AMOUNT) break;
    }
    
    BRRWLockUnlock(&wallet->lock);
    
    if (transaction && balance < amount + feeAmount) { // insufficient funds
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

// sign any inputs in the given transaction that can be signed using private keys from the wallet
int BRWalletSignTransaction(BRWallet *wallet, BRTransaction *tx, const char *authPrompt)
{
    int64_t amount = BRWalletAmountSentByTx(wallet, tx) - BRWalletAmountReceivedFromTx(wallet, tx);
    uint32_t internalIdx[tx->inCount], externalIdx[tx->inCount];
    size_t internalCount = 0, externalCount = 0, seedLen = 0;
    int r = 0;
    
    BRRWLockRead(&wallet->lock);
    
    for (size_t i = 0; i < tx->inCount; i++) {
        for (uint32_t j = 0; j < array_count(wallet->internalChain); j++) {
            if (BRAddressEq(tx->inputs[i].address, &wallet->internalChain[j])) internalIdx[internalCount++] = j;
        }

        for (uint32_t j = 0; j < array_count(wallet->externalChain); j++) {
            if (BRAddressEq(tx->inputs[i].address, &wallet->externalChain[j])) externalIdx[externalCount++] = j;
        }
    }

    BRRWLockUnlock(&wallet->lock);

    BRKey keys[internalCount + externalCount];
    const void *seed = wallet->seed(wallet->seedInfo, authPrompt, (amount > 0) ? amount : 0, &seedLen);
    
    if (seed) {
        BRBIP32PrivKeyList(keys, internalCount, seed, seedLen, 1, internalIdx);
        BRBIP32PrivKeyList(&keys[internalCount], externalCount, seed, seedLen, 0, externalIdx);
        seed = NULL;
        r = BRTransactionSign(tx, keys, internalCount + externalCount);
        for (size_t i = 0; i < internalCount + externalCount; i++) BRKeyClean(&keys[i]);
    }
    else r = 1; // user canceled authentication
    
    return r;
}

// true if the given transaction is associated with the wallet (even if it hasn't been registered)
int BRWalletContainsTransaction(BRWallet *wallet, const BRTransaction *tx)
{
    int r = 0;
    
    BRRWLockRead(&wallet->lock);
    
    for (size_t i = 0; ! r && i < tx->outCount; i++) {
        if (BRSetContains(wallet->allAddrs, tx->outputs[i].address)) r = 1;
    }

    for (size_t i = 0; ! r && i < tx->inCount; i++) {
        BRTransaction *t = BRSetGet(wallet->allTx, &tx->inputs[i].txHash);
        uint32_t n = tx->inputs[i].index;
        
        if (t && n < t->outCount && BRSetContains(wallet->allAddrs, t->outputs[n].address)) r = 1;
    }
    
    BRRWLockUnlock(&wallet->lock);
    return r;
}

// adds a transaction to the wallet, or returns false if it isn't associated with the wallet
int BRWalletRegisterTransaction(BRWallet *wallet, BRTransaction *tx)
{
    int added = 0;
    
    if (BRWalletTransactionForHash(wallet, tx->txHash) != NULL) return 1;
    if (! BRWalletContainsTransaction(wallet, tx)) return 0;
    
    // TODO: verify signatures when possible
    // TODO: handle tx replacement with input sequence numbers (now replacements appear invalid until confirmation)
    
    BRRWLockWrite(&wallet->lock);

    if (! BRSetContains(wallet->allTx, tx)) {
        BRSetAdd(wallet->allTx, tx);
        BRWalletInsertTransaction(wallet, tx);
        for (size_t i = 0; i < tx->inCount; i++) BRSetAdd(wallet->usedAddrs, tx->inputs[i].address);
        for (size_t i = 0; i < tx->outCount; i++) BRSetAdd(wallet->usedAddrs, tx->outputs[i].address);
        BRWalletUpdateBalance(wallet);
        added = 1;
    }
    
    BRRWLockUnlock(&wallet->lock);

    if (added) {
        // when a wallet address is used in a transaction, generate a new address to replace it
        BRWalletUnusedAddrs(wallet, NULL, SEQUENCE_GAP_LIMIT_EXTERNAL, 0);
        BRWalletUnusedAddrs(wallet, NULL, SEQUENCE_GAP_LIMIT_INTERNAL, 1);
        
        if (wallet->txAdded) wallet->txAdded(wallet->callbackInfo, tx);
    }

    return 1;
}

// removes a transaction from the wallet along with any transactions that depend on its outputs
void BRWalletRemoveTransaction(BRWallet *wallet, UInt256 txHash)
{
    BRTransaction *tx = BRSetGet(wallet->allTx, &txHash), *t;
    UInt256 *hashes = NULL;

    if (! tx) return;
    array_new(hashes, 0);
    BRRWLockWrite(&wallet->lock);
    tx = BRSetGet(wallet->allTx, &txHash);

    if (tx) {
        for (size_t i = array_count(wallet->transactions); i > 0; i--) { // find depedent transactions
            t = wallet->transactions[i - 1];
            if (t->blockHeight < tx->blockHeight) break;
            if (BRTransactionEq(tx, t)) continue;
            
            for (size_t j = 0; j < tx->inCount; j++) {
                if (! UInt256Eq(tx->inputs[j].txHash, t->txHash)) continue;
                array_add(hashes, t->txHash);
                break;
            }
        }
    }

    if (array_count(hashes) > 0) {
        BRRWLockUnlock(&wallet->lock);

        for (size_t i = 0; i < array_count(hashes); i++) {
            BRWalletRemoveTransaction(wallet, hashes[i]);
        }
        
        BRWalletRemoveTransaction(wallet, txHash);
    }
    else if (tx) {
        BRSetRemove(wallet->allTx, tx);
    
        for (size_t i = array_count(wallet->transactions); i > 0; i--) {
            if (! BRTransactionEq(wallet->transactions[i - 1], tx)) continue;
            array_rm(wallet->transactions, i - 1);
            break;
        }
    
        BRWalletUpdateBalance(wallet);
        BRRWLockUnlock(&wallet->lock);
        BRTransactionFree(tx);
        if (wallet->txDeleted) wallet->txDeleted(wallet->callbackInfo, txHash);
    }
    
    array_free(hashes);
}

// returns the transaction with the given hash if it's been registered in the wallet
const BRTransaction *BRWalletTransactionForHash(BRWallet *wallet, UInt256 txHash)
{
    BRTransaction *tx;
    
    BRRWLockRead(&wallet->lock);
    tx = BRSetGet(wallet->allTx, &txHash);
    BRRWLockUnlock(&wallet->lock);
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
        BRRWLockRead(&wallet->lock);

        if (! BRSetContains(wallet->allTx, tx)) {
            for (size_t i = 0; r && i < tx->inCount; i++) {
                t = BRSetGet(wallet->allTx, &tx->inputs[i].txHash);
                if ((t && ! BRWalletTransactionIsValid(wallet, t)) ||
                    BRSetContains(wallet->spentOutputs, &tx->inputs[i])) r = 0;
            }
        }
        else r = (! BRSetContains(wallet->invalidTx, tx));

        BRRWLockUnlock(&wallet->lock);
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
            BRRWLockRead(&wallet->lock);
        
            for (size_t i = 0; r && i < tx->inCount; i++) {
                t = BRSetGet(wallet->allTx, &tx->inputs[i].txHash);
                if (t && ! BRWalletTransactionIsVerified(wallet, t)) r = 0;
            }
            
            BRRWLockUnlock(&wallet->lock);
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
        BRRWLockRead(&wallet->lock);
        
        for (size_t i = 0; ! r && i < tx->inCount; i++) { // check if any inputs are known to be postdated
            t = BRSetGet(wallet->allTx, &tx->inputs[i].txHash);
            if (t && BRWalletTransactionIsPostdated(wallet, t, blockHeight)) r = 1;
        }
    
        if ((tx->lockTime > blockHeight + 1 && tx->lockTime < TX_MAX_LOCK_HEIGHT) ||
            (tx->lockTime >= TX_MAX_LOCK_HEIGHT && tx->lockTime >= time(NULL) + 10*60)) {
            for (size_t i = 0; i < tx->inCount; i++) { // lockTime is ignored if all sequence numbers are final
                if (tx->inputs[i].sequence != TXIN_SEQUENCE) r = 1;
            }
        }
        
        BRRWLockUnlock(&wallet->lock);
    }
    
    return r;
}

// set the block heights and timestamps for the given transactions, use a height of TX_UNCONFIRMED and timestamp of 0 to
// indicate a transaction and it's dependents should remain marked as unverified (not 0-conf safe)
void BRWalletUpdateTransactions(BRWallet *wallet, const UInt256 txHashes[], size_t count, uint32_t blockHeight,
                                uint32_t timestamp)
{
    BRTransaction *tx;
    int update = 0;
    
    BRRWLockRead(&wallet->lock);
    
    for (size_t i = 0; i < count; i++) {
        tx = BRSetGet(wallet->allTx, &txHashes[i]);
        if (! tx || (tx->blockHeight == blockHeight && tx->timestamp == timestamp)) continue;
        tx->timestamp = timestamp;
        tx->blockHeight = blockHeight;
        update = 1;
    }
    
    BRRWLockUnlock(&wallet->lock);
    if (update && wallet->txUpdated) wallet->txUpdated(wallet->callbackInfo, txHashes, count, blockHeight, timestamp);
}

// returns the amount received by the wallet from the transaction (total outputs to change and/or receive addresses)
uint64_t BRWalletAmountReceivedFromTx(BRWallet *wallet, const BRTransaction *tx)
{
    uint64_t amount = 0;
    
    BRRWLockRead(&wallet->lock);
    
    // TODO: don't include outputs below TX_MIN_OUTPUT_AMOUNT
    for (size_t i = 0; i < tx->outCount; i++) {
        if (BRSetContains(wallet->allAddrs, tx->outputs[i].address)) amount += tx->outputs[i].amount;
    }
    
    BRRWLockUnlock(&wallet->lock);
    return amount;
}

// retuns the amount sent from the wallet by the trasaction (total wallet outputs consumed, change and fee included)
uint64_t BRWalletAmountSentByTx(BRWallet *wallet, const BRTransaction *tx)
{
    uint64_t amount = 0;
    
    BRRWLockRead(&wallet->lock);
    
    for (size_t i = 0; i < tx->inCount; i++) {
        BRTransaction *t = BRSetGet(wallet->allTx, &tx->inputs[i].txHash);
        uint32_t n = tx->inputs[i].index;
        
        if (t && n < t->outCount && BRSetContains(wallet->allAddrs, t->outputs[n].address)) {
            amount += t->outputs[n].amount;
        }
    }
    
    BRRWLockUnlock(&wallet->lock);
    return amount;
}

// returns the fee for the given transaction if all its inputs are from wallet transactions, UINT64_MAX otherwise
uint64_t BRWalletFeeForTx(BRWallet *wallet, const BRTransaction *tx)
{
    uint64_t amount = 0;
    
    BRRWLockRead(&wallet->lock);
    
    for (size_t i = 0; i < tx->inCount && amount != UINT64_MAX; i++) {
        BRTransaction *t = BRSetGet(wallet->allTx, &tx->inputs[i].txHash);
        uint32_t n = tx->inputs[i].index;
        
        if (t && n < t->outCount) {
            amount += t->outputs[n].amount;
        }
        else amount = UINT64_MAX;
    }
    
    BRRWLockUnlock(&wallet->lock);
    
    for (size_t i = 0; i < tx->outCount && amount != UINT64_MAX; i++) {
        amount -= tx->outputs[i].amount;
    }
    
    return amount;
}

// historical wallet balance after the given transaction, or current balance if transaction is not registered in wallet
uint64_t BRWalletBalanceAfterTx(BRWallet *wallet, const BRTransaction *tx)
{
    uint64_t balance = wallet->balance;
    
    BRRWLockRead(&wallet->lock);
    
    for (size_t i = 0; i < array_count(wallet->transactions); i++) {
        if (! BRTransactionEq(tx, wallet->transactions[i])) continue;
        balance = wallet->balanceHist[i];
        break;
    }

    BRRWLockUnlock(&wallet->lock);
    return balance;
}

// fee that will be added for a transaction of the given size in bytes
uint64_t BRWalletFeeForTxSize(BRWallet *wallet, size_t size)
{
    uint64_t standardFee = ((size + 999)/1000)*TX_FEE_PER_KB, // standard fee based on tx size rounded up to nearest kb
             fee = (((size*wallet->feePerKb/1000) + 99)/100)*100; // fee using feePerKb, rounded up to 100 satoshi
    
    return (fee > standardFee) ? fee : standardFee;
}

// outputs below this amount are uneconomical due to fees
uint64_t BRWalletMinOutputAmount(BRWallet *wallet)
{
    return wallet->feePerKb*3*(34 + 148)/1000;
}

// frees memory allocated for wallet, also calls BRTransactionFree() for all registered transactions
void BRWalletFree(BRWallet *wallet)
{
    BRSetFree(wallet->allAddrs);
    BRSetFree(wallet->usedAddrs);
    BRSetFree(wallet->allTx);
    array_free(wallet->balanceHist);

    for (size_t i = 0; i < array_count(wallet->transactions); i++) {
        BRTransactionFree(wallet->transactions[i]);
    }

    array_free(wallet->transactions);
    array_free(wallet->utxos);
    BRRWLockDestroy(&wallet->lock);
    free(wallet);
}

// returns the given amount in local currency units (pennies, pence, etc...), price is local currency units per bitcoin
int64_t BRLocalAmount(int64_t amount, double price)
{
    return 0;
}

// returns the given local currency amount in satoshis, price is local currency units per bitcoin
int64_t BRBitcoinAmount(int64_t localAmount, double price)
{
    return 0;
}
