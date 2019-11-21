/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models.bdb;

import android.support.annotation.Nullable;

import com.breadwallet.crypto.blockchaindb.models.Utilities;
import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedLong;

import java.util.Collections;
import java.util.Date;
import java.util.List;

import static com.google.common.base.Preconditions.checkNotNull;

public class Transaction {
    // creator

    @JsonCreator
    public static Transaction create(@JsonProperty("transaction_id") String transactionId,
                                     @JsonProperty("identifier") String identifier,
                                     @JsonProperty("hash") String hash,
                                     @JsonProperty("blockchain_id") String blockchainId,
                                     @JsonProperty("size") UnsignedLong size,
                                     @JsonProperty("fee") Amount fee,
                                     @JsonProperty("status") String status,
                                     @JsonProperty("_embedded") @Nullable Embedded embedded,
                                     @JsonProperty("first_seen") @Nullable Date firstSeen,
                                     @JsonProperty("timestamp") @Nullable Date timestamp,
                                     @JsonProperty("index") @Nullable UnsignedLong index,
                                     @JsonProperty("block_hash") @Nullable String blockHash,
                                     @JsonProperty("block_height") @Nullable UnsignedLong blockHeight,
                                     @JsonProperty("acknowledgements") @Nullable UnsignedLong acknowledgements,
                                     @JsonProperty("confirmations") @Nullable UnsignedLong confirmations,
                                     @JsonProperty("raw") @Nullable String raw,
                                     @JsonProperty("proof") @Nullable String proof) {
        return new Transaction(
                checkNotNull(transactionId),
                checkNotNull(identifier),
                checkNotNull(hash),
                checkNotNull(blockchainId),
                checkNotNull(size),
                checkNotNull(fee),
                checkNotNull(status),
                embedded,
                firstSeen,
                timestamp,
                index,
                blockHash,
                blockHeight,
                acknowledgements,
                confirmations,
                raw,
                proof
        );
    }

    // fields

    private final String transactionId;
    private final String identifier;
    private final String hash;
    private final String blockchainId;
    private final UnsignedLong size;
    private final Amount fee;
    private final String status;
    private final @Nullable Embedded embedded;
    private final @Nullable Date firstSeen;
    private final @Nullable Date timestamp;
    private final @Nullable UnsignedLong index;
    private final @Nullable String blockHash;
    private final @Nullable UnsignedLong blockHeight;
    private final @Nullable UnsignedLong acknowledgements;
    private final @Nullable UnsignedLong confirmations;
    private final @Nullable String raw;
    private final @Nullable String proof;

    private Transaction(String transactionId,
                        String identifier,
                        String hash,
                        String blockchainId,
                        UnsignedLong size,
                        Amount fee,
                        String status,
                        @Nullable Embedded embedded,
                        @Nullable Date firstSeen,
                        @Nullable Date timestamp,
                        @Nullable UnsignedLong index,
                        @Nullable String blockHash,
                        @Nullable UnsignedLong blockHeight,
                        @Nullable UnsignedLong acknowledgements,
                        @Nullable UnsignedLong confirmations,
                        @Nullable String raw,
                        @Nullable String proof) {
        this.transactionId = transactionId;
        this.identifier = identifier;
        this.hash = hash;
        this.blockchainId = blockchainId;
        this.size = size;
        this.fee = fee;
        this.status = status;
        this.embedded = embedded;
        this.firstSeen = firstSeen;
        this.timestamp = timestamp;
        this.index = index;
        this.blockHash = blockHash;
        this.blockHeight = blockHeight;
        this.acknowledgements = acknowledgements;
        this.confirmations = confirmations;
        this.raw = raw;
        this.proof = proof;
    }
    // getters

    @JsonProperty("transaction_id")
    public String getId() {
        return transactionId;
    }

    @JsonProperty("identifier")
    public String getIdentifier() {
        return identifier;
    }

    @JsonProperty("hash")
    public String getHash() {
        return hash;
    }

    @JsonProperty("blockchain_id")
    public String getBlockchainId() {
        return blockchainId;
    }

    @JsonProperty("size")
    public UnsignedLong getSize() {
        return size;
    }

    @JsonProperty("fee")
    public Amount getFee() {
        return fee;
    }

    @JsonProperty("status")
    public String getStatus() {
        return status;
    }

    @JsonProperty("first_seen")
    public Optional<Date> getFirstSeen() {
        return Optional.fromNullable(firstSeen);
    }

    @JsonProperty("timestamp")
    public Optional<Date> getTimestamp() {
        return Optional.fromNullable(timestamp);
    }

    @JsonProperty("index")
    public Optional<UnsignedLong> getIndex() {
        return Optional.fromNullable(index);
    }

    @JsonProperty("block_hash")
    public Optional<String> getBlockHash() {
        return Optional.fromNullable(blockHash);
    }

    @JsonProperty("block_height")
    public Optional<UnsignedLong> getBlockHeight() {
        return Optional.fromNullable(blockHeight);
    }

    @JsonProperty("acknowledgements")
    public Optional<UnsignedLong> getAcknowledgements() {
        return Optional.fromNullable(acknowledgements);
    }

    @JsonProperty("confirmations")
    public Optional<UnsignedLong> getConfirmations() {
        return Optional.fromNullable(confirmations);
    }

    @JsonProperty("raw")
    public Optional<String> getRawValue() {
        return Optional.fromNullable(raw);
    }

    @JsonIgnore
    public Optional<byte[]> getRaw() {
        return Utilities.getOptionalBase64Bytes(raw);
    }

    @JsonProperty("proof")
    public Optional<String> getProof() {
        return Optional.fromNullable(proof);
    }

    @JsonProperty("_embedded")
    public Optional<Embedded> getEmbedded() {
        return Optional.fromNullable(embedded);
    }

    @JsonIgnore
    public List<Transfer> getTransfers() {
        return embedded == null ? Collections.emptyList() : embedded.transfers;
    }

    // internal details

    public static class Embedded {

        @JsonProperty
        public List<Transfer> transfers;
    }
}
