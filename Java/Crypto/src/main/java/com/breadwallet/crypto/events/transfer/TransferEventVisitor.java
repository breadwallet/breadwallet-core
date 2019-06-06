/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.events.transfer;

public interface TransferEventVisitor<T> {

    T visit(TransferChangedEvent event);

    T visit(TransferConfirmationEvent event);

    T visit(TransferCreatedEvent event);

    T visit(TransferDeletedEvent event);
}
