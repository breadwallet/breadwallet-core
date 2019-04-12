/*
 * Network
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;
import com.breadwallet.crypto.ethereum.Ethereum;

public class Network {
    enum Type { Bitcoin, Bitcash, Ethereum }

    public static final class Bitcoin {
        final String name;
        final int forkId;
        // chainParams

        public Bitcoin(String name, int forkId) {
            this.name = name;
            this.forkId = forkId;
        }
    }

    public static final class Bitcash {
        // as above

    }

    public static final class Ethereum {
        final String name;
        final int chainId;
        // BREthereumNetwork

        public Ethereum(String name, int chainId) {
            this.name = name;
            this.chainId = chainId;
        }
    }

    public final Currency currency;

    private final Type type;
    private final Bitcoin bitcoin;
    private final Bitcash bitcash;
    private final Ethereum ethereum;

    public Network(Type type, Bitcoin bitcoin, Bitcash bitcash, Ethereum ethereum, Currency currency) {
        this.type = type;
        this.bitcoin = bitcoin;
        this.bitcash = bitcash;
        this.ethereum = ethereum;
        this.currency = currency;
    }

    public Network (Bitcoin bitcoin) {
        this (Type.Bitcoin, bitcoin, null, null, null);
    }

    public Network (Bitcash bitcash) {
        this (Type.Bitcash, null, bitcash, null, null);
    }

    public Network (Ethereum ethereum) {
        this (Type.Ethereum, null, null, ethereum,
                com.breadwallet.crypto.ethereum.Ethereum.currency);
    }
}
