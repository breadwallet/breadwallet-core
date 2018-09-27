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
#include "BRArray.h"
#include "BRSet.h"
#include "BREthereumToken.h"
#include "BREthereum.h"

#define TOKEN_DEFAULT_INITIALIZATION_COUNT   (100)

//
// Address
//
#define ADDRESS_BYTES   (20)

typedef struct {
    uint8_t bytes[ADDRESS_BYTES];
} BREthereumAddressRaw;

static BREthereumAddressRaw
addressRawCreate (const char *address) {
    BREthereumAddressRaw raw;
    if (0 == strncmp ("0x", address, 2)) address = &address[2];
    decodeHex(raw.bytes, sizeof(raw.bytes), address, strlen(address));
    return raw;
}

static inline int
addressHashValue (BREthereumAddressRaw address) {
    return ((UInt160 *) &address)->u32[0];
}

static inline int
addressHashEqual (BREthereumAddressRaw address1,
                  BREthereumAddressRaw address2) {
    return 0 == memcmp (address1.bytes, address2.bytes, ADDRESS_BYTES);
}

//
// Token
//
struct BREthereumTokenRecord {
    /**
     * An Ethereum raw address - for BRSet *must* be first.
     */
    BREthereumAddressRaw raw;

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
};

extern const char *
tokenGetAddress(BREthereumToken token) {
    return token->address;
}

extern const char *
tokenGetSymbol(BREthereumToken token) {
    return token->symbol;
}

extern const char *
tokenGetName(BREthereumToken token) {
    return token->name;
}

extern const char *
tokenGetDescription(BREthereumToken token) {
    return token->description;
}

extern int
tokenGetDecimals(BREthereumToken token) {
    return token->decimals;
}

extern BREthereumGas
tokenGetGasLimit(BREthereumToken token) {
    return token->gasLimit;
}

extern BREthereumGasPrice
tokenGetGasPrice(BREthereumToken token) {
    return token->gasPrice;
}

extern BREthereumContract
tokenGetContract(BREthereumToken token) {
    return contractERC20;
}

static inline size_t
tokenHashValue (const void *t)
{
    return addressHashValue(((BREthereumToken) t)->raw);
}

static inline int
tokenHashEqual (const void *t1, const void *t2) {
    return t1 == t2 || addressHashEqual (((BREthereumToken) t1)->raw,
                                         ((BREthereumToken) t2)->raw);
}

//
// Token Quantity
//
extern BREthereumTokenQuantity
createTokenQuantity(BREthereumToken token,
                    UInt256 valueAsInteger) {
    assert (NULL != token);
    
    BREthereumTokenQuantity quantity;
    quantity.token = token;
    quantity.valueAsInteger = valueAsInteger;
    return quantity;
}

extern BREthereumTokenQuantity
createTokenQuantityString(BREthereumToken token,
                          const char *number,
                          BREthereumTokenQuantityUnit unit,
                          BRCoreParseStatus *status) {
    UInt256 valueAsInteger;
    
    if ((TOKEN_QUANTITY_TYPE_DECIMAL == unit && CORE_PARSE_OK != parseIsDecimal(number))
        || (TOKEN_QUANTITY_TYPE_INTEGER == unit && CORE_PARSE_OK != parseIsInteger(number))) {
        *status = CORE_PARSE_STRANGE_DIGITS;
        valueAsInteger = UINT256_ZERO;
    } else {
        valueAsInteger = (TOKEN_QUANTITY_TYPE_DECIMAL == unit
                          ? createUInt256ParseDecimal(number, token->decimals, status)
                          : createUInt256Parse(number, 10, status));
    }
    
    return createTokenQuantity(token, (CORE_PARSE_OK != *status ? UINT256_ZERO : valueAsInteger));
}

extern const BREthereumToken
tokenQuantityGetToken(BREthereumTokenQuantity quantity) {
    return quantity.token;
}

extern char *
tokenQuantityGetValueString(const BREthereumTokenQuantity quantity,
                            BREthereumTokenQuantityUnit unit) {
    return (TOKEN_QUANTITY_TYPE_INTEGER == unit
            ? coerceString(quantity.valueAsInteger, 10)
            : coerceStringDecimal(quantity.valueAsInteger, quantity.token->decimals));
}

extern BREthereumComparison
tokenQuantityCompare(BREthereumTokenQuantity q1, BREthereumTokenQuantity q2, int *typeMismatch) {
    *typeMismatch = (q1.token != q2.token);
    if (*typeMismatch) return ETHEREUM_COMPARISON_GT;
    switch (compareUInt256(q1.valueAsInteger, q2.valueAsInteger)) {
        case -1:
            return ETHEREUM_COMPARISON_LT;
        case 0:
            return ETHEREUM_COMPARISON_EQ;
        case +1:
            return ETHEREUM_COMPARISON_GT;
        default:
            return ETHEREUM_COMPARISON_GT;
    }
}

/*

{"code":"SNGLS","colors":["FAFAFA","FAFAFA"],"name":"Singular DTV","decimal":"0","address":"0xaeC2E87E0A235266D9C5ADc9DEb4b2E29b54D009"}

var tokensInJSONToC = function (tokens) {
return "static struct BREthereumTokenRecord tokens[] = \n{" +
       tokens.map (function (token) {
    return `
    {
        "${token.address}",
        "${token.code}",
        "${token.name}",
        "",
        ${token.decimal},
        "${token.colors[0]}",
        "${token.colors[1]}",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    }`})
.join (",\n") + "\n};"
}

var result = tokensInJSONToC (tokens);
console.log (result)
*/

static BRSet *tokenSet = NULL;

static void
tokenInitializeIfAppropriate (void) {
    if (NULL == tokenSet) {
        tokenSet = BRSetNew(tokenHashValue, tokenHashEqual, TOKEN_DEFAULT_INITIALIZATION_COUNT);
    }
}

extern BREthereumToken
tokenLookupByAddress (BREthereumAddressRaw address) {
    tokenInitializeIfAppropriate();
    return (BREthereumToken) BRSetGet(tokenSet, &address);
}

extern BREthereumToken
tokenLookup(const char *address) {
    return ((NULL == address || '\0' == address[0])
            ? NULL
            : tokenLookupByAddress (addressRawCreate(address)));
}

extern int
tokenCount() {
    tokenInitializeIfAppropriate();
    return (int) BRSetCount (tokenSet);
}

extern BREthereumToken *
tokenGetAll (void) {
    int tokensCount = tokenCount();
    BREthereumToken *tokens = calloc (tokensCount, sizeof (BREthereumToken));
    BRSetAll (tokenSet, (void**) tokens, tokensCount);
    return tokens;
}

extern void
tokenInstall (const char *address,
              const char *symbol,
              const char *name,
              const char *description,
              int decimals,
              BREthereumGas defaultGasLimit,
              BREthereumGasPrice defaultGasPrice) {
    BREthereumAddressRaw raw   = addressRawCreate (address);
    BREthereumToken      token = tokenLookupByAddress (raw);

    if (NULL == token) {
        token = malloc (sizeof(struct BREthereumTokenRecord));

        token->address     = strdup (address);
        token->symbol      = strdup (symbol);
        token->name        = strdup (name);
        token->description = strdup (description);
        token->decimals    = decimals;
        token->gasLimit    = defaultGasLimit;
        token->gasPrice    = defaultGasPrice;

        token->raw = raw;
        BRSetAdd (tokenSet, token);
    }

    else {
        if (0 != strcasecmp (address    , token->address    )) { free (token->address    ); token->address     = strdup (address    ); }
        if (0 != strcasecmp (symbol     , token->symbol     )) { free (token->symbol     ); token->symbol      = strdup (symbol     ); }
        if (0 != strcasecmp (name       , token->name       )) { free (token->name       ); token->name        = strdup (name       ); }
        if (0 != strcasecmp (description, token->description)) { free (token->description); token->description = strdup (description); }
        token->decimals = decimals;
        token->gasLimit = defaultGasLimit;
        token->gasPrice = defaultGasPrice;
    }
}
