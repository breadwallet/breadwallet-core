package com.breadwallet.corecrypto;

import org.junit.Test;
import static org.junit.Assert.*;

public class CurrencyAIT {

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
}
