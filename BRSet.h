//
//  BRSet.h
//
//  Created by Aaron Voisine on 9/11/15.
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

#ifndef BRSet_h
#define BRSet_h

#include <stddef.h>

typedef struct _BRSet BRSet;

// retruns a newly allocated empty set that must be freed by calling BRSetFree(), hash is a function that returns a hash
// value for a given set item, eq is a function that tests if two set items are equal, capacity is the maximum estimated
// number of items the set will need to hold
BRSet *BRSetNew(size_t (*hash)(const void *), int (*eq)(const void *, const void *), size_t capacity);

// adds given item to set or replaces an equivalent existing item, returns item replaced if any
void *BRSetAdd(BRSet *set, void *item);

// removes item equivalent to given item from set, returns item removed if any
void *BRSetRemove(BRSet *set, const void *item);

// removes all items from set
void BRSetClear(BRSet *set);

// returns the number of items in set
size_t BRSetCount(BRSet *set);

// true if item is contained in set
int BRSetContains(BRSet *set, const void *item);

// true if any items in otherSet are contained in set
int BRSetIntersects(BRSet *set, const BRSet *otherSet);

// returns member item from set equivalent to given item
void *BRSetGet(BRSet *set, const void *item);

// returns an initial random item from set for use when iterating, or NULL if set is empty
void *BRSetFirst(BRSet *set);

// returns the next item after given item when iterating, or NULL if no more items are available
void *BRSetNext(BRSet *set, const void *item);

// writes up to count items from set to allItems, allItems must be large enough to hold itemSize*count bytes
void BRSetAll(BRSet *set, void *allItems, size_t itemSize, size_t count);

// calls map() with each item in set
void BRSetMap(BRSet *set, void *info, void (*map)(void *info, void *item));

// adds or replaces items from otherSet into set
void BRSetUnion(BRSet *set, const BRSet *otherSet);

// removes items contained in otherSet from set
void BRSetMinus(BRSet *set, const BRSet *otherSet);

// removes items not contained in otherSet from set
void BRSetIntersect(BRSet *set, const BRSet *otherSet);

// frees memory allocated for set
void BRSetFree(BRSet *set);

#endif // BRSet_h
