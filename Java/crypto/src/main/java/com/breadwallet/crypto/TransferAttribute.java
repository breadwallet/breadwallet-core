package com.breadwallet.crypto;

import com.google.common.base.Optional;

public interface TransferAttribute {
    String getKey();

    Optional<String> getValue();

    void setValue(Optional<String> value);

    boolean isRequired();

    enum Error {
        REQUIRED_BUT_NOT_PROVIDED,
        MISMATHED_TYPE,
        RELATIONSHIP_INCONSISTENCY
    }

}
