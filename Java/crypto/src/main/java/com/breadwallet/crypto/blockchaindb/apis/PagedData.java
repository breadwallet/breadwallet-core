package com.breadwallet.crypto.blockchaindb.apis;

import android.support.annotation.Nullable;
import com.google.common.base.Optional;

import java.util.List;

public final class PagedData<T> {

    private final List<T> data;
    private final @Nullable String prevUrl;
    private final @Nullable String nextUrl;

    public PagedData(List<T> data, @Nullable String prevUrl, @Nullable String nextUrl) {
        this.data = data;
        this.prevUrl = prevUrl;
        this.nextUrl = nextUrl;
    }

    public List<T> getData() {
        return data;
    }

    public Optional<String> getPrevUrl() {
        return Optional.fromNullable(prevUrl);
    }

    public Optional<String> getNextUrl() {
        return Optional.fromNullable(nextUrl);
    }
}
