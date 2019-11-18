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

public class NetworkAIT {

    @Test
    public void testNetworkBtc() {
        Currency btc = Currency.create("Bitcoin", "Bitcoin", "BTC", "native", null);

        Unit satoshi_btc = Unit.create(btc, "BTC-SAT", "Satoshi", "SAT");
        Unit btc_btc = Unit.create(btc, "BTC-BTC", "Bitcoin", "B", satoshi_btc, UnsignedInteger.valueOf(8));

        NetworkAssociation association = new NetworkAssociation(satoshi_btc, btc_btc, new HashSet<>(Arrays.asList(satoshi_btc, btc_btc)));

        Map<Currency, NetworkAssociation> associations = new HashMap<>();
        associations.put(btc, association);

        NetworkFee fee = NetworkFee.create(UnsignedLong.valueOf(30 * 1000), Amount.create(1000, satoshi_btc));
        List<NetworkFee> fees = Collections.singletonList(fee);

        Network network = Network.create("bitcoin-mainnet", "Bitcoin", true, btc, UnsignedLong.valueOf(100000), associations, fees, UnsignedInteger.valueOf(6));

        assertEquals(network.getUids(), "bitcoin-mainnet");
        assertEquals(network.getName(), "Bitcoin");
        assertTrue(network.isMainnet());
        assertEquals(network.getHeight(), UnsignedLong.valueOf(100000));

        network.setHeight(UnsignedLong.valueOf(2 * 100000));
        assertEquals(network.getHeight(), UnsignedLong.valueOf(2 * 100000));

        assertEquals(network.getCurrency(), btc);
        assertTrue(network.hasCurrency(btc));
        assertTrue(network.getCurrencyByCode("BTC").transform(input -> input.equals(btc)).or(false));
        assertFalse(network.getCurrencyByIssuer("btc").isPresent());

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
        Currency eth = Currency.create("Ethereum", "Ethereum", "ETH", "native", null);

        Unit wei_eth = Unit.create(eth, "ETH-WEI", "WEI", "wei");
        Unit gwei_eth = Unit.create(eth, "ETH-GWEI", "GWEI",  "gwei", wei_eth, UnsignedInteger.valueOf(9));
        Unit ether_eth = Unit.create(eth, "ETH-ETH", "ETHER", "E",    wei_eth, UnsignedInteger.valueOf(18));

        NetworkAssociation association_eth = new NetworkAssociation(wei_eth, ether_eth, new HashSet<>(Arrays.asList(wei_eth, gwei_eth, ether_eth)));

        Currency brd = Currency.create("BRD", "BRD Token", "brd", "erc20", "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6");

        Unit brdi_brd = Unit.create(brd, "BRD_Integer", "BRD Integer", "BRDI");
        Unit brd_brd  = Unit.create(brd, "BRD_Decimal", "BRD_Decimal", "BRD", brdi_brd, UnsignedInteger.valueOf(8));

        NetworkAssociation association_brd = new NetworkAssociation(brdi_brd, brd_brd, new HashSet<>(Arrays.asList(brdi_brd, brd_brd)));

        Currency btc = Currency.create("Bitcoin", "Bitcoin", "btc", "native", null);

        Map<Currency, NetworkAssociation> associations = new HashMap<>();
        associations.put(eth, association_eth);
        associations.put(brd, association_brd);

        NetworkFee fee1 = NetworkFee.create(UnsignedLong.valueOf(1000), Amount.create(2.0, gwei_eth));
        NetworkFee fee2 = NetworkFee.create(UnsignedLong.valueOf(500), Amount.create(3.0, gwei_eth));
        List<NetworkFee> fees = Arrays.asList(fee1, fee2);

        Network network = Network.create("ethereum-mainnet", "ethereum-name", true, eth, UnsignedLong.valueOf(100000), associations, fees, UnsignedInteger.valueOf(6));

        assertEquals("ethereum-name", network.toString());
        assertTrue(network.hasCurrency(eth));
        assertTrue(network.hasCurrency(brd));
        assertFalse(network.hasCurrency(btc));

        assertTrue(network.getCurrencyByCode("ETH").isPresent());
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
