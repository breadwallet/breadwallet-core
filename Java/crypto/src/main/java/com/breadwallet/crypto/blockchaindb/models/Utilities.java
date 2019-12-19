/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models;

import com.google.common.base.Optional;
import com.google.common.io.BaseEncoding;

public final class Utilities {

    public static Optional<byte[]> getOptionalBase64Bytes(String value) {
        if (null == value) {
            return Optional.absent();
        }
        try {
            return Optional.fromNullable(BaseEncoding.base64().decode(value));
        } catch (IllegalArgumentException e) {
            return Optional.absent();
        }
    }
}
