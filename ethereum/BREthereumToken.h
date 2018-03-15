//
//  BREthereumToken
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 3/15/18.
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

#ifndef BR_Ethereum_Token_H
#define BR_Ethereum_Token_H

#include "BREthereumEther.h"
#include "BREthereumGas.h"
#include "BREthereumContract.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * An Ethereum ERC20 Token - conceptually an utter constant (but see gasLimit and gasPrice).
 */
typedef struct BREthereumTokenRecord {
  /**
   * An Ethereum '0x' address for the token's contract.
   */
  char *address;

  /**
   * The (exchange) symbol - "BRD"
   */
  char *symbol;

  /**
   * The name - "Bread Token"
   */
  char *name;

  /**
   * The description - "The Bread Token ..."
   */
  char *description;

  /**
   * The maximum decimals (typically 0 to 18).
   */
  unsigned int decimals;

  /**
   * The (default) Gas Limit for exchanges of this token.
   */
  BREthereumGas gasLimit;           // TODO: Feels modifiable

  /**
   * The (default) Gas Price for exchanges of this token.
   */
  BREthereumGasPrice gasPrice;      // TODO: Feels modifiable

} BREthereumToken;

extern BREthereumToken
tokenCreate(char *address,
            char *symbol,
            char *name,
            char *description,
            BREthereumGas gasLimit,
            BREthereumGasPrice gasPrice);

extern BREthereumToken
tokenCreateNone (void);

extern BREthereumGas
tokenGetGasLimit (BREthereumToken token);

extern BREthereumGasPrice
tokenGetGasPrice (BREthereumToken token);

extern BREthereumContract
tokenGetContract (BREthereumToken token);

extern BREthereumToken tokenBRD;

//
// Token Quantity
//

  /**
   * A BREthereumTokenQuantityUnit defines the (external) representation of a token quantity
   */
typedef enum {
  TOKEN_QUANTITY_TYPE_DECIMAL,
  TOKEN_QUANTITY_TYPE_INTEGER
} BREthereumTokenQuantityUnit;

/**
 * A BREthereumTokenQuantity defines a token amount.
 *
 */
typedef struct {
  BREthereumToken token;
  UInt256 valueAsInteger;
} BREthereumTokenQuantity;

extern BREthereumTokenQuantity
createTokenQuantity (BREthereumToken token,
                     UInt256 valueAsInteger);

extern BREthereumTokenQuantity
createTokenQuantityString (BREthereumToken token,
                           const char *number,
                           BREthereumTokenQuantityUnit unit);

extern BREthereumToken
tokenQuantityGetToken (BREthereumTokenQuantity quantity);

#ifdef __cplusplus
}
#endif

#endif //BR_Ethereum_Token_H

