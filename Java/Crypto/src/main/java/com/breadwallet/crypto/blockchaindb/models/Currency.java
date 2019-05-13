package com.breadwallet.crypto.blockchaindb.models;

import android.support.annotation.Nullable;

import com.google.common.base.Optional;
import com.google.common.collect.ImmutableList;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

public class Currency {

    private static final String ADDRESS_BRD_MAINNET = "0x7108ca7c4718efa810457f228305c9c71390931a";
    private static final String ADDRESS_BRD_TESTNET = "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6";

    private static final String ADDRESS_EOS_MAINNET = "0x86fa049857e0209aa7d9e616f7eb3b3b78ecfdb0";

    public static final List<Currency> DEFAULT_CURRENCIES = ImmutableList.of(
            // Mainnet
            new Currency("Bitcoin", "Bitcoin", "btc", "native", "bitcoin-mainnet", null,
                    ImmutableList.of(CurrencyDenomination.BTC_SATOSHI, CurrencyDenomination.BTC_BITCOIN)),

            new Currency("Bitcash", "Bitcash", "bch", "native", "bitcash-mainnet", null,
                    ImmutableList.of(CurrencyDenomination.BTC_SATOSHI, CurrencyDenomination.BCH_BITCOIN)),

            new Currency("Ethereum", "Ethereum", "eth", "native", "ethereum-mainnet", null,
                    ImmutableList.of(CurrencyDenomination.ETH_WEI, CurrencyDenomination.ETH_GWEI,
                            CurrencyDenomination.ETH_ETHER)),

            new Currency("BRD Token", "BRD Token", "BRD", "erc20", "ethereum-mainnet", ADDRESS_BRD_MAINNET,
                    ImmutableList.of(CurrencyDenomination.BRD_INT, CurrencyDenomination.BRD_BRD)),

            new Currency("EOS Token", "EOS Token", "EOS", "erc20", "ethereum-mainnet", ADDRESS_EOS_MAINNET,
                    ImmutableList.of(CurrencyDenomination.EOS_INT, CurrencyDenomination.EOS_EOS)),

            // Testnet
            new Currency("Bitcoin-Testnet", "Bitcoin", "btc", "native", "bitcoin-testnet", null,
                    ImmutableList.of(CurrencyDenomination.BTC_SATOSHI, CurrencyDenomination.BTC_BITCOIN)),

            new Currency("Bitcash-Testnet", "Bitcash", "bch", "native", "bitcash-testnet", null,
                    ImmutableList.of(CurrencyDenomination.BTC_SATOSHI, CurrencyDenomination.BCH_BITCOIN)),

            new Currency("Ethereum-Testnet", "Ethereum", "eth", "native", "ethereum-testnet", null,
                    ImmutableList.of(CurrencyDenomination.ETH_WEI, CurrencyDenomination.ETH_GWEI,
                            CurrencyDenomination.ETH_ETHER)),

            new Currency("BRD Token Testnet", "BRD Token", "BRD", "erc20", "ethereum-testnet", ADDRESS_BRD_TESTNET,
                    ImmutableList.of(CurrencyDenomination.BRD_INT, CurrencyDenomination.BRD_BRD))
    );

    public static Optional<Currency> asCurrency(JSONObject json) {
        // optional
        String address = json.optString("address", null);

        // required
        try {
            String name = json.getString("name");
            String code = json.getString("code");
            String type = json.getString("type");
            String blockchainId = json.getString("blockchain_id");

            JSONArray jsonDenominations = json.getJSONArray("denominations");
            Optional<List<CurrencyDenomination>> optionalDenominations = CurrencyDenomination.asDenominations(jsonDenominations);
            if (!optionalDenominations.isPresent()) return Optional.absent();
            List<CurrencyDenomination> denominations = optionalDenominations.get();

            return Optional.of(new Currency(name, name, code, type, blockchainId, address, denominations));

        } catch (JSONException e) {
            return Optional.absent();
        }
    }

    public static Optional<List<Currency>> asCurrencies(JSONArray json){
        List<Currency> currencies = new ArrayList<>();
        for (int i = 0; i < json.length(); i++) {
            JSONObject currencyObject = json.optJSONObject(i);
            if (currencyObject == null) {
                return Optional.absent();
            }

            Optional<Currency> optionalCurrency = Currency.asCurrency(currencyObject);
            if (!optionalCurrency.isPresent()) {
                return Optional.absent();
            }

            currencies.add(optionalCurrency.get());
        }
        return Optional.of(currencies);
    }

    private final String id;
    private final String name;
    private final String code;
    private final String type;
    private final String blockchainID;
    private final List<CurrencyDenomination> denominations;

    @Nullable
    private final String address;

    public Currency(String id, String name, String code, String type, String blockchainID, @Nullable String address,
                    List<CurrencyDenomination> denominations) {
        this.id = id;
        this.name = name;
        this.code = code;
        this.type = type;
        this.blockchainID = blockchainID;
        this.address = address;
        this.denominations = denominations;
    }

    public String getId() {
        return id;
    }

    public String getName() {
        return name;
    }

    public String getCode() {
        return code;
    }

    public String getType() {
        return type;
    }

    public String getBlockchainID() {
        return blockchainID;
    }

    public Optional<String> getAddress() {
        return Optional.fromNullable(address);
    }

    public List<CurrencyDenomination> getDenominations() {
        return denominations;
    }
}
