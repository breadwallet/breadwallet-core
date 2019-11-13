package com.breadwallet.crypto.blockchaindb.apis.bdb;

import com.fasterxml.jackson.annotation.JsonProperty;

import java.util.Map;

public class BdbEmbeddedResponse {

    public static class Link {
        public String href;
    }

    public static class Links {
        public Link next;
        public Link prev;
    }

    @JsonProperty("_embedded")
    public Map<String, Object> embedded;

    @JsonProperty("_links")
    public Links links;
}
