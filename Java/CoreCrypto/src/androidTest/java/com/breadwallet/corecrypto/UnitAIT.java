/*
 * UnitAIT
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.breadwallet.crypto.Unit;
import com.google.common.primitives.UnsignedInteger;

import org.junit.Test;
import static org.junit.Assert.*;

public class UnitAIT {

    @Test
    public void testUnitBtc() {
        CurrencyImpl btc = new CurrencyImpl("Bitcoin", "Bitcoin", "BTC", "native");
        CurrencyImpl eth = new CurrencyImpl("Ethereum", "Ethereum", "ETH", "native");

        UnitImpl satoshi_btc = new UnitImpl(btc, "BTC-SAT", "Satoshi", "SAT");
        assertEquals(satoshi_btc.getCurrency().getCode(), btc.getCode());
        assertEquals(satoshi_btc.getName(), "Satoshi");
        assertEquals(satoshi_btc.getSymbol(), "SAT");
        assertTrue(satoshi_btc.hasCurrency(btc));
        assertFalse(satoshi_btc.hasCurrency(eth));
        assertTrue(satoshi_btc.isCompatible(satoshi_btc));
        assertEquals(satoshi_btc.getCoreBRCryptoUnit(), satoshi_btc.getBase().getCoreBRCryptoUnit());

        Unit btc_btc = new UnitImpl(btc, "BTC-BTC", "Bitcoin", "B", satoshi_btc, UnsignedInteger.valueOf(8));
        assertEquals(btc_btc.getCurrency().getCode(), btc.getCode());
        assertTrue(satoshi_btc.isCompatible(btc_btc));
        assertTrue(btc_btc.isCompatible(satoshi_btc));
    }

    @Test
    public void testUnitEth() {
        CurrencyImpl btc = new CurrencyImpl("Bitcoin", "Bitcoin", "BTC", "native");
        CurrencyImpl eth = new CurrencyImpl("Ethereum", "Ethereum", "ETH", "native");

        UnitImpl satoshi_btc = new UnitImpl(btc, "BTC-SAT", "Satoshi", "SAT");
        UnitImpl btc_btc = new UnitImpl(btc, "BTC-BTC", "Bitcoin", "B", satoshi_btc, UnsignedInteger.valueOf(8));
        UnitImpl wei_eth = new UnitImpl(eth, "ETH-WEI", "WEI", "wei");

        assertFalse(wei_eth.isCompatible(btc_btc));
        assertFalse(btc_btc.isCompatible(wei_eth));
    }
}