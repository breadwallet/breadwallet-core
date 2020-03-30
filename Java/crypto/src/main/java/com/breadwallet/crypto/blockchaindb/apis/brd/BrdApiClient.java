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
import com.breadwallet.crypto.blockchaindb.ObjectCoder;
import com.breadwallet.crypto.blockchaindb.ObjectCoder.ObjectCoderException;
import com.breadwallet.crypto.blockchaindb.apis.HttpStatusCodes;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.errors.QueryJsonParseError;
import com.breadwallet.crypto.blockchaindb.errors.QueryModelError;
import com.breadwallet.crypto.blockchaindb.errors.QueryNoDataError;
import com.breadwallet.crypto.blockchaindb.errors.QueryResponseError;
import com.breadwallet.crypto.blockchaindb.errors.QuerySubmissionError;
import com.breadwallet.crypto.blockchaindb.errors.QueryUrlError;
import com.breadwallet.crypto.utility.CompletionHandler;
import com.google.common.collect.ImmutableMultimap;
import com.google.common.collect.Multimap;

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

    private static final MediaType MEDIA_TYPE_JSON = MediaType.parse("application/json; charset=utf-8");

    private final OkHttpClient client;
    private final String baseUrl;
    private final DataTask dataTask;
    private final ObjectCoder coder;

    public BrdApiClient(OkHttpClient client,
                        String baseUrl,
                        DataTask dataTask,
                        ObjectCoder coder) {
        this.client = client;
        this.baseUrl = baseUrl;
        this.dataTask = dataTask;
        this.coder = coder;
    }

    /* package */
    void sendJsonRequest(String networkName,
                         Map json,
                         CompletionHandler<String, QueryError> handler) {
        makeAndSendRequest(
                Arrays.asList("ethq", getNetworkName(networkName), "proxy"),
                ImmutableMultimap.of(),
                json,
                "POST",
                new BrdResponseParser<>(coder, String.class),
                handler);
    }

    /* package */
    void sendQueryRequest(String networkName,
                          Multimap<String, String> params,
                          Map json,
                          CompletionHandler<String, QueryError> handler) {
        makeAndSendRequest(
                Arrays.asList("ethq", getNetworkName(networkName), "query"),
                params,
                json,
                "POST",
                new BrdResponseParser<>(coder, String.class),
                handler);
    }

    /* package */
    <T> void sendQueryForArrayRequest(String networkName,
                                      Multimap<String, String> params,
                                      Map json,
                                      Class<T> clazz,
                                      CompletionHandler<List<T>, QueryError> handler) {
        makeAndSendRequest(
                Arrays.asList("ethq", getNetworkName(networkName), "query"),
                params,
                json,
                "POST",
                new BrdResponseWithStatusParser<>(coder, clazz),
                handler);
    }

    /* package */
    <T> void sendTokenRequest(Class<T> clazz,
                              CompletionHandler<List<T>, QueryError> handler) {
        makeAndSendRequest(
                Collections.singletonList("currencies"),
                ImmutableMultimap.of("type", "erc20"),
                null,
                "GET",
                new ListResponseParser<>(coder, clazz),
                handler);
    }

    private String getNetworkName(String networkName) {
        networkName = networkName.toLowerCase(Locale.ROOT);
        return networkName.equals("testnet") ? "ropsten" : networkName;
    }

    private <T> void makeAndSendRequest(List<String> pathSegments,
                                        Multimap<String, String> params,
                                        @Nullable Map json,
                                        String httpMethod,
                                        ResponseParser<T> parser,
                                        CompletionHandler<T, QueryError> handler) {
        RequestBody httpBody;
        if (json == null) {
            httpBody = null;

        } else try {
            httpBody = RequestBody.create(coder.serializeObject(json), MEDIA_TYPE_JSON);

        } catch (ObjectCoderException e) {
            handler.handleError(new QuerySubmissionError(e.getMessage()));
            return;
        }

        HttpUrl url = HttpUrl.parse(baseUrl);
        if (null == url) {
            handler.handleError(new QueryUrlError("Invalid base URL " + baseUrl));
            return;
        }

        HttpUrl.Builder urlBuilder = url.newBuilder();
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
        requestBuilder.method(httpMethod, httpBody);

        sendRequest(requestBuilder.build(), dataTask, parser, handler);
    }

    private <T> void sendRequest(Request request,
                                 DataTask dataTask,
                                 ResponseParser<T> parser,
                                 CompletionHandler<T, QueryError> handler) {
        dataTask.execute(client, request, new Callback() {
            @Override
            public void onResponse(Call call, Response response) throws IOException {
                T data = null;
                QueryError error = null;
                RuntimeException exception = null;

                try (ResponseBody responseBody = response.body()) {
                    int responseCode = response.code();
                    if (HttpStatusCodes.responseSuccess(request.method()).contains(responseCode)) {
                        if (responseBody == null) {
                            throw new QueryNoDataError();
                        } else {
                            data = parser.parseResponse(responseBody.string());
                        }
                    } else {
                        throw new QueryResponseError(responseCode);
                    }
                } catch (QueryError e) {
                    error = e;
                } catch (RuntimeException e) {
                    exception = e;
                }

                // if anything goes wrong, make sure we report as an error
                if (exception != null) {
                    Log.log(Level.SEVERE, "response failed with runtime exception", exception);
                    handler.handleError(new QuerySubmissionError(exception.getMessage()));
                } else if (error != null) {
                    Log.log(Level.SEVERE, "response failed with error", error);
                    handler.handleError(error);
                } else {
                    handler.handleData(data);
                }
            }

            @Override
            public void onFailure(Call call, IOException e) {
                Log.log(Level.SEVERE, "send request failed", e);
                handler.handleError(new QuerySubmissionError(e.getMessage()));
            }
        });
    }

    private interface ResponseParser<T> {
        @Nullable
        T parseResponse(String responseData) throws QueryError;
    }

    private static class BrdResponseParser<T> implements ResponseParser<T> {

        final ObjectCoder coder;
        final Class<T> clazz;

        BrdResponseParser(ObjectCoder coder, Class<T> clazz) {
            this.coder = coder;
            this.clazz = clazz;
        }

        @Override
        public T parseResponse(String responseData) throws QueryError {
            try {
                BrdJsonRpcResponse resp = coder.deserializeJson(BrdJsonRpcResponse.class, responseData);
                T data = (resp == null || resp.getResult() == null) ?
                        null : coder.deserializeObject(clazz, resp.getResult());
                if (data == null) {
                    throw new QueryModelError("Transform error");
                }

                return data;
            } catch (ObjectCoderException e) {
                throw new QueryJsonParseError(e.getMessage());
            }
        }
    }

    private static class BrdResponseWithStatusParser<T> implements ResponseParser<List<T>> {
        final ObjectCoder coder;
        final Class<T> clazz;

        BrdResponseWithStatusParser(ObjectCoder coder, Class<T> clazz) {
            this.coder = coder;
            this.clazz = clazz;
        }

        @Override
        public List<T> parseResponse(String responseData) throws QueryError {
            try {
                BrdJsonRpcResponse resp = coder.deserializeJson(BrdJsonRpcResponse.class, responseData);
                List<T> data = (resp == null || resp.getResult() == null) ?
                        Collections.emptyList() :
                        coder.deserializeObjectList(clazz, resp.getResult());
                if (data == null) {
                    throw new QueryModelError("Transform error");
                }

                return data;
            } catch (ObjectCoderException e) {
                throw new QueryJsonParseError(e.getMessage());
            }
        }
    }

    private static class ListResponseParser<T> implements ResponseParser<List<T>> {
        final ObjectCoder coder;
        final Class<T> clazz;

        ListResponseParser(ObjectCoder coder, Class<T> clazz) {
            this.coder = coder;
            this.clazz = clazz;
        }

        @Override
        public List<T> parseResponse(String responseData) throws QueryError {
            try {
                List<T> data = coder.deserializeJsonList(clazz, responseData);
                if (null == data) {
                    throw new QueryModelError("Transform error");
                }
                return data;
            } catch (ObjectCoderException e) {
                throw new QueryJsonParseError(e.getMessage());
            }
        }
    }
}
