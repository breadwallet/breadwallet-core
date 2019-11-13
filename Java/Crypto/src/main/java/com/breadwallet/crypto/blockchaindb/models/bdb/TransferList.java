package com.breadwallet.crypto.blockchaindb.models.bdb;

import com.fasterxml.jackson.annotation.JsonProperty;

import java.util.Collections;
import java.util.List;

public class TransferList {

    // fields

    @JsonProperty("_embedded")
    private Embedded embedded;

    // getters

    public List<Transfer> getTransfers() {
        return embedded == null ? Collections.emptyList() : embedded.transfers;
    }

    // internals

    private static class Embedded {
        List<Transfer> transfers;
    }
}
