/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018-2019 Breadwinner AG.  All right reserved.
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

public class Block {

    public static Optional<Block> asBlock(JSONObject json) {
        // optional
        String header = json.optString("header", null);
        String prevHash = json.optString("prevHash", null);
        String nextHash = json.optString("nexthash", null);
        byte[] raw = Utilities.getOptionalBase64Bytes(json, "raw").orNull();

        JSONArray transactionsJson = json.optJSONArray("transactions");
        List<Transaction> transactions = transactionsJson == null ? null : Transaction.asTransactions(transactionsJson).orNull();

        //required
        try {
            String id = json.getString("block_id");
            String bid = json.getString("blockchain_id");
            String hash = json.getString("hash");
            UnsignedLong height = Utilities.getUnsignedLongFromString(json, "height");
            UnsignedLong size = Utilities.getUnsignedLongFromString(json, "size");
            UnsignedLong acks = Utilities.getUnsignedLongFromString(json, "acknowledgements");
            Date mined = Utilities.get8601DateFromString(json,"mined");

            return Optional.of(new Block(id, bid, hash, height, header, raw, mined, size, prevHash, nextHash,
                    transactions, acks));
        } catch (JSONException | NumberFormatException e) {
            return Optional.absent();
        }
    }

    public static Optional<List<Block>> asBlocks(JSONArray json) {
        List<Block> blocks = new ArrayList<>();
        for (int i = 0; i < json.length(); i++) {
            JSONObject blockObject = json.optJSONObject(i);
            if (blockObject == null) {
                return Optional.absent();
            }

            Optional<Block> optionalBlock = Block.asBlock(blockObject);
            if (!optionalBlock.isPresent()) {
                return Optional.absent();
            }

            blocks.add(optionalBlock.get());
        }
        return Optional.of(blocks);
    }

    private final String id;
    private final String blockchainId;
    private final String hash;
    private final UnsignedLong height;
    private final Date mined;
    private final UnsignedLong size;
    private final UnsignedLong ackknowledgements;

    @Nullable
    private final String header;
    @Nullable
    private final byte[] raw;
    @Nullable
    private final String prevHash;
    @Nullable
    private final String nextHash;
    @Nullable
    private final List<Transaction> transactions;

    public Block(String id, String blockchainId, String hash, UnsignedLong height, @Nullable String header,
                 @Nullable byte[] raw, Date mined, UnsignedLong size, @Nullable String prevHash, @Nullable String nextHash,
                 @Nullable List<Transaction> transactions, UnsignedLong ackknowledgements) {
        this.id = id;
        this.blockchainId = blockchainId;
        this.hash = hash;
        this.height = height;
        this.header = header;
        this.raw = raw;
        this.mined = mined;
        this.size = size;
        this.prevHash = prevHash;
        this.nextHash = nextHash;
        this.transactions = transactions;
        this.ackknowledgements = ackknowledgements;
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

    public UnsignedLong getHeight() {
        return height;
    }

    public Date getMined() {
        return mined;
    }

    public UnsignedLong getSize() {
        return size;
    }

    public UnsignedLong getAckknowledgements() {
        return ackknowledgements;
    }

    public Optional<String> getHeader() {
        return Optional.fromNullable(header);
    }

    public Optional<byte[]> getRaw() {
        return Optional.fromNullable(raw);
    }

    public Optional<String> getPrevHash() {
        return Optional.fromNullable(prevHash);
    }

    public Optional<String> getNextHash() {
        return Optional.fromNullable(nextHash);
    }

    public Optional<List<Transaction>> getTransactions() {
        return Optional.fromNullable(transactions);
    }
}
