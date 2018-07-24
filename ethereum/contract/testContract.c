//
//  testContract.c
//  CoreTests
//
//  Created by Ed Gamble on 7/23/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//

#include <stdio.h>
#include <assert.h>

#include "BREthereumContract.h"
#include "BREthereumToken.h"

static void
runTokenParseTests () {
    BRCoreParseStatus status = CORE_PARSE_OK;
    UInt256 value = createUInt256Parse ("5968770000000000000000", 10, &status);

    //  UInt256 valueParseInt = parseTokenQuantity("5968770000000000000000", TOKEN_QUANTITY_TYPE_INTEGER, 18, &error);
    //  UInt256 valueParseDec = parseTokenQuantity("5968770000000000000000", TOKEN_QUANTITY_TYPE_INTEGER, 18, &error);

    BREthereumToken token = tokenGet(0);
    BREthereumTokenQuantity valueInt = createTokenQuantityString(token, "5968770000000000000000", TOKEN_QUANTITY_TYPE_INTEGER, &status);
    assert (CORE_PARSE_OK == status && eqUInt256(value, valueInt.valueAsInteger));

    BREthereumTokenQuantity valueDec = createTokenQuantityString(token, "5968.77", TOKEN_QUANTITY_TYPE_DECIMAL, &status);
    assert (CORE_PARSE_OK == status && eqUInt256(valueInt.valueAsInteger, valueDec.valueAsInteger));
}

void runTokenLookupTests () {
    printf ("==== Token\n");

    BREthereumToken token;

    token = tokenLookup ("0x558ec3152e2eb2174905cd19aea4e34a23de9ad6"); // BRD
    assert (NULL != token);

#if defined (BITCOIN_DEBUG)
    token = tokenLookup("0x3efd578b271d034a69499e4a2d933c631d44b9ad"); // TST: mainnet
    assert (NULL != token);
#endif

    token = tokenLookup("0x86fa049857e0209aa7d9e616f7eb3b3b78ecfdb0"); // EOI
    assert (NULL != token);
}



extern void
runContractTests (void) {
    runTokenParseTests ();
    runTokenLookupTests();
}
