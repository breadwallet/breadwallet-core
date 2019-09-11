package com.breadwallet.cryptodemo;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;

public class Utilities {

    public static boolean isNetworkReachable(Context context) {
        ConnectivityManager cm = (ConnectivityManager)context.getSystemService(Context.CONNECTIVITY_SERVICE);

        NetworkInfo activeNetwork = cm.getActiveNetworkInfo();
        return activeNetwork != null && activeNetwork.isConnectedOrConnecting();
    }

    private Utilities() {}
}
