/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 9/19/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

import com.google.common.base.Optional;

public interface NetworkPeer {

    Network getNetwork();

    String getAddress();

    int getPort();

    Optional<String> getPublicKey();
}
