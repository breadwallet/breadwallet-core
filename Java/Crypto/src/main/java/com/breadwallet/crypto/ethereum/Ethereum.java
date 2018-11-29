package com.breadwallet.crypto.ethereum;

import com.breadwallet.crypto.Currency;
import com.breadwallet.crypto.Network;
import com.breadwallet.crypto.Unit;

public interface Ethereum {
    Currency currency = new Currency ("ETH", "Ξ", "Ethereum", 18, "WEI", "wei");
    interface Units {
        Unit WEI = currency.baseUnit;
        Unit ETHER = currency.defaultUnit;

        Unit GWEI = new Unit ("GWEI", "gwei", 1000000000, WEI);
    }

    interface Networks {
        Network mainnet = new Network(new Network.Ethereum("Eth Mainnet", 1));
        Network ropsten = new Network(new Network.Ethereum("Eth Ropsten", 3));
        Network rinkeby = new Network(new Network.Ethereum("Eth Rinkeby", 4));
    }
}

