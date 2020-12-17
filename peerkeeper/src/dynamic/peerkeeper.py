import pickle

import util.fs as fs
import util.location as loc
from util.printer import *


class PeerKeeper(object):
    '''
    Dynamic store for experiment. Store persists between remote and nodes.
    Note: After nodes have booted, store becomes immutable.

    Members with global availability:
        register: key-value store for user-defined objects. Make sure this objects are pickleable
        repeats: Number of repeats for this experiment
    
    Members with remote availability:
        -

    Members with global node availability:
        executor: Exector for the server/client. Will be delivered in started state. Can be stopped and rebooted
        gid: Global ID. Unique for instances in the same group
        lid: Local ID. Unique for instances in the same group, on the same node 
        log_location: Valid full path to a directory were logs may be written for this experiment.
        repeat: Current repetition of experiment

    Members with client node availability:
        hosts: A tuple of strings resembling '<ip_or_hostname>:<port>'
    '''
    def __init__(self):
        self._register = dict()
        self._executor = None
        self._gid = None
        self._hosts = None
        self._lid = None
        self._log_location = None
        self._repeats = None
        self._repeat = None

    @property
    def executor(self):
        return self._executor
    
    @property
    def hosts(self):
        return self._hosts
    @hosts.setter
    def set_hosts(self):
        raise RuntimeError('You cannot set hosts yourself!')

    @property
    def gid(self):
        return self._gid
    @gid.setter
    def set_gid(self):
        raise RuntimeError('You cannot set the gid yourself!')

    @property
    def lid(self):
        return self._lid
    @lid.setter
    def set_lid(self):
        raise RuntimeError('You cannot set the lid yourself!')

    @property
    def log_location(self):
        return self._log_location
    
    @property
    def register(self):
        return self._register
    
    @property
    def repeats(self):
        return self._repeats
    @repeats.setter
    def set_repeats(self):
        raise RuntimeError('You cannot set repeats number yourself!')

    @property
    def repeat(self):
        return self._repeat
    @repeat.setter
    def set_repeat(self):
        raise RuntimeError('You cannot set repeat number yourself!')


    # Function to completely prohibit changing (i.e. writing, updating, deleting) PeerKeeper register
    def lock(self):
        def __readonly__(self, *args, **kwargs):
            raise RuntimeError('Cannot change PeerKeeper register past the pre_experiment stage')
        try:
            self.register.__setitem__ = __readonly__
        except Exception as e:
            pass
        try:
            self.register.__delitem__ = __readonly__
        except Exception as e:
            pass
        try:
            self.register.pop = __readonly__
        except Exception as e:
            pass
        try:
            self.register.popitem = __readonly__
        except Exception as e:
            pass
        try:
            self.register.clear = __readonly__
        except Exception as e:
            pass
        try:
            self.register.update = __readonly__
        except Exception as e:
            pass
        try:
            self.register.setdefault = __readonly__
        except Exception as e:
            pass
        del __readonly__


    # Function to persist this instance using pickling
    def persist(self):
        with open(fs.join(loc.get_remote_peerkeeper_dir(), '.hidden.persist.pickle'), 'wb') as file:
            try:
                pickle.dump(self, file, pickle.HIGHEST_PROTOCOL)
            except Exception as e:
                printe('Could not persist register. Did you store any Objects that cannot be pickled in the PeerKeeper register?')
                raise e


    # Function to load persisted object using pickling
    @staticmethod
    def load():
        location = fs.join(loc.get_remote_peerkeeper_dir(), '.hidden.persist.pickle')
        if not fs.isfile(location):
            raise RuntimeError('Temporary state file not found at {}'.format(location))
        with open(location, 'rb') as file:
            try:
                obj = pickle.load(file)
                obj.lock()
                return obj
            except Exception as e:
                printe('Could not load register. Did you store any Objects that cannot be pickled in the PeerKeeper register?')
                raise e


    @staticmethod
    def clean():
        fs.rm(fs.join(loc.get_remote_peerkeeper_dir(), '.hidden.persist.pickle'), ignore_errors=True)