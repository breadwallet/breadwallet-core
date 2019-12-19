/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.utility.Cookie;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;

import java.util.Arrays;
import java.util.List;

public class BRCryptoCWMClient extends Structure {

    public Pointer context;
    public BRCryptoCWMClientBtc btc;
    public BRCryptoCWMClientEth eth;
    public BRCryptoCWMClientGen gen;

    public BRCryptoCWMClient() {
        super();
    }

    protected List<String> getFieldOrder() {
        return Arrays.asList("context", "btc", "eth", "gen");
    }

    public BRCryptoCWMClient(Cookie context,
                             BRCryptoCWMClientBtc btc,
                             BRCryptoCWMClientEth eth,
                             BRCryptoCWMClientGen gen) {
        super();
        this.context = context.getPointer();
        this.btc = btc;
        this.eth = eth;
        this.gen = gen;
    }

    public BRCryptoCWMClient(Pointer peer) {
        super(peer);
    }

    public ByValue toByValue() {
        ByValue other = new ByValue();
        other.context = this.context;
        other.btc = this.btc.toByValue();
        other.eth = this.eth.toByValue();
        other.gen = this.gen.toByValue();
        return other;
    }

    public static class ByReference extends BRCryptoCWMClient implements Structure.ByReference {

    }

    public static class ByValue extends BRCryptoCWMClient implements Structure.ByValue {

    }
}
