| Functional Requirements |
|-|
| Generate a SwarmTorrent file defined by the initial Seeder |
| Split a file to be seeded into file fragments |
| Set up Trackers who have a Table with Peers in the Swarm |
| Peers should be able to collect SwarmTorrent files |
| Peers should be able to collect other Peers from the Trackers |
| Peers should be able to join the Swarm |
| Peers should be able to receive updates concerning the Peers in the Swarm |
| Peers can connect with each other over TCP(?) |
| Peers should be able to collect all Fragments of a file |
| Peers can send each other file Fragments |
| Peers should be able to validate their file Fragments |
| Peers should be able to find out from which peer they can receive which Fragment |

| Non-Functional Requirements |
|-|
| Swarm setup should take no more than 5 seconds |
| All file fragments combined should give the original file |
| Availability: As long as 1 tracker is alive, the swarm is alive (peers can share file fragments) |
| If the SwarmTorrent file is available, collecting (and processing) it should take no more than 5 seconds |
| Joining a Swarm should take no more than 5 seconds |
| A change in the participants of the Swarm should take no more than 5 minutes to be known by all Peers |
| A connection with another Peer should take no more than 5 seconds |
| Fragment Collection should have a minimum throughput of 10 MB/s |
| Peers should know at least one Peer for each Fragment at all times (might simply be initial Seeder) |