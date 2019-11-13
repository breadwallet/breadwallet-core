/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models.bdb;

import com.breadwallet.crypto.blockchaindb.models.Utilities;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedLong;

import java.util.Date;

public class Transaction {

    // fields

    @JsonProperty("transaction_id")
    private String transactionId;

    private String identifier;

    private String hash;

    @JsonProperty("blockchain_id")
    private String blockchainId;

    private Date timestamp;

    @JsonProperty("transfers")
    private TransferList transfersList;

    private UnsignedLong size;

    private Amount fee;

    private UnsignedLong confirmations;

    private UnsignedLong index;

    @JsonProperty("block_hash")
    private String blockHash;

    @JsonProperty("block_height")
    private UnsignedLong blockHeight;

    private String status;

    @JsonProperty("first_seen")
    private Date firstSeen;

    private String raw;

    private String proof;

    private UnsignedLong acknowledgements;

    // getters

    public String getId() {
        return transactionId;
    }

    public String getIdentifier() {
        return identifier;
    }

    public String getHash() {
        return hash;
    }

    public String getBlockchainId() {
        return blockchainId;
    }

    public Optional<Date> getTimestamp() {
        return Optional.fromNullable(timestamp);
    }

    public TransferList getTransfersList() {
        return transfersList;
    }

    public UnsignedLong getSize() {
        return size;
    }

    public Amount getFee() {
        return fee;
    }

    public UnsignedLong getConfirmations() {
        return confirmations;
    }

    public UnsignedLong getIndex() {
        return index;
    }

    public String getBlockHash() {
        return blockHash;
    }

    public Optional<UnsignedLong> getBlockHeight() {
        return Optional.fromNullable(blockHeight);
    }

    public String getStatus() {
        return status;
    }

    public Date getFirstSeen() {
        return firstSeen;
    }

    public Optional<byte[]> getRaw() {
        return Utilities.getOptionalBase64Bytes(raw);
    }

    public String getProof() {
        return proof;
    }

    public UnsignedLong getAcknowledgements() {
        return acknowledgements;
    }
}
