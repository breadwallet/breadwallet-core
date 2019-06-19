package com.breadwallet.corecrypto;

import com.google.common.primitives.UnsignedInteger;
import com.google.common.primitives.UnsignedLong;

import org.junit.Test;

import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

public class NetworkAIT {

    @Test
    public void testNetworkBtc() {
        Currency btc = Currency.create("Bitcoin", "Bitcoin", "btc", "native", null);

        Unit satoshi_btc = Unit.create(btc, "BTC-SAT", "Satoshi", "SAT");
        Unit btc_btc = Unit.create(btc, "BTC-BTC", "Bitcoin", "B", satoshi_btc, UnsignedInteger.valueOf(8));

        NetworkAssociation association = new NetworkAssociation(satoshi_btc, btc_btc, new HashSet<>(Arrays.asList(satoshi_btc, btc_btc)));

        Map<Currency, NetworkAssociation> associations = new HashMap();
        associations.put(btc, association);

        Network network = Network.create("bitcoin-mainnet", "Bitcoin", true, btc, UnsignedLong.valueOf(100000), associations);

        assertEquals(network.getUids(), "bitcoin-mainnet");
        assertEquals(network.getName(), "Bitcoin");
        assertTrue(network.isMainnet());
        assertEquals(network.getHeight(), UnsignedLong.valueOf(100000));

        network.setHeight(UnsignedLong.valueOf(2 * 100000));
        assertEquals(network.getHeight(), UnsignedLong.valueOf(2 * 100000));

        assertEquals(network.getCurrency(), btc);
        assertTrue(network.hasCurrency(btc));
        assertTrue(network.getCurrencyByCode("btc").transform(input -> input.equals(btc)).or(false));

        assertTrue(network.baseUnitFor(btc).transform(c -> c.equals(satoshi_btc)).or(false));
        assertTrue(network.defaultUnitFor(btc).transform(c -> c.equals(btc_btc)).or(false));

        assertTrue(network.unitsFor(btc).transform(u -> u.size() == 2).or(false));
        assertTrue(network.hasUnitFor(btc, btc_btc).or(false));
        assertTrue(network.hasUnitFor(btc, satoshi_btc).or(false));

        Currency eth = Currency.create("Ethereum", "Ethereum", "eth", "native", null);

        Unit wei_eth = Unit.create(eth, "ETH-WEI", "WEI", "wei");

        assertFalse(network.hasCurrency(eth));
        assertFalse(network.baseUnitFor(eth).isPresent());
        assertFalse(network.unitsFor(eth).isPresent());

        assertFalse(network.hasUnitFor(eth, wei_eth).or(false));
        assertFalse(network.hasUnitFor(eth, btc_btc).or(false));
        assertFalse(network.hasUnitFor(btc, wei_eth).or(false));
    }

    @Test
    public void testNetworkEth() {
        Currency eth = Currency.create("Ethereum", "Ethereum", "eth", "native", null);

        Unit wei_eth = Unit.create(eth, "ETH-WEI", "WEI", "wei");
        Unit gwei_eth = Unit.create(eth, "ETH-GWEI", "GWEI",  "gwei", wei_eth, UnsignedInteger.valueOf(9));
        Unit ether_eth = Unit.create(eth, "ETH-ETH", "ETHER", "E",    wei_eth, UnsignedInteger.valueOf(18));

        Currency btc = Currency.create("Bitcoin", "Bitcoin", "btc", "native", null);

        NetworkAssociation association = new NetworkAssociation(wei_eth, ether_eth, new HashSet<>(Arrays.asList(wei_eth, gwei_eth, ether_eth)));

        Map<Currency, NetworkAssociation> associations = new HashMap();
        associations.put(eth, association);

        Network network = Network.create("ethereum-mainnet", "Ethereum", true, eth, UnsignedLong.valueOf(100000), associations);

        assertTrue(network.hasCurrency(eth));
        assertFalse(network.hasCurrency(btc));

        assertTrue(network.hasUnitFor(eth, wei_eth).or(false));
        assertTrue(network.hasUnitFor(eth, gwei_eth).or(false));
        assertTrue(network.hasUnitFor(eth, ether_eth).or(false));
    }
}
