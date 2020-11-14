# SwarmTorrent
A UNIX system to launch torrent systems.

This system consists of a peer and a tracker,
which have their own tasks.
In the sections below, we will describe them in detail.
Additionally, we provide some information on the protocols used.


## TODOList
A list of things we still have to do in order to advance the project.

Connections:
 1. Make basic TCP connection in [`/src/shared`](/src/shared/) code.
 1. Make connection factory type to allow us to use generic connection objects, while the driver code below handles TCP/UDP/INET type connections 

Files:
 1. Make basic tracker-table implementation in [`/src/shared/torrent`](/src/shared/torrent), so peers can connect to trackers. Make table binary preferably.
 1. Define the 'file fragments'
 1. Make checksum table for file fragments
 1. Create the rest of the file: (advisory) name, length
 1. (Bonus) Make basic filestructure description, so we can send a directory instead of just 1 file.

## Peers
Peers form the largest part of torrent networks.
Their goal is to send data to users requesting it (*seed*),
and receive data from users requesting it (*leech*).
The data to be seeded/leeched is encoded in a *.st* (SwarmTorrent) file.

Peers are able to simultaneously seed and leech on a *.st* file:
A peer seeds any received file **fragments** to other peers in the network which are requesting it.

## Trackers
A tracker server is in charge of providing information to the swarm.
When a peer reads a *.st* file, it contains a table of tracker servers.
When peers connect to a tracker, the tracker is responsible for providing the tracker with a list of peers that are in possesion of some parts of the file

## SwarmTorrent file
The SwarmTorrent (*.st*) file contains information on the file to be seeded/ leeched and the tracker servers.
This fileformat is based on the BitTorrent protocol from: https://www.bittorrent.org/beps/bep_0003.html
SwarmTorrent has the following content: 
 1. Tracker Table: contains information required to set up a socket with the trackers
 1. Name: suggested name to save the file (advisory)
 1. Length: the total size of the file in bytes
 1. Fragment Length: the size in bytes of one fragment
 1. Fragments: the SHA2's for every fragment

## Considerations

 1. [Distributed Hash Tables (DHT)](https://en.wikipedia.org/wiki/Distributed_hash_table) a system which removes the swarms dependance on the tracker server. *"Responsibility for maintaining the mapping from keys to values is distributed among the nodes, in such a way that a change in the set of participants causes a minimal amount of disruption."*
 1. [Peer EXchange (PEX)](https://en.wikipedia.org/wiki/Peer_exchange), so a peer in a swarm can tell others that a new peer has joined/one has left

