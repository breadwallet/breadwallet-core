//
//  BRBloomFilter.h
//
//  Created by Aaron Voisine on 9/2/15.
//  Copyright (c) 2015 breadwallet LLC.
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

#ifndef BRBloomFilter_h
#define BRBloomFilter_h

#include <stdint.h>

#define BLOOM_DEFAULT_FALSEPOSITIVE_RATE 0.0005 // same as bitcoinj, use 0.00005 for less data, 0.001 for good anonymity
#define BLOOM_REDUCED_FALSEPOSITIVE_RATE 0.00005
#define BLOOM_UPDATE_NONE                0
#define BLOOM_UPDATE_ALL                 1
#define BLOOM_UPDATE_P2PUBKEY_ONLY       2
#define BLOOM_MAX_FILTER_LENGTH          36000 // this allows for 10,000 elements with a <0.0001% false positive rate

typedef struct {
    uint8_t *filter;
    size_t length;
    uint32_t hashFuncs;
    size_t elemCount;
    uint32_t tweak;
    uint8_t flags;
} BRBloomFilter;

static const BRBloomFilter BR_BLOOM_FULL_MATCH = { (uint8_t *)"\xFF", 1, 0, 0, 0, BLOOM_UPDATE_NONE };

BRBloomFilter *BRBloomFilterCreate(void *(*alloc)(size_t), double falsePositiveRate, size_t elemCount, uint32_t tweak,
                                   uint8_t flags);

BRBloomFilter *BRBloomFilterDeserialize(void *(*alloc)(size_t), const uint8_t *buf, size_t len);

size_t BRBloomFilterSerialize(BRBloomFilter *filter, uint8_t *buf, size_t len);

int BRBloomFilterContainsData(BRBloomFilter *filter, const uint8_t *buf, size_t len);

void BRBloomFilterInsertData(BRBloomFilter *filter, const uint8_t *buf, size_t len);

void BRBloomFilterFree(BRBloomFilter *filter, void (*free)(void *));

#endif // BRBloomFilter_h
