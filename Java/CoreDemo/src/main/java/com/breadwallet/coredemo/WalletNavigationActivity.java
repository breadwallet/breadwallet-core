package com.breadwallet.coredemo;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;

import com.breadwallet.core.ethereum.BREthereumNetwork;

public class WalletNavigationActivity extends AppCompatActivity {
    static { System.loadLibrary("core"); }

    static CoreDemoEthereumClient client = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        System.out.println ("Starting");

        client = new CoreDemoEthereumClient(BREthereumNetwork.mainnet,
            "boring head harsh green empty clip fatal typical found crane dinner timber");

        client.ewm.announceToken("0x722dd3f80bac40c951b51bdd28dd19d435762180",
                "BRD",
                "BRD Token",
                "",
                18,
                "92000",
                "1000000000",
                0);
        client.ewm.connect();

        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_wallet_navigation);
    }
}
