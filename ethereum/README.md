# Ethereum

Provides an Ethereum P2P implementation.  Acts as a 'Light Client' and communicates with either GETH
LESv2 or Parity PIPv1 nodes.  Interacts, through callbacks, with an Application, notably an IOS or
an Android application.

A .xcodeproj file is defined in '../Swift/'; a gradle project is defined in '../Java/' for both the
Bitcoin and the Etheruem code.

## Code Modules

### EWM

Implements an 'Ethereum Wallet Manager' (EWM) which uses a BIP-39-derived account and connects to a
specified Etheruem network ('foundation',  'ropsten' or 'rinkeby').  Based on the account's address
the EWM synchronizes the full block chain to determine transactions and log events for the provided
address.  EWM maintains a 'wallet holding ETH' and based on specified ERC20 tokens will create
and  populate a 'wallet holding <TOKEN>' for each specified ERC20 token.

The EWM uses a 'client' - typically implemented in the IOS or Android application.  The client is
used to gather data from BRD Services and to announce Events related to changes in EWM, Wallet or
Transaction states. 

The EWM synchronizes based on a specfied mode.  If the mode is 'BRD_ONLY' then EWM uses BRD (cloud)
services to get a User's transactions and logs (using the public address).  If the mode is
'P2P_ONLY' then EWM uses the P2P network for synchronization.  A unique algorithm is used to rapidly
sync the entire block chain (however, a limitation in the algorihm requires BRD services to provide
ERC20  events there address is the target). There are other intermeiate modes which use a
combination of BRD+P2P for synchronization and transfer submission.

The EWM module defines key abstractions for: Wallet Manager, Wallet, Transfer (absraction on
transaction and log), Amount and Account. 

### BCS

Implements a 'Block Chain Slice' (BCS) which is the 'slice' of the Ethereum block chain relevent to
the User's address.  The BCS chains together blocks (as blocks are announced by the P2P network) and
keeps set of transations and logs.  When a transaction or log is discovered it is announced to the
EWM where it is maintained in the corresponding wallet (ETH for transactions, a 'wallet holding
<TOKEN>' or logs).

BCS using LES for interaction with the Ethereum P2P network.

### LES

Implements the 'Light Ethereum Subprotocol' for communication with Geth LESv2 or Parity PIPv1
nodes. LES implements an abstraction over LESv2 and PIPv1 given that those two protocols are
incompatible.  BCS uses the abstraction.

LES works hard, very hard, to find nodes supporting light clients.

LES uses the DIS (v4) protocol to discover Ethereum nodes; LES tries to maintain at least N (~100)
acceptable nodes.  LES connects with up to M (~3) nodes to actively query the block chain.  When
submitting a transaction LES will submit the all active nodes.  As a node announces a new block, a
BCS callback in invoked; that callback includes a reference to the node that announded the block.
BCS then queries that specific node.

### Block Chain

The basic Ethereum concepts of: Account State, Block, Bloom Filter, Log, Network, Transaction,
Transaction Status and Transaction Receipt.  These concepts are based nearly %100 on the Ethereum
Yellow Paper specification.

### Base

Lowest level concepts of: Address, Ether, Gas, Hash and Signature

### Contract

A quasi-generic (but-not-really) implementation of a Smart Contract.  Definition of an ERC20 token.

### Event

A message-passing implementation.

### RLP

RLP encoding of Ethereum data.  Includes the Ethereum-specified 'Network' type but also additional
types, notably 'Archive' which is used fo persistent storage.

### Util

Math operations for UInt256 numbers.  Support for 'Keccak'


