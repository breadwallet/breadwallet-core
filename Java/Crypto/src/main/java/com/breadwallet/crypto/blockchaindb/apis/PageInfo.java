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
