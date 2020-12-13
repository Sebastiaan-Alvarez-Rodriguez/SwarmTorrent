# P2T Communication
Peer2Tracker communication consists of several mechanisms.

 1. Peer subscribes/unsubscribes on tracker
 2. Peer requests current peertable for a given file on tracker
 3. Peer registers itself for a peertable on tracker

## Subscribe/Unsubscribe
Pattern is simple:
 1. Peer sends a [TrackerMessage](/src/shared/connection/message/tracker/message.h) containing tag `SUBSCRIBE` or `UNSUBSCRIBE`, with the hash for the torrentfile in the message body to (un)subscribe.
 2. Tracker receives, performs action. Tracker returns [standard message](/src/shared/connection/message/message.h) containing `OK` on success, `ERROR` otherwise.
 3. Peer receives status. Done.

## Requests for peertable
The idea is that the peer ends up receiving the peertable of the tracker.
 1. Peer sends a [TrackerMessage](/src/shared/connection/message/tracker/message.h) containing tag `RECEIVE`, with the hash for the torrentfile in the message body to get the PeerTable for.
 2. Tracker receives, fetches the peertable for specified torrentfile, finally returns [standard message](/src/shared/connection/message/message.h) containing `OK` on success, `ERROR` otherwise. 
 When `OK`, the body contains first the address of the peer, followed by the requested peertable.
 We send the (global) address of the peer individually, because the peer must filter out its own address from the peertable before usage.
 If it fails to do so, it might accidentally connect to itself and start requesting its own fragments, which makes no sense.


## Register for a peertable
When peers want, they can register themselves on a list of trackers.
Once registered, fellow peers will send requests to send_join_req a local network.
Peers register using a [TrackerMessage](/src/shared/connection/message/tracker/message.h) containing tag `REGISTER`, with in the body a hash for a torrentfile and a port `x` to register.
The system sends `OK` on successfully registering.
It sends `ERROR` if no such torrentfile hash is found.

## Local Discovery
Trackers can ask peers to share their peertables. The tracker uses this to update its list of known peers for a given torrent.
It goes like this:
 1. Tracker sends a [Message](/src/shared/connection/message/message.h) containing tag `LOCAL_DISCOVERY`, with in the body the hash for the torrentfile we are currently processing for.
 2. Receiving peers send a message `OK`, with in the body of the message the requested peertable for torrentfile for given hash. If the peer no longer participates in the network, it does not send back anything.