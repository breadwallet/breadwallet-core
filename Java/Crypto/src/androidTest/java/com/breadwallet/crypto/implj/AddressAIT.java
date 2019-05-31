package com.breadwallet.crypto.implj;

import com.breadwallet.crypto.Address;
import com.google.common.base.Optional;

import org.junit.Test;
import static org.junit.Assert.*;

public class AddressAIT {

    @Test
    public void testAddressCreateAsEth() {
        Optional<AddressImpl> oe1 = AddressImpl.createAsEth("0xb0F225defEc7625C6B5E43126bdDE398bD90eF62");
        assertTrue(oe1.isPresent());
        Address e1 = oe1.get();
        assertEquals("0xb0F225defEc7625C6B5E43126bdDE398bD90eF62", e1.toString());

        Optional<AddressImpl> oe2 = AddressImpl.createAsEth("0xd3CFBA03Fc13dc01F0C67B88CBEbE776D8F3DE8f");
        assertTrue(oe2.isPresent());
        Address e2 = oe2.get();
        assertEquals("0xd3CFBA03Fc13dc01F0C67B88CBEbE776D8F3DE8f", e2.toString());

        Optional<AddressImpl> oe3 = AddressImpl.createAsEth("0xb0F225defEc7625C6B5E43126bdDE398bD90eF62");
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
        Optional<AddressImpl> ob1 = AddressImpl.createAsBtc("mm7DDqVkFd35XcWecFipfTYM5dByBzn7nq");
        assertTrue(ob1.isPresent());
        Address b1 = ob1.get();
        assertEquals("mm7DDqVkFd35XcWecFipfTYM5dByBzn7nq", b1.toString());

        // TODO: Expand coverage
    }

    @Test
    public void testAddressMixedEquals() {
        Address e1 = AddressImpl.createAsEth("0xb0F225defEc7625C6B5E43126bdDE398bD90eF62").get();
        Address b1 = AddressImpl.createAsBtc("mm7DDqVkFd35XcWecFipfTYM5dByBzn7nq").get();

        assertNotEquals(e1, b1);
        assertNotEquals(b1, e1);
    }
}
