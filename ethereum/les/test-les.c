//
//  test-les.h
//  breadwallet-core Ethereum
//
//  Created by Lamont Samuels on 4/16/18.
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

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <regex.h>
#include "test-les.h"
#include "BRInt.h"
#include "BREthereumLES.h"
#include "BREthereumNode.h"
#include "BREthereumNodeEventHandler.h"
#include "BREthereumNodeManager.h"
#include "BREthereumNetwork.h"
#include "BREthereumNodeDiscovery.h"
#include "BREthereumRandom.h"
#include "BRCrypto.h"
#include "BREthereum.h"
#include "BREthereumHandshake.h"

// LES Tests

#define TEST_PAPER_KEY "army van defense carry jealous true garbage claim echo media make crunch"
#define DEFAULT_UDP_PORT 30303
#define DEFAULT_TCP_PORT 30303 
#define PRI_KEY_BYTES 32
#define PUB_KEY_BYTES 64
/*
int find_ip_address(char *hostname, char *ip_address)
{
      struct hostent *host_name;
      struct in_addr **ipaddress;
      int count;
      if((host_name = gethostbyname(hostname)) == NULL)
      {
            herror("\nIP Address Not Found\n");
            return 1;
      }
      else
      {
            ipaddress = (struct in_addr **) host_name->h_addr_list;
            for(count = 0; ipaddress[count] != NULL; count++)
            {
                  strcpy(ip_address, inet_ntoa(*ipaddress[count]));
                  return 0;
            }
      }
      return 1;
}
*/


void runEthereumNodeDiscoveryTests(void) {
    
    BREthereumAccount account = createAccount(TEST_PAPER_KEY);
    BRKey key = accountGetPrimaryAddressPrivateKey (account,TEST_PAPER_KEY);
    
    size_t pkLen =  0;
    
    //Going to Ping the Mainnet bootnode for Ethereum
    // "enode://3f1d12044546b76342d59d4a05532c14b85aa669704bfe1f864fe079415aa2c02d743e03218e57a33fb94523adb54032871a6c51b2cc5514cb7c7e35b3ed0a99@13.93.211.84:30303",  // US-WEST
    
    //Ping the node to get it's remote id
    BREthereumEndpoint to   = ethereumNodeDiscoveryCreateEndpoint(AF_INET,  "13.93.211.84", DEFAULT_UDP_PORT, DEFAULT_TCP_PORT);
    BREthereumEndpoint from = ethereumNodeDiscoveryCreateEndpoint(AF_INET,  "127.0.0.1", DEFAULT_UDP_PORT, DEFAULT_TCP_PORT);
    BREthereumPingNode pingNode = ethereumNodeDiscoveryCreatePing(to, from);
    
    BRKey remoteNodeKey;
    BREthereumPongNode pongNode;
    
    int error = ethereumNodeDiscoveryPing(&key, pingNode, pongNode, &remoteNodeKey);
    
    assert(error == 0);
    
    char* mainnetPubKey = "3f1d12044546b76342d59d4a05532c14b85aa669704bfe1f864fe079415aa2c02d743e03218e57a33fb94523adb54032871a6c51b2cc5514cb7c7e35b3ed0a99";
    
    UInt256 upperHalf = uint256(mainnetPubKey);
    UInt256 lowerHalf = uint256(&mainnetPubKey[64]);
   
    uint8_t gotPubKey[65];
    BRKeyPubKey(&remoteNodeKey, gotPubKey, 65);

    UInt256 gotUpperHalf = UINT256_ZERO;
    UInt256 gotLowerHalf = UINT256_ZERO;
    
    UInt256Set(&gotPubKey[1], gotUpperHalf);
    UInt256Set(&gotPubKey[1 + 32], gotLowerHalf);

    printf("Expected=%s::::%s\n", u256hex(upperHalf),u256hex(gotLowerHalf));
    printf("Got     =%s::::%s\n", u256hex(gotUpperHalf),u256hex(gotUpperHalf));

    assert(memcmp(upperHalf.u8,&gotPubKey[1], 32) == 0);
    assert(memcmp(lowerHalf.u8,&gotPubKey[1 + 32], 32) == 0);

    free(to);
    free(from);
    free(pingNode);

}
void runEthereumNodeEventHandlerTests() {

    BREthereumNodeEvent event, out;
    BREthereumNodeEventHandler handler = ethereumNodeEventHandlerCreate();
    assert(handler != NULL);
    assert(ethereumNodeEventHandlerSize(handler) == 0);
    ethereumNodeEventHandlerDequeue(handler, &out);
    assert(ethereumNodeEventHandlerSize(handler) == 0);
    assert(ETHEREUM_BOOLEAN_IS_FALSE(ethereumNodeEventHandlerHasEvent(handler,BRE_NODE_EVENT_SUBMIT_TRANSACTION)));

    
    event.type = BRE_NODE_EVENT_FREE;
    ethereumNodeEventHandlerEnqueue(handler, event);
    assert(ethereumNodeEventHandlerSize(handler) == 1);
    event.type = BRE_NODE_EVENT_DISCONNECT;
    ethereumNodeEventHandlerEnqueue(handler, event);
    assert(ethereumNodeEventHandlerSize(handler) == 2);
    event.type = BRE_NODE_EVENT_FREE;
    ethereumNodeEventHandlerEnqueue(handler, event);
    assert(ethereumNodeEventHandlerSize(handler) == 3);
    
    ethereumNodeEventHandlerDequeue(handler, &out);
    assert(ethereumNodeEventHandlerSize(handler) == 2);
    assert(out.type == BRE_NODE_EVENT_FREE);
    
    assert(ETHEREUM_BOOLEAN_IS_FALSE(ethereumNodeEventHandlerHasEvent(handler,BRE_NODE_EVENT_SUBMIT_TRANSACTION)));
    
    event.type = BRE_NODE_EVENT_SUBMIT_TRANSACTION;
    uint8_t handler_trans[] = {97,98,99,100};
    event.u.submit_transaction.transaction = handler_trans;
    event.u.submit_transaction.size = sizeof(handler_trans);
    ethereumNodeEventHandlerEnqueue(handler, event);
    assert(ethereumNodeEventHandlerSize(handler) == 3);
    uint8_t handler_trans2[] = {65,66,67};
    event.u.submit_transaction.transaction = handler_trans2;
    event.u.submit_transaction.size = sizeof(handler_trans2);
    ethereumNodeEventHandlerEnqueue(handler, event);
    assert(ethereumNodeEventHandlerSize(handler) == 4);
    
    assert(ETHEREUM_BOOLEAN_IS_TRUE(ethereumNodeEventHandlerHasEvent(handler,BRE_NODE_EVENT_SUBMIT_TRANSACTION)));

    
    size_t count = 4;
    for(int idx= 0; idx <7; ++idx){
        event.type = BRE_NODE_EVENT_FREE;
        ethereumNodeEventHandlerEnqueue(handler, event);
        assert(ethereumNodeEventHandlerSize(handler) == ++count);
    }
    
    ethereumNodeEventHandlerDequeue(handler, &out);
    assert(ethereumNodeEventHandlerSize(handler) == 10);
    assert(out.type == BRE_NODE_EVENT_DISCONNECT);
    
    ethereumNodeEventHandlerDequeue(handler, &out);
    assert(ethereumNodeEventHandlerSize(handler) == 9);
    assert(out.type == BRE_NODE_EVENT_FREE);
    
    ethereumNodeEventHandlerDequeue(handler, &out);
    assert(ethereumNodeEventHandlerSize(handler) == 8);
    assert(out.type == BRE_NODE_EVENT_SUBMIT_TRANSACTION);
    assert(out.u.submit_transaction.size == 4);
    assert(out.u.submit_transaction.transaction[0] == 97);
    assert(out.u.submit_transaction.transaction[1] == 98);
    assert(out.u.submit_transaction.transaction[2] == 99);
    assert(out.u.submit_transaction.transaction[3] == 100);
    
    ethereumNodeEventHandlerDequeue(handler, &out);
    assert(ethereumNodeEventHandlerSize(handler) == 7);
    assert(out.type == BRE_NODE_EVENT_SUBMIT_TRANSACTION);
    assert(out.u.submit_transaction.size == 3);
    assert(out.u.submit_transaction.transaction[0] == 65);
    assert(out.u.submit_transaction.transaction[1] == 66);
    assert(out.u.submit_transaction.transaction[2] == 67);

    count = 7;
    for(int idx= 0; idx <7; ++idx){
        ethereumNodeEventHandlerDequeue(handler, &out);
        assert(out.type == BRE_NODE_EVENT_FREE);
        assert(ethereumNodeEventHandlerSize(handler) == --count);
    }
    assert(ethereumNodeEventHandlerSize(handler) == 0);
    ethereumNodeEventHandlerDequeue(handler, &out);
    assert(ethereumNodeEventHandlerSize(handler) == 0);
    
    ethereumNodeEventHandlerRelease(handler); 
}

void runEthereumManagerTests() {

    //TODO: Write tests for EthereumManagerTests;
    //BREthereumNodeManager manager = ethereumNodeManagerCreate(ethereumMainnet);
    //assert(manager != NULL);
    //ethereumNodeMangerConnect(manager);
    //Wait for up to 24 seconds, to give the node manager time to connect to a remote node.
    sleep(24);

}

void runEthereumNodeTests() {
    /*
    BREthereumAccount account = createAccount(TEST_PAPER_KEY);
    BRKey key = accountGetPrimaryAddressPrivateKey (account,TEST_PAPER_KEY);
//35.232.131.113
    //Create the endpoint to the full node
    BREthereumEndpoint to  = ethereumEndpointCreate(ETHEREUM_BOOLEAN_TRUE, "35.232.131.113", DEFAULT_UDP_PORT, DEFAULT_TCP_PORT);

    //Create the remote public key
    BRKey remoteKey;
    uint8_t pubKey[64];
    decodeHex (pubKey, 64, "70801a3643843361ae9f562cc4e48a106b417d951a4c5a7070a536bcda9e4c87be4f776fcbf1dc09e79bc68dd0366f3be63fee2c62b2d9d2ca1d7055dee1e17f", 128);
    remoteKey.pubKey[0] = 0x0;
    memcpy(&remoteKey.pubKey[1], pubKey, 64);

    //Create the peer configuration for the node
    BREthereumPeerConfig config;
    config.endpoint = to;
    config.remoteKey = &remoteKey;
   
  
    //Create the Random local ephemeral and nonce
    BRKey ephemeral;
    UInt256 nonce;
    BREthereumRandomContext ctx = ethereumRandomCreate(key.secret.u8, 32);
    ethereumRandomGenPriKey(ctx,&ephemeral);
    ethereumRandomGenUInt256(ctx, &nonce);

    //Create the ethereum node
    BREthereumManagerCallback callbacks;
    callbacks.connectedFuc = _connectedCallback;
    callbacks.disconnectFunc = _disconnetCallback;
    callbacks.networkReachableFunc = _networkReachableCallback;
    callbacks.receivedMsgFunc = _receivedMessageCallback;
    ethereumNodeCreate(key, &nonce, &ephemeral, callbacks);
    BREthereumNode node = ethereumNodeCreate(config, &key, nonce, &ephemeral, _disconnectFailFunc, ETHEREUM_BOOLEAN_TRUE);
    
    //Connect to the endpoint
    assert(ethereumNodeConnect(node) == 0);
    
    
    //Wait for up to 24 seconds, to give the nodes time to start and find each other to perform the handshake;
    sleep(24);

    assert(ethereumNodeStatus(node) == BRE_NODE_CONNECTED);
 
    ethereumNodeRelease(node);
    ethereumEndpointRelease(to);
    accountFree(account);
    free(pubKey);
    */
}
void _disconnect(BREthereumManagerCallbackContext info, BREthereumNode node, BREthereumDisconnect reason) {

}
void _receievedMessage(BREthereumManagerCallbackContext info, BREthereumNode node, uint64_t packetType, BRRlpData message){

}
void _connect(BREthereumManagerCallbackContext info, BREthereumNode node, uint8_t** status, size_t* statusSize){

}
void _reachable(BREthereumManagerCallbackContext info, BREthereumNode node, BREthereumBoolean isReachable){

}

//This data comes from https://gist.github.com/fjl/3a78780d17c755d22df2
#define INITIATOR_PRIVATE_KEY "5e173f6ac3c669587538e7727cf19b782a4f2fda07c1eaa662c593e5e85e3051"
#define RECEIVER_PRIVATE_KEY  "c45f950382d542169ea207959ee0220ec1491755abe405cd7498d6b16adb6df8"
#define INITIATOR_EPHEMERAL_PRIVATE_KEY "19c2185f4f40634926ebed3af09070ca9e029f2edd5fae6253074896205f5f6c"
#define RECEIVER_EPHEMERAL_PRIVATE_KEY "d25688cf0ab10afa1a0e2dba7853ed5f1e5bf1c631757ed4e103b593ff3f5620"
#define AUTH_PLAINTEXT "884c36f7ae6b406637c1f61b2f57e1d2cab813d24c6559aaf843c3f48962f32f46662c066d39669b7b2e3ba14781477417600e7728399278b1b5d801a519aa570034fdb5419558137e0d44cd13d319afe5629eeccb47fd9dfe55cc6089426e46cc762dd8a0636e07a54b31169eba0c7a20a1ac1ef68596f1f283b5c676bae4064abfcce24799d09f67e392632d3ffdc12e3d6430dcb0ea19c318343ffa7aae74d4cd26fecb93657d1cd9e9eaf4f8be720b56dd1d39f190c4e1c6b7ec66f077bb1100"
#define AUTHRESP_PLAINTEST "802b052f8b066640bba94a4fc39d63815c377fced6fcb84d27f791c9921ddf3e9bf0108e298f490812847109cbd778fae393e80323fd643209841a3b7f110397f37ec61d84cea03dcc5e8385db93248584e8af4b4d1c832d8c7453c0089687a700"
#define AUTH_CIPHERTEXT "04a0274c5951e32132e7f088c9bdfdc76c9d91f0dc6078e848f8e3361193dbdc43b94351ea3d89e4ff33ddcefbc80070498824857f499656c4f79bbd97b6c51a514251d69fd1785ef8764bd1d262a883f780964cce6a14ff206daf1206aa073a2d35ce2697ebf3514225bef186631b2fd2316a4b7bcdefec8d75a1025ba2c5404a34e7795e1dd4bc01c6113ece07b0df13b69d3ba654a36e35e69ff9d482d88d2f0228e7d96fe11dccbb465a1831c7d4ad3a026924b182fc2bdfe016a6944312021da5cc459713b13b86a686cf34d6fe6615020e4acf26bf0d5b7579ba813e7723eb95b3cef9942f01a58bd61baee7c9bdd438956b426a4ffe238e61746a8c93d5e10680617c82e48d706ac4953f5e1c4c4f7d013c87d34a06626f498f34576dc017fdd3d581e83cfd26cf125b6d2bda1f1d56"
#define AUTHRESP_CIPHERTEXT "049934a7b2d7f9af8fd9db941d9da281ac9381b5740e1f64f7092f3588d4f87f5ce55191a6653e5e80c1c5dd538169aa123e70dc6ffc5af1827e546c0e958e42dad355bcc1fcb9cdf2cf47ff524d2ad98cbf275e661bf4cf00960e74b5956b799771334f426df007350b46049adb21a6e78ab1408d5e6ccde6fb5e69f0f4c92bb9c725c02f99fa72b9cdc8dd53cff089e0e73317f61cc5abf6152513cb7d833f09d2851603919bf0fbe44d79a09245c6e8338eb502083dc84b846f2fee1cc310d2cc8b1b9334728f97220bb799376233e113"
    
#define ECDHE_SHARED_SECRET "e3f407f83fc012470c26a93fdff534100f2c6f736439ce0ca90e9914f7d1c381"
#define INITIATOR_NONCE "cd26fecb93657d1cd9e9eaf4f8be720b56dd1d39f190c4e1c6b7ec66f077bb11"
#define RECEIVER_NONCE "f37ec61d84cea03dcc5e8385db93248584e8af4b4d1c832d8c7453c0089687a7"
#define AES_SECRET "c0458fa97a5230830e05f4f20b7c755c1d4e54b1ce5cf43260bb191eef4e418d"
#define MAC_SECRET  "48c938884d5067a1598272fcddaa4b833cd5e7d92e8228c0ecdfabbe68aef7f1"
#define TOKEN "3f9ec2592d1554852b1f54d228f042ed0a9310ea86d038dc2b401ba8cd7fdac4"
#define INITIAL_EGRESS_MAC "09771e93b1a6109e97074cbe2d2b0cf3d3878efafe68f53c41bb60c0ec49097e"
#define INITIAL_INGRESS_MAC "75823d96e23136c89666ee025fb21a432be906512b3dd4a3049e898adb433847"
#define INITIATOR_HELLO_PACKET  "6ef23fcf1cec7312df623f9ae701e63b550cdb8517fefd8dd398fc2acd1d935e6e0434a2b96769078477637347b7b01924fff9ff1c06df2f804df3b0402bbb9f87365b3c6856b45e1e2b6470986813c3816a71bff9d69dd297a5dbd935ab578f6e5d7e93e4506a44f307c332d95e8a4b102585fd8ef9fc9e3e055537a5cec2e9"
#define RECEIVER_HELLO_PACKET "6ef23fcf1cec7312df623f9ae701e63be36a1cdd1b19179146019984f3625d4a6e0434a2b96769050577657247b7b02bc6c314470eca7e3ef650b98c83e9d7dd4830b3f718ff562349aead2530a8d28a8484604f92e5fced2c6183f304344ab0e7c301a0c05559f4c25db65e36820b4b909a226171a60ac6cb7beea09376d6d8"
#define DEFAULT_AUTH_PORT 30303
void _createPrivKey(BRKey* key, char*hex, size_t hexSize){
    uint8_t privKey[PRI_KEY_BYTES];
    decodeHex (privKey, PRI_KEY_BYTES, hex, hexSize);
    memcpy(key->secret.u8, privKey, PRI_KEY_BYTES);
    key->compressed = 0; 
    uint8_t pubKey[65] = {0};
    memcpy(key->pubKey, pubKey, 65);
}
#define TESTING_HANDSHAKE 1
void runAuthTests() {
    
    //Create the remote public key
    BRKey initPrivKey;
    BRKey recvPrivKey;
    BRKey initEphemeralKey;
    BRKey recvEphemeralKey;

    _createPrivKey(&initPrivKey, INITIATOR_PRIVATE_KEY, 64);
    _createPrivKey(&recvPrivKey, RECEIVER_PRIVATE_KEY, 64);
    _createPrivKey(&initEphemeralKey, INITIATOR_EPHEMERAL_PRIVATE_KEY, 64);
    _createPrivKey(&recvEphemeralKey, RECEIVER_EPHEMERAL_PRIVATE_KEY, 64);
    
    //Create an BREthereum node to connect
    BREthereumEndpoint to  = ethereumEndpointCreate(ETHEREUM_BOOLEAN_TRUE, "127.0.0.1", DEFAULT_AUTH_PORT, DEFAULT_AUTH_PORT);

    //Create the peer configuration for the node
    BREthereumPeerConfig config;
    config.endpoint = to;
    config.remoteKey = &recvPrivKey;
    
    //INITIATOR_NONCE
    UInt256 initNonce;
    decodeHex(initNonce.u8, 32, INITIATOR_NONCE, 64);
    

    //RECEIVER_NONCE
    UInt256 receiverNonce;
    decodeHex(receiverNonce.u8, 32, RECEIVER_NONCE, 64);
    
    //Create the ethereum node for the initiator
    BREthereumManagerCallback callbacks;
    callbacks.connectedFuc = _connect;
    callbacks.disconnectFunc = _disconnect;
    callbacks.networkReachableFunc = _reachable;
    callbacks.receivedMsgFunc = _receievedMessage;
    
    BREthereumNode initiator = ethereumNodeCreate(config, &initPrivKey, &initNonce, &initEphemeralKey, callbacks, ETHEREUM_BOOLEAN_TRUE);
    
    //Create a handhsake for testing
    BREthereumHandshake handshake = ethereumHandshakeCreate(initiator);
   testInitatorHandshake(handshake, &recvEphemeralKey);
        
    
    //Create the ethereum node for the initiator
    _createPrivKey(&initPrivKey, INITIATOR_PRIVATE_KEY, 64);
    _createPrivKey(&recvPrivKey, RECEIVER_PRIVATE_KEY, 64);
    _createPrivKey(&initEphemeralKey, INITIATOR_EPHEMERAL_PRIVATE_KEY, 64);
    _createPrivKey(&recvEphemeralKey, RECEIVER_EPHEMERAL_PRIVATE_KEY, 64);
    decodeHex(receiverNonce.u8, 32, RECEIVER_NONCE, 64);
    decodeHex(initNonce.u8, 32, INITIATOR_NONCE, 64);
    
    config.remoteKey = &initPrivKey;
    BREthereumNode responder = ethereumNodeCreate(config, &recvPrivKey, &receiverNonce, &recvEphemeralKey, callbacks, ETHEREUM_BOOLEAN_FALSE);
    
    //Create a handhsake for testing
    BREthereumHandshake handshake2 = ethereumHandshakeCreate(responder);
    testReceiverHandshake(handshake2, &initPrivKey, &initEphemeralKey);
    
    /*
    //RECEIVER_HELLO_PACKET
    size_t recvHelloPacketHexSize = strlen(RECEIVER_HELLO_PACKET);
    size_t recvHelloPacketSize = recvHelloPacketHexSize/2;
    
    uint8_t* recvHelloPacket[recvHelloPacketSize];
    decodeHex(recvHelloPacket, recvHelloPacketSize, RECEIVER_HELLO_PACKET, recvHelloPacketHexSize);
    */
}
void _announceCallback (BREthereumLESAnnounceContext context,
                                  BREthereumHash headHash,
                                  uint64_t headNumber,
                                  uint64_t headTotalDifficulty) {
    
 
}
void transactionStatusCallback(BREthereumLESTransactionStatusContext context,
                               BREthereumHash transaction,
                               BREthereumTransactionStatus status){
    
    char transactionHashStr[] = "c070b1e539e9a329b14c95ec960779359a65be193137779bf2860dc239248d7c";
    BREthereumHash transactionHash;
    decodeHex(transactionHash.bytes, 32, transactionHashStr, strlen(transactionHashStr));
    
    assert(memcmp(transaction.bytes, transactionHash.bytes, 32) == 0);
    assert(status.type == TRANSACTION_STATUS_INCLUDED);
}

void runLESTest() {

    //Prepare values to be given to an les context
    BREthereumHash headHash;
    char headHashStr[] = "d4e56740f876aef8c010b86a40d5f56745a118d0906a34e69aec8c0db1cb8fa3";
    assert(32 == (strlen(headHashStr)/2));
    decodeHex(headHash.bytes, 32, headHashStr, strlen(headHashStr));

    uint64_t headNumber = 0;
    uint64_t headTD = 0x400000000;
    
    BREthereumHash genesisHash;
    decodeHex(genesisHash.bytes, 32, headHashStr, strlen(headHashStr));

    // Create an LES context
    BREthereumLES les = lesCreate(ethereumMainnet, NULL, _announceCallback, headHash, headNumber, headTD, genesisHash);
  
    //Sleep for a bit to allow the les context to connect to the network
    sleep(3);
    
    eth_log("LES-TESTS", "%s", "Sending Transaction Status Message");
  
    // Prepare values to be given to a send tranactions status message
    char transactionHashStr[] = "c070b1e539e9a329b14c95ec960779359a65be193137779bf2860dc239248d7c";
    assert(32 == (strlen(transactionHashStr)/2));
    BREthereumHash transactionHash;
    decodeHex(transactionHash.bytes, 32, headHashStr, strlen(headHashStr));
    
 //   assert(lesGetTransactionStatusOne(les, NULL, transactionStatusCallback, transactionHash) == LES_SUCCESS);
    
    //Sleep for a bit to allow the les context to connect to the network
    //sleep(600);
}
void runLEStests(void) {
    
     runLESTest();
  // runEthereumNodeTests();
  // runEthereumNodeEventHandlerTests();
  // runEthereumNodeDiscoveryTests();
  //   runAuthTests();
  // runEthereumNodeTests();
  //   runLESMessagesTest();

}

