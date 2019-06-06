/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.events.walletmanager;

import com.google.common.primitives.UnsignedLong;

public final class WalletManagerBlockUpdatedEvent implements WalletManagerEvent {

    private final UnsignedLong height;

    public WalletManagerBlockUpdatedEvent(UnsignedLong height) {
        this.height = height;
    }

    public UnsignedLong getHeight() {
        return height;
    }

    @Override
    public <T> T accept(WalletManagerEventVisitor<T> visitor) {
        return visitor.visit(this);
    }
}
