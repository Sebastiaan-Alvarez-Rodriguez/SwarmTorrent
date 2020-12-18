import time
import os

from experiments.interface import ExperimentInterface
import util.location as loc
import util.fs as fs

# We suggest all experiments which print anything to the console
# to use below import statement. This forces in-order printing. 
from util.printer import *

def get_experiment():
    '''Pass your defined experiment class in this function so MetaZoo can find it'''
    return PerformanceExperiment

class PerformanceExperiment(ExperimentInterface):
    '''
    A most useful experiment.
    Check <root dir>/peerkeeper/experiments/example_simple/example.py 
    for an example implementation.
    Also, check <root dir>/peerkeeper/dynamic/peerkeeper.py
    to find out how peerkeeper variables work.
    '''

    def num_trackers(self):
        '''Get amount of tracker nodes to allocate'''
        return 1

    def num_peers(self):
        '''get amount of peer nodes to allocate'''
        return 2

    def trackers_use_infiniband(self):
        '''True if trackers must communicate with eachother over infiniband, False otherwise'''
        return True

    def peers_use_infiniband(self):
        '''True if peers must communicate with trackers over infinband, False otherwise'''
        return True

    def trackers_core_affinity(self):
        '''Amount of tracker processes which may be mapped on the same physical node'''
        return 1

    def peers_core_affinity(self):
        '''Amount of peer processes which may be mapped on the same physical node'''
        return 1

    def check_ready(self):
        return len(list(fs.ls(loc.get_swarmtorrent_log_dir()))) == num_peers()-1

    def pre_experiment(self, peerkeeper):
        '''Execution before experiment starts. Executed on the remote once.'''
        print('Hi there! I am executed before the experiment starts!')
        fs.mkdir(loc.get_swarmtorrent_log_dir(), exist_ok=True)
        fs.mkdir(loc.get_swarmtorrent_torrentfile_dir(), exist_ok=True)


    def get_peer_run_command(self, peerkeeper):
        '''Get peer run command, executed in All peer nodes'''
        register = peerkeeper.lid == 0
        port = 2322 if register else 2321
        workpath = loc.get_initial_file_dir() if register else loc.get_output_loc()
        torrentfile = loc.get_swarmtorrent_torrentfile()
        command = './peer torrent -p {} -w {} -f {}'.format(port, workpath, torrentfile)
        if register:
            command += ' -r'
        return command

    def get_tracker_run_command(self, peerkeeper):
        '''Get tracker run command, executed in All tracker nodes'''
        port = 2323
        return './tracker -p {}'.format(port)


    def experiment_peer(self, peerkeeper, executor):
        '''Execution occuring on ALL peer nodes'''
        while (True):
            time.sleep(30)
            if check_ready():
                break
        executor.stop()
        #TODO: output file delete

    def experiment_tracker(self, peerkeeper, executor):
        '''Execution occuring on ALL tracker nodes'''
        while (True):
            time.sleep(30)
            if check_ready():
                break
        executor.stop()

    def post_experiment(self, peerkeeper):
        '''get amount of peer nodes to allocate'''
        #TODO: process logs
        #TODO: remove log files
        #TODO: remove torrentfile