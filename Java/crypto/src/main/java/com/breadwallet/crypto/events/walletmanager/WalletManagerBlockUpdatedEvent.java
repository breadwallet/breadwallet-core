/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.events.walletmanager;

import com.breadwallet.crypto.Transfer;
import com.google.common.primitives.UnsignedLong;

/**
 * An event capturing a change in the block height of the network associated with a
 * {@link com.breadwallet.crypto.WalletManager WalletManager}.
 *
 * Developers should listen for this event when making use of
 * {@link Transfer#getConfirmations() Transfer::getConfirmations()}, as that value is calculated based on the
 * associated network's block height. Displays or caches of that confirmation count should be updated when this
 * event occurs.
 */
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

    @Override
    public String toString() {
        return "WalletManagerBlockUpdatedEvent{" +
                "height=" + height +
                '}';
    }
}
