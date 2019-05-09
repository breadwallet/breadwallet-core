package com.breadwallet.crypto;

import com.google.common.base.Optional;

import org.junit.Test;
import static org.junit.Assert.*;

public class AddressAIT {

    @Test
    public void testAddressCreateAsEth() {
        Optional<Address> oe1 = Address.createAsEth("0xb0F225defEc7625C6B5E43126bdDE398bD90eF62");
        assertTrue(oe1.isPresent());
        Address e1 = oe1.get();
        assertEquals("0xb0F225defEc7625C6B5E43126bdDE398bD90eF62", e1.toString());

        Optional<Address> oe2 = Address.createAsEth("0xd3CFBA03Fc13dc01F0C67B88CBEbE776D8F3DE8f");
        assertTrue(oe2.isPresent());
        Address e2 = oe2.get();
        assertEquals("0xd3CFBA03Fc13dc01F0C67B88CBEbE776D8F3DE8f", e2.toString());

        Optional<Address> oe3 = Address.createAsEth("0xb0F225defEc7625C6B5E43126bdDE398bD90eF62");
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
    public void testAddressCreateAsBtc() {
        Optional<Address> ob1 = Address.createAsBtc("1CC3X2gu58d6wXUWMffpuzN9JAfTUWu4Kj");
        assertTrue(ob1.isPresent());
        Address b1 = ob1.get();
        assertEquals("1CC3X2gu58d6wXUWMffpuzN9JAfTUWu4Kj", b1.toString());

        // TODO: Expand coverage
    }

    @Test
    public void testAddressMixedEquals() {
        Address e1 = Address.createAsEth("0xb0F225defEc7625C6B5E43126bdDE398bD90eF62").get();
        Address b1 = Address.createAsBtc("1CC3X2gu58d6wXUWMffpuzN9JAfTUWu4Kj").get();

        assertNotEquals(e1, b1);
        assertNotEquals(b1, e1);
    }
}
