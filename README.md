# TrustChainTechnologies - MetaChain

[![TrustChainTechnologies](https://www.trustchaintechnologies.io/logo.png)](https://www.trustchaintechnologies.io/)

These are the MetaChain sources of the TCT blockchain technology. It features block signing, PoT with PtA and an extensive MetaChain API.

### Important links
 - The [roadmap] can be found in our forums and will be updated frequently.
 - More technical info in the [whitepaper].

### Development

You want to contribute? Have the guts to tackle crypto development?
Perfect, then register in our [forum] and get in touch with us!

# Installation

Since we're currently in heavy development, we don't suggest installing the MetaChain yet. As soon as this changes we'll publish the installation details.
#### Using binaries
currently not supported
#### Building for source on windows
- clone the git repository using github for Desktop
- open the MetaChain.sln in Microsoft Visual Studio
- build for Debug or Release
#### Building for source on linux
g++, automake and autoconf as well as regular dev tools are required!
Clone the github repository:
```sh
$ git clone https://github.com/TrustChainTechnologies/metachain.git
$ cd metachain
$ ./configure
$ ./make
```

# External resources used in the source code
- [brofield/simpleini] - for parsing ini files
- [bitcoin/bitcoin] - parts of the network communication, parts of the crypto sources (e.g. sha256 etc). Everything heavily modified, changed for our demands and integrated into our structure

License
----

GPL


**(c) the TCT-Devs**

[//]: # (These are reference links used in the body of this note and get stripped out when the markdown processor does its job. There is no need to format nicely because it shouldn't be seen. Thanks SO - http://stackoverflow.com/questions/4823468/store-comments-in-markdown-syntax)


   [roadmap]: <https://forum.trustchaintechnologies.io/showthread.php?tid=13&pid=21#pid21>
   [whitepaper]: <https://backoffice.trustchaintechnologies.io/downloads/whitepaper.pdf>
   [forum]: <https://forum.trustchaintechnologies.io>
   [brofield/simpleini]: <https://github.com/brofield/simpleini>
   [bitcoin/bitcoin]: <https://github.com/bitcoin/bitcoin>