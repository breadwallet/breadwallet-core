/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models.bdb;

import android.support.annotation.Nullable;

import com.breadwallet.crypto.blockchaindb.models.Utilities;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedLong;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

public class Transaction {

    public static Optional<Transaction> asTransaction(JSONObject json) {
        // optional
        UnsignedLong index = Utilities.getOptionalUnsignedLongFromString(json, "index").orNull();
        UnsignedLong blockHeight = Utilities.getOptionalUnsignedLongFromString(json, "block_height").orNull();
        String blockHash = json.optString("block_hash", null);
        byte[] raw = Utilities.getOptionalBase64Bytes(json, "raw").orNull();

        Date firstSeen = Utilities.getOptional8601DateFromString(json, "first_seen").orNull();
        Date timestamp = Utilities.getOptional8601DateFromString(json, "timestamp").orNull();
        UnsignedLong acks = Utilities.getOptionalUnsignedLongFromString(json, "acknowledgements").or(UnsignedLong.ZERO);
        UnsignedLong confirmations = Utilities.getOptionalUnsignedLongFromString(json, "confirmations").orNull();

        //required
        try {
            String id = json.getString("transaction_id");
            String bid = json.getString("blockchain_id");
            String hash = json.getString("hash");
            String identifier = json.getString("identifier");
            String status = json.getString("status");
            UnsignedLong size = Utilities.getUnsignedLongFromString(json, "size");

            JSONArray jsonTransfers = json.getJSONArray("transfers");
            Optional<List<Transfer>> optionalTransfers = Transfer.asTransfers(jsonTransfers);
            if (!optionalTransfers.isPresent()) return Optional.absent();
            List<Transfer> transfers = optionalTransfers.get();

            return Optional.of(new Transaction(id, bid, hash, identifier, blockHash, blockHeight, index,
                    confirmations, status, size, timestamp, firstSeen, raw,
                    transfers, acks));

        } catch (JSONException | NumberFormatException e) {
            return Optional.absent();
        }
    }

    public static Optional<List<Transaction>> asTransactions(JSONArray json) {
        List<Transaction> transactions = new ArrayList<>();
        for (int i = 0; i < json.length(); i++) {
            JSONObject transacationsObject = json.optJSONObject(i);
            if (transacationsObject == null) {
                return Optional.absent();
            }

            Optional<Transaction> optionalTransacation = Transaction.asTransaction(transacationsObject);
            if (!optionalTransacation.isPresent()) {
                return Optional.absent();
            }

            transactions.add(optionalTransacation.get());
        }
        return Optional.of(transactions);
    }

    private final String id;
    private final String blockchainId;
    private final String hash;
    private final String identifier;
    private final String status;
    private final UnsignedLong size;
    private final List<Transfer> transfers;
    private final UnsignedLong acknowledgements;

    @Nullable
    private final Date firstSeen;
    @Nullable
    private final String blockHash;
    @Nullable
    private final UnsignedLong blockHeight;
    @Nullable
    private final UnsignedLong index;
    @Nullable
    private final Date timestamp;
    @Nullable
    private final byte[] raw;
    @Nullable
    private final UnsignedLong confirmations;

    public Transaction(String id, String blockchainId, String hash, String identifier, @Nullable String blockHash,
                       @Nullable UnsignedLong blockHeight, @Nullable UnsignedLong index, @Nullable UnsignedLong confirmations, String status,
                       UnsignedLong size, @Nullable Date timestamp, @Nullable Date firstSeen, @Nullable byte[] raw,
                       List<Transfer> transfers, UnsignedLong acknowledgements) {
        this.id = id;
        this.blockchainId = blockchainId;
        this.hash = hash;
        this.identifier = identifier;
        this.blockHash = blockHash;
        this.blockHeight = blockHeight;
        this.index = index;
        this.confirmations = confirmations;
        this.status = status;
        this.size = size;
        this.timestamp = timestamp;
        this.firstSeen = firstSeen;
        this.raw = raw;
        this.transfers = transfers;
        this.acknowledgements = acknowledgements;
    }

    public String getId() {
        return id;
    }

    public String getBlockchainId() {
        return blockchainId;
    }

    public String getHash() {
        return hash;
    }

    public String getIdentifier() {
        return identifier;
    }

    public Optional<String> getBlockHash() {
        return Optional.fromNullable(blockHash);
    }

    public Optional<UnsignedLong> getBlockHeight() {
        return Optional.fromNullable(blockHeight);
    }

    public Optional<UnsignedLong> getIndex() {
        return Optional.fromNullable(index);
    }

    public Optional<UnsignedLong> getConfirmations() {
        return Optional.fromNullable(confirmations);
    }

    public String getStatus() {
        return status;
    }

    public UnsignedLong getSize() {
        return size;
    }

    public Optional<Date> getTimestamp() {
        return Optional.fromNullable(timestamp);
    }

    public Optional<Date> getFirstSeen() {
        return Optional.fromNullable(firstSeen);
    }

    public Optional<byte[]> getRaw() {
        return Optional.fromNullable(raw);
    }

    public List<Transfer> getTransfers() {
        return transfers;
    }

    public UnsignedLong getAcknowledgements() {
        return acknowledgements;
    }
}
