//
//  BREthereumToken
//  Core Ethereum
//
//  Created by Ed Gamble on 2/25/18.
//  Copyright © 2018-2019 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <string.h>
#include "support/BRArray.h"
#include "support/BRSet.h"
#include "BREthereumToken.h"

#define TOKEN_DEFAULT_INITIALIZATION_COUNT   (100)

//
// Token
//
struct BREthereumTokenRecord {
    /**
     * An Ethereum raw address - for BRSet *must* be first.
     */
    BREthereumAddress raw;

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

extern BREthereumAddress
tokenGetAddressRaw (BREthereumToken token) {
    return token->raw;
}

extern const char *
tokenGetAddress(BREthereumToken token) {
    return token->address;
}

extern BREthereumBoolean
tokenHasAddress (BREthereumToken token,
                 const char *address) {
    return addressEqual (token->raw, addressCreate (address));
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

extern BREthereumHash
tokenGetHash (BREthereumToken token) {
    return addressGetHash(token->raw);
}

extern BREthereumToken
tokenCreate (const char *address,
             const char *symbol,
             const char *name,
             const char *description,
             int decimals,
             BREthereumGas defaultGasLimit,
             BREthereumGasPrice defaultGasPrice) {
    BREthereumToken token = malloc (sizeof(struct BREthereumTokenRecord));

    token->address     = strdup (address);
    token->symbol      = strdup (symbol);
    token->name        = strdup (name);
    token->description = strdup (description);
    token->decimals    = decimals;
    token->gasLimit    = defaultGasLimit;
    token->gasPrice    = defaultGasPrice;
    token->raw         = addressCreate (address);

    return token;
}

extern void
tokenRelease (BREthereumToken token) {
    free (token->address);
    free (token->symbol);
    free (token->name);
    free (token->description);
    free (token);
}

extern void
tokenUpdate (BREthereumToken token,
             const char *symbol,
             const char *name,
             const char *description,
             int decimals,
             BREthereumGas defaultGasLimit,
             BREthereumGasPrice defaultGasPrice) {

    if (0 != strcasecmp (symbol     , token->symbol     )) { free (token->symbol     ); token->symbol      = strdup (symbol     ); }
    if (0 != strcasecmp (name       , token->name       )) { free (token->name       ); token->name        = strdup (name       ); }
    if (0 != strcasecmp (description, token->description)) { free (token->description); token->description = strdup (description); }
    token->decimals = decimals;
    token->gasLimit = defaultGasLimit;
    token->gasPrice = defaultGasPrice;
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

extern BRSetOf(BREthereumToken)
tokenSetCreate (size_t capacity) {
    return BRSetNew (tokenHashValue, tokenHashEqual, capacity);
}

extern BRRlpItem
tokenEncode (BREthereumToken token,
             BRRlpCoder coder) {
    return rlpEncodeList (coder, 7,
                          addressRlpEncode (token->raw, coder),
                          rlpEncodeString (coder, token->symbol),
                          rlpEncodeString (coder, token->name),
                          rlpEncodeString (coder, token->description),
                          rlpEncodeUInt64 (coder, token->decimals, 0),
                          gasRlpEncode (token->gasLimit, coder),
                          gasPriceRlpEncode (token->gasPrice, coder));
}

extern BREthereumToken
tokenDecode (BRRlpItem item,
             BRRlpCoder coder) {
    BREthereumToken token = malloc (sizeof(struct BREthereumTokenRecord));

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList (coder, item, &itemsCount);
    assert (7 == itemsCount);

    token->raw     = addressRlpDecode (items[0], coder);
    token->address = addressGetEncodedString(token->raw, 1);
    token->symbol  = rlpDecodeString (coder, items[1]);
    token->name    = rlpDecodeString (coder, items[2]);
    token->description = rlpDecodeString(coder, items[3]);
    token->decimals    = (unsigned int) rlpDecodeUInt64 (coder, items[4], 0);
    token->gasLimit    = gasRlpDecode (items[5], coder);
    token->gasPrice    = gasPriceRlpDecode (items[6], coder);

    return token;
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
    return TOKEN_QUANTITY_TYPE_INTEGER == unit
           ? coerceString(quantity.valueAsInteger, 10)
           : coerceStringDecimal(quantity.valueAsInteger, quantity.token->decimals);
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
