//
//  BRTransaction.h
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

#ifndef BRTransaction_h
#define BRTransaction_h

#include "BRKey.h"
#include "BRInt.h"

#define TX_FEE_PER_KB        5000ULL     // standard tx fee per kb of tx size, rounded up to nearest kb
#define TX_MIN_OUTPUT_AMOUNT (TX_FEE_PER_KB*3*(34 + 148)/1000) // no txout can be below this amount (or it won't relay)
#define TX_MAX_SIZE          100000      // no tx can be larger than this size in bytes
#define TX_FREE_MAX_SIZE     1000        // tx must not be larger than this size in bytes without a fee
#define TX_FREE_MIN_PRIORITY 57600000ULL // tx must not have a priority below this value without a fee
#define TX_UNCONFIRMED       INT32_MAX   // block height indicating transaction is unconfirmed
#define TX_MAX_LOCK_HEIGHT   500000000u  // a lockTime below this value is a block height, otherwise a timestamp

#define TXIN_SEQUENCE        UINT32_MAX  // sequence number for a finalized tx input

#define BR_RAND_MAX          RAND_MAX

// returns a random number less than upperBound, for non-cryptographic use only
uint32_t BRRand(uint32_t upperBound);

typedef struct {
    UInt256 txHash;
    uint32_t index;
    char address[36];
    uint8_t *script;
    size_t scriptLen;
    uint8_t *signature;
    size_t sigLen;
    uint32_t sequence;
} BRTxInput;

void BRTxInputSetAddress(BRTxInput *input, const char *address);
void BRTxInputSetScript(BRTxInput *input, const uint8_t *script, size_t scriptLen);
void BRTxInputSetSignature(BRTxInput *input, const uint8_t *signature, size_t sigLen);

typedef struct {
    char address[36];
    uint64_t amount;
    uint8_t *script;
    size_t scriptLen;
} BRTxOutput;

void BRTxOutputSetAddress(BRTxOutput *output, const char *address);
void BRTxOutputSetScript(BRTxOutput *output, const uint8_t *script, size_t scriptLen);

typedef struct {
    UInt256 txHash;
    uint32_t version;
    size_t inCount;
    BRTxInput *inputs;
    size_t outCount;
    BRTxOutput *outputs;
    uint32_t lockTime;
    uint32_t blockHeight;
    uint32_t timestamp; // time interval since unix epoch
} BRTransaction;

// returns a newly allocated empty transaction that must be freed by calling BRTransactionFree()
BRTransaction *BRTransactionNew();

// buf must contain a serialized tx, result must be freed by calling BRTransactionFree()
BRTransaction *BRTransactionParse(const uint8_t *buf, size_t len);

// returns number of bytes written to buf, or total len needed if buf is NULL
size_t BRTransactionSerialize(BRTransaction *tx, uint8_t *buf, size_t len);

// adds an input to tx
void BRTransactionAddInput(BRTransaction *tx, UInt256 txHash, uint32_t index, const uint8_t *script, size_t scriptLen,
                           const uint8_t *signature, size_t sigLen, uint32_t sequence);

// adds an output to tx
void BRTransactionAddOutput(BRTransaction *tx, uint64_t amount, const uint8_t *script, size_t scriptLen);

// shuffles order of tx outputs
void BRTransactionShuffleOutputs(BRTransaction *tx);

// size in bytes if signed, or estimated size assuming compact pubkey sigs
size_t BRTransactionSize(const BRTransaction *tx);

// minimum transaction fee needed for tx to relay across the bitcoin network
uint64_t BRTransactionStandardFee(BRTransaction *tx);

// checks if all signatures exist, but does not verify them
int BRTransactionIsSigned(BRTransaction *tx);

// adds signatures to any inputs with NULL signatures that can be signed with any privKeys, returns true if tx is signed
int BRTransactionSign(BRTransaction *tx, BRKey keys[], size_t count);

// returns a hash value for tx suitable for use in a hashtable
inline static size_t BRTransactionHash(const void *tx)
{
    return *(const size_t *)&((const BRTransaction *)tx)->txHash;
}

// true if tx and otherTx have equal txHash values
inline static int BRTransactionEq(const void *tx, const void *otherTx)
{
    return UInt256Eq(((const BRTransaction *)tx)->txHash, ((const BRTransaction *)otherTx)->txHash);
}

// frees memory allocated for tx
void BRTransactionFree(BRTransaction *tx);

#endif // BRTransaction_h
