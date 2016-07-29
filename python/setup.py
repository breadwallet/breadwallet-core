import os
from setuptools import setup, Extension

def here(*args):
    return os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), *args))

def fromroot(*args):
    return here(os.pardir, *args)

sources = [
    fromroot('BRAddress.c'),
    fromroot('BRBase58.c'),
    fromroot('BRBIP32Sequence.c'),
    fromroot('BRBIP38Key.c'),
    fromroot('BRBIP39Mnemonic.c'),
    fromroot('BRBloomFilter.c'),
    fromroot('BRCrypto.c'),
    fromroot('BRKey.c'),
    fromroot('BRMerkleBlock.c'),
    fromroot('BRPaymentProtocol.c'),
    fromroot('BRPeer.c'),
    fromroot('BRPeerManager.c'),
    fromroot('BRSet.c'),
    fromroot('BRTransaction.c'),
    fromroot('BRWallet.c'),
    here('bindings.c')
]

includes = [fromroot(), fromroot('secp256k1')]

breadwallet_mainnet = Extension(
    'breadwallet_mainnet',
    sources=sources,
    include_dirs=includes
)

breadwallet_testnet = Extension(
    'breadwallet_testnet',
    sources=sources,
    include_dirs=includes,
    define_macros=[('BITCOIN_TESTNET', '1')]
)

setup(
    name='breadwallet-core',
    version='0.1',
    description='A simple, easy to use SPV wallet.',
    packages=['breadwallet'],
    ext_modules=[breadwallet_mainnet, breadwallet_testnet],
    test_suite='nose.collector',
    tests_require=['nose']
)
