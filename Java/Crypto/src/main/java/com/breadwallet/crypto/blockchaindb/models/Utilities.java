/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models;

import com.google.common.base.Optional;
import com.google.common.io.BaseEncoding;
import com.google.common.primitives.UnsignedInteger;
import com.google.common.primitives.UnsignedInts;
import com.google.common.primitives.UnsignedLong;
import com.google.common.primitives.UnsignedLongs;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.text.DateFormat;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Locale;

public final class Utilities {

    private static final ThreadLocal<DateFormat> ISO_8601_FORMAT = new ThreadLocal<DateFormat>() {
        @Override protected DateFormat initialValue() {
            return new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSSZ", Locale.ROOT);
        }
    };

    public static Date get8601DateFromString(JSONObject json, String name) throws JSONException {
        try {
            return ISO_8601_FORMAT.get().parse(json.getString(name));
        } catch (ParseException e) {
            throw new JSONException("Invalid date value for " + name);
        }
    }

    public static Optional<Date> getOptional8601DateFromString(JSONObject json, String name) {
        String value = json.optString(name, null);
        if (null == value) {
            return Optional.absent();
        }
        try {
            return Optional.fromNullable(ISO_8601_FORMAT.get().parse(value));
        } catch (ParseException e) {
            return Optional.absent();
        }
    }

    public static Optional<byte[]> getOptionalBase64Bytes(JSONObject json, String name) {
        String value = json.optString(name, null);
        if (null == value) {
            return Optional.absent();
        }
        try {
            return Optional.fromNullable(BaseEncoding.base64().decode(value));
        } catch (IllegalArgumentException e) {
            return Optional.absent();
        }
    }

    public static UnsignedInteger getUnsignedIntFromString(JSONObject json, String name) throws JSONException {
        try {
            return UnsignedInteger.valueOf(json.getInt(name));
        } catch (NumberFormatException e) {
            throw new JSONException("Invalid unsigned long value for " + name);
        }
    }

    public static UnsignedLong getUnsignedLongFromString(JSONObject json, String name) throws JSONException {
        try {
            return UnsignedLong.valueOf(json.getLong(name));
        } catch (NumberFormatException e) {
            throw new JSONException("Invalid unsigned long value for " + name);
        }
    }

    public static Optional<UnsignedLong> getOptionalUnsignedLongFromString(JSONObject json, String name) {
        Object value = json.opt(name);
        if (null == value) {
            return Optional.absent();
        }
        try {
            return Optional.of(UnsignedLong.valueOf(json.getLong(name)));
        } catch (NumberFormatException | JSONException e) {
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
}
