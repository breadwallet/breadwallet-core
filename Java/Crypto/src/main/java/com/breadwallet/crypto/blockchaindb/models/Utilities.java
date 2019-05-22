package com.breadwallet.crypto.blockchaindb.models;

import android.util.Base64;

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

public final class Utilities {

    private static DateFormat ISO_8601_FORMAT = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSSZ");

    public static Date get8601DateFromString(JSONObject json, String name) throws JSONException {
        try {
            return ISO_8601_FORMAT.parse(json.getString(name));
        } catch (ParseException e) {
            throw new JSONException("Invalid date value for " + name);
        }
    }

    public static Optional<Date> getOptional8601DateFromString(JSONObject json, String name) {
        try {
            return Optional.fromNullable(ISO_8601_FORMAT.parse(json.optString(name, null)));
        } catch (ParseException e) {
            return Optional.absent();
        }
    }

    public  static String get8601StringFromDate(Date date) {
        return ISO_8601_FORMAT.format(date);
    }

    public static Optional<byte[]> getOptionalBase64Bytes(JSONObject json, String name) {
        try {
            return Optional.fromNullable(Base64.decode(json.optString(name, null), Base64.DEFAULT));
        } catch (IllegalArgumentException e) {
            return Optional.absent();
        }
    }

    public static long getLongFromString(JSONObject json, String name) throws JSONException {
        try {
            return Long.decode(json.getString(name));
        } catch (NumberFormatException e) {
            throw new JSONException("Invalid long value for " + name);
        }
    }

    public static Optional<Long> getOptionalLongFromString(JSONObject json, String name) {
        try {
            return Optional.of(Long.decode(json.optString(name, null)));
        } catch (NumberFormatException e) {
            return Optional.absent();
        }
    }

    public static List<String> getStringList(JSONObject json, String name) throws JSONException {
        JSONArray jsonArray = json.getJSONArray(name);

        List<String> items = new ArrayList<>();
        for (int i = 0; i < jsonArray.length(); i++) {
            String object = jsonArray.getString(i);
            items.add(object);
        }
        return items;
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
            items.add(object);
        }
        return Optional.of(items);
    }
}
