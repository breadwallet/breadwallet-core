/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.events.system;

import android.support.annotation.Nullable;

public abstract class DefaultSystemEventVisitor<T> implements SystemEventVisitor<T> {

    @Nullable
    public T visit(SystemCreatedEvent event) {
        return null;
    }

    @Nullable
    public T visit(SystemManagerAddedEvent event) {
        return null;
    }

    @Nullable
    public T visit(SystemNetworkAddedEvent event) {
        return null;
    }

    @Nullable
    public T visit(SystemDiscoveredNetworksEvent event) {
        return null;
    }
}
