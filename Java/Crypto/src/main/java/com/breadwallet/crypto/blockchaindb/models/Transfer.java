package com.breadwallet.crypto.blockchaindb.models;

import android.support.annotation.Nullable;

import com.breadwallet.crypto.blockchaindb.JsonUtilities;
import com.google.common.base.Optional;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

public class Transfer {

    public static Optional<Transfer> asTransfer(JSONObject json) {
        // optional
        String source = json.optString("from_address", null);
        String target = json.optString("to_address", null);
        String tid = json.optString("transaction_id", null);

        //required
        try {
            String id = json.getString("transfer_id");
            String bid = json.getString("blockchain_id");
            long acknowledgements = JsonUtilities.getLongFromString(json, "acknowledgements");
            long index = JsonUtilities.getLongFromString(json, "index");
            JSONObject amount = json.getJSONObject("amount");
            String value = amount.getString("amount");
            String currency = amount.getString("currency_id");
            return Optional.of(new Transfer(id, source, target, value, currency, acknowledgements, index, tid, bid));

        } catch (JSONException | NumberFormatException e) {
            return Optional.absent();
        }
    }

    public static Optional<List<Transfer>> asTransfers(JSONArray json) {
        List<Transfer> transfers = new ArrayList<>();
        for (int i = 0; i < json.length(); i++) {
            JSONObject transfersObject = json.optJSONObject(i);
            if (transfersObject == null) {
                return Optional.absent();
            }

            Optional<Transfer> optionalTransfer = Transfer.asTransfer(transfersObject);
            if (!optionalTransfer.isPresent()) {
                return Optional.absent();
            }

            transfers.add(optionalTransfer.get());
        }
        return Optional.of(transfers);
    }

    private final String id;
    private final String amountValue;
    private final String amountCurrency;
    private final long acknowledgements;
    private final long index;
    private final String blockchainId;

    @Nullable
    private final String transactionId;
    @Nullable
    private final String source;
    @Nullable
    private final String target;

    public Transfer(String id, @Nullable String source, @Nullable String target, String amountValue,
                    String amountCurrency, long acknowledgements, long index, @Nullable String transactionId,
                    String blockchainId) {
        this.id = id;
        this.source = source;
        this.target = target;
        this.amountValue = amountValue;
        this.amountCurrency = amountCurrency;
        this.acknowledgements = acknowledgements;
        this.index = index;
        this.transactionId = transactionId;
        this.blockchainId = blockchainId;
    }

    public String getId() {
        return id;
    }

    public Optional<String> getSource() {
        return Optional.fromNullable(source);
    }

    public Optional<String> getTarget() {
        return Optional.fromNullable(target);
    }

    public String getAmountValue() {
        return amountValue;
    }

    public String getAmountCurrency() {
        return amountCurrency;
    }

    public long getAcknowledgements() {
        return acknowledgements;
    }

    public long getIndex() {
        return index;
    }

    public Optional<String> getTransactionId() {
        return Optional.fromNullable(transactionId);
    }

    public String getBlockchainId() {
        return blockchainId;
    }
}
