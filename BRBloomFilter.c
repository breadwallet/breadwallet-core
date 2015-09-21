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

// returns a newly allocated BRBloomFilter struct that must be freed by calling BRBloomFilterFree()
BRBloomFilter *BRBloomFilterNew(double falsePositiveRate, size_t elemCount, uint32_t tweak, uint8_t flags)
{
    return NULL;
}

// buf must contain a serialized filter, result must be freed by calling BRBloomFilterFree()
BRBloomFilter *BRBloomFilterDeserialize(const uint8_t *buf, size_t len)
{
    return NULL;
}

// returns number of bytes written to buf, or total len needed if buf is NULL
size_t BRBloomFilterSerialize(BRBloomFilter *filter, uint8_t *buf, size_t len)
{
    return 0;
}

// true if data is matched by filter
int BRBloomFilterContainsData(BRBloomFilter *filter, const uint8_t *data, size_t len)
{
    return 0;
}

// add data to filter
void BRBloomFilterInsertData(BRBloomFilter *filter, const uint8_t *data, size_t len)
{
}

// frees memory allocated for filter
void BRBloomFilterFree(BRBloomFilter *filter)
{
}