/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models.bdb;

import android.support.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedLong;

import java.util.Map;

import static com.google.common.base.Preconditions.checkNotNull;

public class Transfer {
    // creators

    @JsonCreator
    public static Transfer create(@JsonProperty("transfer_id") String transferId,
                                  @JsonProperty("blockchain_id") String blockchainId,
                                  @JsonProperty("index") UnsignedLong index,
                                  @JsonProperty("amount") Amount amount,
                                  @JsonProperty("meta") Map<String, String> meta,
                                  @JsonProperty("from_address") @Nullable String fromAddress,
                                  @JsonProperty("to_address") @Nullable String toAddress,
                                  @JsonProperty("transaction_id") @Nullable String transactionId,
                                  @JsonProperty("acknowledgements") @Nullable UnsignedLong acknowledgements) {
        return new Transfer(
                checkNotNull(transferId),
                checkNotNull(blockchainId),
                checkNotNull(index),
                checkNotNull(amount),
                checkNotNull(meta),
                fromAddress,
                toAddress,
                transactionId,
                acknowledgements
        );
    }

    // fields

    private final String transferId;
    private final String blockchainId;
    private final UnsignedLong index;
    private final Amount amount;
    private final Map<String, String> meta;
    private final @Nullable String fromAddress;
    private final @Nullable String toAddress;
    private final @Nullable String transactionId;
    private final @Nullable UnsignedLong acknowledgements;

    private Transfer(String transferId,
                     String blockchainId,
                     UnsignedLong index,
                     Amount amount,
                     Map<String, String> meta,
                     @Nullable String fromAddress,
                     @Nullable String toAddress,
                     @Nullable String transactionId,
                     @Nullable UnsignedLong acknowledgements) {
        this.transferId = transferId;
        this.blockchainId = blockchainId;
        this.index = index;
        this.amount = amount;
        this.meta = meta;
        this.fromAddress = fromAddress;
        this.toAddress = toAddress;
        this.transactionId = transactionId;
        this.acknowledgements = acknowledgements;
    }

    // getters

    @JsonProperty("transfer_id")
    public String getId() {
        return transferId;
    }

    @JsonProperty("blockchain_id")
    public String getBlockchainId() {
        return blockchainId;
    }

    @JsonProperty("index")
    public UnsignedLong getIndex() {
        return index;
    }

    @JsonProperty("amount")
    public Amount getAmount() {
        return amount;
    }

    @JsonProperty("meta")
    public Map<String, String> getMeta() {
        return meta;
    }

    @JsonProperty("from_address")
    public Optional<String> getFromAddress() {
        return Optional.fromNullable(fromAddress);
    }

    @JsonProperty("to_address")
    public Optional<String> getToAddress() {
        return Optional.fromNullable(toAddress);
    }

    @JsonProperty("transaction_id")
    public Optional<String> getTransactionId() {
        return Optional.fromNullable(transactionId);
    }

    @JsonProperty("acknowledgements")
    public Optional<UnsignedLong> getAcknowledgements() {
        return Optional.fromNullable(acknowledgements);
    }
}
