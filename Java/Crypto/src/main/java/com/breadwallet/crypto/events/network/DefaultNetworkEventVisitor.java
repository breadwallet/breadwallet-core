/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.events.network;

import android.support.annotation.Nullable;

public abstract class DefaultNetworkEventVisitor<T> implements NetworkEventVisitor<T> {

    @Nullable
    public T visit(NetworkCreatedEvent event) {
        return null;
    }

    @Nullable
    public T visit(NetworkFeesUpdatedEvent event) {
        return null;
    }
}
