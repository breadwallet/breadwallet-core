//
//  testBase.c
//  CoreTests
//
//  Created by Ed Gamble on 7/23/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//

#include <stdio.h>
#include <assert.h>
#include "BREthereumBase.h"

static void
runEtherParseTests () {
    BRCoreParseStatus status;
    BREthereumEther e;

    e = etherCreateString("1", WEI, &status);
    assert (CORE_PARSE_OK == status
            && ETHEREUM_BOOLEAN_TRUE == etherIsEQ(e, etherCreateNumber(1, WEI)));

    e = etherCreateString("100", WEI, &status);
    assert (CORE_PARSE_OK == status
            && ETHEREUM_BOOLEAN_TRUE == etherIsEQ(e, etherCreateNumber(100, WEI)));

    e = etherCreateString("100.0000", WEI, &status);
    assert (CORE_PARSE_OK == status
            && ETHEREUM_BOOLEAN_TRUE == etherIsEQ(e, etherCreateNumber(100, WEI)));

    e = etherCreateString("0.001", WEI+1, &status);
    assert (CORE_PARSE_OK == status
            && ETHEREUM_BOOLEAN_TRUE == etherIsEQ(e, etherCreateNumber(1, WEI)));

    e = etherCreateString("0.00100", WEI+1, &status);
    assert (CORE_PARSE_OK == status
            && ETHEREUM_BOOLEAN_TRUE == etherIsEQ(e, etherCreateNumber(1, WEI)));

    e = etherCreateString("0.001002", ETHER, &status);
    assert (CORE_PARSE_OK == status
            && ETHEREUM_BOOLEAN_TRUE == etherIsEQ(e, etherCreateNumber(1002, ETHER-2)));

    e = etherCreateString("12.03", ETHER, &status);
    assert (CORE_PARSE_OK == status
            && ETHEREUM_BOOLEAN_TRUE == etherIsEQ(e, etherCreateNumber(12030, ETHER-1)));

    e = etherCreateString("12.03", WEI, &status);
    //  assert (ETHEREUM_ETHER_PARSE_UNDERFLOW == status);
    assert (CORE_PARSE_OK != status);

    e = etherCreateString("100000000000000000000000000000000000000000000000000000000000000000000000000000000", WEI, &status);
    //  assert (ETHEREUM_ETHER_PARSE_OVERFLOW == status);
    assert (CORE_PARSE_OK != status);

    e = etherCreateString("1000000000000000000000", WEI, &status);
    assert (CORE_PARSE_OK == status
            && ETHEREUM_BOOLEAN_TRUE == etherIsEQ (e, etherCreateNumber(1, KETHER)));

    e = etherCreateString("2000000000000000000000.000000", WEI, &status);
    assert (CORE_PARSE_OK == status
            && ETHEREUM_BOOLEAN_TRUE == etherIsEQ (e, etherCreateNumber(2, KETHER)));

    char *s;
    e = etherCreateString("123", WEI, &status);
    s = etherGetValueString(e, WEI);
    assert (0 == strcmp (s, "123"));
    free (s);

    e = etherCreateString("1234", WEI, &status);
    s = etherGetValueString(e, WEI+1);
    assert (0 == strcmp (s, "1.234"));
    free (s);

    e = etherCreateString("123", WEI, &status);
    s = etherGetValueString(e, WEI+2);
    assert (0 == strcmp (s, "0.000123"));
    free (s);
}


extern void
runBaseTests () {
    runEtherParseTests();
}
