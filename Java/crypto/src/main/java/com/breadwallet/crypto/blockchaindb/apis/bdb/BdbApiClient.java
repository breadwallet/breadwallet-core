/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.apis.bdb;

import android.support.annotation.Nullable;

import com.breadwallet.crypto.blockchaindb.DataTask;
import com.breadwallet.crypto.blockchaindb.ObjectCoder;
import com.breadwallet.crypto.blockchaindb.ObjectCoder.ObjectCoderException;
import com.breadwallet.crypto.blockchaindb.apis.HttpStatusCodes;
import com.breadwallet.crypto.blockchaindb.apis.PagedData;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.errors.QueryJsonParseError;
import com.breadwallet.crypto.blockchaindb.errors.QueryModelError;
import com.breadwallet.crypto.blockchaindb.errors.QueryNoDataError;
import com.breadwallet.crypto.blockchaindb.errors.QueryResponseError;
import com.breadwallet.crypto.blockchaindb.errors.QuerySubmissionError;
import com.breadwallet.crypto.blockchaindb.errors.QueryUrlError;
import com.breadwallet.crypto.utility.CompletionHandler;
import com.google.common.collect.Multimap;

import java.io.IOException;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
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

public class BdbApiClient {

    private static final Logger Log = Logger.getLogger(BdbApiClient.class.getName());

    private static final MediaType MEDIA_TYPE_JSON = MediaType.parse("application/json; charset=utf-8");

    private final OkHttpClient client;
    private final String baseUrl;
    private final DataTask dataTask;
    private final ObjectCoder coder;

    public BdbApiClient(OkHttpClient client, String baseUrl, DataTask dataTask, ObjectCoder coder) {
        this.client = client;
        this.baseUrl = baseUrl;
        this.dataTask = dataTask;
        this.coder = coder;
    }

    // Create (Crud)

    void sendPost(String resource,
                  Multimap<String, String> params,
                  Object body,
                  CompletionHandler<Void, QueryError> handler) {
        makeAndSendRequest(
                Collections.singletonList(resource),
                params,
                body,
                "POST",
                new EmptyResponseParser(),
                handler);
    }

    <T> void sendPost(String resource,
                      Multimap<String, String> params,
                      Object body,
                      Class<T> clazz,
                      CompletionHandler<T, QueryError> handler) {
        makeAndSendRequest(
                Collections.singletonList(resource),
                params,
                body,
                "POST",
                new RootObjectResponseParser<>(coder, clazz),
                handler);
    }

    // Read (cRud)

    /* package */
    <T> void sendGet(String resource,
                     Multimap<String, String> params,
                     Class<T> clazz,
                     CompletionHandler<T, QueryError> handler) {
        makeAndSendRequest(
                Collections.singletonList(resource),
                params,
                null,
                "GET",
                new RootObjectResponseParser<>(coder, clazz),
                handler);
    }

    /* package */
    <T> void sendGetForArray(String resource,
                             Multimap<String, String> params,
                             Class<T> clazz,
                             CompletionHandler<List<T>, QueryError> handler) {
        makeAndSendRequest(
                Collections.singletonList(resource),
                params,
                null,
                "GET",
                new EmbeddedArrayResponseParser<>(resource, coder, clazz),
                handler);
    }

    /* package */
    <T> void sendGetForArrayWithPaging(String resource,
                                       Multimap<String, String> params,
                                       Class<T> clazz,
                                       CompletionHandler<PagedData<T>, QueryError> handler) {
        makeAndSendRequest(
                Collections.singletonList(resource),
                params,
                null,
                "GET",
                new EmbeddedPagedArrayResponseHandler<>(resource, coder, clazz),
                handler);
    }

    /* package */
    <T> void sendGetForArrayWithPaging(String resource,
                                       String url,
                                       Class<T> clazz,
                                       CompletionHandler<PagedData<T>, QueryError> handler) {
        makeAndSendRequest(
                url,
                "GET",
                new EmbeddedPagedArrayResponseHandler<>(resource, coder, clazz),
                handler);
    }

    /* package */
    <T> void sendGetWithId(String resource,
                           String id,
                           Multimap<String, String> params,
                           Class<T> clazz,
                           CompletionHandler<T, QueryError> handler) {
        makeAndSendRequest(
                Arrays.asList(resource, id),
                params,
                null,
                "GET",
                new RootObjectResponseParser<>(coder, clazz),
                handler);
    }

    // Update (crUd)

    <T> void sendPut(String resource,
                     Multimap<String, String> params,
                     Object body,
                     Class<T> clazz,
                     CompletionHandler<T, QueryError> handler) {
        makeAndSendRequest(
                Collections.singletonList(resource),
                params,
                body,
                "PUT",
                new RootObjectResponseParser<>(coder, clazz),
                handler);
    }

    <T> void sendPutWithId(String resource,
                           String id,
                           Multimap<String, String> params,
                           Object json,
                           Class<T> clazz,
                           CompletionHandler<T, QueryError> handler) {
        makeAndSendRequest(
                Arrays.asList(resource, id),
                params,
                json,
                "PUT",
                new RootObjectResponseParser<>(coder, clazz),
                handler);
    }

    // Delete (crdD)

    /* package */
    void sendDeleteWithId(String resource,
                          String id,
                          Multimap<String, String> params,
                          CompletionHandler<Void, QueryError> handler) {
        makeAndSendRequest(
                Arrays.asList(resource, id),
                params,
                null,
                "DELETE",
                new EmptyResponseParser(),
                handler);
    }

    private <T> void makeAndSendRequest(String fullUrl,
                                        String httpMethod,
                                        ResponseParser<T> parser,
                                        CompletionHandler<T, QueryError> handler) {
        HttpUrl url = HttpUrl.parse(fullUrl);
        if (null == url) {
            handler.handleError(new QueryUrlError("Invalid base URL " + fullUrl));
            return;
        }

        HttpUrl.Builder urlBuilder = url.newBuilder();
        HttpUrl httpUrl = urlBuilder.build();
        Log.log(Level.FINE, String.format("Request: %s: Method: %s", httpUrl, httpMethod));

        Request.Builder requestBuilder = new Request.Builder();
        requestBuilder.url(httpUrl);
        requestBuilder.header("Accept", "application/json");
        requestBuilder.method(httpMethod, null);

        sendRequest(requestBuilder.build(), dataTask, parser, handler);
    }

    private <T> void makeAndSendRequest(List<String> pathSegments,
                                        Multimap<String, String> params,
                                        @Nullable Object json,
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

    private static class EmptyResponseParser implements ResponseParser<Void> {

        @Override
        public Void parseResponse(String responseData) {
            return null;
        }
    }

    private static class RootObjectResponseParser<T> implements ResponseParser<T> {

        private final ObjectCoder coder;
        private final Class<T> clazz;

        RootObjectResponseParser(ObjectCoder coder,
                                 Class<T> clazz) {
            this.coder = coder;
            this.clazz = clazz;
        }

        @Override
        public T parseResponse(String responseData) throws QueryError {
            try {
                T resp = coder.deserializeJson(clazz, responseData);
                if (resp == null) {
                    throw new QueryModelError("Transform error");
                }

                return resp;
            } catch (ObjectCoderException e) {
                throw new QueryJsonParseError(e.getMessage());
            }
        }
    }

    private static class EmbeddedArrayResponseParser<T> implements ResponseParser<List<T>> {

        private final String path;
        private final ObjectCoder coder;
        private final Class<T> clazz;

        EmbeddedArrayResponseParser(String path,
                                    ObjectCoder coder,
                                    Class<T> clazz) {
            this.path = path;
            this.coder = coder;
            this.clazz = clazz;
        }

        @Override
        public List<T> parseResponse(String responseData) throws QueryError {
            try {
                BdbEmbeddedResponse resp = coder.deserializeJson(BdbEmbeddedResponse.class, responseData);
                List<T> data = (resp == null || !resp.containsEmbedded(path)) ?
                        Collections.emptyList() :
                        coder.deserializeObjectList(clazz, resp.getEmbedded(path).get());
                if (data == null) {
                    throw new QueryModelError("Transform error");
                }

                return data;
            } catch (ObjectCoderException e) {
                throw new QueryJsonParseError(e.getMessage());
            }
        }
    }

    private static class EmbeddedPagedArrayResponseHandler<T> implements ResponseParser<PagedData<T>> {

        private final String path;
        private final ObjectCoder coder;
        private final Class<T> clazz;

        EmbeddedPagedArrayResponseHandler(String path,
                                          ObjectCoder coder,
                                          Class<T> clazz) {
            this.path = path;
            this.coder = coder;
            this.clazz = clazz;
        }

        @Override
        public PagedData<T> parseResponse(String responseData) throws QueryError {
            try {
                BdbEmbeddedResponse resp = coder.deserializeJson(BdbEmbeddedResponse.class, responseData);
                List<T> data = (resp == null || !resp.containsEmbedded(path)) ?
                        Collections.emptyList() :
                        coder.deserializeObjectList(clazz, resp.getEmbedded(path).get());
                if (data == null) {
                    throw new QueryModelError("Transform error");
                }

                String prevUrl = resp == null ? null : resp.getPreviousUrl().orNull();
                String nextUrl = resp == null ? null : resp.getNextUrl().orNull();
                return new PagedData<>(data, prevUrl, nextUrl);
            } catch (ObjectCoderException e) {
                throw new QueryJsonParseError(e.getMessage());
            }
        }
    }

    // JSON methods
}
