package com.breadwallet.cryptodemo;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        SharedSystem.getInstance(this.getApplicationContext());

        setContentView(R.layout.activity_main);
    }
}
