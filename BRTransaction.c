//
//  BRTransaction.c
//
//  Created by Aaron Voisine on 8/31/15.
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

#include "BRTransaction.h"
#include "BRKey.h"
#include "BRAddress.h"
#include <time.h>
#include <unistd.h>

#define TX_VERSION    0x00000001u
#define TX_LOCKTIME   0x00000000u
#define TXIN_SEQUENCE UINT32_MAX
#define SIGHASH_ALL   0x00000001u

// returns a random number less than upperBound, for non-cryptographic use only
uint32_t BRRand(uint32_t upperBound)
{
    static int first = ! 0;
    uint32_t r;
    
    if (first) srand(((unsigned)time(NULL) ^ getpid())*0x01000193); // seed = (time xor pid)*FNV_PRIME
    first = 0;
    if (upperBound == 0 || upperBound > BR_RAND_MAX) upperBound = BR_RAND_MAX;
    
    do { // to avoid modulo bias, find a rand value not less than 0x100000000 % upperBound
        r = rand();
    } while (r < ((0xffffffff - upperBound*2) + 1) % upperBound); // ((0x100000000 - x*2) % x) == (0x100000000 % x)

    return r % upperBound;
}

static size_t BRTransactionData(BRTransaction *tx, uint8_t *data, size_t len, size_t subscriptIdx)
{
    return 0;
}

// returns a newly allocated empty transaction that must be freed by calling BRTransactionFree()
BRTransaction *BRTransactionNew()
{
    BRTransaction *tx = calloc(1, sizeof(BRTransaction));

    array_new(tx->inputs, 1);
    array_new(tx->outputs, 2);
    return tx;
}

// buf must contain a serialized tx, result must be freed by calling BRTransactionFree()
BRTransaction *BRTransactionDeserialize(const uint8_t *buf, size_t len)
{
    return NULL;
}

// returns number of bytes written to buf, or total len needed if buf is NULL
size_t BRTransactionSerialize(BRTransaction *tx, uint8_t *buf, size_t len)
{
    return BRTransactionData(tx, buf, len, SIZE_MAX);
}

// adds an input to tx
void BRTransactionAddInput(BRTransaction *tx, BRTxInput *input)
{
    array_add(tx->inputs, *input);
    tx->inCount = array_count(tx->inputs);
}

// adds an output to tx
void BRTransactionAddOutput(BRTransaction *tx, BRTxOutput *output)
{
    array_add(tx->outputs, *output);
    tx->outCount = array_count(tx->outputs);
}

// shuffles order of tx outputs
void BRTransactionShuffleOutputs(BRTransaction *tx)
{
    for (uint32_t i = 0; i + 1 < tx->outCount; i++) { // fischer-yates shuffle
        uint32_t j = i + BRRand((uint32_t)tx->outCount - i);
        BRTxOutput t;
        
        if (j == i) continue;
        t = tx->outputs[i];
        tx->outputs[i] = tx->outputs[j];
        tx->outputs[j] = t;
    }
}

// size in bytes if signed, or estimated size assuming compact pubkey sigs
size_t BRTransactionSize(BRTransaction *tx)
{
    return 0;
}

// minimum transaction fee needed for tx to relay across the bitcoin network
uint64_t BRTransactionStandardFee(BRTransaction *tx)
{
    return 0;
}

// checks if all signatures exist, but does not verify them
int BRTransactionIsSigned(BRTransaction *tx)
{
    return 0;
}

// adds signatures to any inputs with NULL signatures that can be signed with any privKeys, returns true if tx is signed
int BRTransactionSign(BRTransaction *tx, const char *privKeys[], size_t count)
{
    BRKey keys[count];
    BRAddress addrs[count];
    size_t i, j;
    
    for (i = 0, j = 0; i < count; i++) {
        if (BRKeySetPrivKey(&keys[j], privKeys[i]) && BRKeyAddress(&keys[j], addrs[j].s, sizeof(addrs[j]))) j++;
    }
    
    count = j;

    for (i = 0; i < tx->inCount; i++) {
        BRTxInput *in = &tx->inputs[i];
        BRAddress a = BR_ADDRESS_NONE;
        
        if (! BRAddressFromScriptPubKey(a.s, sizeof(a), in->script, in->scriptLen)) continue;

        for (j = 0; j < count; j++) {
            if (! BRAddressEq(&addrs[j], &a)) continue;
            
            size_t len, elemsCount = BRScriptElements(NULL, 0, in->script, in->scriptLen);
            const uint8_t *elems[elemsCount];
            uint8_t sig[72], data[BRTransactionData(tx, NULL, 0, i)];
            UInt256 hash = UINT256_ZERO;
            
            BRScriptElements(elems, elemsCount, in->script, in->scriptLen);
            BRTransactionData(tx, data, sizeof(data), i);
            BRSHA256_2(&hash, data, sizeof(data));
            if (in->signature) array_free(in->signature);
            in->sigLen = 0;
            array_new(in->signature, 74);
            array_add(in->signature, SIGHASH_ALL);
            len = BRKeySign(&keys[j], sig, sizeof(sig), hash);
            
            if (sig > 0) {
                array_add(in->signature, len);
                array_add_array(in->signature, sig, len);
            
                if (elemsCount >= 2 && *elems[elemsCount - 2] == OP_EQUALVERIFY) { // pay-to-pubkey-hash scriptSig
                    uint8_t pubKey[BRKeyPubKey(&keys[j], NULL, 0)];
                
                    BRKeyPubKey(&keys[j], pubKey, sizeof(pubKey));
                    array_add(in->signature, sizeof(pubKey));
                    array_add_array(in->signature, pubKey, sizeof(pubKey));
                }
                
                in->sigLen = array_count(in->signature);
            }
            else array_free(in->signature);
            
            break;
        }
    }
    
    for (i = 0; i < count; i++) BRKeyClean(&keys[i]);
    if (! BRTransactionIsSigned(tx)) return 0;
    
    uint8_t data[BRTransactionData(tx, NULL, 0, SIZE_MAX)];
    
    BRTransactionData(tx, data, sizeof(data), SIZE_MAX);
    BRSHA256_2(&tx->txHash, data, sizeof(data));
    return ! 0;
}

// frees memory allocated for tx
void BRTransactionFree(BRTransaction *tx)
{
    for (size_t i = 0; i < tx->inCount; i++) {
        if (tx->inputs[i].script) array_free(tx->inputs[i].script);
        if (tx->inputs[i].signature) array_free(tx->inputs[i].signature);
    }

    for (size_t i = 0; i < tx->outCount; i++) {
        if (tx->outputs[i].script) array_free(tx->outputs[i].script);
    }

    array_free(tx->outputs);
    array_free(tx->inputs);
    free(tx);
}
