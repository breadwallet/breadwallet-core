/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 11/15/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.apis.brd;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;

public class BrdJsonRpcResponse {

    // fields

    @JsonProperty("jsonrpc")
    private String jsonrpc;

    @JsonProperty("id")
    private String id;

    @JsonProperty("message")
    private String message;

    @JsonProperty("status")
    private String status;

    @JsonProperty("result")
    private Object result;

    // getters

    @JsonIgnore
    public String getJsonrpc() {
        return jsonrpc;
    }

    @JsonIgnore
    public String getId() {
        return id;
    }

    @JsonIgnore
    public String getMessage() {
        return message;
    }

    @JsonIgnore
    public String getStatus() {
        return status;
    }

    @JsonIgnore
    public Object getResult() {
        return result;
    }
}
