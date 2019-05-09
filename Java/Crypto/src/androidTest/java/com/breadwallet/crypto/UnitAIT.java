/*
 * UnitAIT
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

import org.junit.Test;
import static org.junit.Assert.*;

public class UnitAIT {

    @Test
    public void testUnitBtc() {
        Currency btc = new Currency("Bitcoin", "Bitcoin", "BTC", "native");
        Currency eth = new Currency("Ethereum", "Ethereum", "ETH", "native");

        Unit satoshi_btc = new Unit(btc, "BTC-SAT", "Satoshi", "SAT");
        assertEquals(satoshi_btc.getCurrency().getCode(), btc.getCode());
        assertEquals(satoshi_btc.getName(), "Satoshi");
        assertEquals(satoshi_btc.getSymbol(), "SAT");
        assertTrue(satoshi_btc.hasCurrency(btc));
        assertFalse(satoshi_btc.hasCurrency(eth));
        assertTrue(satoshi_btc.isCompatible(satoshi_btc));
        assertEquals(satoshi_btc.core, satoshi_btc.getBase().core);

        Unit btc_btc = new Unit(btc, "BTC-BTC", "Bitcoin", "B", satoshi_btc, (byte) 8);
        assertEquals(btc_btc.getCurrency().getCode(), btc.getCode());
        assertTrue(satoshi_btc.isCompatible(btc_btc));
        assertTrue(btc_btc.isCompatible(satoshi_btc));
    }

    @Test
    public void testUnitEth() {
        Currency btc = new Currency("Bitcoin", "Bitcoin", "BTC", "native");
        Currency eth = new Currency("Ethereum", "Ethereum", "ETH", "native");

        Unit satoshi_btc = new Unit(btc, "BTC-SAT", "Satoshi", "SAT");
        Unit btc_btc = new Unit(btc, "BTC-BTC", "Bitcoin", "B", satoshi_btc, (byte) 8);
        Unit wei_eth = new Unit(eth, "ETH-WEI", "WEI", "wei");

        assertFalse(wei_eth.isCompatible(btc_btc));
        assertFalse(btc_btc.isCompatible(wei_eth));
    }
}