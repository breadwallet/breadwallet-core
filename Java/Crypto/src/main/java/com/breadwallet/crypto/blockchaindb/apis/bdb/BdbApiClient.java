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
import com.breadwallet.crypto.blockchaindb.apis.HttpStatusCodes;
import com.breadwallet.crypto.blockchaindb.apis.PagedCompletionHandler;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.errors.QueryJsonParseError;
import com.breadwallet.crypto.blockchaindb.errors.QueryModelError;
import com.breadwallet.crypto.blockchaindb.errors.QueryNoDataError;
import com.breadwallet.crypto.blockchaindb.errors.QueryResponseError;
import com.breadwallet.crypto.blockchaindb.errors.QuerySubmissionError;
import com.breadwallet.crypto.blockchaindb.errors.QueryUrlError;
import com.breadwallet.crypto.utility.CompletionHandler;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.DeserializationFeature;
import com.fasterxml.jackson.databind.JavaType;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.type.TypeFactory;
import com.google.common.collect.Multimap;

import java.io.IOException;
import java.util.ArrayList;
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

import static com.google.common.base.Preconditions.checkState;

public class BdbApiClient {

    private static final Logger Log = Logger.getLogger(BdbApiClient.class.getName());

    private static final MediaType MEDIA_TYPE_JSON = MediaType.parse("application/json; charset=utf-8");

    private final OkHttpClient client;
    private final String baseUrl;
    private final DataTask dataTask;

    public BdbApiClient(OkHttpClient client, String baseUrl, DataTask dataTask) {
        this.client = client;
        this.baseUrl = baseUrl;
        this.dataTask = dataTask;
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
                new EmptyResponseHandler(handler));
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
                new RootObjectResponseHandler<>(clazz, handler));
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
                new RootObjectResponseHandler<>(clazz, handler));
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
                new EmbeddedArrayResponseHandler<>(resource, clazz, handler));
    }

    /* package */
    <T> void sendGetForArrayWithPaging(String resource,
                                       Multimap<String, String> params,
                                       Class<T> clazz,
                                       PagedCompletionHandler<List<T>, QueryError> handler) {
        makeAndSendRequest(
                Collections.singletonList(resource),
                params,
                null,
                "GET",
                new EmbeddedPagedArrayResponseHandler<>(resource, clazz, handler));
    }

    /* package */
    <T> void sendGetForArrayWithPaging(String resource,
                                       String url,
                                       Class<T> clazz,
                                       PagedCompletionHandler<List<T>, QueryError> handler) {
        makeAndSendRequest(
                url,
                "GET",
                new EmbeddedPagedArrayResponseHandler<>(resource, clazz, handler));
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
                new RootObjectResponseHandler<>(clazz, handler));
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
                new RootObjectResponseHandler<>(clazz, handler));
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
                new RootObjectResponseHandler<>(clazz, handler));
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
                new EmptyResponseHandler(handler));
    }

    private <T> void makeAndSendRequest(String fullUrl,
                                        String httpMethod,
                                        ResponseHandler handler) {
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

        sendRequest(requestBuilder.build(), dataTask, handler);
    }

    private <T> void makeAndSendRequest(List<String> pathSegments,
                                        Multimap<String, String> params,
                                        @Nullable Object json,
                                        String httpMethod,
                                        ResponseHandler handler) {
        RequestBody httpBody;
        if (json == null) {
            httpBody = null;

        } else try {
            httpBody = RequestBody.create(serializeObject(json), MEDIA_TYPE_JSON);

        } catch (JsonProcessingException e) {
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
                            } catch (JsonProcessingException e) {
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
        void handleResponse(String response) throws JsonProcessingException;
        void handleError(QueryError error);
    }

    private static class EmptyResponseHandler implements ResponseHandler {

        private final CompletionHandler<Void, QueryError> handler;

        EmptyResponseHandler(CompletionHandler<Void, QueryError> handler) {
            this.handler = handler;
        }

        @Override
        public void handleResponse(String response) {
            handler.handleData(null);
        }

        @Override
        public void handleError(QueryError error) {
            handler.handleError(error);
        }
    }

    private static class RootObjectResponseHandler<T> implements ResponseHandler {

        private final Class<T> clazz;
        private final CompletionHandler<T, QueryError> handler;

        RootObjectResponseHandler(Class<T> clazz, CompletionHandler<T, QueryError> handler) {
            this.clazz = clazz;
            this.handler = handler;
        }

        @Override
        public void handleResponse(String responseData) throws JsonProcessingException{
            T resp = deserializeObject(clazz, responseData);

            if (resp == null) {
                QueryError e = new QueryModelError("Transform error");
                Log.log(Level.SEVERE, "parsing error", e);
                handler.handleError(e);

            } else {
                handler.handleData(resp);
            }
        }

        @Override
        public void handleError(QueryError error) {
            handler.handleError(error);
        }
    }

    private static class EmbeddedArrayResponseHandler<T> implements ResponseHandler {

        private final String path;
        private final Class<T> clazz;
        private final CompletionHandler<List<T>, QueryError> handler;

        EmbeddedArrayResponseHandler(String path, Class<T> clazz, CompletionHandler<List<T>, QueryError> handler) {
            this.path = path;
            this.clazz = clazz;
            this.handler = handler;
        }

        @Override
        public void handleResponse(String responseData) throws JsonProcessingException{
            BdbEmbeddedResponse resp = deserializeObject(BdbEmbeddedResponse.class, responseData);
            List<T> data = (resp == null || resp.embedded == null || !resp.embedded.containsKey(path)) ?
                    Collections.emptyList() :
                    deserializeList(clazz, resp.embedded.get(path));
            if (data == null) {
                QueryError e = new QueryModelError("Transform error");
                Log.log(Level.SEVERE, "parsing error", e);
                handler.handleError(e);
                return;
            }

            handler.handleData(data);
        }

        @Override
        public void handleError(QueryError error) {
            handler.handleError(error);
        }
    }

    private static class EmbeddedPagedArrayResponseHandler<T> implements ResponseHandler {

        private final String path;
        private final Class<T> clazz;
        private final PagedCompletionHandler<List<T>, QueryError> handler;


        EmbeddedPagedArrayResponseHandler(String path, Class<T> clazz, PagedCompletionHandler<List<T>, QueryError> handler) {
            this.path = path;
            this.clazz = clazz;
            this.handler = handler;
        }

        @Override
        public void handleResponse(String responseData) throws JsonProcessingException{
            BdbEmbeddedResponse resp = deserializeObject(BdbEmbeddedResponse.class, responseData);
            List<T> data = (resp == null || resp.embedded == null || !resp.embedded.containsKey(path)) ?
                    Collections.emptyList() :
                    deserializeList(clazz, resp.embedded.get(path));
            if (data == null) {
                QueryError e = new QueryModelError("Transform error");
                Log.log(Level.SEVERE, "parsing error", e);
                handler.handleError(e);
                return;
            }

            checkState(resp != null);
            checkState(resp.links != null);

            handler.handleData(
                    data,
                    resp.links.prev != null ? resp.links.prev.href : null,
                    resp.links.next != null ? resp.links.next.href : null
            );
        }

        @Override
        public void handleError(QueryError error) {
            handler.handleError(error);
        }
    }

    // JSON methods

    private static final ObjectMapper MAPPER_JSON = new ObjectMapper()
            .configure(DeserializationFeature.FAIL_ON_UNKNOWN_PROPERTIES, false);

    private static String serializeObject(Object obj) throws JsonProcessingException {
        return MAPPER_JSON.writeValueAsString(obj);
    }

    private static <X> X deserializeObject(Class<X> clazz, String json) throws JsonProcessingException {
        return MAPPER_JSON.readValue(json, clazz);
    }

    private static <X> List<X> deserializeList(Class<X> clazz, Object obj) throws IllegalArgumentException {
        TypeFactory typeFactory = MAPPER_JSON.getTypeFactory();
        JavaType type = typeFactory.constructCollectionLikeType(ArrayList.class, clazz);
        return MAPPER_JSON.convertValue(obj, type);
    }
}
