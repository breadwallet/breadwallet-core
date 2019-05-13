package com.breadwallet.crypto.blockchaindb.models;

import com.google.common.base.Optional;
import com.google.common.collect.ImmutableMap;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

public class BlockchainFee {

    public static Optional<BlockchainFee> asBlockchainFee(JSONObject json) {
        String value = json.optString("value", "1");

        try {
            String amoung = json.getJSONObject("fee").getString("amount");
            String tier = json.getString("tier");

            return Optional.of(new BlockchainFee(amoung, tier, value));

        } catch (JSONException e) {
            return Optional.absent();
        }
    }

    public static Optional<List<BlockchainFee>> asBlockchainFees(JSONArray json) {
        List<BlockchainFee> blockchainFees = new ArrayList<>();
        for (int i = 0; i < json.length(); i++) {
            JSONObject blockchainFeeObject = json.optJSONObject(i);
            if (blockchainFeeObject == null) {
                return Optional.absent();
            }

            Optional<BlockchainFee> optionalBlockchainFee = BlockchainFee.asBlockchainFee(blockchainFeeObject);
            if (!optionalBlockchainFee.isPresent()) {
                return Optional.absent();
            }

            blockchainFees.add(optionalBlockchainFee.get());
        }
        return Optional.of(blockchainFees);
    }

    public static JSONObject asJson(BlockchainFee fee) {
        return new JSONObject(ImmutableMap.of(
                "amount", fee.amount,
                "tier", fee.tier,
                "confirmations", fee.confirmations
        ));
    }

    private final String amount;
    private final String tier;
    private final String confirmations;

    public BlockchainFee(String amount, String tier, String confirmations) {
        this.amount = amount;
        this.tier = tier;
        this.confirmations = confirmations;
    }
}
