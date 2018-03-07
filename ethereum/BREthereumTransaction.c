//
//  BBREthereumTransaction.c
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 2/21/2018.
//  Copyright (c) 2018 breadwallet LLC
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#include <stdlib.h>
#include <string.h>
#include "BREthereumTransaction.h"
#include "BREthereumHolding.h"
#include "BREthereumAccount.h"

struct BREthereumTransactionRecord {
    BREthereumAddress sourceAddress;
    BREthereumAddress targetAddress;
    BREthereumHolding amount;
    BREthereumGasPrice gasPrice;
    BREthereumGas gasLimit;

    char *data;

    /**
     * The signature, if signed (signer is not NULL).  This is a 'VRS' signature.
     */
    BREthereumSignature signature;

    /**
     * The signing account, if signed.  NULL is not signed.
     */
    BREthereumAccount signer;

    uint64_t nonce;
    // hash

    // EIP-135 - chainId - "Since EIP-155 use chainId for v"
};

extern BREthereumTransaction
transactionCreate(BREthereumAddress sourceAddress,
                  BREthereumAddress targetAddress,
                  BREthereumHolding amount,
                  BREthereumGasPrice gasPrice,
                  BREthereumGas gasLimit,
                  uint64_t nonce) {
    BREthereumTransaction transaction = calloc (1, sizeof (struct BREthereumTransactionRecord));

    transaction->sourceAddress = sourceAddress;
    transaction->targetAddress = targetAddress;
    transaction->amount = amount;
    transaction->gasPrice = gasPrice;
    transaction->gasLimit = gasLimit;
    transaction->nonce = nonce;

    transaction->signer = NULL;

    return transaction;
}


extern BREthereumAddress
transactionGetSourceAddress(BREthereumTransaction transaction) {
    return transaction->sourceAddress;
}

extern BREthereumAddress
transactionGetTargetAddress(BREthereumTransaction transaction) {
    return transaction->targetAddress;
}

extern BREthereumHolding
transactionGetAmount(BREthereumTransaction transaction) {
    return transaction->amount;
}

extern BREthereumGasPrice
transactionGetGasPrice (BREthereumTransaction transaction) {
    return transaction->gasPrice;
}

extern BREthereumGas
transactionGetGasLimit (BREthereumTransaction transaction) {
    return transaction->gasLimit;
}

//
// Data
//
extern void
transactionSetData (BREthereumTransaction transaction, char *data) {
    transaction->data = malloc(1 + strlen(data));
    strcpy(transaction->data, data);
}

//
// Sign
//
extern void
transactionSign(BREthereumTransaction transaction,
                BREthereumAccount signer,
                BREthereumSignature signature) {
    transaction->signer = signer;
    transaction->signature = signature;
}

extern BREthereumAccount
transactionGetSigner (BREthereumTransaction transaction) {
    return transaction->signer; // NULL is not signed.
}


extern BREthereumBoolean
transactionIsSigned (BREthereumTransaction transaction) {
  return NULL != transactionGetSigner (transaction) ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE;
}

//
// RLP
//

extern BRRlpData
transactionEncodeRLP (BREthereumTransaction transaction,
                      BREthereumTransactionRLPType type) {

  BRRlpCoder coder = rlpCoderCreate();

  BRRlpItem items[10];
  size_t itemsCount = 0;

  items[0] = rlpEncodeItemUInt64(coder, transaction->nonce);
  items[1] = gasPriceRlpEncode(transaction->gasPrice, coder);
  items[2] = gasRlpEncode(transaction->gasLimit, coder);
  items[3] = addressRlpEncode(transaction->targetAddress, coder);
  items[4] = holdingRlpEncode(transaction->amount, coder);
  items[5] = rlpEncodeItemString(coder, transaction->data);
  itemsCount = 6;

  switch (type) {
    case TRANSACTION_RLP_UNSIGNED:
      break;

    case TRANSACTION_RLP_SIGNED:
      items[6] = rlpEncodeItemUInt64(coder, transaction->signature.sig.recoverable.v);

      items[7] = rlpEncodeItemBytes (coder,
                          transaction->signature.sig.recoverable.r,
                          sizeof (transaction->signature.sig.recoverable.r));

      items[8] = rlpEncodeItemBytes (coder,
                          transaction->signature.sig.recoverable.s,
                          sizeof (transaction->signature.sig.recoverable.s));
      itemsCount += 3;
      break;
  }

  BRRlpItem encoding = rlpEncodeListItems(coder, items, itemsCount);
  BRRlpData result;

  rlpGetData(coder, encoding, &result.bytes, &result.bytesCount);
  rlpCoderRelease(coder);

  return result;
}

extern BREthereumTransaction
createTransactionDecodeRLP (BRRlpData data,
                            BREthereumTransactionRLPType type) {
    BREthereumTransaction transaction;

  memset (&transaction, sizeof(struct BREthereumTransactionRecord), 0);
  return transaction;
}

//
// Transaction Result
//
struct BREthereumTransactionResult {
    BREthereumTransaction transaction;
    BREthereumGas gas;
    // block hash
    // block number
    // transaction index
};

/*
     https://github.com/ethereum/pyethereum/blob/develop/ethereum/transactions.py#L22
     https://github.com/ethereum/pyrlp/blob/develop/rlp/sedes/lists.py#L135

     A transaction is stored as:
    [nonce, gasprice, startgas, to, value, data, v, r, s]
    nonce is the number of transactions already sent by that account, encoded
    in binary form (eg.  0 -> '', 7 -> '\x07', 1000 -> '\x03\xd8').
    (v,r,s) is the raw Electrum-style signature of the transaction without the
    signature made with the private key corresponding to the sending account,
    with 0 <= v <= 3. From an Electrum-style signature (65 bytes) it is
    possible to extract the public key, and thereby the address, directly.
    A valid transaction is one where:
    (i) the signature is well-formed (ie. 0 <= v <= 3, 0 <= r < P, 0 <= s < N,
        0 <= r < P - N if v >= 2), and
    (ii) the sending account has enough funds to pay the fee and the value.
    """

    fields = [
        ('nonce', big_endian_int),
        ('gasprice', big_endian_int),
        ('startgas', big_endian_int),
        ('to', utils.address),
        ('value', big_endian_int),
        ('data', binary),
        ('v', big_endian_int),
        ('r', big_endian_int),
        ('s', big_endian_int),
    ]

 */


/*
 $ curl -X POST -H  "Content-Type: application/json" --data '{"jsonrpc":"2.0","method":"eth_getTransactionByHash","params":["0x3104b0ee2aba4197f4da656d6144e5978c0b7bcb08890ed7bd6228bc9dbe745e"],"id":1}' http://localhost:8545
    {"jsonrpc":"2.0","id":1,
    "result":{"blockHash":"0xbf197f8ce876514b8922af10824efba5b4ce3fc7ab9ef97443ef9c56bd0cae32",
        "blockNumber":"0x1b930a",
        "from":"0x888197521cfe05ff89960c50012252008819b2cb",
        "gas":"0x1d8a8",
        "gasPrice":"0x4a817c800",
        "hash":"0x3104b0ee2aba4197f4da656d6144e5978c0b7bcb08890ed7bd6228bc9dbe745e",
        "input":"0x",
        "nonce":"0x0",
        "to":"0xf8e60edd24bc15f32bb4260ec2cea7c54cced121",
        "transactionIndex":"0x3",
        "value":"0xde0b6b3a7640000",
        "v":"0x2b",
        "r":"0xa571650cb08199d808b6646f634a8f7431cfd103a243654263faf2518e3efd40",
        "s":"0x4d2774147ccb90d1e7ad9358eb895c5f5d24db26b9d3e880bcee4fa06e5b3e1b"}}
 */

/*
 Signing

 https://bitcoin.stackexchange.com/questions/38351/ecdsa-v-r-s-what-is-v

 > msgSha = web3.sha3('Now it the time')
"0x8b3942af68acfd875239181babe9ce093c420ca78d15b178fb63cf839dcf0971"

 > personal.unlockAccount(eth.accounts[<n>], "password", 3600)

 $ curl -X POST -H  "Content-Type: application/json" --data '{"jsonrpc":"2.0","method":"eth_sign","
    params":["0xf8e60edd24bc15f32bb4260ec2cea7c54cced121", "0x8b3942af68acfd875239181babe9ce093c420ca78d15b178fb63cf839dcf0971"],
    "id":1}'
    http://localhost:8545

 {"jsonrpc":"2.0","id":1,
    "result":"0xe79ba93e981e8ee50b8d07b0be7ae4526bc4d9bf7dcffe88dff62c502b2a126d7f772e2374869b41b0f5c0061d6d828348c96a7021f0c3227e73431d8ebbf1331b"}
 */

/*
 * r, s, v

  signature = signature.substr(2); //remove 0x
  const r = '0x' + signature.slice(0, 64)
  const s = '0x' + signature.slice(64, 128)
  const v = '0x' + signature.slice(128, 130)
  const v_decimal = web3.toDecimal(v)

 > web3.eth.sign ("0xf8e60edd24bc15f32bb4260ec2cea7c54cced121", "0x8b3942af68acfd875239181babe9ce093c420ca78d15b178fb63cf839dcf0971")
"0xe79ba93e981e8ee50b8d07b0be7ae4526bc4d9bf7dcffe88dff62c502b2a126d7f772e2374869b41b0f5c0061d6d828348c96a7021f0c3227e73431d8ebbf1331b"
 */

/*
 > msg = 'Try again'
 > msgSha = web3.sha3(msg)
 > sig = web3.eth.sign ("0xf8e60edd24bc15f32bb4260ec2cea7c54cced121", msgSha) // account, sha-ed msg
 > personal.ecRecover (msgSha, sig)
 "0xf8e60edd24bc15f32bb4260ec2cea7c54cced121"
 > eth.accounts[1]
 "0xf8e60edd24bc15f32bb4260ec2cea7c54cced121"
 */


/*

 ===== RLP Encode - =====

 All this method use some form of rlp.encode(<transaction>, ...)
   sigHash = utils.sha3 (rlp.encode (self, UnsignedTransaction)

     @property
    def sender(self):
        if not self._sender:
            # Determine sender
            if self.r == 0 and self.s == 0:
                self._sender = null_address
            else:
                if self.v in (27, 28):
                    vee = self.v
                    sighash = utils.sha3(rlp.encode(self, UnsignedTransaction))
                elif self.v >= 37:
                    vee = self.v - self.network_id * 2 - 8
                    assert vee in (27, 28)
                    rlpdata = rlp.encode(rlp.infer_sedes(self).serialize(self)[
                                         :-3] + [self.network_id, '', ''])
                    sighash = utils.sha3(rlpdata)
                else:
                    raise InvalidTransaction("Invalid V value")
                if self.r >= secpk1n or self.s >= secpk1n or self.r == 0 or self.s == 0:
                    raise InvalidTransaction("Invalid signature values!")
                pub = ecrecover_to_pub(sighash, vee, self.r, self.s)
                if pub == b'\x00' * 64:
                    raise InvalidTransaction(
                        "Invalid signature (zero privkey cannot sign)")
                self._sender = utils.sha3(pub)[-20:]
        return self._sender

    @property
    def network_id(self):
        if self.r == 0 and self.s == 0:
            return self.v
        elif self.v in (27, 28):
            return None
        else:
            return ((self.v - 1) // 2) - 17

    @sender.setter
    def sender(self, value):
        self._sender = value

    def sign(self, key, network_id=None):
        """Sign this transaction with a private key.
        A potentially already existing signature would be overridden.
        """
        if network_id is None:
            rawhash = utils.sha3(rlp.encode(self, UnsignedTransaction))
        else:
            assert 1 <= network_id < 2**63 - 18
            rlpdata = rlp.encode(rlp.infer_sedes(self).serialize(self)[
                                 :-3] + [network_id, b'', b''])
            rawhash = utils.sha3(rlpdata)

        key = normalize_key(key)

        self.v, self.r, self.s = ecsign(rawhash, key)
        if network_id is not None:
            self.v += 8 + network_id * 2

        self._sender = utils.privtoaddr(key)
        return self

    @property
    def hash(self):
        return utils.sha3(rlp.encode(self))

    def to_dict(self):
        d = {}
        for name, _ in self.__class__.fields:
            d[name] = getattr(self, name)
            if name in ('to', 'data'):
                d[name] = '0x' + encode_hex(d[name])
        d['sender'] = '0x' + encode_hex(self.sender)
        d['hash'] = '0x' + encode_hex(self.hash)
        return d


 */


/*

 ##
    # Sign this transaction with a private key.
    #
    # A potentially already existing signature would be override.
    #
    def sign(key)
      raise InvalidTransaction, "Zero privkey cannot sign" if [0, '', Constant::PRIVKEY_ZERO, Constant::PRIVKEY_ZERO_HEX].include?(key)

      rawhash = Utils.keccak256 signing_data(:sign)
      key = PrivateKey.new(key).encode(:bin)

      vrs = Secp256k1.recoverable_sign rawhash, key
      self.v = encode_v(vrs[0])
      self.r = vrs[1]
      self.s = vrs[2]

      self.sender = PrivateKey.new(key).to_address

      self
    end


     def signing_data(mode)
      case mode
      when :sign
        if v == 0 # use encoding rules before EIP155
          RLP.encode(self, sedes: UnsignedTransaction)
        else
          raise InvalidTransaction, "invalid signature"
        end
      when :verify
        if v == V_ZERO || v == V_ONE # encoded v before EIP155
          RLP.encode(self, sedes: UnsignedTransaction)
        end
      else
        raise InvalidTransaction, "invalid signature"
      end
    end
  end


 def encode(obj, sedes=None, infer_serializer=True, cache=False):
    """Encode a Python object in RLP format.
    By default, the object is serialized in a suitable way first (using :func:`rlp.infer_sedes`)
    and then encoded. Serialization can be explicitly suppressed by setting `infer_serializer` to
    ``False`` and not passing an alternative as `sedes`.
    If `obj` has an attribute :attr:`_cached_rlp` (as, notably, :class:`rlp.Serializable`) and its
    value is not `None`, this value is returned bypassing serialization and encoding, unless
    `sedes` is given (as the cache is assumed to refer to the standard serialization which can be
    replaced by specifying `sedes`).
    If `obj` is a :class:`rlp.Serializable` and `cache` is true, the result of the encoding will be
    stored in :attr:`_cached_rlp` if it is empty and :meth:`rlp.Serializable.make_immutable` will
    be invoked on `obj`.
    :param sedes: an object implementing a function ``serialize(obj)`` which will be used to
                  serialize ``obj`` before encoding, or ``None`` to use the infered one (if any)
    :param infer_serializer: if ``True`` an appropriate serializer will be selected using
                             :func:`rlp.infer_sedes` to serialize `obj` before encoding
    :param cache: cache the return value in `obj._cached_rlp` if possible and make `obj` immutable
                  (default `False`)
    :returns: the RLP encoded item
    :raises: :exc:`rlp.EncodingError` in the rather unlikely case that the item is too big to
             encode (will not happen)
    :raises: :exc:`rlp.SerializationError` if the serialization fails
    """


 https://github.com/ethereum/pyrlp/blob/develop/rlp/sedes/lists.py
 *
 */



/*
     public byte[] getEncoded() {

        if (rlpEncoded != null) return rlpEncoded;

        // parse null as 0 for nonce
        byte[] nonce = null;
        if (this.nonce == null || this.nonce.length == 1 && this.nonce[0] == 0) {
            nonce = RLP.encodeElement(null);
        } else {
            nonce = RLP.encodeElement(this.nonce);
        }
        byte[] gasPrice = RLP.encodeElement(this.gasPrice);
        byte[] gasLimit = RLP.encodeElement(this.gasLimit);
        byte[] receiveAddress = RLP.encodeElement(this.receiveAddress);
        byte[] value = RLP.encodeElement(this.value);
        byte[] data = RLP.encodeElement(this.data);

        byte[] v, r, s;

        if (signature != null) {
            int encodeV;
            if (chainId == null) {
                encodeV = signature.v;
            } else {
                encodeV = signature.v - LOWER_REAL_V;
                encodeV += chainId * 2 + CHAIN_ID_INC;
            }
            v = RLP.encodeInt(encodeV);
            r = RLP.encodeElement(BigIntegers.asUnsignedByteArray(signature.r));
            s = RLP.encodeElement(BigIntegers.asUnsignedByteArray(signature.s));
        } else {
            // Since EIP-155 use chainId for v
            v = chainId == null ? RLP.encodeElement(EMPTY_BYTE_ARRAY) : RLP.encodeInt(chainId);
            r = RLP.encodeElement(EMPTY_BYTE_ARRAY);
            s = RLP.encodeElement(EMPTY_BYTE_ARRAY);
        }

        this.rlpEncoded = RLP.encodeList(nonce, gasPrice, gasLimit,
                receiveAddress, value, data, v, r, s);

        this.hash = this.getHash();

        return rlpEncoded;
    }


     public synchronized void rlpParse() {
        if (parsed) return;
        try {
            RLPList decodedTxList = RLP.decode2(rlpEncoded);
            RLPList transaction = (RLPList) decodedTxList.get(0);

            // Basic verification
            if (transaction.size() > 9 ) throw new RuntimeException("Too many RLP elements");
            for (RLPElement rlpElement : transaction) {
                if (!(rlpElement instanceof RLPItem))
                    throw new RuntimeException("Transaction RLP elements shouldn't be lists");
            }

            this.nonce = transaction.get(0).getRLPData();
            this.gasPrice = transaction.get(1).getRLPData();
            this.gasLimit = transaction.get(2).getRLPData();
            this.receiveAddress = transaction.get(3).getRLPData();
            this.value = transaction.get(4).getRLPData();
            this.data = transaction.get(5).getRLPData();
            // only parse signature in case tx is signed
            if (transaction.get(6).getRLPData() != null) {
                byte[] vData =  transaction.get(6).getRLPData();
                BigInteger v = ByteUtil.bytesToBigInteger(vData);
                this.chainId = extractChainIdFromV(v);
                byte[] r = transaction.get(7).getRLPData();
                byte[] s = transaction.get(8).getRLPData();
                this.signature = ECDSASignature.fromComponents(r, s, getRealV(v));
            } else {
                logger.debug("RLP encoded tx is not signed!");
            }
            this.parsed = true;
            this.hash = getHash();
        } catch (Exception e) {
            throw new RuntimeException("Error on parsing RLP", e);
        }
    }

 */
