package com.breadwallet.crypto.blockchaindb;

import android.support.annotation.Nullable;
import android.util.Base64;

import com.breadwallet.crypto.blockchaindb.models.Blockchain;
import com.google.common.base.Optional;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.text.DateFormat;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

// TODO: Do we want this class here?
public final class JsonUtilities {

    private static DateFormat ISO_8601_FORMAT = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSSZ");

    public static Optional<byte[]> getOptionalBase64Bytes(JSONObject json, String name) {
        try {
            return Optional.fromNullable(Base64.decode(json.optString(name, null), Base64.DEFAULT));
        } catch (IllegalArgumentException e) {
            return Optional.absent();
        }
    }

    public static Date get8601DateFromString(JSONObject json, String name) throws JSONException {
        try {
            return ISO_8601_FORMAT.parse(json.getString(name));
        } catch (ParseException e) {
            throw new JSONException("Invalid date value for " + name);
        }
    }

    public static long getLongFromString(JSONObject json, String name) throws JSONException {
        try {
            return Long.parseLong(json.getString(name));
        } catch (NumberFormatException e) {
            throw new JSONException("Invalid long value for " + name);
        }
    }

    public static Optional<Long> getOptionalLongFromString(JSONObject json, String name) {
        try {
            return Optional.fromNullable(Long.parseLong(json.optString(name, null)));
        } catch (NumberFormatException e) {
            return Optional.absent();
        }
    }

    public static Optional<List<String>> getOptionalStringList(JSONObject json, String name) {
        JSONArray jsonArray = json.optJSONArray(name);
        if (jsonArray == null) {
            return Optional.absent();
        }

        List<String> items = new ArrayList<>();
        for (int i = 0; i < jsonArray.length(); i++) {
            String object = jsonArray.optString(i, null);
            if (object == null) {
                return Optional.absent();
            }
        }
        return Optional.of(items);
    }

    public static Optional<List<Blockchain>> getOptionalBlockchainList(JSONObject json, String name) {
        JSONArray jsonArray = json.optJSONArray(name);
        if (jsonArray == null) {
            return Optional.absent();
        }

        return Blockchain.asBlockchains(jsonArray);
    }
}
