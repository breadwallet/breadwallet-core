//
//  BREthereumHolding
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 2/25/18.
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

#ifndef BR_Ethereum_Holding_H
#define BR_Ethereum_Holding_H

#include "BREthereumEther.h"
#include "BREthereumToken.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * An Ethereum Wallet holds either ETHER or TOKENs.
 */
typedef enum {
    WALLET_HOLDING_ETHER,
    WALLET_HOLDING_TOKEN
} BREthereumWalletHoldingType;

/**
 * An Ethereum Wallet holds a specific amount of ETHER or TOKENs
 */
typedef struct BREthereumHoldingRecord {
  BREthereumWalletHoldingType type;
  union {
    BREthereumEther ether;
    BREthereumTokenQuantity tokenQuantity;
  } u;
} BREthereumHolding;

extern BREthereumHolding
holdingCreateEther (BREthereumEther ether);

// TODO: what is 'scale' - replace with 'decimals'?
extern BREthereumHolding
holdingCreateToken (BREthereumToken token, UInt256 valueAsInteger);

extern BREthereumWalletHoldingType
holdingGetType (BREthereumHolding holding);

/**
 * The holding's ether if holding ETHER; otherwise fatal.
 */
extern BREthereumEther
holdingGetEther (BREthereumHolding holding);

/**
 * The holding's tokenQuantity if holding TOKEN; otherwise fatal.
 */
extern BREthereumTokenQuantity
holdingGetTokenQuantity (BREthereumHolding holding);

extern BRRlpItem
holdingRlpEncode(BREthereumHolding holding, BRRlpCoder coder);

//
// Parsing
//
/**
 *
 * Parse a string of base-10 digits with one optional decimal point into an Ether holding and
 * assign the status.  If status is 'OK' then the holding will contain some amount of Ether;
 * otherwise status will indicate the failure and holding will contain zero Ether.
 *
 * Examples of number are: "12.3", "12000000000", "0.00000000012", "1.000000000023"
 *
 * Status Errors are:
 *     STRANGE_DIGITS: 12a.3f <all characters must be [0-9\.]
 *          UNDERFLOW: 0.1 WEI
 *           OVERFLOW: 1000000 TETHER
 *
 * @param number
 * @param unit
 * @param status
 *
 */
extern BREthereumHolding
holdingCreateEtherString (const char *number,
                          BREthereumEtherUnit unit,
                          BREthereumEtherParseStatus *status);

extern BREthereumHolding
holdingCreateTokenQuantityString (BREthereumToken token,
                                  const char *number,
                                  BREthereumTokenQuantityUnit unit,
                                  BREthereumEtherParseStatus *status);

#ifdef __cplusplus
}
#endif

#endif //BR_Ethereum_Holding_H
