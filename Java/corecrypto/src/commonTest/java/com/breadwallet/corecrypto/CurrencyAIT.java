/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 10/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.breadwallet.crypto.CurrencyPair;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedInteger;

import org.junit.Before;
import org.junit.Ignore;
import org.junit.Test;
import static org.junit.Assert.*;

@Ignore
public class CurrencyAIT {

    @Before
    public void setup() {
        HelpersAIT.registerCryptoApiProvider();
    }

    @Test
    public void testCurrencyBtc() {
        Currency btc = Currency.create("Bitcoin", "Bitcoin", "btc", "native", null);
        assertEquals(btc.getName(), "Bitcoin");
        assertEquals(btc.getCode(), "btc");
        assertEquals(btc.getType(), "native");
    }

    @Test
    public void testCurrencyEth() {
        Currency eth = Currency.create("Ethereum", "Ethereum", "eth", "native", null);
        assertEquals(eth.getName(), "Ethereum");
        assertEquals(eth.getCode(), "eth");
        assertEquals(eth.getType(), "native");
    }

    @Test
    public void testCurrencyEquals() {
        Currency btc = Currency.create("Bitcoin", "Bitcoin", "btc", "native", null);
        Currency eth = Currency.create("Ethereum", "Ethereum", "eth", "native", null);

        assertNotEquals(btc.getName(), eth.getName());
        assertNotEquals(btc.getCode(), eth.getCode());
        assertEquals(btc.getType(), eth.getType());
    }

    @Test
    public void testCurrencyPair() {
        Currency btc = Currency.create("Bitcoin", "Bitcoin", "btc", "native", null);

        Unit satoshi_btc = Unit.create(btc, "BTC-SAT", "Satoshi", "SAT");
        Unit btc_btc = Unit.create(btc, "BTC-BTC", "Bitcoin", "B", satoshi_btc, UnsignedInteger.valueOf(8));

        Currency usd = Currency.create("USDollar", "USDollar", "usd", "fiat", null);

        Unit usd_cents = Unit.create(usd, "USD-Cents", "Cents", "c");
        Unit usd_dollar = Unit.create(usd, "USD-Dollar", "Dollars", "$", usd_cents, UnsignedInteger.valueOf(2));

        CurrencyPair pair = new CurrencyPair (btc_btc, usd_dollar, 10000);
        assertEquals("Bitcoin/Dollars=10000.0", pair.toString());

        // BTC -> USD
        Optional<com.breadwallet.crypto.Amount> oneBTCinUSD = pair.exchangeAsBase(Amount.create(1.0, btc_btc));
        assertTrue(oneBTCinUSD.isPresent());
        assertEquals (10000.0, oneBTCinUSD.get().doubleAmount(usd_dollar).get(), 1e-6);

        // USD -> BTC
        Optional<com.breadwallet.crypto.Amount> oneUSDinBTC = pair.exchangeAsQuote(Amount.create(1.0, usd_dollar));
        assertTrue(oneUSDinBTC.isPresent());
        assertEquals (1/10000.0, oneUSDinBTC.get().doubleAmount(btc_btc).get(), 1e-6);

        Amount oneBTC = Amount.create(1.0, btc_btc);
        assertEquals("$10,000.00", oneBTC.toStringFromPair(pair).get());
    }
}
