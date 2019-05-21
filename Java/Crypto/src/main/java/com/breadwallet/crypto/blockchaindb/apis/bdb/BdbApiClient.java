package com.breadwallet.crypto.blockchaindb.apis.bdb;

import android.support.annotation.Nullable;

import com.breadwallet.crypto.blockchaindb.BlockchainCompletionHandler;
import com.breadwallet.crypto.blockchaindb.BlockchainDataTask;
import com.breadwallet.crypto.blockchaindb.apis.ArrayResponseParser;
import com.breadwallet.crypto.blockchaindb.apis.ObjectResponseParser;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.errors.QueryJsonParseError;
import com.breadwallet.crypto.blockchaindb.errors.QueryModelError;
import com.breadwallet.crypto.blockchaindb.errors.QueryNoDataError;
import com.breadwallet.crypto.blockchaindb.errors.QuerySubmissionError;
import com.breadwallet.crypto.blockchaindb.errors.QueryUrlError;
import com.google.common.base.Optional;
import com.google.common.collect.Multimap;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.IOException;
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

import static com.google.common.base.Preconditions.checkArgument;

public class BdbApiClient {

    public static final MediaType MEDIA_TYPE_JSON = MediaType.parse("application/json; charset=utf-8");

    private final OkHttpClient client;
    private final String baseUrl;
    private final BlockchainDataTask dataTask;

    public BdbApiClient(OkHttpClient client, String baseUrl, BlockchainDataTask dataTask) {
        this.client = client;
        this.baseUrl = baseUrl;
        this.dataTask = dataTask;
    }

    public <T> void sendGetRequest(String path, Multimap<String, String> params, ObjectResponseParser<T> parser, BlockchainCompletionHandler<T> handler) {
        makeAndSendRequest(path, params, null, "GET", new ObjectHandler<T>(parser, handler));
    }

    public <T> void sendGetRequest(String path, Multimap<String, String> params, ArrayResponseParser<T> parser, BlockchainCompletionHandler<T> handler) {
        makeAndSendRequest(path, params, null, "GET", new ArrayHandler<T>(path, parser, handler));
    }

    public <T> void sendRequest(String path, Multimap<String, String> params, @Nullable JSONObject json,
                                String httpMethod, ObjectResponseParser<T> parser, BlockchainCompletionHandler<T> handler) {
        makeAndSendRequest(path, params, json, httpMethod, new ObjectHandler<T>(parser, handler));
    }

    public <T> void sendRequest(String path, Multimap<String, String> params, @Nullable JSONObject json,
                            String httpMethod, ArrayResponseParser<T> parser, BlockchainCompletionHandler<T> handler) {
        makeAndSendRequest(path, params, json, httpMethod, new ArrayHandler(path, parser, handler));
    }

    private void makeAndSendRequest(String path,
                                    Multimap<String, String> params, @Nullable JSONObject json, String httpMethod,
                                    ResponseHandler handler) {
        HttpUrl.Builder urlBuilder = HttpUrl.parse(baseUrl).newBuilder();
        urlBuilder.addPathSegments(path);
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

    private interface ResponseHandler {
        void handleData(JSONObject data);
        void handleError(QueryError error);
    }

    private static class ObjectHandler<T> implements ResponseHandler {

        private final ObjectResponseParser<T> parser;
        private final BlockchainCompletionHandler<T> handler;

        ObjectHandler(ObjectResponseParser<T> parser, BlockchainCompletionHandler<T> handler) {
            this.parser = parser;
            this.handler = handler;
        }

        @Override
        public void handleData(JSONObject json) {
            JSONObject jsonPage = json.optJSONObject("page");
            boolean full = (jsonPage == null ? 0 : jsonPage.optInt("total_pages", 0)) > 1;

            // TODO: why is this always set to false in the swift
            boolean more = false && full;

            checkArgument(!more);
            Optional<T> data = parser.parse(json);
            if (data.isPresent()) {
                handler.handleData(data.get());
            } else {
                handler.handleError(new QueryModelError("Transform error"));
            }
        }

        @Override
        public void handleError(QueryError error) {
            handler.handleError(error);
        }
    }

    private static class ArrayHandler<T> implements ResponseHandler {

        private final String path;
        private final ArrayResponseParser<T> parser;
        private final BlockchainCompletionHandler<T> handler;


        ArrayHandler(String path, ArrayResponseParser<T> parser, BlockchainCompletionHandler<T> handler) {
            this.path = path;
            this.parser = parser;
            this.handler = handler;
        }

        @Override
        public void handleData(JSONObject json) {
            JSONObject jsonEmbedded = json.optJSONObject("_embedded");
            JSONArray jsonEmbeddedData = jsonEmbedded == null ? null : jsonEmbedded.optJSONArray(path);

            JSONObject jsonPage = json.optJSONObject("page");
            boolean full = (jsonPage == null ? 0 : jsonPage.optInt("total_pages", 0)) > 1;

            // TODO: why is this always set to false in the swift
            boolean more = false && full;

            if (jsonEmbedded == null || jsonEmbeddedData == null) {
                handler.handleError(new QueryModelError("Data expected"));

            } else {
                Optional<T> data = parser.parse(jsonEmbeddedData);
                if (data.isPresent()) {
                    handler.handleData(data.get());
                } else {
                    handler.handleError(new QueryModelError("Transform error"));
                }
            }
        }

        @Override
        public void handleError(QueryError error) {
            handler.handleError(error);
        }
    }
}
