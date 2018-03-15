//
//  BREthereumToken
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

#include <string.h>
#include "BREthereumToken.h"

//
// Token
//
extern BREthereumToken
tokenCreate(char *address,
            char *symbol,
            char *name,
            char *description,
            BREthereumGas gasLimit,
            BREthereumGasPrice gasPrice) {
  BREthereumToken token = tokenCreateNone();

  // TODO: Copy address or what (BREthereumToken is a value type....)
  token.address = address;
  token.symbol  = symbol;
  token.name    = name;
  token.description = description;
  token.gasLimit = gasLimit;
  token.gasPrice = gasPrice;

  return token;
}

extern BREthereumToken
tokenCreateNone (void) {
  BREthereumToken token;
  memset (&token, sizeof (BREthereumToken), 0);
  return token;
}

extern BREthereumGas
tokenGetGasLimit (BREthereumToken token) {
  return token.gasLimit;
}


extern BREthereumGasPrice
tokenGetGasPrice (BREthereumToken token) {
  return token.gasPrice;
}

extern BREthereumContract
tokenGetContract (BREthereumToken token) {
  return contractERC20;
}

BREthereumToken tokenBRD = {
  "0x5250776FAD5A73707d222950de7999d3675a2722",
  "BRD",
  "Bread Token",
  "The Bread Token ...",
  18,                               // Decimals
  { 50000 },                        // Default gasLimit
  { { { .u64 = {0, 0, 0, 0}}}}      // Default gasPrice
};

//
// Token Quantity
//
extern BREthereumTokenQuantity
createTokenQuantity (BREthereumToken token,
                     UInt256 valueAsInteger) {
  BREthereumTokenQuantity quantity;
  quantity.token = token;
  quantity.valueAsInteger = valueAsInteger;
  return quantity;
}

static UInt256
parseTokenQuantity (const char *number, BREthereumTokenQuantityUnit unit, int decimals, int *error) {
  return UINT256_ZERO;
}

extern BREthereumTokenQuantity
createTokenQuantityString (BREthereumToken token,
                           const char *number,
                           BREthereumTokenQuantityUnit unit) {
  int error = 0;
  UInt256 valueAsInteger = parseTokenQuantity(number, unit, token.decimals, &error);
  return createTokenQuantity(token, valueAsInteger);
}

extern BREthereumToken
tokenQuantityGetToken (BREthereumTokenQuantity quantity) {
  return quantity.token;
}
