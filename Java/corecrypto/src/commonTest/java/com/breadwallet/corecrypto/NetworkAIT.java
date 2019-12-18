/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 10/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.google.common.primitives.UnsignedInteger;
import com.google.common.primitives.UnsignedLong;

import org.junit.Ignore;
import org.junit.Test;

import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

@Ignore
public class NetworkAIT {

    @Test
    public void testNetworkBtc() {
        Currency btc = Currency.create("bitcoin-mainnet:__native__", "Bitcoin", "btc", "native", null);

        Unit satoshi_btc = Unit.create(btc, "sat", "Satoshi", "SAT");
        Unit btc_btc = Unit.create(btc, "btc", "Bitcoin", "B", satoshi_btc, UnsignedInteger.valueOf(8));

        Network network = Network.findBuiltin("bitcoin-mainnet").get();

        assertEquals(network.getUids(), "bitcoin-mainnet");
        assertEquals(network.getName(), "Bitcoin");
        assertTrue(network.isMainnet());

        UnsignedLong newheight = network.getHeight().times(UnsignedLong.valueOf(2));
        network.setHeight(newheight);
        assertEquals(network.getHeight(), newheight);

        assertEquals(network.getCurrency(), btc);
        assertTrue(network.hasCurrency(btc));
        assertTrue(network.getCurrencyByCode("btc").transform(input -> input.equals(btc)).or(false));
        assertFalse(network.getCurrencyByIssuer("btc").isPresent());

        assertTrue(network.baseUnitFor(btc).transform(c -> c.equals(satoshi_btc)).or(false));
        assertTrue(network.defaultUnitFor(btc).transform(c -> c.equals(btc_btc)).or(false));

        assertTrue(network.unitsFor(btc).transform(u -> u.size() == 2).or(false));
        assertTrue(network.hasUnitFor(btc, btc_btc).or(false));
        assertTrue(network.hasUnitFor(btc, satoshi_btc).or(false));

        Currency eth = Currency.create("ethereum-mainnet:__native__", "Ethereum", "eth", "native", null);

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
        Currency eth = Currency.create("ethereum-mainnet:__native__", "Ethereum", "ETH", "native", null);

        Unit wei_eth = Unit.create(eth, "wei", "WEI", "wei");
        Unit gwei_eth = Unit.create(eth, "gwei", "GWEI",  "gwei", wei_eth, UnsignedInteger.valueOf(9));
        Unit ether_eth = Unit.create(eth, "eth", "ETHER", "E",    wei_eth, UnsignedInteger.valueOf(18));

        Currency brd = Currency.create("ethereum-mainnet:0x558ec3152e2eb2174905cd19aea4e34a23de9ad6", "BRD Token", "brd", "erc20", "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6");
        Currency btc = Currency.create("bitcoin-mainnet:__native__", "Bitcoin", "btc", "native", null);

        NetworkFee fee1 = NetworkFee.create(UnsignedLong.valueOf(1 * 60 * 1000), Amount.create(2.0, gwei_eth));

        Network network = Network.findBuiltin("ethereum-mainnet").get();

        assertEquals("Ethereum", network.toString());
        assertTrue(network.hasCurrency(eth));
        assertTrue(network.hasCurrency(brd));
        assertFalse(network.hasCurrency(btc));

        assertTrue(network.getCurrencyByCode("eth").isPresent());
        assertTrue(network.getCurrencyByCode("brd").isPresent());

        assertTrue(network.getCurrencyByIssuer("0x558ec3152e2eb2174905cd19aea4e34a23de9ad6").isPresent());
        assertTrue(network.getCurrencyByIssuer("0x558ec3152e2eb2174905cd19aea4e34a23de9ad6".toUpperCase()).isPresent());
        assertFalse(network.getCurrencyByIssuer("foo").isPresent());

        assertTrue(network.hasUnitFor(eth, wei_eth).or(false));
        assertTrue(network.hasUnitFor(eth, gwei_eth).or(false));
        assertTrue(network.hasUnitFor(eth, ether_eth).or(false));

        assertEquals(fee1, network.getMinimumFee());

        assertFalse(network.defaultUnitFor(btc).isPresent());
        assertFalse(network.baseUnitFor(btc).isPresent());
    }
}
