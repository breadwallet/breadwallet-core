/*
 * BreadWallet
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.core;

/**
 *
 */
public class BRCoreAddress extends BRCoreJniReference {
    public static BRCoreAddress createAddress (String address) {
        return null == address || address.isEmpty()
                ? null
                : new BRCoreAddress (address);
    }

    public BRCoreAddress (String address) {
        this (createCoreAddress (address));
    }

    protected BRCoreAddress (long jniReferenceAddress) {
        super (jniReferenceAddress);
    }

    protected static native long createCoreAddress (String address);

    protected static native long createCoreAddressFromScriptPubKey (byte[] script);

    public static BRCoreAddress fromScriptPubKey (byte[] script) {
        return new BRCoreAddress (createCoreAddressFromScriptPubKey (script));
    }

    protected static native long createCoreAddressFromScriptSignature (byte[] script);

    public static BRCoreAddress fromScriptSignature (byte[] script) {
        return new BRCoreAddress (createCoreAddressFromScriptSignature (script));
    }

    public native String stringify ();

    public native boolean isValid ();

    public native byte[] getPubKeyScript();

    /**
     * Decode a bitcash address into a bitcoin address.
     *
     * @param bcashAddr the bitcash address
     * @return the bitcoin address or NULL if unable to decode
     */
    public static native String bcashDecodeBitcoin (String bcashAddr);

    /**
     * Encode a bitcash address from a bitcoin address.
     *
     * @param bitcoinAddr the bitcoin address
     * @return a bitcash address or NULL if unable to encode
     */
    public static native String bcashEncodeBitcoin (String bitcoinAddr);
}
