/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.events.transfer;

import com.google.common.primitives.UnsignedLong;

public final class TransferConfirmationEvent implements TranferEvent {

    private final UnsignedLong count;

    public TransferConfirmationEvent(UnsignedLong count) {
        this.count = count;
    }

    public UnsignedLong getCount() {
        return count;
    }

    @Override
    public <T> T accept(TransferEventVisitor<T> visitor) {
        return visitor.visit(this);
    }
}
