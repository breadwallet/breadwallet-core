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
#include <assert.h>
#include "BRArray.h"
#include "BREthereumToken.h"
#include "BREthereum.h"

// For tokenBRD define some default for Gas Limit and Price. Argubly never up to date
// and thus changed in BREtherEthereumWallet.
#define TOKEN_BRD_DEFAULT_GAS_LIMIT  50000
#define TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64  20000000000 // 20 GWEI

//
// Token
//

struct BREthereumTokenRecord {
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
    
    /**
     * True(1) if allocated statically
     */
    int staticallyAllocated;
};

extern const char *
tokenGetAddress (BREthereumToken token) {
    return token->address;
}

extern BREthereumGas
tokenGetGasLimit (BREthereumToken token) {
    return token->gasLimit;
}


extern BREthereumGasPrice
tokenGetGasPrice (BREthereumToken token) {
    return token->gasPrice;
}

extern BREthereumContract
tokenGetContract (BREthereumToken token) {
    return contractERC20;
}

static BREthereumToken *tokens = NULL;

private_extern BREthereumToken
tokenLookup (const char *address) {
    if (NULL == tokens) {
        array_new(tokens, 10);
        array_add(tokens, tokenBRD);
        array_add(tokens, tokenUNK);
    }
    if (NULL != address && '\0' != address[0])
        for (int i = 0; i < array_count(tokens); i++)
            if (0 == strcasecmp (address, tokens[i]->address))
                return tokens[i];
    return NULL;
}

struct BREthereumTokenRecord tokenBRDRecord = {
    "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6", // "0x5250776FAD5A73707d222950de7999d3675a2722",
    "BRD",
    "Bread Token",
    "The Bread Token ...",
    18,                               // Decimals
    { TOKEN_BRD_DEFAULT_GAS_LIMIT },                        // Default gasLimit
    { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},     // Default gasPrice
    1
};
const BREthereumToken tokenBRD = &tokenBRDRecord;

struct BREthereumTokenRecord tokenUNKRecord = {
    "0x722dd3f80bac40c951b51bdd28dd19d435762180", // "0x5250776FAD5A73707d222950de7999d3675a2722",
    "UNK",
    "Unknown Token",
    "An Unknown Token ...",
    4,                               // Decimals
    { TOKEN_BRD_DEFAULT_GAS_LIMIT },                        // Default gasLimit
    { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},     // Default gasPrice
    1
};
const BREthereumToken tokenUNK = &tokenUNKRecord;

//
// Token Quantity
//
extern BREthereumTokenQuantity
createTokenQuantity (BREthereumToken token,
                     UInt256 valueAsInteger) {
    assert (NULL != token);
    
    BREthereumTokenQuantity quantity;
    quantity.token = token;
    quantity.valueAsInteger = valueAsInteger;
    return quantity;
}

extern BREthereumTokenQuantity
createTokenQuantityString (BREthereumToken token,
                           const char *number,
                           BREthereumTokenQuantityUnit unit,
                           BRCoreParseStatus *status) {
    UInt256 valueAsInteger;
    
    if ((TOKEN_QUANTITY_TYPE_DECIMAL == unit && CORE_PARSE_OK != parseIsDecimal(number))
        || (TOKEN_QUANTITY_TYPE_INTEGER == unit && CORE_PARSE_OK != parseIsInteger(number))) {
        *status = CORE_PARSE_STRANGE_DIGITS;
        valueAsInteger = UINT256_ZERO;
    }
    else {
        valueAsInteger = (TOKEN_QUANTITY_TYPE_DECIMAL == unit
                          ? createUInt256ParseDecimal(number, token->decimals, status)
                          : createUInt256Parse (number, 10, status));
    }
    
    return createTokenQuantity(token, (CORE_PARSE_OK != *status ? UINT256_ZERO : valueAsInteger));
}

extern const BREthereumToken
tokenQuantityGetToken (BREthereumTokenQuantity quantity) {
    return quantity.token;
}

extern char *
tokenQuantityGetValueString(const BREthereumTokenQuantity quantity,
                            BREthereumTokenQuantityUnit unit) {
    return TOKEN_QUANTITY_TYPE_DECIMAL == unit
    ? coerceString(quantity.valueAsInteger, 10)
    : coerceStringDecimal(quantity.valueAsInteger, quantity.token->decimals);
}
