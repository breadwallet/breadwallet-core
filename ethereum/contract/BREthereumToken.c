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
ethTokenGetAddressRaw (BREthereumToken token) {
    return token->raw;
}

extern const char *
ethTokenGetAddress(BREthereumToken token) {
    return token->address;
}

extern BREthereumBoolean
ethTokenHasAddress (BREthereumToken token,
                    const char *address) {
    return ethAddressEqual (token->raw, ethAddressCreate (address));
}

extern const char *
ethTokenGetSymbol(BREthereumToken token) {
    return token->symbol;
}

extern const char *
ethTokenGetName(BREthereumToken token) {
    return token->name;
}

extern const char *
ethTokenGetDescription(BREthereumToken token) {
    return token->description;
}

extern int
ethTokenGetDecimals(BREthereumToken token) {
    return token->decimals;
}

extern BREthereumGas
ethTokenGetGasLimit(BREthereumToken token) {
    return token->gasLimit;
}


extern BREthereumGasPrice
ethTokenGetGasPrice(BREthereumToken token) {
    return token->gasPrice;
}

extern BREthereumContract
ethTokenGetContract(BREthereumToken token) {
    return ethContractERC20;
}

extern BREthereumHash
ethTokenGetHash (BREthereumToken token) {
    return ethAddressGetHash(token->raw);
}

extern BREthereumToken
ethTokenCreate (const char *address,
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
    token->raw         = ethAddressCreate (address);

    return token;
}

extern void
ethTokenRelease (BREthereumToken token) {
    free (token->address);
    free (token->symbol);
    free (token->name);
    free (token->description);
    free (token);
}

extern void
ethTokenUpdate (BREthereumToken token,
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
    return ethAddressHashValue(((BREthereumToken) t)->raw);
}

static inline int
tokenHashEqual (const void *t1, const void *t2) {
    return t1 == t2 || ethAddressHashEqual (((BREthereumToken) t1)->raw,
                                            ((BREthereumToken) t2)->raw);
}

extern BRSetOf(BREthereumToken)
ethTokenSetCreate (size_t capacity) {
    return BRSetNew (tokenHashValue, tokenHashEqual, capacity);
}

extern BRRlpItem
ethTokenRlpEncode (BREthereumToken token,
                   BRRlpCoder coder) {
    return rlpEncodeList (coder, 7,
                          ethAddressRlpEncode (token->raw, coder),
                          rlpEncodeString (coder, token->symbol),
                          rlpEncodeString (coder, token->name),
                          rlpEncodeString (coder, token->description),
                          rlpEncodeUInt64 (coder, token->decimals, 0),
                          ethGasRlpEncode (token->gasLimit, coder),
                          ethGasPriceRlpEncode (token->gasPrice, coder));
}

extern BREthereumToken
ethTokenRlpDecode (BRRlpItem item,
                   BRRlpCoder coder) {
    BREthereumToken token = malloc (sizeof(struct BREthereumTokenRecord));

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList (coder, item, &itemsCount);
    assert (7 == itemsCount);

    token->raw     = ethAddressRlpDecode (items[0], coder);
    token->address = ethAddressGetEncodedString(token->raw, 1);
    token->symbol  = rlpDecodeString (coder, items[1]);
    token->name    = rlpDecodeString (coder, items[2]);
    token->description = rlpDecodeString(coder, items[3]);
    token->decimals    = (unsigned int) rlpDecodeUInt64 (coder, items[4], 0);
    token->gasLimit    = ethGasRlpDecode (items[5], coder);
    token->gasPrice    = ethGasPriceRlpDecode (items[6], coder);

    return token;
}


//
// Token Quantity
//
extern BREthereumTokenQuantity
ethTokenQuantityCreate(BREthereumToken token,
                       UInt256 valueAsInteger) {
    assert (NULL != token);

    BREthereumTokenQuantity quantity;
    quantity.token = token;
    quantity.valueAsInteger = valueAsInteger;
    return quantity;
}

extern BREthereumTokenQuantity
ethTokenQuantityCreateString(BREthereumToken token,
                             const char *number,
                             BREthereumTokenQuantityUnit unit,
                             BRCoreParseStatus *status) {
    UInt256 valueAsInteger;

    if ((TOKEN_QUANTITY_TYPE_DECIMAL == unit && CORE_PARSE_OK != stringParseIsDecimal(number))
        || (TOKEN_QUANTITY_TYPE_INTEGER == unit && CORE_PARSE_OK != stringParseIsInteger(number))) {
        *status = CORE_PARSE_STRANGE_DIGITS;
        valueAsInteger = UINT256_ZERO;
    } else {
        valueAsInteger = (TOKEN_QUANTITY_TYPE_DECIMAL == unit
                          ? uint256CreateParseDecimal(number, token->decimals, status)
                          : uint256CreateParse(number, 10, status));
    }

    return ethTokenQuantityCreate(token, (CORE_PARSE_OK != *status ? UINT256_ZERO : valueAsInteger));
}

extern const BREthereumToken
ethTokenQuantityGetToken(BREthereumTokenQuantity quantity) {
    return quantity.token;
}

extern char *
ethTokenQuantityGetValueString(const BREthereumTokenQuantity quantity,
                               BREthereumTokenQuantityUnit unit) {
    return TOKEN_QUANTITY_TYPE_INTEGER == unit
    ? uint256CoerceString(quantity.valueAsInteger, 10)
    : uint256CoerceStringDecimal(quantity.valueAsInteger, quantity.token->decimals);
}

extern BREthereumComparison
ethTokenQuantityCompare(BREthereumTokenQuantity q1, BREthereumTokenQuantity q2, int *typeMismatch) {
    *typeMismatch = (q1.token != q2.token);
    if (*typeMismatch) return ETHEREUM_COMPARISON_GT;
    switch (uint256Compare(q1.valueAsInteger, q2.valueAsInteger)) {
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
