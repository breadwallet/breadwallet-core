//
//  BBREthereumAddress.c
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 2/21/2018.
//  Copyright (c) 2018 breadwallet LLC
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

#include <stdlib.h>
#include "BREthereumEther.h"

static UInt256
addUInt256PP (UInt256 x, UInt256 y);

static UInt256
divideUInt256 (UInt256 numerator, UInt256 denominator);

static UInt256
multiplyUInt256 (UInt256 numerator, UInt256 denominator);

static UInt256
decodeUInt256 (const char *string);

static char *
encodeUInt256 (UInt256 value);

static int
gtUInt256 (UInt256 x);
/*
(define ether (lambda (x) (values (quotient x max64) (remainder x max64)))
    > (ether #e1e21) -> 54, 3875820019684212736
    > (ether #e1e24) -> 54210, 2003764205206896640
    > (ether #e1e27) -> 54210108, 11515845246265065472
    > (ether #e1e30) -> 54210108624, 5076944270305263616
*/

static UInt256 etherUnitScaleFactor [NUMBER_OF_ETHER_UNITS] = {
        { .u64 = { 0, 0, 0,                   1 } },            /* wei       - 1    */
        { .u64 = { 0, 0, 0,                1000 } },            /* kwei      - 1e3  */
        { .u64 = { 0, 0, 0,             1000000 } },             /* mwei      - 1e6  */
        { .u64 = { 0, 0, 0,          1000000000 } },            /* gwei      - 1e9  */
        { .u64 = { 0, 0, 0,       1000000000000 } },            /* szabo     - 1e12 */
        { .u64 = { 0, 0, 0,    1000000000000000 } },            /* finney    - 1e15 */
        { .u64 = { 0, 0, 0, 1000000000000000000 } },            /* ether     - 1e18 */
        { .u64 = { 0, 0,          54,  3875820019684212736u } }, /* kether    - 1e21 */
        { .u64 = { 0, 0,       54210,  2003764205206896640u } }, /* mether    - 1e24 */
        { .u64 = { 0, 0,    54210108, 11515845246265065472u } }, /* gether    - 1e27 */
        { .u64 = { 0, 0, 54210108624,  5076944270305263616u } }  /* tether    - 1e30 */
};

// positive
extern UInt256
etherGetValue(const BREthereumEther ether,
              BREthereumEtherUnit unit) {
    switch (unit) {
        case WEI:
            return ether.valueInWEI;
        default:
            return divideUInt256 (ether.valueInWEI, etherUnitScaleFactor[unit]);
    }
}

// positive
extern char *
etherGetValueString(const BREthereumEther ether,
                    BREthereumEtherUnit unit) {
    return encodeUInt256(etherGetValue(ether, unit));
}

extern BREthereumEther
etherCreate(const UInt256 value, BREthereumEtherUnit unit) {
    BREthereumEther ether;
    ether.positive = ETHEREUM_BOOLEAN_TRUE;

    switch (unit) {
        case WEI: ether.valueInWEI = value;
        default:  ether.valueInWEI = multiplyUInt256 (value, etherUnitScaleFactor[unit]);
    }
    return ether;
}

extern BREthereumEther
etherCreateString(const char *string, BREthereumEtherUnit unit) {
    return etherCreate(decodeUInt256(string), unit);
}

extern BREthereumEther
etherCreateNumber (uint64_t number, BREthereumEtherUnit unit) {
    UInt256 value = { .u64 = { 0, 0, 0, number } };
    return etherCreate (value, unit);
}

extern BREthereumEther
etherCreateZero() {
    return etherCreate(UINT256_ZERO, WEI);
}

extern BREthereumEther
etherNegate (BREthereumEther e) {
    BREthereumEther ether;

    ether.positive = e.positive == ETHEREUM_BOOLEAN_TRUE ? ETHEREUM_BOOLEAN_FALSE : ETHEREUM_BOOLEAN_TRUE;
    ether.valueInWEI = e.valueInWEI;

    return ether;
}

extern BREthereumEther
etherAdd (BREthereumEther e1, BREthereumEther e2) {
    if (ETHEREUM_BOOLEAN_IS_TRUE(e1.positive) && ETHEREUM_BOOLEAN_IS_TRUE(e2.positive)) {
        return etherCreate(addUInt256PP(e1.valueInWEI, e2.valueInWEI), WEI);
    }
    else if (ETHEREUM_BOOLEAN_IS_TRUE(e1.positive) && ETHEREUM_BOOLEAN_IS_FALSE(e2.positive)) {
        return etherSub (e1, etherNegate(e2));
    }
    else {
        return etherNegate (etherAdd (etherNegate(e1), etherNegate(e2)));
    }
}

extern BREthereumEther
etherSub (BREthereumEther e1, BREthereumEther e2) {
    if (ETHEREUM_BOOLEAN_IS_TRUE(e1.positive) && ETHEREUM_BOOLEAN_IS_TRUE(e2.positive)) {
        // TODO: Implement
        return etherCreateZero();
    }
    else if (ETHEREUM_BOOLEAN_IS_TRUE(e1.positive) && ETHEREUM_BOOLEAN_IS_FALSE(e2.positive)) {
        return etherAdd (e1, etherNegate(e2));
    }
    else {
        return etherNegate (etherSub (etherNegate(e1), etherNegate(e2)));
    }
}

//
// Comparisons
//
extern BREthereumBoolean
etherIsEQ (BREthereumEther e1, BREthereumEther e2) {
    return UInt256Eq (e1.valueInWEI, e2.valueInWEI) ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE;
}

//

extern BREthereumBoolean
etherIsGT (BREthereumEther e1, BREthereumEther e2) {
    return ETHEREUM_BOOLEAN_FALSE;
}

extern BREthereumBoolean
etherIsGE (BREthereumEther e1, BREthereumEther e2) {
    return etherIsEQ (e1, e2)|| etherIsGT (e1, e2);
}

extern BREthereumBoolean
etherIsZero (BREthereumEther e) {
    return UInt256IsZero (e.valueInWEI) ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE;
}


static UInt256
addUInt256PP (UInt256 x, UInt256 y) {
    // TODO: Implement
    return UINT256_ZERO;
}

static UInt256
divideUInt256 (UInt256 numerator, UInt256 denominator) {
    // TODO: Implement
    return UINT256_ZERO;
}

static UInt256
multiplyUInt256 (UInt256 numerator, UInt256 denominator) {
    // TODO: Implement
    return UINT256_ZERO;
}

static UInt256
decodeUInt256 (const char *string) {
    // TODO: Implement
    return UINT256_ZERO;
}

static char *
encodeUInt256 (UInt256 value) {
    // TODO: Implement
    return NULL;
}

static int
gtUInt256 (UInt256 x) {
    // TODO: Implement
    return 0;
}
