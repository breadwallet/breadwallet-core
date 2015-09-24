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
    return 0;
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
}

// adds signatures to any inputs with NULL signatures that can be signed with any privKeys
void BRTransactionSign(BRTransaction *tx, const char *privKeys[], size_t count)
{
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
