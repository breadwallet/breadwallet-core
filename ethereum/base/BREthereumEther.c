//
//  BBREthereumEther.c
//  Core Ethereum
//
//  Created by Ed Gamble on 2/21/2018.
//  Copyright © 2018-2019 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "support/BRAssert.h"
#include "BREthereumEther.h"

#if LITTLE_ENDIAN != BYTE_ORDER
#error "Must be a `LITTLE ENDIAN` cpu architecture"
#endif

//    > (define ether (lambda (x) (values (quotient x max64) (remainder x max64)))
//    > (ether #e1e21) -> 54, 3875820019684212736
//    > (ether #e1e24) -> 54210, 2003764205206896640
//    > (ether #e1e27) -> 54210108, 11515845246265065472
//    > (ether #e1e30) -> 54210108624, 5076944270305263616

static UInt256 etherUnitScaleFactor [NUMBER_OF_ETHER_UNITS] = {   /* LITTLE ENDIAN    */
    { .u64 = {                     1,            0, 0, 0 } }, /* wei       - 1    */
    { .u64 = {                  1000,            0, 0, 0 } }, /* kwei      - 1e3  */
    { .u64 = {               1000000,            0, 0, 0 } }, /* mwei      - 1e6  */
    { .u64 = {            1000000000,            0, 0, 0 } }, /* gwei      - 1e9  */
    { .u64 = {         1000000000000,            0, 0, 0 } }, /* szabo     - 1e12 */
    { .u64 = {      1000000000000000,            0, 0, 0 } }, /* finney    - 1e15 */
    { .u64 = {   1000000000000000000,            0, 0, 0 } }, /* ether     - 1e18 */
    { .u64 = {   3875820019684212736u,          54, 0, 0 } }, /* kether    - 1e21 */
    { .u64 = {   2003764205206896640u,       54210, 0, 0 } }, /* mether    - 1e24 */
    { .u64 = {  11515845246265065472u,    54210108, 0, 0 } }, /* gether    - 1e27 */
    { .u64 = {   5076944270305263616u, 54210108624, 0, 0 } }  /* tether    - 1e30 */
};

extern BREthereumEther
ethEtherCreate(const UInt256 value) {
    BREthereumEther ether;
    ether.valueInWEI = value;
    return ether;
}

extern BREthereumEther
ethEtherCreateUnit(const UInt256 value, BREthereumEtherUnit unit, int *overflow) {
    assert (NULL != overflow);
    
    BREthereumEther ether;
    switch (unit) {
        case WEI:
            ether.valueInWEI = value;
            *overflow = 0;
            break;
        default: {
            ether.valueInWEI = uint256Mul_Overflow(value, etherUnitScaleFactor[unit], overflow);
            break;
        }
    }
    return ether;
}

extern BREthereumEther
ethEtherCreateNumber (uint64_t number, BREthereumEtherUnit unit) {
    int overflow;
    UInt256 value = { .u64 = { number, 0, 0, 0 } };
    BREthereumEther ether = ethEtherCreateUnit(value, unit, &overflow);
    assert (0 == overflow);
    return ether;
}

extern BREthereumEther
ethEtherCreateZero(void) {
    return ethEtherCreate(UINT256_ZERO);
}

extern BREthereumEther
ethEtherCreateString(const char *number, BREthereumEtherUnit unit, BRCoreParseStatus *status) {
    int decimals = 3 * unit;
    
    UInt256 value = uint256CreateParseDecimal(number, decimals, status);
    return ethEtherCreate(value);
}


extern UInt256 // Can't be done: 1 WEI in ETHER... not representable as UInt256
ethEtherGetValue(const BREthereumEther ether,
              BREthereumEtherUnit unit) {
    switch (unit) {
        case WEI:
            return ether.valueInWEI;
        default:
            // TODO: CRITICAL
            return UINT256_ZERO; /* divideUInt256 (ether.valueInWEI, etherUnitScaleFactor[unit]); */
    }
}

extern char * // Perhaps can be done. 1 WEI -> 1e-18 Ether
ethEtherGetValueString(const BREthereumEther ether, BREthereumEtherUnit unit) {
    return uint256CoerceStringDecimal(ether.valueInWEI, 3 * unit);
}

extern BRRlpItem
ethEtherRlpEncode (const BREthereumEther ether, BRRlpCoder coder) {
    return rlpEncodeUInt256(coder, ether.valueInWEI, 1);
}

extern BREthereumEther
ethEtherRlpDecode (BRRlpItem item, BRRlpCoder coder) {
    return ethEtherCreate(rlpDecodeUInt256(coder, item, 1));
}

extern BREthereumEther
ethEtherAdd (BREthereumEther e1, BREthereumEther e2, int *overflow) {
    BREthereumEther result;
    result.valueInWEI = uint256Add_Overflow(e1.valueInWEI, e2.valueInWEI, overflow);
    return result;
}

extern BREthereumEther
ethEtherSub (BREthereumEther e1, BREthereumEther e2, int *negative) {
    BREthereumEther result;
    result.valueInWEI = uint256Sub_Negative(e1.valueInWEI, e2.valueInWEI, negative);
    return result;
    
}

//
// Comparisons
//
extern BREthereumBoolean
ethEtherIsEQ (BREthereumEther e1, BREthereumEther e2) {
    return uint256EQL (e1.valueInWEI, e2.valueInWEI) ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE;
}

extern BREthereumBoolean
ethEtherIsGT (BREthereumEther e1, BREthereumEther e2) {
    return uint256GT(e1.valueInWEI, e2.valueInWEI) ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE;
}

extern BREthereumBoolean
ethEtherIsGE (BREthereumEther e1, BREthereumEther e2) {
    return uint256GE(e1.valueInWEI, e2.valueInWEI) ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE;
}

extern BREthereumBoolean
ethEtherIsLT (BREthereumEther e1, BREthereumEther e2) {
    return uint256LT(e1.valueInWEI, e2.valueInWEI) ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE;
}

extern BREthereumBoolean
ethEtherIsLE (BREthereumEther e1, BREthereumEther e2) {
    return uint256LE(e1.valueInWEI, e2.valueInWEI) ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE;
}

extern BREthereumBoolean
ethEtherIsZero (BREthereumEther e) {
    return UInt256IsZero (e.valueInWEI) ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE;
}

extern BREthereumComparison
ethEtherCompare (BREthereumEther e1, BREthereumEther e2) {
    switch (uint256Compare(e1.valueInWEI, e2.valueInWEI))
    {
        case -1: return ETHEREUM_COMPARISON_LT;
        case  0: return ETHEREUM_COMPARISON_EQ;
        case +1: return ETHEREUM_COMPARISON_GT;
        default: BRFail();
    }
}
