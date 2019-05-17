package com.breadwallet.crypto.blockchaindb.apis.brd;

import android.support.annotation.Nullable;

import com.breadwallet.crypto.blockchaindb.BlockchainDataTask;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.errors.QueryJsonParseError;
import com.breadwallet.crypto.blockchaindb.errors.QueryModelError;
import com.breadwallet.crypto.blockchaindb.errors.QueryNoDataError;
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

    public static final MediaType MEDIA_TYPE_JSON = MediaType.parse("application/json; charset=utf-8");

    private final OkHttpClient client;
    private final String baseUrl;
    private final BlockchainDataTask dataTask;

    public BrdApiClient(OkHttpClient client, String baseUrl, BlockchainDataTask dataTask) {
        this.client = client;
        this.baseUrl = baseUrl;
        this.dataTask = dataTask;
    }

    public void makeRequestJson(String networkName, JSONObject json, StringCompletionHandler handler) {
        makeAndSendRequest(Arrays.asList("ethq", networkName, "proxy"), ImmutableMultimap.of(), json, "POST",
                new StringHandler(handler));
    }

    public void makeRequestQuery(String networkName, Multimap<String, String> params, JSONObject json,
                                 StringCompletionHandler handler) {
        makeAndSendRequest(Arrays.asList("ethq", networkName, "query"), params, json, "POST",
                new StringHandler(handler));
    }

    public void makeRequestQuery(String networkName, Multimap<String, String> params, JSONObject json,
                                 ArrayCompletionHandler handler) {
        makeAndSendRequest(Arrays.asList("ethq", networkName, "query"), params, json, "POST",
                new ArrayHandler(handler));
    }

    public void makeRequestToken(StringCompletionHandler handler) {
        makeAndSendRequest(Arrays.asList("currencies"), ImmutableMultimap.of("type", "erc20"), null, "GET",
                new StringHandler(handler));
    }


    private interface ResponseHandler {
        void handleData(JSONObject data);

        void handleError(QueryError error);
    }

    private void makeAndSendRequest(List<String> segments,
                                    Multimap<String, String> params, @Nullable JSONObject json, String httpMethod,
                                    ResponseHandler handler) {
        HttpUrl.Builder urlBuilder = HttpUrl.parse(baseUrl).newBuilder();

        for (String segment : segments) {
            urlBuilder.addPathSegment(segment);
        }

        for (Map.Entry<String, String> entry : params.entries()) {
            String key = entry.getKey();
            String value = entry.getValue();
            urlBuilder.addQueryParameter(key, value);
        }

        Request.Builder requestBuilder = new Request.Builder();
        requestBuilder.url(urlBuilder.build());
        requestBuilder.addHeader("accept", "application/json");
        requestBuilder.method(httpMethod, json == null ? null : RequestBody.create(MEDIA_TYPE_JSON, json.toString()));

        sendRequest(requestBuilder.build(), dataTask, handler);
    }

    private <T> void sendRequest(Request request, BlockchainDataTask dataTask, ResponseHandler handler) {
        dataTask.execute(client, request, new Callback() {
            @Override
            public void onResponse(Call call, Response response) throws IOException {
                int responseCode = response.code();
                if (responseCode == 200) {
                    try (ResponseBody responseBody = response.body()) {
                        if (responseBody == null) {
                            handler.handleError(new QueryNoDataError());
                        } else {
                            try {
                                handler.handleData(new JSONObject(responseBody.string()));
                            } catch (JSONException e) {
                                handler.handleError(new QueryJsonParseError(e.getMessage()));
                            }
                        }
                    }
                } else {
                    handler.handleError(new QueryUrlError("Status: " + responseCode));
                }
            }

            @Override
            public void onFailure(Call call, IOException e) {
                // TODO: Do we want to propagate this?
                handler.handleError(new QuerySubmissionError(e.getMessage()));
            }
        });
    }

    private static class StringHandler implements ResponseHandler {

        private final StringCompletionHandler handler;

        StringHandler(StringCompletionHandler handler) {
            this.handler = handler;
        }

        @Override
        public void handleData(JSONObject json) {
            Optional<String> result = Optional.fromNullable(json.optString("result", null));
            handler.handleData(result);
        }

        @Override
        public void handleError(QueryError error) {
            handler.handleError(error);
        }
    }

    private static class ArrayHandler implements ResponseHandler {

        private final ArrayCompletionHandler handler;

        ArrayHandler(ArrayCompletionHandler handler) {
            this.handler = handler;
        }

        @Override
        public void handleData(JSONObject json) {
            String status = json.optString("status", null);
            String message = json.optString("status", null);
            JSONArray result = json.optJSONArray("result");

            if (status == null || message == null || result == null) {
                handler.handleError(new QueryModelError("Data expected"));

            } else {
                handler.handleData(result);
            }
        }

        @Override
        public void handleError(QueryError error) {
            handler.handleError(error);
        }
    }
}
