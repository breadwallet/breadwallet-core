/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.apis.brd;

import android.support.annotation.Nullable;
import android.util.Log;

import com.breadwallet.crypto.blockchaindb.CompletionHandler;
import com.breadwallet.crypto.blockchaindb.DataTask;
import com.breadwallet.crypto.blockchaindb.apis.ArrayResponseParser;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.errors.QueryJsonParseError;
import com.breadwallet.crypto.blockchaindb.errors.QueryModelError;
import com.breadwallet.crypto.blockchaindb.errors.QueryNoDataError;
import com.breadwallet.crypto.blockchaindb.errors.QueryResponseError;
import com.breadwallet.crypto.blockchaindb.errors.QuerySubmissionError;
import com.breadwallet.crypto.blockchaindb.errors.QueryUrlError;
import com.google.common.base.Optional;
import com.google.common.collect.ImmutableMultimap;
import com.google.common.collect.Multimap;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.IOException;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Map;

import okhttp3.Call;
import okhttp3.Callback;
import okhttp3.HttpUrl;
import okhttp3.MediaType;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;
import okhttp3.ResponseBody;

public class BrdApiClient {

    private static final String TAG = BrdApiClient.class.getName();

    private static final MediaType MEDIA_TYPE_JSON = MediaType.parse("application/json; charset=utf-8");

    private final OkHttpClient client;
    private final String baseUrl;
    private final DataTask dataTask;

    public BrdApiClient(OkHttpClient client, String baseUrl, DataTask dataTask) {
        this.client = client;
        this.baseUrl = baseUrl;
        this.dataTask = dataTask;
    }

    /* package */
    void sendJsonRequest(String networkName, JSONObject json, CompletionHandler<String> handler) {
        makeAndSendRequest(Arrays.asList("ethq", networkName, "proxy"), ImmutableMultimap.of(), json, "POST",
                new EmbeddedStringHandler(handler));
    }

    /* package */
    void sendQueryRequest(String networkName, Multimap<String, String> params, JSONObject json,
                          CompletionHandler<String> handler) {
        makeAndSendRequest(Arrays.asList("ethq", networkName, "query"), params, json, "POST",
                new EmbeddedStringHandler(handler));
    }

    /* package */
    <T> void sendQueryForArrayRequest(String networkName, Multimap<String, String> params, JSONObject json,
                                      ArrayResponseParser<T> parser, CompletionHandler<T> handler) {
        makeAndSendRequest(Arrays.asList("ethq", networkName, "query"), params, json, "POST",
                new EmbeddedArrayHandler<T>(parser, handler));
    }

    /* package */
    <T> void sendTokenRequest(ArrayResponseParser<T> parser, CompletionHandler<T> handler) {
        makeAndSendRequest(Collections.singletonList("currencies"), ImmutableMultimap.of("type", "erc20"), null, "GET",
                new RootArrayHandler<T>(parser, handler));
    }

    private <T> void makeAndSendRequest(List<String> pathSegments,
                                    Multimap<String, String> params, @Nullable JSONObject json, String httpMethod,
                                    ResponseHandler<T> handler) {
        HttpUrl.Builder urlBuilder = HttpUrl.parse(baseUrl).newBuilder();

        for (String segment : pathSegments) {
            urlBuilder.addPathSegment(segment);
        }

        for (Map.Entry<String, String> entry : params.entries()) {
            String key = entry.getKey();
            String value = entry.getValue();
            urlBuilder.addQueryParameter(key, value);
        }

        HttpUrl httpUrl = urlBuilder.build();
        Log.d(TAG, String.format("Request: %s: Method: %s: Data: %s", httpUrl, httpMethod, json));

        Request.Builder requestBuilder = new Request.Builder();
        requestBuilder.url(httpUrl);
        requestBuilder.addHeader("accept", "application/json");
        requestBuilder.method(httpMethod, json == null ? null : RequestBody.create(MEDIA_TYPE_JSON, json.toString()));

        sendRequest(requestBuilder.build(), dataTask, handler);
    }

    private <T> void sendRequest(Request request, DataTask dataTask, ResponseHandler<T> handler) {
        dataTask.execute(client, request, new Callback() {
            @Override
            public void onResponse(Call call, Response response) throws IOException {
                int responseCode = response.code();
                if (responseCode == 200) {
                    try (ResponseBody responseBody = response.body()) {
                        if (responseBody == null) {
                            Log.e(TAG, "response failed with null body");
                            handler.handleError(new QueryNoDataError());
                        } else {
                            T data = null;

                            try {
                                data = handler.parseData(responseBody.string());
                            } catch (JSONException e) {
                                Log.e(TAG, "response failed parsing json", e);
                                handler.handleError(new QueryJsonParseError(e.getMessage()));
                            }

                            if (data != null) {
                                handler.handleData(data);
                            }
                        }
                    }
                } else {
                    Log.e(TAG, "response failed with status " + responseCode);
                    handler.handleError(new QueryResponseError(responseCode));
                }
            }

            @Override
            public void onFailure(Call call, IOException e) {
                Log.e(TAG, "send request failed", e);
                handler.handleError(new QuerySubmissionError(e.getMessage()));
            }
        });
    }

    private interface ResponseHandler<T> {
        T parseData(String data) throws JSONException;

        void handleData(T data);

        void handleError(QueryError error);
    }

    private static class EmbeddedStringHandler implements ResponseHandler<JSONObject> {

        private final CompletionHandler<String> handler;

        EmbeddedStringHandler(CompletionHandler<String> handler) {
            this.handler = handler;
        }

        @Override
        public JSONObject parseData(String data) throws JSONException {
            return new JSONObject(data);
        }

        @Override
        public void handleData(JSONObject json) {
            String result = json.optString("result", null);
            if (result == null) {
                QueryError e = new QueryModelError("'result' expected");
                Log.e(TAG, "missing 'result' in response", e);
                handler.handleError(e);

            } else {
                handler.handleData(result);
            }
        }

        @Override
        public void handleError(QueryError error) {
            handler.handleError(error);
        }
    }

    private static class EmbeddedArrayHandler<T> implements ResponseHandler<JSONObject> {

        private final ArrayResponseParser<T> parser;
        private final CompletionHandler<T> handler;

        EmbeddedArrayHandler(ArrayResponseParser<T> parser, CompletionHandler<T> handler) {
            this.parser = parser;
            this.handler = handler;
        }

        @Override
        public JSONObject parseData(String data) throws JSONException {
            return new JSONObject(data);
        }

        @Override
        public void handleData(JSONObject json) {
            String status = json.optString("status", null);
            String message = json.optString("message", null);
            JSONArray result = json.optJSONArray("result");

            if (status == null) {
                QueryError e = new QueryModelError("'status' expected");
                Log.e(TAG, "missing 'status' in response", e);
                handler.handleError(e);

            } else if (message == null) {
                QueryError e = new QueryModelError("'message' expected");
                Log.e(TAG, "missing 'message' in response", e);
                handler.handleError(e);

            } else if (result == null) {
                QueryError e = new QueryModelError("'result' expected");
                Log.e(TAG, "missing 'result' in response", e);
                handler.handleError(e);

            } else {
                Optional<T> data = parser.parse(result);
                if (data.isPresent()) {
                    handler.handleData(data.get());
                } else {
                    QueryError e = new QueryModelError("Transform error");
                    Log.e(TAG, "parsing error", e);
                    handler.handleError(e);
                }
            }
        }

        @Override
        public void handleError(QueryError error) {
            handler.handleError(error);
        }
    }

    private static class RootArrayHandler<T> implements ResponseHandler<JSONArray> {

        private final ArrayResponseParser<T> parser;
        private final CompletionHandler<T> handler;

        RootArrayHandler(ArrayResponseParser<T> parser, CompletionHandler<T> handler) {
            this.parser = parser;
            this.handler = handler;
        }

        @Override
        public JSONArray parseData(String data) throws JSONException {
            return new JSONArray(data);
        }

        @Override
        public void handleData(JSONArray json) {
            Optional<T> data = parser.parse(json);
            if (data.isPresent()) {
                handler.handleData(data.get());

            } else {
                QueryError e = new QueryModelError("Transform error");
                Log.e(TAG, "parsing error", e);
                handler.handleError(e);
            }
        }

        @Override
        public void handleError(QueryError error) {
            handler.handleError(error);
        }
    }
}
