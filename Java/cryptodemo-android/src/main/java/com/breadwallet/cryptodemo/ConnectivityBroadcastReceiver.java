/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.cryptodemo;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;

import java.util.logging.Level;
import java.util.logging.Logger;

public class ConnectivityBroadcastReceiver extends BroadcastReceiver {

    private static final Logger Log = Logger.getLogger(ConnectivityBroadcastReceiver.class.getName());

    /* package */
    ConnectivityBroadcastReceiver() {

    }

    @Override
    public void onReceive(Context context, Intent intent) {
        if (ConnectivityManager.CONNECTIVITY_ACTION.equals(intent.getAction())) {
            boolean isNetworkReachable = Utilities.isNetworkReachable(context);

            Log.log(Level.FINE, "isNetworkReachable: " + isNetworkReachable);
            CoreCryptoApplication.getSystem().setNetworkReachable(isNetworkReachable);
        }
    }
}
