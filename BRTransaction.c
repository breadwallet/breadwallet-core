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
#include "BRArray.h"
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <time.h>
#include <unistd.h>

#define TX_VERSION  0x00000001u
#define TX_LOCKTIME 0x00000000u
#define SIGHASH_ALL 0x00000001u

// returns a random number less than upperBound, for non-cryptographic use only
uint32_t BRRand(uint32_t upperBound)
{
    static int first = 1;
    uint32_t r;
    
    if (first) srand(((unsigned)time(NULL) ^ getpid())*0x01000193); // seed = (time xor pid)*FNV_PRIME
    first = 0;
    if (upperBound == 0 || upperBound > BR_RAND_MAX) upperBound = BR_RAND_MAX;
    
    do { // to avoid modulo bias, find a rand value not less than 0x100000000 % upperBound
        r = rand();
    } while (r < ((0xffffffff - upperBound*2) + 1) % upperBound); // ((0x100000000 - x*2) % x) == (0x100000000 % x)

    return r % upperBound;
}

void BRTxInputSetAddress(BRTxInput *input, const char *address)
{
    if (input->script) array_free(input->script);
    input->script = NULL;
    input->scriptLen = 0;
    memset(input->address, 0, sizeof(input->address));

    if (address) {
        strncpy(input->address, address, sizeof(input->address) - 1);
        input->scriptLen = BRAddressScriptPubKey(NULL, 0, address);
        array_new(input->script, input->scriptLen);
        array_set_count(input->script, input->scriptLen);
        BRAddressScriptPubKey(input->script, input->scriptLen, address);
    }
}

void BRTxInputSetScript(BRTxInput *input, const uint8_t *script, size_t scriptLen)
{
    if (input->script) array_free(input->script);
    input->script = NULL;
    input->scriptLen = 0;
    memset(input->address, 0, sizeof(input->address));
    
    if (script) {
        input->scriptLen = scriptLen;
        array_new(input->script, scriptLen);
        array_add_array(input->script, script, scriptLen);
        BRAddressFromScriptPubKey(input->address, sizeof(input->address), script, scriptLen);
    }
}

void BRTxInputSetSignature(BRTxInput *input, const uint8_t *signature, size_t sigLen)
{
    if (input->signature) array_free(input->signature);
    input->signature = NULL;
    input->sigLen = 0;
    
    if (signature) {
        input->sigLen = sigLen;
        array_new(input->signature, sigLen);
        array_add_array(input->signature, signature, sigLen);
        if (! input->address[0]) BRAddressFromScriptSig(input->address, sizeof(input->address), signature, sigLen);
    }
}

void BRTxOutputSetAddress(BRTxOutput *output, const char *address)
{
    if (output->script) array_free(output->script);
    output->script = NULL;
    output->scriptLen = 0;
    memset(output->address, 0, sizeof(output->address));

    if (address) {
        strncpy(output->address, address, sizeof(output->address));
        output->scriptLen = BRAddressScriptPubKey(NULL, 0, address);
        array_new(output->script, output->scriptLen);
        array_set_count(output->script, output->scriptLen);
        BRAddressScriptPubKey(output->script, output->scriptLen, address);
    }
}

void BRTxOutputSetScript(BRTxOutput *output, const uint8_t *script, size_t scriptLen)
{
    if (output->script) array_free(output->script);
    output->script = NULL;
    output->scriptLen = 0;
    memset(output->address, 0, sizeof(output->address));

    if (script) {
        output->scriptLen = scriptLen;
        array_new(output->script, scriptLen);
        array_add_array(output->script, script, scriptLen);
        BRAddressFromScriptPubKey(output->address, sizeof(output->address), script, scriptLen);
    }
}

// Writes the binary transaction data that needs to be hashed and signed with the private key for the tx input at
// subscriptIdx. A subscriptIdx of SIZE_MAX will return the entire signed transaction. Returns number of bytes written,
// or total len needed if data is NULL.
static size_t _BRTransactionData(const BRTransaction *tx, uint8_t *data, size_t len, size_t subscriptIdx)
{
    size_t i, off = 0;

    if (data && off + sizeof(uint32_t) <= len) *(uint32_t *)&data[off] = le32(tx->version);
    off += sizeof(uint32_t);
    off += BRVarIntSet((data ? &data[off] : NULL), (off <= len ? len - off : 0), tx->inCount);

    for (i = 0; i < tx->inCount; i++) {
        BRTxInput *in = &tx->inputs[i];
        
        if (data && off + sizeof(UInt256) <= len) memcpy(&data[off], &in->txHash, sizeof(UInt256));
        off += sizeof(UInt256);
        if (data && off + sizeof(uint32_t) <= len) *(uint32_t *)&data[off] = le32(in->index);
        off += sizeof(uint32_t);

        if (subscriptIdx == SIZE_MAX && in->signature && in->sigLen > 0) {
            off += BRVarIntSet((data ? &data[off] : NULL), (off <= len ? len - off : 0), in->sigLen);
            if (data && off + in->sigLen <= len) memcpy(&data[off], in->signature, in->sigLen);
            off += in->sigLen;
        }
        else if (subscriptIdx == i && in->script && in->scriptLen > 0) {
            // TODO: to fully match the reference implementation, OP_CODESEPARATOR related checksig logic should go here
            off += BRVarIntSet((data ? &data[off] : NULL), (off <= len ? len - off : 0), in->scriptLen);
            if (data && off + in->scriptLen <= len) memcpy(&data[off], in->script, in->scriptLen);
            off += in->scriptLen;

        }
        else off += BRVarIntSet((data ? &data[off] : NULL), (off <= len ? len - off : 0), 0);
        
        if (data && off + sizeof(uint32_t) <= len) *(uint32_t *)&data[off] = le32(in->sequence);
        off += sizeof(uint32_t);
    }
    
    off += BRVarIntSet((data ? &data[off] : NULL), (off <= len ? len - off : 0), tx->outCount);
    
    for (i = 0; i < tx->outCount; i++) {
        BRTxOutput *out = &tx->outputs[i];
        
        if (data && off + sizeof(uint64_t) <= len) *(uint64_t *)&data[off] = le64(out->amount);
        off += sizeof(uint64_t);
        off += BRVarIntSet((data ? &data[off] : NULL), (off <= len ? len - off : 0), out->scriptLen);
        if (data && off + out->scriptLen <= len) memcpy(&data[off], out->script, out->scriptLen);
        off += out->scriptLen;
    }
    
    if (data && off + sizeof(uint32_t) <= len) *(uint32_t *)&data[off] = le32(tx->lockTime);
    off += sizeof(uint32_t);

    if (subscriptIdx < tx->inCount) {
        if (data && off + sizeof(uint32_t) <= len) *(uint32_t *)&data[off] = le32(SIGHASH_ALL);
        off += sizeof(uint32_t);
    }

    return (! data || off <= len) ? off : 0;
}

// returns a newly allocated empty transaction that must be freed by calling BRTransactionFree()
BRTransaction *BRTransactionNew()
{
    BRTransaction *tx = calloc(1, sizeof(BRTransaction));

    tx->version = TX_VERSION;
    array_new(tx->inputs, 1);
    array_new(tx->outputs, 2);
    tx->lockTime = TX_LOCKTIME;
    tx->blockHeight = TX_UNCONFIRMED;
    return tx;
}

// buf must contain a serialized tx
// retruns a transaction that must be freed by calling BRTransactionFree()
BRTransaction *BRTransactionParse(const uint8_t *buf, size_t len)
{
    if (! buf) return NULL;

    size_t i, off = 0, l = 0;
    BRTransaction *tx = BRTransactionNew();

    tx->version = (off + sizeof(uint32_t) <= len) ? le32(*(uint32_t *)&buf[off]) : 0;
    off += sizeof(uint32_t);
    tx->inCount = BRVarInt(&buf[off], (off <= len ? len - off : 0), &l);
    off += l;
    array_set_count(tx->inputs, tx->inCount);
    
    for (i = 0; i < tx->inCount; i++) {
        BRTxInput *input = &tx->inputs[i];

        input->txHash = (off + sizeof(UInt256) <= len) ? *(UInt256 *)&buf[off] : UINT256_ZERO;
        off += sizeof(UInt256);
        input->index = (off + sizeof(uint32_t) <= len) ? le32(*(uint32_t *)&buf[off]) : 0;
        off += sizeof(uint32_t);
        input->sigLen = BRVarInt(&buf[off], (off <= len ? len - off : 0), &l);
        off += l;
        if (off + input->sigLen <= len) BRTxInputSetSignature(input, &buf[off], input->sigLen);
        off += input->sigLen;
        input->sequence = (off + sizeof(uint32_t) <= len) ? le32(*(uint32_t *)&buf[off]) : 0;
        off += sizeof(uint32_t);
    }

    tx->outCount = BRVarInt(&buf[off], (off <= len ? len - off : 0), &l);
    off += l;
    array_set_count(tx->outputs, tx->outCount);
    
    for (i = 0; i < tx->outCount; i++) {
        BRTxOutput *output = &tx->outputs[i];

        output->amount = (off + sizeof(uint64_t) <= len) ? le64(*(uint64_t *)&buf[off]) : 0;
        off += sizeof(uint64_t);
        output->scriptLen = BRVarInt(&buf[off], (off <= len ? len - off : 0), &l);
        off += l;
        if (off + output->scriptLen <= len) BRTxOutputSetScript(output, &buf[off], output->scriptLen);
        off += output->scriptLen;
    }
    
    tx->lockTime = (off + sizeof(uint32_t) <= len) ? le32(*(uint32_t *)&buf[off]) : 0;
    off += sizeof(uint32_t);

    if (tx->inCount > 0) {
        uint8_t data[_BRTransactionData(tx, NULL, 0, SIZE_MAX)];
    
        l = _BRTransactionData(tx, data, sizeof(data), SIZE_MAX);
        BRSHA256_2(&tx->txHash, data, l);
    }
    else {
        BRTransactionFree(tx);
        tx = NULL;
    }
    
    return tx;
}

// returns number of bytes written to buf, or total len needed if buf is NULL
// (blockHeight and timestamp are not serialized)
size_t BRTransactionSerialize(BRTransaction *tx, uint8_t *buf, size_t len)
{
    return _BRTransactionData(tx, buf, len, SIZE_MAX);
}

// adds an input to tx
void BRTransactionAddInput(BRTransaction *tx, UInt256 txHash, uint32_t index, const uint8_t *script, size_t scriptLen,
                           const uint8_t *signature, size_t sigLen, uint32_t sequence)
{
    BRTxInput input = { txHash, index, "", NULL, 0, NULL, 0, sequence };

    if (script) BRTxInputSetScript(&input, script, scriptLen);
    if (signature) BRTxInputSetSignature(&input, signature, sigLen);
    array_add(tx->inputs, input);
    tx->inCount = array_count(tx->inputs);
}

// adds an output to tx
void BRTransactionAddOutput(BRTransaction *tx, uint64_t amount, const uint8_t *script, size_t scriptLen)
{
    BRTxOutput output = { "", amount, NULL, 0 };
    
    BRTxOutputSetScript(&output, script, scriptLen);
    array_add(tx->outputs, output);
    tx->outCount = array_count(tx->outputs);
}

// shuffles order of tx outputs
void BRTransactionShuffleOutputs(BRTransaction *tx)
{
    for (uint32_t i = 0; i + 1 < tx->outCount; i++) { // fischer-yates shuffle
        uint32_t j = i + BRRand((uint32_t)tx->outCount - i);
        BRTxOutput t;
        
        if (j != i) {
            t = tx->outputs[i];
            tx->outputs[i] = tx->outputs[j];
            tx->outputs[j] = t;
        }
    }
}

// size in bytes if signed, or estimated size assuming compact pubkey sigs
size_t BRTransactionSize(const BRTransaction *tx)
{
    if (! UInt256IsZero(tx->txHash)) return _BRTransactionData(tx, NULL, 0, SIZE_MAX);
    return 8 + BRVarIntSize(tx->inCount) + tx->inCount*TX_INPUT_SIZE + BRVarIntSize(tx->outCount) +
           tx->outCount*TX_OUTPUT_SIZE;
}

// minimum transaction fee needed for tx to relay across the bitcoin network
uint64_t BRTransactionStandardFee(BRTransaction *tx)
{
    return ((BRTransactionSize(tx) + 999)/1000)*TX_FEE_PER_KB;
}

// checks if all signatures exist, but does not verify them
int BRTransactionIsSigned(BRTransaction *tx)
{
    for (size_t i = 0; i < tx->inCount; i++) {
        if (! tx->inputs[i].signature || tx->inputs[i].sigLen == 0) return 0;
    }

    return 1;
}

// adds signatures to any inputs with NULL signatures that can be signed with any privKeys
// returns true if tx is signed
int BRTransactionSign(BRTransaction *tx, BRKey keys[], size_t count)
{
    BRAddress addrs[count], address;
    size_t i, j;
    
    for (i = 0; i < count; i++) {
        if (! BRKeyAddress(&keys[i], addrs[i].s, sizeof(addrs[i]))) addrs[i] = BR_ADDRESS_NONE;
    }
    
    for (i = 0; i < tx->inCount; i++) {
        BRTxInput *in = &tx->inputs[i];
        
        if (! BRAddressFromScriptPubKey(address.s, sizeof(address), in->script, in->scriptLen)) continue;
        j = 0;
        while (j < count && ! BRAddressEq(&addrs[j], &address)) j++;
        if (j >= count) continue;

        const uint8_t *elems[BRScriptElements(NULL, 0, in->script, in->scriptLen)];
        uint8_t sig[OP_PUSHDATA1 - 1], pubKey[65], data[_BRTransactionData(tx, NULL, 0, i)];
        size_t len = _BRTransactionData(tx, data, sizeof(data), i);
        UInt256 hash = UINT256_ZERO;

        BRSHA256_2(&hash, data, len);
        len = BRKeySign(&keys[j], sig, sizeof(sig) - 1, hash);
        if (len == 0 || len >= OP_PUSHDATA1) continue;
        sig[len++] = SIGHASH_ALL;
        if (in->signature) array_free(in->signature);
        array_new(in->signature, 1 + len + 34);
        array_add(in->signature, len);
        array_add_array(in->signature, sig, len);
        len = BRScriptElements(elems, sizeof(elems)/sizeof(*elems), in->script, in->scriptLen);
        
        if (len >= 2 && *elems[len - 2] == OP_EQUALVERIFY) { // pay-to-pubkey-hash scriptSig
            len = BRKeyPubKey(&keys[j], pubKey, sizeof(pubKey));
            if (len == 0 || len >= OP_PUSHDATA1) continue;
            array_add(in->signature, len);
            array_add_array(in->signature, pubKey, len);
        }
        
        in->sigLen = array_count(in->signature);
    }
    
    if (BRTransactionIsSigned(tx)) {
        uint8_t data[_BRTransactionData(tx, NULL, 0, SIZE_MAX)];
        size_t len = _BRTransactionData(tx, data, sizeof(data), SIZE_MAX);

        BRSHA256_2(&tx->txHash, data, len);
        return 1;
    }
    else return 0;
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
