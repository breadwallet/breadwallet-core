package com.breadwallet.crypto.blockchaindb.apis;

import android.support.annotation.Nullable;

import com.breadwallet.crypto.blockchaindb.BlockchainCompletionHandler;
import com.breadwallet.crypto.blockchaindb.BlockchainDataTask;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.errors.QueryJsonParseError;
import com.breadwallet.crypto.blockchaindb.errors.QueryModelError;
import com.breadwallet.crypto.blockchaindb.errors.QueryNoDataError;
import com.breadwallet.crypto.blockchaindb.errors.QuerySubmissionError;
import com.breadwallet.crypto.blockchaindb.errors.QueryUrlError;
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

    public void makeRequest(String path, Multimap<String, String> params, JsonApiCompletionObjectHandler handler) {
        makeRequest(path, params, null, "GET", new BlockchainCompletionHandler<JSONObject>() {
            @Override
            public void handleData(JSONObject json) {
                JSONObject jsonPage = json.optJSONObject("page");
                boolean full = (jsonPage == null ? 0 : jsonPage.optInt("total_pages", 0)) > 1;

                // TODO: why is this always set to false in the swift
                boolean more = false && full;
                handler.handleData(json, more);
            }

            @Override
            public void handleError(QueryError error) {
                handler.handleError(error);
            }
        });
    }

    public void makeRequest(String path, Multimap<String, String> params, JsonApiCompletionArrayHandler handler) {
        makeRequest(path, params, null, "GET", new BlockchainCompletionHandler<JSONObject>() {
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
                    handler.handleData(jsonEmbeddedData, more);
                }
            }

            @Override
            public void handleError(QueryError error) {
                handler.handleError(error);
            }
        });
    }

    public <T> void makeRequest(String path,
                                Multimap<String, String> params, @Nullable JSONObject json, String httpMethod,
                                BlockchainCompletionHandler<T> handler) {
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

    private <T> void sendRequest(Request request, BlockchainDataTask dataTask, BlockchainCompletionHandler handler) {
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

}
