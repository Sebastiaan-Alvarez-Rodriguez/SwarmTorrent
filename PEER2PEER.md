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
 1. Peer `A` sends a [PeerMessage](/src/peer/connection/message/peer/message.h) containing tag `EXCHANGE_REQ`, with the hash for the torrentfile in the message body.
 2. Peer `B` receives, and replies with a [standard message](/src/shared/connection/message/message.h) containing `OK` to accept, `REJECT` otherwise.
 If `B` accepts, it will send all future requests to `A` by using this port. It expects `A` to send all future requests to the port `A` sent its `EXCHANGE_REQ` to.

> Note: In the future, `B` will probably not reply anything instead of `REJECT`. It is up to the sender to set a reasonable timeout and move on then.
 3. `A` receives status. 


### Leave
 1. Peer `A` sends a [PeerMessage](/src/peer/connection/message/peer/message.h) containing tag `EXCHANGE_CLOSE`, with the hash for the torrentfile in the message body.
 2. Peer `B` receives. `B` replies with a [standard message](/src/shared/connection/message/message.h) containing `OK`.

> Note: In the future, `B` will probably not reply anything. It is up to the sender (`A`) to remove the receiver (`B`) from its session then.
 3. Peer receives status. Done.


### Requests for data
The idea is that peer `A` ends up receiving data fragment `x` of another peer `B`.
 1. Peer `A` sends a [PeerMessage](/src/peer/connection/message/peer/message.h) containing tag `DATA_REQ`, with the fragment number `x` in the body.
 2. Peer `B` receives, reads fragment `x` from disk, finally returns [standard message](/src/shared/connection/message/message.h) containing `OK` on success, `REJECT` when this fragment is unavailable. When `OK`, the body contains the requested fragment `x`. When `REJECT`, the body contains a boolean vector, where `true` values represent fragments that are present in `B`, and `false` represents fragment not (yet) present in `B`.

