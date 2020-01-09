'use strict';
const RippleAPI = require('ripple-lib').RippleAPI; // require('ripple-lib')

// To fund a test account
// 1. create a new test account on Ripple testnet - this account
//    will be used to fund your wallet account on ripple-testnet
//    copy the address and secret below
// 2. In your wallet - find the the ripple address for your test
//    account and copy it into the destination address below
// 3. The test account above only has a max of 1000 XRP, set the
//    amount for the destination to whaterver you want. If your test
//    account needs more than 900 XRP then you will have to repeat step
//    1 and get another funding account
const address = '';
const secret = '';

const api = new RippleAPI({server: 'wss://s.altnet.rippletest.net:51233'});
const instructions = {maxLedgerVersionOffset: 5};

const payment = {
  source: {
    address: address,
    maxAmount: {
      value: '900',
      currency: 'XRP'
    }
  },
  destination: {
	 // ripple address of your test account 
    address: 'r...',
    amount: {
      value: '100',
      currency: 'XRP'
    }
  }
};

function quit(message) {
  console.log(message);
  process.exit(0);
}

function fail(message) {
  console.error(message);
  process.exit(1);
}

api.connect().then(() => {
  console.log('Connected...');
  return api.preparePayment(address, payment, instructions).then(prepared => {
    console.log('Payment transaction prepared...');
    console.log(prepared.txJSON);
    const {signedTransaction} = api.sign(prepared.txJSON, secret);
    console.log('Payment transaction signed...');
    api.submit(signedTransaction).then(quit, fail);
  });
}).catch(fail);
