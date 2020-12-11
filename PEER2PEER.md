# Port allocation & communication
Once a peer `A` joins another peer `B`, both `A` and `B` should exchange information.
Before we can describe that, we have to consider sourcePort allocation for peers.

It would be best if a peer `A` can receive messages from all other peers on some sourcePort `x`, and send replies back using this sourcePort.
However, this setup would mean we use one sourcePort very intensively, while there might be tens to hundreds of connected peers, which all try to connect, send messages, and wait for answers.
Could this clog the network?

## Current communication protocol
Peer2Peer communication consists of several mechanisms.

 1. Peer joins/leaves other peer
 2. Peer requests data fragment X

### Join
Pattern is simple:
 1. Peer `A` sends a [PeerMessage](/src/peer/connection/message/peer/message.h) containing tag `JOIN`, with a port `x` to connect to and the hash for the torrentfile in the message body.
 2. Peer `B` receives, and replies with a [standard message](/src/shared/connection/message/message.h) containing `OK` to accept, `REJECT` otherwise.
 If `B` accepts, it will send all future requests to `A` by using port `x`. It expects `A` to send all future requests to the port `A` sent its `JOIN` to.

> Note: In the future, `B` will probably not reply anything instead of `REJECT`. It is up to the sender to set a reasonable timeout and move on then.
 3. `A` receives status. 


### Leave
 1. Peer `A` sends a [PeerMessage](/src/peer/connection/message/peer/message.h) containing tag `LEAVE`, with a registered port `x` to connect to and the hash for the torrentfile in the message body.
 2. Peer `B` receives `LEAVE`, removes `A` from peer list if and only if the sent port `x` is the registered port of the `JOIN` from before, and does not reply anything.
 3. Peer receives status. Done.

Suppose the above goes wrong somehow:
If communication fails, and the `LEAVE` from `A` does not reach `B` somehow, then `A` still removes `B` from its peerlist. When `B` attempts to make data requests, `A` will send a `REJECT`. `B` then knows it should leave.


### Requests for data
The idea is that peer `A` ends up receiving data fragment `x` of another peer `B`.
 1. Peer `A` sends a [PeerMessage](/src/peer/connection/message/peer/message.h) containing tag `DATA_REQ`, with the fragment number `x` in the body.
 2. Peer `B` receives, reads fragment `x` from disk, finally returns [standard message](/src/shared/connection/message/message.h) containing `OK` on success, `REJECT` when this fragment is unavailable. When `OK`, the body contains the requested fragment `x`. When `REJECT`, the body contains a boolean vector, where `true` values represent fragments that are present in `B`, and `false` represents fragment not (yet) present in `B`.

