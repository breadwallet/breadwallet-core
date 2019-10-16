/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.apis;

import android.support.annotation.Nullable;

public final class PageInfo {
    public PageInfo(@Nullable String nextUrl,
                    @Nullable String prevUrl,
                    @Nullable String selfUrl) {
        this.nextUrl = nextUrl;
        this.prevUrl = prevUrl;
        this.selfUrl = selfUrl;
    }

    public final @Nullable String nextUrl;

    public final @Nullable String prevUrl;

    public final @Nullable String selfUrl;
}
