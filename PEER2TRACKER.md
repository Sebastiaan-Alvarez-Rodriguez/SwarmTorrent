# P2T Communication
Peer2Tracker communication consists of several mechanisms.

 1. Peer subscribes/unsubscribes on tracker
 2. Peer requests current peertable for a given file on tracker

## Subscribe/Unsubscribe
Pattern is simple:
 1. Peer sends a [TrackerMessage](/src/shared/connection/mesage/tracker/message.h) containing tag `SUBSCRIBE` or `UNSUBSCRIBE`, with the hash for the torrentfile in the message body to (un)subscribe.
 2. Tracker receives, performs action. Tracker returns [StatusMessage](/src/shared/connection/mesage/status/message.h) containing `OK` on success, `FAILURE` otherwise.
 3. Peer receives status. Done.

## Requests for peertable
The idea is that the peer ends up receiving the peertable of the tracker.
 1. Peer sends a [TrackerMessage](/src/shared/connection/mesage/tracker/message.h) containing tag `REQUEST`, with the hash for the torrentfile in the message body to get the PeerTable for.
 2. Tracker receives, fetches the peertable for specified torrentfile, finally returns [StatusMessage](/src/shared/connection/mesage/status/message.h) containing `OK` on success, `FAILURE` otherwise. When `OK`, the body contains the requested peertable