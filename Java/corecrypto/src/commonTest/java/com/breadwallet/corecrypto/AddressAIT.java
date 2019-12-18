/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 10/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.breadwallet.crypto.AddressScheme;
import com.google.common.base.Optional;
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

import static org.junit.Assert.*;

@Ignore
public class AddressAIT {

    @Test
    public void testAddressCreateAsEthOnMainnet() {
        Network network = Network.findBuiltin("ethereum-mainnet").get();

        Optional<Address> oe1 = Address.create("0xb0F225defEc7625C6B5E43126bdDE398bD90eF62", network);
        assertTrue(oe1.isPresent());
        Address e1 = oe1.get();
        assertEquals("0xb0F225defEc7625C6B5E43126bdDE398bD90eF62", e1.toString());

        Optional<Address> oe2 = Address.create("0xd3CFBA03Fc13dc01F0C67B88CBEbE776D8F3DE8f", network);
        assertTrue(oe2.isPresent());
        Address e2 = oe2.get();
        assertEquals("0xd3CFBA03Fc13dc01F0C67B88CBEbE776D8F3DE8f", e2.toString());

        Optional<Address> oe3 = Address.create("0xb0F225defEc7625C6B5E43126bdDE398bD90eF62", network);
        assertTrue(oe3.isPresent());
        Address e3 = oe3.get();
        assertEquals("0xb0F225defEc7625C6B5E43126bdDE398bD90eF62", e3.toString());

        assertEquals(e1, e1);
        assertEquals(e1, e3);
        assertEquals(e3, e1);

        assertNotEquals(e1, e2);
        assertNotEquals(e2, e1);
    }

    @Test
    public void testAddressCreateAsBtcOnMainnet() {
        Network network = Network.findBuiltin("bitcoin-mainnet").get();

        Optional<Address> ob1 = Address.create("1CC3X2gu58d6wXUWMffpuzN9JAfTUWu4Kj", network);
        assertTrue(ob1.isPresent());
        Address b1 = ob1.get();
        assertEquals("1CC3X2gu58d6wXUWMffpuzN9JAfTUWu4Kj", b1.toString());

        assertFalse(Address.create("qp0k6fs6q2hzmpyps3vtwmpx80j9w0r0acmp8l6e9v", network).isPresent());
        assertFalse(Address.create("bitcoincash:qp0k6fs6q2hzmpyps3vtwmpx80j9w0r0acmp8l6e9v", network).isPresent());
    }

    @Test
    public void testAddressCreateAsBchOnMainnet() {
        Network network = Network.findBuiltin("bitcoincash-mainnet").get();

        Optional<Address> ob1 = Address.create("bitcoincash:qp0k6fs6q2hzmpyps3vtwmpx80j9w0r0acmp8l6e9v", network);
        assertTrue(ob1.isPresent());
        Address b1 = ob1.get();
        assertEquals("bitcoincash:qp0k6fs6q2hzmpyps3vtwmpx80j9w0r0acmp8l6e9v", b1.toString());

        ob1 = Address.create("qp0k6fs6q2hzmpyps3vtwmpx80j9w0r0acmp8l6e9v", network);
        assertTrue(ob1.isPresent());
        b1 = ob1.get();
        assertEquals("bitcoincash:qp0k6fs6q2hzmpyps3vtwmpx80j9w0r0acmp8l6e9v", b1.toString());

        assertFalse(Address.create("bc1qar0srrr7xfkvy5l643lydnw9re59gtzzwf5mdq", network).isPresent());
        assertFalse(Address.create("1BvBMSEYstWetqTFn5Au4m4GFg7xJaNVN2", network).isPresent());
    }

    @Test
    public void testAddressMixedEqualsOnMainnet() {
        //
        // BTC
        //
        Network network_btc = Network.findBuiltin("bitcoin-mainnet").get();

        //
        // ETH
        //
        Network network_eth = Network.findBuiltin("ethereum-mainnet").get();

        Address e1 = Address.create("0xb0F225defEc7625C6B5E43126bdDE398bD90eF62", network_eth).get();
        Address b1 = Address.create("1CC3X2gu58d6wXUWMffpuzN9JAfTUWu4Kj", network_btc).get();

        assertNotEquals(e1, b1);
        assertNotEquals(b1, e1);
    }

    @Test
    public void testAddressCreateAsEthOnTestnet() {
        Network network = Network.findBuiltin("ethereum-ropsten").get();

        Optional<Address> oe1 = Address.create("0xb0F225defEc7625C6B5E43126bdDE398bD90eF62", network);
        assertTrue(oe1.isPresent());
        Address e1 = oe1.get();
        assertEquals("0xb0F225defEc7625C6B5E43126bdDE398bD90eF62", e1.toString());

        Optional<Address> oe2 = Address.create("0xd3CFBA03Fc13dc01F0C67B88CBEbE776D8F3DE8f", network);
        assertTrue(oe2.isPresent());
        Address e2 = oe2.get();
        assertEquals("0xd3CFBA03Fc13dc01F0C67B88CBEbE776D8F3DE8f", e2.toString());

        Optional<Address> oe3 = Address.create("0xb0F225defEc7625C6B5E43126bdDE398bD90eF62", network);
        assertTrue(oe3.isPresent());
        Address e3 = oe3.get();
        assertEquals("0xb0F225defEc7625C6B5E43126bdDE398bD90eF62", e3.toString());

        assertEquals(e1, e1);
        assertEquals(e1, e3);
        assertEquals(e3, e1);

        assertNotEquals(e1, e2);
        assertNotEquals(e2, e1);
    }

    @Test
    public void testAddressCreateAsBtcOnTestnet() {
        Network network = Network.findBuiltin("bitcoin-testnet").get();

        Optional<Address> ob1 = Address.create("mm7DDqVkFd35XcWecFipfTYM5dByBzn7nq", network);
        assertTrue(ob1.isPresent());
        Address b1 = ob1.get();
        assertEquals("mm7DDqVkFd35XcWecFipfTYM5dByBzn7nq", b1.toString());

        // TODO: Expand coverage
    }

    @Test
    public void testAddressCreateAsBchOnTestnet() {
        Network network = Network.findBuiltin("bitcoincash-testnet").get();

        Optional<Address> ob1 = Address.create("bchtest:pr6m7j9njldwwzlg9v7v53unlr4jkmx6eyvwc0uz5t", network);
        assertTrue(ob1.isPresent());
        Address b1 = ob1.get();
        assertEquals("bchtest:pr6m7j9njldwwzlg9v7v53unlr4jkmx6eyvwc0uz5t", b1.toString());

        ob1 = Address.create("pr6m7j9njldwwzlg9v7v53unlr4jkmx6eyvwc0uz5t", network);
        assertTrue(ob1.isPresent());
        b1 = ob1.get();
        assertEquals("bchtest:pr6m7j9njldwwzlg9v7v53unlr4jkmx6eyvwc0uz5t", b1.toString());

        assertFalse(Address.create("bc1qar0srrr7xfkvy5l643lydnw9re59gtzzwf5mdq", network).isPresent());
    }

    @Test
    public void testAddressMixedEqualsOnTestnet() {
        //
        // BTC
        //
        Network network_btc = Network.findBuiltin("bitcoin-testnet").get();

        //
        // ETH
        //
        Network network_eth = Network.findBuiltin("ethereum-ropsten").get();

        Address e1 = Address.create("0xb0F225defEc7625C6B5E43126bdDE398bD90eF62", network_eth).get();
        Address b1 = Address.create("mm7DDqVkFd35XcWecFipfTYM5dByBzn7nq", network_btc).get();

        assertNotEquals(e1, b1);
        assertNotEquals(b1, e1);
    }

    @Test
    public void testAddressScheme() {
        assertEquals("BTC Legacy",  AddressScheme.BTC_LEGACY.toString());
        assertEquals("BTC Segwit",  AddressScheme.BTC_SEGWIT.toString());
        assertEquals("ETH Default", AddressScheme.ETH_DEFAULT.toString());
        assertEquals("GEN Default", AddressScheme.GEN_DEFAULT.toString());
    }
}
