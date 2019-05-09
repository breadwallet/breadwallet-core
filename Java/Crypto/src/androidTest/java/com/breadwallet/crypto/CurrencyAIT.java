package com.breadwallet.crypto;

import org.junit.Test;
import static org.junit.Assert.*;

public class CurrencyAIT {

    @Test
    public void testCurrencyBtc() {
        Currency btc = new Currency("Bitcoin", "Bitcoin", "BTC", "native");
        assertEquals(btc.getName(), "Bitcoin");
        assertEquals(btc.getCode(), "BTC");
        assertEquals(btc.getType(), "native");
    }

    @Test
    public void testCurrencyEth() {
        Currency eth = new Currency("Ethereum", "Ethereum", "ETH", "native");
        assertEquals(eth.getName(), "Ethereum");
        assertEquals(eth.getCode(), "ETH");
        assertEquals(eth.getType(), "native");
    }

    @Test
    public void testCurrencyEquals() {
        Currency btc = new Currency("Bitcoin", "Bitcoin", "BTC", "native");
        Currency eth = new Currency("Ethereum", "Ethereum", "ETH", "native");

        assertNotEquals(btc.getName(), eth.getName());
        assertNotEquals(btc.getCode(), eth.getCode());
        assertEquals(btc.getType(), eth.getType());
    }
}
