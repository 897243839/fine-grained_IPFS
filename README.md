![kubo, an IPFS node in Go](https://ipfs.io/ipfs/bafykbzacecaesuqmivkauix25v6i6xxxsvsrtxknhgb5zak3xxsg2nb4dhs2u/ipfs.go.png)


IPFS is a global, versioned, peer-to-peer filesystem. It combines good ideas from previous systems such as Git, BitTorrent, Kademlia, SFS, and the Web. It is like a single BitTorrent swarm, exchanging git objects. IPFS provides an interface as simple as the HTTP web, but with permanence built-in. You can also mount the world at /ipfs.
# what is ipfs
For more info see: https://docs.ipfs.tech/concepts/what-is-ipfs/
## What is a fine-grained compression scheme for the native IPFS protocol?

  Interplanetary File System (IPFS) network, a prominent peer-to-peer decentralized file-sharing
network, is extensively utilized by decentralized applications such as Blockchain and Metaverse for
data sharing. With a substantial upsurge in data, compression schemes have been incorporated into the
native IPFS protocol to enhance the efficiency of block transmission over an IPFS network. However,
the protocols extended by these schemes drastically impair data shareability with the native protocol,
contravening the core principles of IPFS-based distributed storage.


  To address the issue, this paper presents a fine-grained compression scheme for the native IPFS
protocol. Its primary conceptis that blocks rather thanfilesareconsideredasthesmallestcompressible
units for block transmission, thereby ensuring the preservation of the data shareability with the
protocol. To achieve the scheme, a three-layer architecture is firstly proposed and incorporated into the
protocol as an independent compression component to support a variety of compression algorithms.
Consequently, Exchange layer and Storage layer in the protocol are extended by leveraging the
component to achieve block-level compression and decompression in the workflows of upload and
download for block transmission acceleration over an IPFS network. Furthermore, a block pre-request
block is proposed and incorporated into Exchange layer to improve the block request mechanism
in the layer which frequently causes block request awaiting, reducing block provision speed for
compression algorithms, thus downgrading the acceleration. A comprehensive evaluation indicates
that this extended IPFS protocol by our scheme has the same level of data shareability as the native
IPFS protocol, and contributes to a block transmission performance enhancement in download by up
to 69% and in upload by as much as 201% relative to the current leading coarse-grained compression
scheme, referred to as IPFSz

## Table of Contents

- [Install](#install)
- [Download and Compile IPFS](#download-and-compile-ipfs)
-  [install nvcomp](#install-nvcomp)
-  [Compile libhigdecomcache2.so](#Compile-libhigdecomcache2.so)
-  [install libhigdecomcache2.so](#install-libhigdecomcache2.so)
-  [install cmake](#install-cmake)
-  [install native IPFS](#install-native-IPFS)


# Install

You'll need to add Go's bin directories to your $PATH environment variable e.g., by adding these lines to your /etc/profile (for a system-wide installation) or $HOME/.profile:

    export PATH=$PATH:/usr/local/go/bin
    export PATH=$PATH:$GOPATH/bin
    export PATH=/usr/local/cmake-3.26.3-linux-x86_64/bin:$PATH
    export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
    export PATH=/usr/local/cuda-12.1/bin${PATH:+:${PATH}}
    export LD_LIBRARY_PATH=/usr/local/cuda-12.1/lib64${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}
    export LD_LIBRARY_PATH=/usr/local/nvcomp:$LD_LIBRARY_PATH
    export PATH=$PATH:/usr/local/go/bin

# Download and Compile IPFS
    $ git clone https://github.com/zcp/ipfs.git

## install nvcomp

    $ tar zxvf  nvcomp_3.0.5_x86_64_12.x.tgz -C /usr/local/
    $  cp  cuda_g.h   /usr/local/include/

## Compile libhigdecomcache2.so
    $ nvcc -shared double_high.cpp -o  libhigdecomcache2.so  -lnvcomp -Xcompiler -fPIC

## install libhigdecomcache2.so
    $ cp  libhigdecomcache2.so   /usr/local/lib/

## install cmake
    $ sudo sh cmake-3.26.4-linux-x86_64.sh --prefix=/usr/local --exclude-subdir

## install native IPFS
    $ cd native_IPFS
    $ unzip vendor.zip
    $ make install

Our fine-grained compression strategy IPFS has all the source code dependencies in the "vendor" folder.
When the vendor.zip file is extracted, the following items are obtained.