/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 11/15/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.apis.bdb;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.google.common.base.Optional;

import java.util.Map;

public class BdbEmbeddedResponse {

    // fields

    @JsonProperty("_embedded")
    private Map<String, Object> embedded;

    @JsonProperty("_links")
    private Links links;

    // getters

    @JsonIgnore
    public boolean containsEmbedded(String path) {
        return embedded != null && embedded.containsKey(path);
    }

    @JsonIgnore
    public Optional<Object> getEmbedded(String path) {
        return embedded != null ? Optional.fromNullable(embedded.get(path)) : Optional.absent();
    }

    @JsonIgnore
    public Optional<String> getNextUrl() {
        if (null != links && null != links.next) {
            return Optional.fromNullable(links.next.href);
        }

        return Optional.absent();
    }

    @JsonIgnore
    public Optional<String> getPreviousUrl() {
        if (null != links && null != links.prev) {
            return Optional.fromNullable(links.prev.href);
        }

        return Optional.absent();
    }

    // internals

    public static class Link {
        @JsonProperty
        public String href;
    }

    public static class Links {
        @JsonProperty
        public Link next;

        @JsonProperty
        public Link prev;
    }
}
