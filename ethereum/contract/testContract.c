//
//  testContract.c
//  CoreTests
//
//  Created by Ed Gamble on 7/23/18.
//  Copyright © 2018-2019 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <stdio.h>
#include <assert.h>

#include "ethereum/contract/BREthereumContract.h"
#include "ethereum/contract/BREthereumToken.h"

#if defined (BITCOIN_TESTNET)
const char *tokenBRDAddress = "0x7108ca7c4718efa810457f228305c9c71390931a"; // testnet
#else
const char *tokenBRDAddress = "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6"; // mainnet
#endif

#if defined (BITCOIN_DEBUG)
#  if defined (BITCOIN_TESTNET)
static const char *tokenTSTAddress = "0x722dd3f80bac40c951b51bdd28dd19d435762180"; // testnet,
#  else
static const char *tokenTSTAddress = "0x3efd578b271d034a69499e4a2d933c631d44b9ad"; // mainnet
#  endif
#endif

static const char *tokenEOSAddress = "0x86fa049857e0209aa7d9e616f7eb3b3b78ecfdb0";
static const char *tokenKNCAddress = "0xdd974d5c2e2928dea5f71b9825b8b646686bd200";

static BRSetOf(BREthereumToken) tokens;

extern BREthereumToken
tokenLookupTest (const char *address) {
    BREthereumAddress addr = addressCreate(address);
    return BRSetGet (tokens, &addr);
}

extern void
installTokensForTest (void) {
    static int needInstall = 1;
    if (!needInstall) return;
    needInstall = 0;
    
    BREthereumGas defaultGasLimit = gasCreate(TOKEN_BRD_DEFAULT_GAS_LIMIT);
    BREthereumGasPrice defaultGasPrice = gasPriceCreate(etherCreateNumber(TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64, WEI));

    tokens = tokenSetCreate(10);

    BREthereumToken token;

    token = tokenCreate (tokenBRDAddress,
                         "BRD",
                         "BRD Token",
                         "",
                         18,
                         defaultGasLimit,
                         defaultGasPrice);
    BRSetAdd (tokens, token);

#if defined (BITCOIN_DEBUG)
    token = tokenCreate (tokenTSTAddress,
                         "TST",
                         "Test Standard Token",
                         "TeST Standard Token (TST) for TeSTing (TST)",
                         18,
                         defaultGasLimit,
                         defaultGasPrice);
    BRSetAdd (tokens, token);

#endif
    token = tokenCreate (tokenEOSAddress,
                         "EOS",
                         "EOS Token",
                         "",
                         18,
                         defaultGasLimit,
                         defaultGasPrice);
    BRSetAdd (tokens, token);

    token = tokenCreate (tokenKNCAddress,
                         "KNC",
                         "KNC token",
                         "",
                         18,
                         defaultGasLimit,
                         defaultGasPrice);
    BRSetAdd (tokens, token);
}

static void
runTokenParseTests () {
    BRCoreParseStatus status = CORE_PARSE_OK;
    UInt256 value = createUInt256Parse ("5968770000000000000000", 10, &status);

    //  UInt256 valueParseInt = parseTokenQuantity("5968770000000000000000", TOKEN_QUANTITY_TYPE_INTEGER, 18, &error);
    //  UInt256 valueParseDec = parseTokenQuantity("5968770000000000000000", TOKEN_QUANTITY_TYPE_INTEGER, 18, &error);

    BREthereumAddress address;

    address = addressCreate (tokenBRDAddress);
    BREthereumToken token = BRSetGet (tokens, &address);
    BREthereumTokenQuantity valueInt = createTokenQuantityString(token, "5968770000000000000000", TOKEN_QUANTITY_TYPE_INTEGER, &status);
    assert (CORE_PARSE_OK == status && eqUInt256(value, valueInt.valueAsInteger));

    BREthereumTokenQuantity valueDec = createTokenQuantityString(token, "5968.77", TOKEN_QUANTITY_TYPE_DECIMAL, &status);
    assert (CORE_PARSE_OK == status && eqUInt256(valueInt.valueAsInteger, valueDec.valueAsInteger));
}

void runTokenLookupTests () {
    printf ("==== Token\n");

    BREthereumToken token;
    BREthereumAddress address;

    address = addressCreate (tokenBRDAddress);

    token = BRSetGet (tokens, &address); // BRD
    assert (NULL != token);

#if defined (BITCOIN_DEBUG)
    address = addressCreate (tokenTSTAddress);
    token = BRSetGet (tokens, &address); // TST
    assert (NULL != token);
#endif

#if !defined(BITCOIN_TESTNET)
    address = addressCreate ("0x86fa049857e0209aa7d9e616f7eb3b3b78ecfdb0");
    token = BRSetGet (tokens, &address); // EOI
    assert (NULL != token);
#endif
}



extern void
runContractTests (void) {
    installTokensForTest();
    runTokenParseTests ();
    runTokenLookupTests();
}
