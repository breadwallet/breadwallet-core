package com.breadwallet.crypto;

import com.google.common.base.Optional;

import javax.annotation.Nullable;

public interface TransferAttribute {
    String getKey();

    Optional<String> getValue();

    void setValue(@Nullable String value);

    boolean isRequired();

    enum Error {
        REQUIRED_BUT_NOT_PROVIDED,
        MISMATCHED_TYPE,
        RELATIONSHIP_INCONSISTENCY
    }
}
