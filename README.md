# TrustChainTechnologies - MetaChain

[![TrustChainTechnologies](https://www.trustchaintechnologies.io/logo.png)](https://www.trustchaintechnologies.io/)

These are the MetaChain sources of the TCT blockchain technology. It features block signing, PoT with PtA and an extensive MetaChain API.

### Important links
 - The [roadmap] can be found in our forums and will be updated frequently.
 - More info in the [whitepaper].
 - Technical info in the [techpaper].

### Development

You want to contribute? Have the guts to tackle crypto development?
Perfect, then register in our [forum] and get in touch with us!

# Installation

Since we're currently in heavy development, we don't suggest installing the MetaChain yet. As soon as this changes we'll publish the installation details.
#### Using binaries
currently not supported
#### Dependencies
- for compiling from source you'll need a working [boost] installation with all libraries built.
- [openssl] is widely used in this project. you'll need a working installation with all libraries.
- [rocksdb] is our default meta info backend, even when you use mysql or other db engines, we still rely on [rocksdb]. A working install must be present!
- [brofield/simpleini] is used for ini parsing
- [bitcoin-core/secp256k1] used as C backend for secp256k1 ECDA algorithm
- [cryptopp] a widely known and usefull crypto library supporting us with crypto algorithms
- [curl] curl is used for fast and easy access of files stored in the internet. Version 7.55.1 is used
- [zlib] zlib is used for automatic decompression for autoupdating die node. Version 1.2.11 is used
- C++ compiler needs to have C++11 compability. As soon as C++17.2 is available, changes will be made to use the C++17.2 standard as dependency!
We provide forks of all the needed dependencies in our github organization account. These are the tested versions and those forks will be updated frequently. You can rely on those forks in terms of building the MetaChain.
#### Building for source on windows
- clone the git repository using github for Desktop
- place all dependencies in a folder named "dependencies" in the project root. clone the corresponding git repositories of the dependencies into this folder
- open the MetaChain.sln in Microsoft Visual Studio
- build for Debug or Release
#### Building for source on linux
g++ (Version > 6.2), openssl-devel, openssl libs, boost-devel, boost-libs, pkg-config, automake and autoconf as well as regular dev tools are required!
Clone the github repository:
```sh
$ git clone https://github.com/TrustChainTechnologies/metachain.git
$ cd metachain
$ ./configure ## --enable-debug for debug-build
$ ./make
```
# Configuration
#### Command line arguments
-v, --version: print version info
-?, -h, --help: print all command line arguments
-c, --conf=<file>: use this ini file for configuration (default: node.ini)
#### node.ini
```
; general configuration
[general]
daemonize = true    ; use daemonize if OS supports it
mode = fn			; defines the running mode (FN = Full Node, CL = Client) 

; logging configuration
[logging]
log_to_stdout = true    ; log everything to stdout
log_to_file = true      ; log to a logfile
log_file = output.log   ; name of the logfile

; network configuration
[network]
listening_ip = 127.0.0.1                    ; ip to bind listening socket to
listening_port = 5634                       ; port to bind listening socket to
peer_file = peers.dat                       ; file that stores the known peers which is automatically updated
ban_file = bans.dat                         ; file that stores banned peers which is automatically updated
connect_timeout = 5000                      ; connection timeout for a new node in ms
max_outgoing_connections = 1000             ; number of maximum outgoing connections
max_incoming_connections = 1000             ; number of maximum incoming connections
time_between_unsuccessfull_connects = 30    ; delay between two connects to the same node

; autoupdate configuration
[autoupdate]
ticks_until_update_triggered = 10           ; how many nodes need to have a newer version to trigger update
do_autoupdate = true                        ; autoupdate? if false, node exits automatically

; data configuration
[data]
data_dir = data				; directory where the data is stored
raw_dir = raw				; directory for raw metachain data in [general].[mode] = fn
storage_engine = rdb		; db engine used for processed data. rdb = rocks db, mysql = mysql db

; the following is only needed for [data].[storage_engine] = rdb
[bdb]
dir = rdb					; directory where to store the processed db

; the following is only needed for [data].[storage_engine] = mysql
; the database needs to be initialized with the mysql data structure provided
; the user needs to be setup and have the full rights for this database
[mysql]
connection_type = socket				; "socket" for socket connection, "tcp" for [host]:[port] tcp/ip connection
socket = /var/lib/mysql/mysql.sock
host = localhost
port = 3306
username = tct
password = supersecretpw
database = trustchain
```

# External resources used in the source code
- [bitcoin/bitcoin] - parts of the network communication, parts of the crypto sources (e.g. sha256 etc). Everything heavily modified, changed for our demands and integrated into our structure
- and dependencies as declared earlier

License
----

GPLv3


**(c) the TCT-Devs**

[//]: # (These are reference links used in the body of this note and get stripped out when the markdown processor does its job. There is no need to format nicely because it shouldn't be seen. Thanks SO - http://stackoverflow.com/questions/4823468/store-comments-in-markdown-syntax)


   [roadmap]: <https://forum.trustchaintechnologies.io/showthread.php?tid=13&pid=21#pid21>
   [whitepaper]: <https://backoffice.trustchaintechnologies.io/downloads/whitepaper.pdf>
   [techpaper]: <https://backoffice.trustchaintechnologies.io/downloads/techpaper.pdf>
   [forum]: <https://forum.trustchaintechnologies.io>
   [brofield/simpleini]: <https://github.com/brofield/simpleini>
   [bitcoin/bitcoin]: <https://github.com/bitcoin/bitcoin>
   [boost]: <http://www.boost.org/>
   [openssl]: <https://github.com/openssl/openssl>
   [rocksdb]: <https://github.com/facebook/rocksdb/>
   [bitcoin-core/secp256k1]: <https://github.com/bitcoin-core/secp256k1>
   [cryptopp]: <https://github.com/weidai11/cryptopp>
   [curl]: <https://curl.haxx.se/>
   [zlib]: <http://zlib.net/>