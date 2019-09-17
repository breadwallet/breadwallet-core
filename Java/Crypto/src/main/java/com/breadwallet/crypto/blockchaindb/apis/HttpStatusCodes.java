/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.apis;

import com.google.common.collect.ImmutableSet;

import java.util.Set;

public final class HttpStatusCodes {

    /// Source: https://tools.ietf.org/html/rfc7231#page-24

    // The 200 (OK) status code indicates that the request has succeeded.
    private static Set<Integer> SUCCESS_CODES_GET = ImmutableSet.of(200);

    // If one or more resources has been created on the origin server as a
    // result of successfully processing a POST request, the origin server
    // SHOULD send a 201 (Created) response containing a Location header
    // field that provides an identifier for the primary resource created
    // Section 7.1.2) and a representation that describes the status of the
    // request while referring to the new resource(s).
    //
    // Responses to POST requests are only cacheable when they include
    // explicit freshness information (see Section 4.2.1 of [RFC7234]).
    // However, POST caching is not widely implemented.  For cases where an
    // origin server wishes the client to be able to cache the result of a
    // POST in a way that can be reused by a later GET, the origin server
    // MAY send a 200 (OK) response containing the result and a
    // Content-Location header field that has the same value as the POST's
    // effective request URI (Section 3.1.4.2).
    private static Set<Integer> SUCCESS_CODES_POST = ImmutableSet.of(200, 201);

    // If a DELETE method is successfully applied, the origin server SHOULD
    // send a 202 (Accepted) status code if the action will likely succeed
    // but has not yet been enacted, a 204 (No Content) status code if the
    // action has been enacted and no further information is to be supplied,
    // or a 200 (OK) status code if the action has been enacted and the
    // response message includes a representation describing the status.
    private static Set<Integer> SUCCESS_CODES_DELETE = ImmutableSet.of(200, 202, 204);

    // If the target resource does not have a current representation and the
    // PUT successfully creates one, then the origin server MUST inform the
    // user agent by sending a 201 (Created) response.  If the target
    // resource does have a current representation and that representation
    // is successfully modified in accordance with the state of the enclosed
    // representation, then the origin server MUST send either a 200 (OK) or
    // a 204 (No Content) response to indicate successful completion of the
    // request.
    private static Set<Integer> SUCCESS_CODES_PUT = ImmutableSet.of(200, 201, 204);

    private static Set<Integer> SUCCESS_CODES_DEFAULT = ImmutableSet.of(200);

    public static Set<Integer> responseSuccess (String httpMethod){
        switch (httpMethod) {
            case "GET":
                return SUCCESS_CODES_GET;
            case "POST":
                return SUCCESS_CODES_POST;
            case "DELETE":
                return SUCCESS_CODES_DELETE;
            case "PUT":
                return SUCCESS_CODES_PUT;
            default:
                return SUCCESS_CODES_DEFAULT;
        }
    }

    private HttpStatusCodes() {
        // don't allow instantiation
    }
}
