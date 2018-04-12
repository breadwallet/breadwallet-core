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
     * Color Transition
     */
    char *colorLeft;
    char *colorRight;

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

extern const char *
tokenGetSymbol (BREthereumToken token) {
    return token->symbol;
}

extern const char *
tokenGetName (BREthereumToken token) {
    return token->name;
}

extern const char *
tokenGetDescription(BREthereumToken token) {
    return token->description;
}

extern int
tokenGetDecimals (BREthereumToken token) {
    return token->decimals;
}

extern BREthereumGas
tokenGetGasLimit (BREthereumToken token) {
    return token->gasLimit;
}


extern BREthereumGasPrice
tokenGetGasPrice (BREthereumToken token) {
    return token->gasPrice;
}

extern const char *
tokenGetColorLeft (BREthereumToken token) {
    return token->colorLeft;
}

extern const char *
tokenGetColorRight (BREthereumToken token) {
    return token->colorRight;
}

extern BREthereumContract
tokenGetContract (BREthereumToken token) {
    return contractERC20;
}

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

// var tf = function (tokens) { return 1; }
// {"code":"SNGLS","colors":["FAFAFA","FAFAFA"],"name":"Singular DTV","decimal":"0","address":"0xaeC2E87E0A235266D9C5ADc9DEb4b2E29b54D009"}
//var tokensInJSONToC = function (tokens) {
//    tokens.map (function (token) {
//        console.log("{");
//        console.log("    \"" + token.address + "\",");
//        console.log("    \"" + token.code + "\",");
//        console.log("    \"" + token.name + "\",");
//        console.log("    \"\",");
//        console.log("    " + token.decimal + ",");
//        console.log("    \"" + token.colors[0] + "\",");
//        console.log("    \"" + token.colors[1] + "\",");
//        console.log("    { TOKEN_BRD_DEFAULT_GAS_LIMIT },");
//        console.log("    { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},");
//        console.log("    1")
//        // colors
//        console.log("},");
//    });
//}

//
//
//
static struct BREthereumTokenRecord tokens[] =
{
    {   // BRD first... so we can find it.
        "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6",
        "BRD",
        "BRD Token",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0xE41d2489571d322189246DaFA5ebDe1F4699F498",
        "ZRX",
        "0x",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x4470BB87d77b963A013DB939BE332f927f2b992e",
        "ADX",
        "AdEx",
        "",
        4,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0xD0D6D6C5Fe4a677D343cC433536BB717bAe167dD",
        "ADT",
        "adToken",
        "",
        9,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0xbf2179859fc6d5bee9bf9158632dc51678a4100e",
        "ELF",
        "Aelf",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x5ca9a71b1d01849c0a95490cc00559717fcf0d1d",
        "AE",
        "Aeternity",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x4CEdA7906a5Ed2179785Cd3A40A69ee8bc99C466",
        "AION",
        "Aion",
        "",
        8,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x960b236A07cf122663c4303350609A66A7B288C0",
        "ANT",
        "Aragon",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0xb98d4c97425d9908e66e53a6fdf673acca0be986",
        "ABT",
        "Arcblock",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0xb98d4c97425d9908e66e53a6fdf673acca0be986",
        "ABT",
        "ArcBlock",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0xE94327D07Fc17907b4DB788E5aDf2ed424adDff6",
        "REP",
        "Augur",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x1F573D6Fb3F13d689FF844B4cE37794d79a7FF1C",
        "BNT",
        "Bancor",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x0D8775F648430679A709E98d2b0Cb6250d2887EF",
        "BAT",
        "Basic Attention",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0xb8c77482e45f1f44de1745f52c74426c631bdd52",
        "BNB",
        "Binance Coin",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x340d2bde5eb28c1eed91b2f790723e3b160613b7",
        "VEE",
        "BLOCKv",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0xcb97e65f07da24d46bcdd078ebebd7c6e6e3d750",
        "BTM",
        "Bytom",
        "",
        8,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x6531f133e6DeeBe7F2dcE5A0441aA7ef330B4e53",
        "TIME",
        "Chronobank",
        "",
        8,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0xd4c435f5b09f855c3317c8524cb1f586e42795fa",
        "CND",
        "Cindicator",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x41e5560054824ea6b0732e656e3ad64e20e94e45",
        "CVC",
        "Civic",
        "",
        8,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x12FEF5e57bF45873Cd9B62E9DBd7BFb99e32D73e",
        "CFI",
        "Cofound.it",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0xf85fEea2FdD81d51177F6b8F35F0e6734Ce45F5F",
        "CMT",
        "CyberMiles",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x0F5D2fB29fb7d3CFeE444a200298f468908cC942",
        "MANA",
        "Decentraland",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x3597bfd533a99c9aa083587b074434e61eb0a258",
        "DENT",
        "DENT",
        "",
        8,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0xE0B7927c4aF23765Cb51314A0E0521A9645F0E2A",
        "DGD",
        "DigixDaAO",
        "",
        9,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x0abdace70d3790235af448c88547603b945604ea",
        "DNT",
        "district0x",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x419c4db4b9e25d6db2ad9691ccb832c8d9fda05e",
        "DRGN",
        "Dragon",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x08711D3B02C8758F2FB3ab4e80228418a7F8e39c",
        "EDG",
        "Edgeless",
        "",
        0,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0xced4e93198734ddaff8492d525bd258d49eb388e",
        "EDO",
        "Eidoo",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0xF629cBd94d3791C9250152BD8dfBDF380E2a3B9c",
        "ENJ",
        "EnjinCoin",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x86fa049857e0209aa7d9e616f7eb3b3b78ecfdb0",
        "EOS",
        "EOS",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0xAf30D2a7E90d7DC361c8C4585e9BB7D2F6f15bc7",
        "1ST",
        "FirstBlood",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x419D0d8BdD9aF5e606Ae2232ed285Aff190E711b",
        "FUN",
        "FunFair",
        "",
        8,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x103c3A209da59d3E7C4A89307e66521e081CFDF0",
        "GVT",
        "Genesis Vision",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x6810e776880C02933D47DB1b9fc05908e5386b96",
        "GNO",
        "Gnosis",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0xa74476443119A942dE498590Fe1f2454d7D4aC0d",
        "GNT",
        "Golem",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0xb5a5f22694352c15b00323844ad545abb2b11028",
        "ICX",
        "ICON",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x888666CA69E0f178DED6D75b5726Cee99A87D698",
        "ICN",
        "ICONOMI",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x607F4C5BB672230e8672085532f7e901544a7375",
        "RLC",
        "iExec RLC",
        "",
        9,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x5e6b6d9abad9093fdc861ea1600eba1b355cd940",
        "ITC",
        "IOT Chain",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0xdd974D5C2e2928deA5F71b9825b8b646686BD200",
        "KNC",
        "Kyber Network",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0xa4e8c3ec456107ea67d3075bf9e3df3a75823db0",
        "LOOM",
        "Loom",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0xEF68e7C694F40c8202821eDF525dE3782458639f",
        "LRC",
        "Loopring",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0xfa05A73FfE78ef8f1a739473e462c54bae6567D9",
        "LUN",
        "Lunyr",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0xf7B098298f7C69Fc14610bf71d5e02c60792894C",
        "GUP",
        "Matchpool",
        "",
        3,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0xF433089366899D83a9f26A773D59ec7eCF30355e",
        "MTL",
        "Metal",
        "",
        8,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0xB63B606Ac810a52cCa15e44bB630fd42D8d1d83d",
        "MCO",
        "Monaco",
        "",
        8,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0xb91318f35bdb262e9423bc7c7c2a3a93dd93c92c",
        "NULS",
        "Nuls",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x1776e1F26f98b1A5dF9cD347953a26dd3Cb46671",
        "NMR",
        "Numeraire",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0xd26114cd6EE289AccF82350c8d8487fedB8A0C07",
        "OMG",
        "OmiseGO",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x2C4e8f2D746113d0696cE89B35F0d8bF88E0AEcA",
        "OST",
        "OST",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x1844b21593262668b7248d0f57a220caaba46ab9",
        "PRL",
        "Oyster Pearl",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x8Ae4BF2C33a8e667de34B54938B0ccD03Eb8CC06",
        "PTOY",
        "Patientory",
        "",
        8,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x0e0989b1f9b8a38983c2ba8053269ca62ec9b195",
        "POE",
        "Po.et",
        "",
        8,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x9992eC3cF6A55b00978cdDF2b27BC6882d88D1eC",
        "POLY",
        "Polymath",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0xd4fa1460F537bb9085d22C7bcCB5DD450Ef28e3a",
        "PPT",
        "Populous",
        "",
        8,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x595832f8fc6bf59c85c527fec3740a1b7a361269",
        "POWR",
        "Power Ledger",
        "",
        6,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x697beac28B09E122C4332D163985e8a73121b97F",
        "QRL",
        "QRL",
        "",
        8,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x99ea4dB9EE77ACD40B119BD1dC4E33e1C070b80d",
        "QSP",
        "Quantstamp",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x255aa6df07540cb5d3d297f0d0d4d84cb52bc8e6",
        "RDN",
        "Raiden Network",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x8f8221aFbB33998d8584A2B05749bA73c37a938a",
        "REQ",
        "Request",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0xf970b8e36e23f7fc3fd752eea86f8be8d83375a6",
        "RCN",
        "Ripio Credit",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x4156D3342D5c385a87D264F90653733592000581",
        "SALT",
        "SALT",
        "",
        8,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x7C5A0CE9267ED19B22F8cae653F198e3E8daf098",
        "SAN",
        "SAN",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0xaeC2E87E0A235266D9C5ADc9DEb4b2E29b54D009",
        "SNGLS",
        "Singular DTV",
        "",
        0,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x68d57c9a1C35f63E2c83eE8e49A64e9d70528D25",
        "SRN",
        "SIRIN",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x983F6d60db79ea8cA4eB9968C6aFf8cfA04B3c63",
        "SNM",
        "SONM",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x744d70FDBE2Ba4CF95131626614a1763DF805B9E",
        "SNT",
        "Status",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0xB64ef51C888972c908CFacf59B47C1AfBC0Ab8aC",
        "STORJ",
        "Storj",
        "",
        8,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0xD0a4b8946Cb52f0661273bfbC6fD0E0C75Fc6433",
        "STORM",
        "Storm",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x006BeA43Baa3f7A6f765F14f10A1a1b08334EF45",
        "STX",
        "Stox",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x12480E24eb5bec1a9D4369CaB6a80caD3c0A377A",
        "SUB",
        "SubContract",
        "",
        2,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x12480E24eb5bec1a9D4369CaB6a80caD3c0A377A",
        "SUB",
        "Substratum",
        "",
        2,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0xB97048628DB6B661D4C2aA833e95Dbe1A905B280",
        "PAY",
        "TenX",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x3883f5e181fccaf8410fa61e12b59bad963fb645",
        "THETA",
        "Theta Token",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0xf230b790e05390fc8295f4d3f60332c93bed42e2",
        "TRX",
        "TRON",
        "",
        6,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0xD850942eF8811f2A866692A623011bDE52a462C1",
        "VEN",
        "VeChain",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x2C974B2d0BA1716E644c1FC59982a89DDD2fF724",
        "VIB",
        "Viberate",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x39Bb259F66E1C59d5ABEF88375979b4D20D98022",
        "WAX",
        "WAX",
        "",
        8,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x667088b212ce3d06a1b553a7221E1fD19000d9aF",
        "WINGS",
        "Wings",
        "",
        18,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },
    {
        "0x05f4a42e251f2d52b8ed15E9FEdAacFcEF1FAD27",
        "ZIL",
        "Zilliqa",
        "",
        12,
        "FAFAFA",
        "FAFAFA",
        { TOKEN_BRD_DEFAULT_GAS_LIMIT },
        { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},
        1
    },

    // Terminator
    { NULL }
};

struct BREthereumTokenRecord tokenUNKRecord = {
    "0x722dd3f80bac40c951b51bdd28dd19d435762180", // "0x5250776FAD5A73707d222950de7999d3675a2722",
    "UNK",
    "Unknown Token",
    "An Unknown Token ...",
    4,                               // Decimals
    "FAFAFA",
    "FAFAFA",
    { TOKEN_BRD_DEFAULT_GAS_LIMIT },                        // Default gasLimit
    { { { .u64 = {TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, 0, 0, 0}}}},     // Default gasPrice
    1
};

const BREthereumToken tokenUNK = &tokenUNKRecord;
const BREthereumToken tokenBRD = &tokens[0];

extern BREthereumToken
tokenLookup (const char *address) {
    if (NULL != address && '\0' != address[0])
        for (int i = 0; NULL != tokens[i].address; i++)
            if (0 == strcasecmp (address, tokens[i].address))
                return &tokens[i];

    // Testnet TST Token
    if (0 == strcmp (address, tokenUNK->address)) return tokenUNK;
    return NULL;
}

