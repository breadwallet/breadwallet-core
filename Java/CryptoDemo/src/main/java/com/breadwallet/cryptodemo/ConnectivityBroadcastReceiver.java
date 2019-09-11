package com.breadwallet.cryptodemo;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.util.Log;

public class ConnectivityBroadcastReceiver extends BroadcastReceiver {

    private static final String TAG = ConnectivityBroadcastReceiver.class.getName();

    /* package */
    ConnectivityBroadcastReceiver() {

    }

    @Override
    public void onReceive(Context context, Intent intent) {
        if (ConnectivityManager.CONNECTIVITY_ACTION.equals(intent.getAction())) {
            boolean isNetworkReachable = Utilities.isNetworkReachable(context);

            Log.d(TAG, "isNetworkReachable: " + isNetworkReachable);
            CoreCryptoApplication.getSystem().setNetworkReachable(isNetworkReachable);
        }
    }
}
