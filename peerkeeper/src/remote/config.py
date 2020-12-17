import abc
import os
import socket

import remote.util.identifier as idr


# # Constructs a peer config, populates it, and returns it
# def config_construct_peer(experiment, hosts):
#     nodenumbers = [int(nodename[4:]) for nodename in os.environ['HOSTS'].split()]
#     nodenumbers.sort()
#     if not len(nodenumbers) == experiment.num_peers:
#         raise RuntimeError('Allocated incorrect number of nodes ({}) for {} peers'.format(len(nodenumbers), experiment.num_peers))
#     return ClientConfig(experiment, nodenumbers, hosts)

# # Constructs a tracker config, populates it, and returns it
# def config_construct_tracker(experiment):
#     nodenumbers = [int(nodename[4:]) for nodename in os.environ['HOSTS'].split()]
#     nodenumbers.sort()
#     if not len(nodenumbers) == experiment.num_trackers:
#         raise RuntimeError('Allocated incorrect number of nodes ({}) for {} trackers'.format(len(nodenumbers), experiment.num_trackers))
#     return ServerConfig(experiment, nodenumbers)

def config_construct(experiment, is_tracker):
    nodenumbers = [int(nodename[4:]) for nodename in os.environ['HOSTS'].split()]
    nodenumbers.sort()
    if is_tracker and not len(nodenumbers) == experiment.num_trackers:
        raise RuntimeError('Allocated incorrect number of nodes ({}) for {} trackers'.format(len(nodenumbers), experiment.num_trackers))
    if not is_tracker and not len(nodenumbers) == experiment.num_peers:
        raise RuntimeError('Allocated incorrect number of nodes ({}) for {} peers'.format(len(nodenumbers), experiment.num_peers))
    return Config(experiment, nodenumbers)

class Config(metaclass=abc.ABCMeta):
    '''
    Base config class. Config implementations should extend this class,
    since it provides storage for and access to
    essential variables for trackers and peers.
    '''
    def __init__(self, experiment, nodes):
        self._nodes = nodes

        self._tracker_infiniband = experiment.trackers_use_infiniband
        self._peer_infiniband = experiment.peers_use_infiniband

        self._gid = idr.identifier_global()
        self._lid = idr.identifier_local()

    # True if trackers should communicate using infiniband, False otherwise
    @property
    def tracker_infiniband(self):
        return self._tracker_infiniband
    
    # True if peers should communicate with trackers using infiniband, False otherwise
    @property
    def peer_infiniband(self):
        return self._peer_infiniband

    # Local id, different for each allocated process on the same node
    @property
    def lid(self):
        return self._lid

    # Global id, different for each grouped allocated process
    @property
    def gid(self):
        return self._gid

    # Sorted list of tracker node numbers and peer node numbers
    # (e.g. [114, 116,...],corresponding to 'node114' and 'node116' hosts)
    @property
    def nodes(self):
        return self._nodes



# class TrackerConfig(Config):
#     '''Config implementation for trackers'''

#     def __init__(self, experiment, nodes):
#         super(ServerConfig, self).__init__(experiment, nodes)
#         # Directory containing data for this tracker
#         self._port = None
#     @property
#     def tracker_infiniband(self):
#         return super().tracker_infiniband
    
#     @property
#     def peer_infiniband(self):
#         return super().peer_infiniband

#     @property
#     def lid(self):
#         return super().lid

#     @property
#     def gid(self):
#         return super().gid

#     @property
#     def nodes(self):
#         return super().nodes

#     # port value of this tracker's config
#     @property
#     def port(self):
#         return self._port
    
#     @port.setter
#     def port(self, value):
#         self.port = value


# class PeerConfig(Config):
#     '''Config implementation for peers'''

#     def __init__(self, experiment, nodes, hosts):
#         super(ClientConfig, self).__init__(experiment, nodes)
#         self._hosts = hosts
#         self._port = None

#     @property
#     def tracker_infiniband(self):
#         return super().tracker_infiniband
    
#     @property
#     def peer_infiniband(self):
#         return super().peer_infiniband

#     @property
#     def lid(self):
#         return super().lid

#     @property
#     def gid(self):
#         return super().gid

#     @property
#     def nodes(self):
#         return super().nodes

#     # List of trackers running. Each tracker is listed as '<ip>:<port>'
#     @property
#     def hosts(self):
#         return self._hosts

#     # port value of this peers's config
#     @property
#     def port(self):
#         return self._port
    
#     @port.setter
#     def port(self, value):
#         self.port = value

