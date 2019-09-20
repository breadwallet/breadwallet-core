//
//  BRBase.h
//  BRCore
//
//  Created by Michael Carrara on 7/1/19.
//  Copyright © 2019 breadwallet LLC
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

#ifndef BRBase_h
#define BRBase_h

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OwnershipGiven
#define OwnershipKept

/**
 * A Block Height in a block chain.  Note: uint32_t is perhaps enough; conservatively use
 * uint64_t.
 */
typedef uint64_t BRBlockHeight;

// Inclusive block chain height bounds
#define BLOCK_HEIGHT_MINIMUM       (0)
#define BLOCK_HEIGHT_MAXIMUM       (UINT64_MAX - 5)  // Leave room for special values

/**
 * Check if `height` is between the MINIMUM and MAXIMUM values.
 */
#define BLOCK_HEIGHT_IS_IN_RANGE(height) \
  (BLOCK_HEIGHT_MINIMUM <= (height) && (height) <= BLOCK_HEIGHT_MAXIMUM)

// Special values
#define BLOCK_HEIGHT_UNBOUND       (UINT64_MAX)

#ifdef __cplusplus
}
#endif

#endif /* BRBase_h */
