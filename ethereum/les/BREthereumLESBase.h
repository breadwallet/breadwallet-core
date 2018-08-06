//
//  BREthereumLESBase.h
//  breadwallet-core Ethereum
//
//  Created by Lamont Samuels on 4/24/18.
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

/**
 *
 * Etheruem LES specific data structures & algorithms needed for the p2p network.
 *
 */
#ifndef BR_Ethereum_LES_Base_h
#define BR_Ethereum_LES_Base_h

#include <inttypes.h>
#include "BRKey.h"
#include "BRInt.h"
#include "../base/BREthereumBase.h"
#include "../blockchain/BREthereumBlockChain.h"

#define ETH_LOG_TOPIC "LES"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * XOrs two uin8t arrays
 * @post the lengths of the operands and resulting arrays should all have size the size of the "len" parameter
 * @param op1 - the first operand of bytes
 * @param op2 - the second operand of bytes
 * @param result - the destination of the XOR computation
 */
extern void bytesXOR(uint8_t * op1, uint8_t* op2, uint8_t* result, size_t len);

#ifdef __cplusplus
}
#endif


#endif /* BR_Ethereum_LES_Base_h */
