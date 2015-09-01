//
//  BRTransaction.h
//  breadwallet-core
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

#include "BRTypes.h"
#include <stddef.h>

typedef struct {
    UInt256 txHash;
    unsigned index;
    const char *script;
    size_t scriptLen;
    const char *signature;
    size_t sigLen;
    unsigned sequence;
} BRTxInput;

typedef struct {
    unsigned long long amount;
    const char *script;
    size_t scriptLen;
} BRTxOutput;

typedef struct {
    unsigned version;
    unsigned long inCount;
    BRTxInput *inputs;
    unsigned long outCount;
    BRTxOutput *outputs;
    unsigned lockTime;
    UInt256 txHash;
    unsigned blockHeight;
    unsigned timestamp; // time interval since unix epoch
} BRTransaction;

//XXX need to decided on allocation scheme, either pass in all buffers, or pass in allocator and add a free function

size_t BRTransactionSerialize(BRTransaction *tx, char *buf, size_t len);

int BRTransactionDeserialize(BRTransaction *tx, const char *buf, size_t len);

void BRTransactionShuffleOutputs(BRTransaction *tx);

void BRTransactionSign(BRTransaction *tx, const char **privKeys, unsigned long count);

#endif // BRTransaction_h
