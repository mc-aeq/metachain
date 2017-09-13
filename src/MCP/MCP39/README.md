# MCP39
MCP39 is heavily based on Bitcoins [BIP39] but due to drastic changes from the hashing algorithms we changed the internal name to MCP39.
BIP39 uses SHA256 and SHA512 aswell as SHA512-KMAC to provide the functionality of hashing and getting corresponding Mnemoniac seeds.
MCP39 uses SHA3-256 and SHA3-KMAC instead, changing the output completely and thus the renaming was necessary.

The functionality of MCP39 is the same as BIP39, please refer to [BIP39] for further documentation.

There is still plenty of code from the BIP39 in the MCP39, all credit belongs to those who developed it and are mentioned in the headers copyright.

Please look in the [forum] for a closer description and documentation.

License
----

GPLv3


**(c) the TCT-Devs**

[//]: # (These are reference links used in the body of this note and get stripped out when the markdown processor does its job. There is no need to format nicely because it shouldn't be seen. Thanks SO - http://stackoverflow.com/questions/4823468/store-comments-in-markdown-syntax)

[forum]: <https://forum.trustchaintechnologies.io/showthread.php?tid=21>
[BIP39]: <https://github.com/bitcoin/bips/blob/master/bip-0039.mediawiki>
