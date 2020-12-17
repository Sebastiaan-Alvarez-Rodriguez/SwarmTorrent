import inspect
import sys

from dynamic.peerkeeper import PeerKeeper
from experiments.interface import ExperimentInterface
import util.fs as fs
import util.importer as imp
import util.location as loc
import util.ui as ui


class Experiment(object):
    '''
    Object to handle communication with user-defined experiment interface
    Almost all attributes are lazy, so the dynamic code is used minimally.
    '''
    def __init__(self, timestamp, location, clazz):
        self.timestamp = timestamp
        self.location = location
        self.instance = clazz()
        self._peerkeeper = PeerKeeper()
        self._num_trackers = None
        self._num_peers = None
        self._trackers_use_infiniband = None
        self._peers_use_infiniband = None
        self._trackers_core_affinity = None
        self._peers_core_affinity = None

        self._tracker_periodic_clean = None

    @property
    def num_trackers(self):
        if self._num_trackers == None:
            self._num_trackers = int(self.instance.num_trackers())
            if self._num_trackers < 2:
                raise RuntimeError('Experiment must specify num_trackers >=2 (currently got {})'.format(self._num_trackers))
        return self._num_trackers

    @property
    def num_peers(self):
        if self._num_peers == None:
            self._num_peers = int(self.instance.num_peers())
            if self._num_peers < 1:
                raise RuntimeError('Experiment must specify num_peers >=1 (currently got {})'.format(self._num_peers))
        return self._num_peers
    
    @property
    def trackers_use_infiniband(self):
        if self._trackers_use_infiniband == None:
            self._trackers_use_infiniband = bool(self.instance.trackers_use_infiniband())
        return self._trackers_use_infiniband

    @property
    def peers_use_infiniband(self):
        if self._peers_use_infiniband == None:
            self._peers_use_infiniband = bool(self.instance.peers_use_infiniband())
        return self._peers_use_infiniband

    @property
    def trackers_core_affinity(self):
        if self._trackers_core_affinity == None:
            self._trackers_core_affinity = int(self.instance.trackers_core_affinity())
            if self._trackers_core_affinity < 1:
                raise RuntimeError('Experiment must specify trackers_core_affinity >= 1 (currently got {})'.format(self._trackers_core_affinity))
            if self.num_trackers % self._trackers_core_affinity != 0:
                raise RuntimeError('Number of trackers must be divisible by tracker core affinity. {} % {} = {} != 0'.format(self.num_trackers, sef._trackers_core_affinity, (self.num_trackers % self._trackers_core_affinity)))
        return self._trackers_core_affinity

    @property
    def peers_core_affinity(self):
        if self._peers_core_affinity == None:
            self._peers_core_affinity = int(self.instance.peers_core_affinity())
            if self._peers_core_affinity < 1:
                raise RuntimeError('Experiment must specify peers_core_affinity >= 1 (currently got {})'.format(self._peers_core_affinity))
            if self.num_peers % self._peers_core_affinity != 0:
                raise RuntimeError('Number of peers must be divisible by peer core affinity. {} % {} = {} != 0'.format(self.num_peers, sef._peers_core_affinity, (self.num_peers % self._peers_core_affinity)))
        return self._peers_core_affinity

    @property
    def tracker_periodic_clean(self):
        if self._tracker_periodic_clean == None:
            self._tracker_periodic_clean = int(self.instance.tracker_periodic_clean())
            if self._tracker_periodic_clean < 0:
                raise RuntimeError('Server periodic clean designation must be either 0 (never clean) or an integer representing time (s)')
        return self._tracker_periodic_clean

    @property
    def peerkeeper(self):
        return self._peerkeeper

    def pre_experiment(self, repeats):
        self._peerkeeper._repeats = repeats
        val = self.instance.pre_experiment(self._peerkeeper)
        self.persist()
        return val


    def get_peer_run_command(self, config, repeat):
        self._peerkeeper._gid = config.gid
        self._peerkeeper._lid = config.lid
        self._peerkeeper._hosts = tuple(config.hosts)
        self._peerkeeper._repeat = repeat
        self._peerkeeper._log_location = fs.join(loc.get_peerkeeper_results_dir(), self.timestamp, repeat, 'experiment_logs')
        return self.instance.get_peer_run_command(self._peerkeeper)


    def experiment_peer(self, config, executor, repeat):
        self._peerkeeper._gid = config.gid
        self._peerkeeper._lid = config.lid
        self._peerkeeper._hosts = tuple(config.hosts)
        self._peerkeeper._executor = executor
        self._peerkeeper._repeat = repeat
        self._peerkeeper._log_location = fs.join(loc.get_peerkeeper_results_dir(), self.timestamp, repeat, 'experiment_logs')

        return self.instance.experiment_peer(self._peerkeeper)


    def experiment_tracker(self, config, executor, repeat, is_leader_func):
        self._peerkeeper._gid = config.gid
        self._peerkeeper._lid = config.lid
        self._peerkeeper._executor = executor
        self._peerkeeper._repeat = repeat
        self._peerkeeper._log_location = fs.join(loc.get_peerkeeper_results_dir(), self.timestamp, repeat, 'experiment_logs')
        return self.instance.experiment_tracker(self._peerkeeper)


    def post_experiment(self):
        val = self.instance.post_experiment(self._peerkeeper)
        return val


    # Save all required information, so nodes can reconstruct our object
    def persist(self):
        self._peerkeeper.persist()
        fs.rm(fs.join(loc.get_peerkeeper_experiment_dir(), '.elected.hidden'), ignore_errors=True)
        with open(fs.join(loc.get_peerkeeper_experiment_dir(), '.elected.hidden'), 'w') as file:
            file.write('{}|{}'.format(self.timestamp, self.location))

    # Construct object from persisted information
    @staticmethod
    def load():
        with open(fs.join(loc.get_peerkeeper_experiment_dir(), '.elected.hidden'), 'r') as file:
            timestamp, location = file.read().split('|')
            exp = load_experiment(timestamp, location)
            exp._peerkeeper = MetaZoo.load()
            return exp

    # Cleans persisted information
    def clean(self):
        fs.rm(fs.join(loc.get_peerkeeper_experiment_dir(), '.elected.hidden'), ignore_errors=True)
        self._peerkeeper.clean()


# Loads an experiment in the node stage and returns it
def load_experiment(timestamp, location):
    module = imp.import_full_path(location)
    try:
        return Experiment(timestamp, location, module.get_experiment())
    except AttributeError as e:
        raise RuntimeError('Could not fetch Experiment module in file {}. Did you define get_experiment() there?'.format(location))


# Standalone function to get an experiment instance
def get_experiments(timestamp):
  
    candidates = []
    for item in fs.ls(loc.get_peerkeeper_experiment_dir(), full_paths=True, only_files=True):
        if item.endswith(fs.join(fs.sep(), 'interface.py')) or not item.endswith('.py'):
            continue
        try:
            module = imp.import_full_path(item)
            candidates.append((item, module.get_experiment(),))
        except AttributeError:
            print('Item had no get_experiment(): {}'.format(item))

    if len(candidates) == 0:
        raise RuntimeError('Could not find a subclass of "ExperimentInterface" in directory {}. Make a ".py" file there, with a class extending "ExperimentInterface". See the example implementation for more details.'.format(loc.get_peerkeeper_experiment_dir()))
    elif len(candidates) == 1:
        return [Experiment(timestamp, candidates[0][0], candidates[0][1])]
    else:
        idcs = ui.ask_pick_multiple('Multiple suitable experiments found. Please pick experiments:', [x[0] for x in candidates])
        return [Experiment(timestamp+'_'+str(idx), (candidates[x])[0], (candidates[x])[1]) for idx, x in enumerate(idcs)]