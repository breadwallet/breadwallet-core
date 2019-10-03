/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 10/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.migration;

import static com.google.common.base.Preconditions.checkArgument;

public final class BlockHash {

    public final byte[] bytes = new byte[256 / 8];

    public BlockHash(byte[] bytes) {
        checkArgument(bytes.length == this.bytes.length);
        System.arraycopy(bytes, 0, this.bytes, 0, this.bytes.length);
    }
}
