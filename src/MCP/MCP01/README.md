# MCP01
MCP01 describes the generation of a wallet address, the structuring and generation of the data.

### Structure of a wallet address
Example:
`MC-TCT-3XyzTDWs4aVJkign-rmtLespuNQXhR3Bx-rcPyR2FvgLgLE9To-Wgk8KhcJCc6Y5qRb-uCh4LnAR8k79E3xt-ZRzWyKE6H4gyQtf2`
Where the following variables are:
* `MC`: identifier that it's a MetaChain address. Needs to be set for all subchains
* `TCT`: identifies the subchain. This is a regular string with maximum length of 4. Defined by the subchain while generating their genesis block
* `3XyzTDWs4aVJkign-rmtLespuNQXhR3Bx-rcPyR2FvgLgLE9To-Wgk8KhcJCc6Y5qRb-uCh4LnAR8k79E3xt-ZRzWyKE6H4gyQtf2`: base58 encoded address (structuring see below)
 
### Structuring of the base58 encoded address
The address which is later in the process prefixed with the MetaChain and SubChain identifier is base58 encoded. The raw data before base58 encoding contains the following:
* `4 byte Checksum`: C
* `1 byte Flags`: F
* `64 byte hashed public key 'K' with padded 2 byte ChainIdentifier`: X
* `1 byte Version`: V
 
The total byte in raw format of the address is 70.
#### C
The Checksum is calculated by `SHA3-KECCAK-128( SHA3-KECCAK-128([F][X][V]))`.
Input is 66 byte, Output is 32 byte.
Only the first 4 bytes are taken from this checksum and used for comparison.
#### F
F is a 1 byte flag field which contains:
* Testnet: 000X 0000 - 1 if TestNet, 0 if MainNet
* ECDSA Signing Algorithm: 0000 000X - 1 if SECP256k1, 0 if SECP256r1
#### X
Calculating X is done by `SHA3-512([public key][2 byte chain identifier])`.
The input for `SHA3-512` is therefore 66 byte, output is 64 byte. The ChainIdentifier is a unique number that is assigned to the SubChain by the MetaChain upon Genesis Block creation and can be retrieved through the Meta Database.
#### V
The Version byte is used if future changes in the wallet address format require different computation styles. This is done to achieve backwards compability between different versions. It's a simple unsigned byte.

### Validating a Wallet Address
The following process is suitable for checking the integrity and validity of a wallet address:
* Check if `MC-` exists
* Extract the ChainIdentifier and get the corresponding ChainIdentifier ID
* Extract the Version Byte `V`
* [V=1] Extract the Checksum `C`
* [V=1] Extract the flag byte `F`
* [V=1] Calculate the Checksum for `X` and compare the first 4 bytes with `C`

Please look in the [forum] for a closer description and documentation.

License
----

GPLv3


**(c) the TCT-Devs**

[//]: # (These are reference links used in the body of this note and get stripped out when the markdown processor does its job. There is no need to format nicely because it shouldn't be seen. Thanks SO - http://stackoverflow.com/questions/4823468/store-comments-in-markdown-syntax)
[forum]: <https://forum.trustchaintechnologies.io/showthread.php?tid=22>
