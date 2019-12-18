/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 10/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.google.common.primitives.UnsignedInteger;

import org.junit.Ignore;
import org.junit.Test;
import static org.junit.Assert.*;

@Ignore
public class UnitAIT {

    @Test
    public void testUnitBtc() {
        Currency btc = Currency.create("Bitcoin", "Bitcoin", "btc", "native", null);
        Currency eth = Currency.create("Ethereum", "Ethereum", "eth", "native", null);

        Unit satoshi_btc = Unit.create(btc, "BTC-SAT", "Satoshi", "SAT");
        assertEquals(satoshi_btc.getCurrency().getCode(), btc.getCode());
        assertEquals(satoshi_btc.getName(), "Satoshi");
        assertEquals(satoshi_btc.getSymbol(), "SAT");
        assertTrue(satoshi_btc.hasCurrency(btc));
        assertFalse(satoshi_btc.hasCurrency(eth));
        assertTrue(satoshi_btc.isCompatible(satoshi_btc));
        assertEquals(satoshi_btc.getCoreBRCryptoUnit(), satoshi_btc.getBase().getCoreBRCryptoUnit());

        Unit btc_btc = Unit.create(btc, "BTC-BTC", "Bitcoin", "B", satoshi_btc, UnsignedInteger.valueOf(8));
        assertEquals(btc_btc.getCurrency().getCode(), btc.getCode());
        assertTrue(satoshi_btc.isCompatible(btc_btc));
        assertTrue(btc_btc.isCompatible(satoshi_btc));
    }

    @Test
    public void testUnitEth() {
        Currency btc = Currency.create("Bitcoin", "Bitcoin", "btc", "native", null);
        Currency eth = Currency.create("Ethereum", "Ethereum", "eth", "native", null);

        Unit satoshi_btc = Unit.create(btc, "BTC-SAT", "Satoshi", "SAT");
        Unit btc_btc = Unit.create(btc, "BTC-BTC", "Bitcoin", "B", satoshi_btc, UnsignedInteger.valueOf(8));
        Unit wei_eth = Unit.create(eth, "ETH-WEI", "WEI", "wei");

        assertFalse(wei_eth.isCompatible(btc_btc));
        assertFalse(btc_btc.isCompatible(wei_eth));
    }
}