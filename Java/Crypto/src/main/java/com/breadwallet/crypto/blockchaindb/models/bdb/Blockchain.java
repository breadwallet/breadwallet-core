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
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedInteger;
import com.google.common.primitives.UnsignedLong;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

public class Blockchain {

    private static long BLOCK_HEIGHT_INTERNAL = -1;

    public static Optional<Blockchain> asBlockchain(JSONObject json) {
        try {
            String id = json.getString("id");
            String name = json.getString("name");
            String network = json.getString("network");
            boolean isMainnet = json.getBoolean("is_mainnet");
            String currency = json.getString("native_currency_id");
            UnsignedInteger confirmationsUntilFinal = Utilities.getUnsignedIntFromString(json, "confirmations_until_final");

            long blockHeightLong = json.getLong("block_height");
            UnsignedLong blockHeight = BLOCK_HEIGHT_INTERNAL == blockHeightLong ?
                    null : UnsignedLong.valueOf(blockHeightLong);

            JSONArray feeEstimatesJson = json.getJSONArray("fee_estimates");
            Optional<List<BlockchainFee>> feeEstimatesOption = BlockchainFee.asBlockchainFees(feeEstimatesJson);
            if (!feeEstimatesOption.isPresent()) return Optional.absent();
            List<BlockchainFee> feeEstimates = feeEstimatesOption.get();

            return Optional.of(
                    new Blockchain(
                            id,
                            name,
                            network,
                            isMainnet,
                            currency,
                            blockHeight,
                            feeEstimates,
                            confirmationsUntilFinal
                    )
            );

        } catch (JSONException | NumberFormatException e) {
            return Optional.absent();
        }
    }


    public static Optional<List<Blockchain>> asBlockchains(JSONArray json) {
        List<Blockchain> blockchains = new ArrayList<>();
        for (int i = 0; i < json.length(); i++) {
            JSONObject blockchainsObject = json.optJSONObject(i);
            if (blockchainsObject == null) {
                return Optional.absent();
            }

            Optional<Blockchain> optionalBlockchains = Blockchain.asBlockchain(blockchainsObject);
            if (!optionalBlockchains.isPresent()) {
                return Optional.absent();
            }

            blockchains.add(optionalBlockchains.get());
        }
        return Optional.of(blockchains);
    }

    private final String id;
    private final String name;
    private final String network;
    private final boolean isMainnet;
    private final String currency;
    private final List<BlockchainFee> feeEstimates;
    private final UnsignedInteger confirmationsUntilFinal;

    @Nullable
    private final UnsignedLong blockHeight;

    public Blockchain(String id,
                      String name,
                      String network,
                      boolean isMainnet,
                      String currency,
                      @Nullable UnsignedLong blockHeight,
                      List<BlockchainFee> feeEstimates,
                      UnsignedInteger confirmationsUntilFinal) {
        this.id = id;
        this.name = name;
        this.network = network;
        this.isMainnet = isMainnet;
        this.currency = currency;
        this.blockHeight = blockHeight;
        this.feeEstimates = feeEstimates;
        this.confirmationsUntilFinal = confirmationsUntilFinal;
    }

    public String getId() {
        return id;
    }

    public String getName() {
        return name;
    }

    public String getNetwork() {
        return network;
    }

    public boolean isMainnet() {
        return isMainnet;
    }

    public String getCurrency() {
        return currency;
    }

    public Optional<UnsignedLong> getBlockHeight() {
        return Optional.fromNullable(blockHeight);
    }

    public List<BlockchainFee> getFeeEstimates() {
        return feeEstimates;
    }

    public UnsignedInteger getConfirmationsUntilFinal() {
        return confirmationsUntilFinal;
    }
}
