/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 10/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.sun.jna.Pointer;
import com.sun.jna.Structure;

import java.util.Arrays;
import java.util.List;

public class BRCryptoWalletMigratorStatus extends Structure {

    // these values must be in sync with the enum values for BRCryptoWalletMigratorStatusType
    public static final int CRYPTO_WALLET_MIGRATOR_SUCCESS = 0;

    public int type;

    public BRCryptoWalletMigratorStatus() {
        super();
    }

    protected List<String> getFieldOrder() {
        return Arrays.asList("type");
    }

    public BRCryptoWalletMigratorStatus(int type) {
        super();
        this.type = type;
    }

    public BRCryptoWalletMigratorStatus(Pointer peer) {
        super(peer);
    }

    public static class ByReference extends BRCryptoWalletMigratorStatus implements Structure.ByReference {

    }

    public static class ByValue extends BRCryptoWalletMigratorStatus implements Structure.ByValue {

    }
}
