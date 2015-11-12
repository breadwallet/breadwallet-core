//
//  BRBloomFilter.c
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

#include "BRBloomFilter.h"
#include "BRHash.h"
#include "BRAddress.h"
#include "BRTypes.h"
#include <float.h>
#include <math.h>
#include <stdlib.h>

#define BLOOM_MAX_HASH_FUNCS 50

inline static uint32_t BRBloomFilterHash(BRBloomFilter *filter, const uint8_t *data, size_t len, uint32_t hashNum)
{
    return BRMurmur3_32(data, len, hashNum*0xfba4c795 + filter->tweak) % (filter->length*8);
}

// returns a newly allocated BRBloomFilter struct that must be freed by calling BRBloomFilterFree()
BRBloomFilter *BRBloomFilterNew(double falsePositiveRate, size_t elemCount, uint32_t tweak, uint8_t flags)
{
    BRBloomFilter *filter = calloc(1, sizeof(BRBloomFilter));

    filter->length = (falsePositiveRate < DBL_EPSILON) ? BLOOM_MAX_FILTER_LENGTH :
                     (-1.0/pow(M_LN2, 2))*elemCount*log(falsePositiveRate)/8.0;
    if (filter->length > BLOOM_MAX_FILTER_LENGTH) filter->length = BLOOM_MAX_FILTER_LENGTH;
    if (filter->length < 1) filter->length = 1;
    filter->filter = calloc(filter->length, sizeof(uint8_t));
    filter->hashFuncs = ((filter->length*8.0)/elemCount)*M_LN2;
    if (filter->hashFuncs > BLOOM_MAX_HASH_FUNCS) filter->hashFuncs = BLOOM_MAX_HASH_FUNCS;
    filter->tweak = tweak;
    filter->flags = flags;

    if (! filter->filter) {
        free(filter);
        filter = NULL;
    }

    return filter;
}

// buf must contain a serialized filter, result must be freed by calling BRBloomFilterFree()
BRBloomFilter *BRBloomFilterParse(const uint8_t *buf, size_t len)
{
    BRBloomFilter *filter = calloc(1, sizeof(BRBloomFilter));
    size_t off = 0, l = 0;
    
    filter->length = BRVarInt(buf + off, len - off, &l);
    off += l;
    l = filter->length;
    filter->filter = (off + l <= len) ? calloc(l, sizeof(uint8_t)) : NULL;
    if (filter->filter) memcpy(filter->filter, buf + off, l);
    off += l;
    filter->hashFuncs = (off + sizeof(uint32_t) <= len) ? le32(*(const uint32_t *)(buf + off)) : 0;
    off += sizeof(uint32_t);
    filter->tweak = (off + sizeof(uint32_t) <= len) ? le32(*(const uint32_t *)(buf + off)) : 0;
    off += sizeof(uint32_t);
    filter->flags = (off + sizeof(uint8_t) <= len) ? buf[off] : 0;
    off += sizeof(uint8_t);
    
    if (! filter->filter) {
        free(filter);
        filter = NULL;
    }

    return filter;
}

// returns number of bytes written to buf, or total len needed if buf is NULL
size_t BRBloomFilterSerialize(BRBloomFilter *filter, uint8_t *buf, size_t len)
{
    size_t off = 0,
           l = BRVarIntSize(filter->length) + filter->length + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint8_t);
    
    if (! buf) return l;
    if (len < l) return 0;
    off += BRVarIntSet(buf + off, len - off, filter->length);
    memcpy(buf + off, filter->filter, filter->length);
    *(uint32_t *)(buf + off) = le32(filter->hashFuncs);
    off += sizeof(uint32_t);
    *(uint32_t *)(buf + off) = le32(filter->tweak);
    off += sizeof(uint32_t);
    buf[off] = filter->flags;
    off += sizeof(uint8_t);
    return l;
}

// true if data is matched by filter
int BRBloomFilterContainsData(BRBloomFilter *filter, const uint8_t *data, size_t len)
{
    uint32_t i, idx;
    
    for (i = 0; i < filter->hashFuncs; i++) {
        idx = BRBloomFilterHash(filter, data, len, i);
        if (! (filter->filter[idx >> 3] & (1 << (7 & idx)))) return 0;
    }
    
    return 1;
}

// add data to filter
void BRBloomFilterInsertData(BRBloomFilter *filter, const uint8_t *data, size_t len)
{
    uint32_t i, idx;
    
    for (i = 0; i < filter->hashFuncs; i++) {
        idx = BRBloomFilterHash(filter, data, len, i);
        filter->filter[idx >> 3] |= (1 << (7 & idx));
    }
    
    filter->elemCount++;
}

// frees memory allocated for filter
void BRBloomFilterFree(BRBloomFilter *filter)
{
    if (filter->filter) free(filter->filter);
    free(filter);
}
