/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models.bdb;

import com.breadwallet.crypto.blockchaindb.models.Utilities;
import com.google.common.base.Optional;
import com.google.common.collect.ImmutableMap;
import com.google.common.primitives.UnsignedLong;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

public class BlockchainFee {

    public static Optional<BlockchainFee> asBlockchainFee(JSONObject json) {
        try {
            json.getJSONObject("fee").getString("currency_id");
            String amount = json.getJSONObject("fee").getString("amount");
            String tier = json.getString("tier");
            UnsignedLong confirmationTime = Utilities.getUnsignedLongFromString(json, "estimated_confirmation_in");

            return Optional.of(new BlockchainFee(amount, tier, confirmationTime));

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

    private final String amount;
    private final String tier;
    private final UnsignedLong comfirmationTimeInMilliseconds;

    public BlockchainFee(String amount, String tier, UnsignedLong confirmationTimeInMilliseconds) {
        this.amount = amount;
        this.tier = tier;
        this.comfirmationTimeInMilliseconds = confirmationTimeInMilliseconds;
    }

    public String getAmount() {
        return amount;
    }

    public String getTier() {
        return tier;
    }

    public UnsignedLong getConfirmationTimeInMilliseconds() {
        return comfirmationTimeInMilliseconds;
    }
}
