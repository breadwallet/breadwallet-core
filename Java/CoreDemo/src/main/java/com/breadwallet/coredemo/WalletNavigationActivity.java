package com.breadwallet.coredemo;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;

import com.breadwallet.core.ethereum.BREthereumNetwork;

public class WalletNavigationActivity extends AppCompatActivity {
    static { System.loadLibrary("core"); }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_wallet_navigation);


        BREthereumNetwork mainnet = BREthereumNetwork.mainnet;
    }
}
