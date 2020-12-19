import abc

def get_experiment():
    '''Implement this function in your experiment, make it return your experiment class'''
    return InterfaceExperiment

class ExperimentInterface(metaclass=abc.ABCMeta):
    '''
    This interface provides hooks, which get triggered on specific moments in execution.
    Experiments must be defined inside this interface.
    PeerKeeper dynamically imports and executes it.

    Check <root dir>/peerkeeper/experiments/examples/example_simple.py 
    for an example implementation.
    Also, check <root dir>/peerkeeper/dynamic/peerkeeper.py
    to find out how peerkeeper works and what it provides.
    '''
    @classmethod
    def __subclasshook__(cls, subclass):
        return (hasattr(subclass, 'num_trackers') and callable(subclass.num_trackers) and 
                hasattr(subclass, 'num_peers') and callable(subclass.num_peers) and 
                hasattr(subclass, 'trackers_use_infiniband') and callable(subclass.trackers_use_infiniband) and 
                hasattr(subclass, 'peers_use_infiniband') and callable(subclass.peers_use_infiniband) and 
                hasattr(subclass, 'trackers_core_affinity') and callable(subclass.trackers_core_affinity) and 
                hasattr(subclass, 'peers_core_affinity') and callable(subclass.peers_core_affinity) and 
                hasattr(subclass, 'pre_experiment') and callable(subclass.pre_experiment) and 
                hasattr(subclass, 'get_peer_run_command') and callable(subclass.get_peer_run_command) and 
                hasattr(subclass, 'experiment_peer') and callable(subclass.experiment_peer) and 
                hasattr(subclass, 'experiment_tracker') and callable(subclass.experiment_tracker) and 
                hasattr(subclass, 'post_experiment') and callable(subclass.post_experiment) or NotImplemented)


    @abc.abstractmethod
    def num_trackers(self):
        '''Get amount of tracker nodes to allocate'''
        raise NotImplementedError

    @abc.abstractmethod
    def num_peers(self):
        '''get amount of peer nodes to allocate'''
        raise NotImplementedError

    @abc.abstractmethod
    def trackers_use_infiniband(self):
        '''True if trackers must communicate with eachother over infiniband, False otherwise'''
        raise NotImplementedError

    @abc.abstractmethod
    def peers_use_infiniband(self):
        '''True if peers must communicate with trackers over infinband, False otherwise'''
        raise NotImplementedError

    @abc.abstractmethod
    def trackers_core_affinity(self):
        '''Amount of tracker processes which may be mapped on the same physical node'''
        raise NotImplementedError

    @abc.abstractmethod
    def peers_core_affinity(self):
        '''Amount of peer processes which may be mapped on the same physical node'''
        raise NotImplementedError

    @abc.abstractmethod
    def pre_experiment(self, peerkeeper):
        '''Execution before experiment starts. Executed on the remote once.'''
        raise NotImplementedError

    @abc.abstractmethod
    def get_seeder_make_command(self, peerkeeper):
        '''Get peer run command, executed in All peer nodes'''
        raise NotImplementedError

    @abc.abstractmethod
    def get_peer_run_command(self, peerkeeper):
        '''Get peer run command, executed in All peer nodes'''
        raise NotImplementedError

    @abc.abstractmethod
    def get_tracker_run_command(self, peerkeeper):
        '''Get peer run command, executed in All peer nodes'''
        raise NotImplementedError

    @abc.abstractmethod
    def experiment_peer(self, peerkeeper):
        '''Execution occuring on ALL peer nodes'''
        raise NotImplementedError

    @abc.abstractmethod
    def experiment_tracker(self, peerkeeper):
        '''Execution occuring on ALL tracker nodes'''
        raise NotImplementedError


    @abc.abstractmethod
    def post_experiment(self, peerkeeper):
        '''Execution after experiment finishes. Executed no the remote once.'''
        raise NotImplementedError