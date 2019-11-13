/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.apis.brd;

import android.support.annotation.Nullable;

import com.breadwallet.crypto.blockchaindb.DataTask;
import com.breadwallet.crypto.blockchaindb.apis.HttpStatusCodes;
import com.breadwallet.crypto.blockchaindb.apis.bdb.BdbApiClient;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.errors.QueryJsonParseError;
import com.breadwallet.crypto.blockchaindb.errors.QueryModelError;
import com.breadwallet.crypto.blockchaindb.errors.QueryNoDataError;
import com.breadwallet.crypto.blockchaindb.errors.QueryResponseError;
import com.breadwallet.crypto.blockchaindb.errors.QuerySubmissionError;
import com.breadwallet.crypto.utility.CompletionHandler;
import com.google.common.collect.ImmutableMultimap;
import com.google.common.collect.Multimap;
import com.google.gson.Gson;
import com.google.gson.JsonElement;
import com.google.gson.JsonSyntaxException;

import java.io.IOException;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.logging.Level;
import java.util.logging.Logger;

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

    private static final Logger Log = Logger.getLogger(BrdApiClient.class.getName());

    private static final Gson GSON = new Gson();
    private static final MediaType MEDIA_TYPE_JSON = MediaType.parse("application/json; charset=utf-8");

    private final OkHttpClient client;
    private final String baseUrl;
    private final DataTask dataTask;

    public BrdApiClient(OkHttpClient client,
                        String baseUrl,
                        DataTask dataTask) {
        this.client = client;
        this.baseUrl = baseUrl;
        this.dataTask = dataTask;
    }

    /* package */
    void sendJsonRequest(String networkName,
                         Map json,
                         CompletionHandler<String, QueryError> handler) {
        makeAndSendRequest(Arrays.asList("ethq", getNetworkName(networkName), "proxy"), ImmutableMultimap.of(), json, "POST",
                new EmbeddedStringResponseHandler(handler));
    }

    /* package */
    void sendQueryRequest(String networkName,
                          Multimap<String, String> params,
                          Map json,
                          CompletionHandler<String, QueryError> handler) {
        makeAndSendRequest(Arrays.asList("ethq", getNetworkName(networkName), "query"), params, json, "POST",
                new EmbeddedStringResponseHandler(handler));
    }

    /* package */
    <T> void sendQueryForArrayRequest(String networkName,
                                      Multimap<String, String> params,
                                      Map json,
                                      Class<T[]> clazz,
                                      CompletionHandler<List<T>, QueryError> handler) {
        makeAndSendRequest(Arrays.asList("ethq", getNetworkName(networkName), "query"), params, json, "POST",
                new EmbeddedArrayResponseHandler<T>(clazz, handler));
    }

    /* package */
    <T> void sendTokenRequest(Class<T[]> clazz,
                              CompletionHandler<List<T>, QueryError> handler) {
        makeAndSendRequest(Collections.singletonList("currencies"), ImmutableMultimap.of("type", "erc20"), null, "GET",
                new RootArrayResponseHandler<T>(clazz, handler));
    }

    private String getNetworkName(String networkName) {
        networkName = networkName.toLowerCase(Locale.ROOT);
        return networkName.equals("testnet") ? "ropsten" : networkName;
    }

    private <T> void makeAndSendRequest(List<String> pathSegments,
                                        Multimap<String, String> params,
                                        @Nullable Map json,
                                        String httpMethod,
                                        ResponseHandler handler) {
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
        Log.log(Level.FINE, String.format("Request: %s: Method: %s: Data: %s", httpUrl, httpMethod, json));

        Request.Builder requestBuilder = new Request.Builder();
        requestBuilder.url(httpUrl);
        requestBuilder.header("Accept", "application/json");
        requestBuilder.method(httpMethod, json == null ? null : RequestBody.create(MEDIA_TYPE_JSON, GSON.toJson(json)));

        sendRequest(requestBuilder.build(), dataTask, handler);
    }

    private <T> void sendRequest(Request request, DataTask dataTask, ResponseHandler handler) {
        dataTask.execute(client, request, new Callback() {
            @Override
            public void onResponse(Call call, Response response) throws IOException {
                int responseCode = response.code();
                if (HttpStatusCodes.responseSuccess(request.method()).contains(responseCode)) {
                    try (ResponseBody responseBody = response.body()) {
                        if (responseBody == null) {
                            Log.log(Level.SEVERE, "response failed with null body");
                            handler.handleError(new QueryNoDataError());
                        } else {
                            try {
                                handler.handleResponse(responseBody.string());
                            } catch (JsonSyntaxException e) {
                                Log.log(Level.SEVERE, "response failed parsing json", e);
                                handler.handleError(new QueryJsonParseError(e.getMessage()));
                            }
                        }
                    }
                } else {
                    Log.log(Level.SEVERE, "response failed with status " + responseCode);
                    handler.handleError(new QueryResponseError(responseCode));
                }
            }

            @Override
            public void onFailure(Call call, IOException e) {
                Log.log(Level.SEVERE, "send request failed", e);
                handler.handleError(new QuerySubmissionError(e.getMessage()));
            }
        });
    }

    private interface ResponseHandler {
        void handleResponse(String responseData);
        void handleError(QueryError error);
    }

    private static class EmbeddedStringResponseHandler implements ResponseHandler {

        private final CompletionHandler<String, QueryError> handler;

        EmbeddedStringResponseHandler(CompletionHandler<String, QueryError> handler) {
            this.handler = handler;
        }

        @Override
        public void handleResponse(String responseData) {
            EmbeddedStringResponse resp = GSON.fromJson(responseData, EmbeddedStringResponse.class);

            if (resp.result == null) {
                QueryError e = new QueryModelError("'result' expected");
                Log.log(Level.SEVERE, "missing 'result' in response", e);
                handler.handleError(e);

            } else {
                handler.handleData(resp.result);
            }
        }

        @Override
        public void handleError(QueryError error) {
            handler.handleError(error);
        }
    }

    public static class EmbeddedStringResponse {
        public String result;
    }

    private static class EmbeddedArrayResponseHandler<T> implements ResponseHandler {

        private final Class<T[]> clazz;
        private final CompletionHandler<List<T>, QueryError> handler;

        EmbeddedArrayResponseHandler(Class<T[]> clazz, CompletionHandler<List<T>, QueryError> handler) {
            this.clazz = clazz;
            this.handler = handler;
        }

        @Override
        public void handleResponse(String responseData) {
            EmbeddedArrayResponse resp = GSON.fromJson(responseData, EmbeddedArrayResponse.class);

            if (resp.status == null) {
                QueryError e = new QueryModelError("'status' expected");
                Log.log(Level.SEVERE, "missing 'status' in response", e);
                handler.handleError(e);

            } else if (resp.message == null) {
                QueryError e = new QueryModelError("'message' expected");
                Log.log(Level.SEVERE, "missing 'message' in response", e);
                handler.handleError(e);

            } else if (resp.result == null) {
                QueryError e = new QueryModelError("'result' expected");
                Log.log(Level.SEVERE, "missing 'result' in response", e);
                handler.handleError(e);

            } else {
                T[] data = GSON.fromJson(resp.result, clazz);
                if (null != data) {
                    handler.handleData(Arrays.asList(data));
                } else {
                    QueryError e = new QueryModelError("Transform error");
                    Log.log(Level.SEVERE, "parsing error", e);
                    handler.handleError(e);
                }
            }
        }

        @Override
        public void handleError(QueryError error) {
            handler.handleError(error);
        }
    }

    private static class RootArrayResponseHandler<T> implements ResponseHandler {

        private final Class<T[]> clazz;
        private final CompletionHandler<List<T>, QueryError> handler;

        RootArrayResponseHandler(Class<T[]> clazz, CompletionHandler<List<T>, QueryError> handler) {
            this.clazz = clazz;
            this.handler = handler;
        }

        @Override
        public void handleResponse(String responseData) {
            T[] data = GSON.fromJson(responseData, clazz);
            if (null != data) {
                handler.handleData(Arrays.asList(data));

            } else {
                QueryError e = new QueryModelError("Transform error");
                Log.log(Level.SEVERE, "parsing error", e);
                handler.handleError(e);
            }
        }

        @Override
        public void handleError(QueryError error) {
            handler.handleError(error);
        }
    }

    public static class EmbeddedArrayResponse {
        public String status;
        public String message;
        public JsonElement result;
    }
}
