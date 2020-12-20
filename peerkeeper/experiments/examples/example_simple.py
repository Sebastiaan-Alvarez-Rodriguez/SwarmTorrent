import time

from experiments.interface import ExperimentInterface

# We suggest all experiments which print anything to the console
# to use below import statement. This forces in-order printing. 
from util.printer import *

def get_experiment():
    '''Pass your defined experiment class in this function so PeerKeeper can find it'''
    return ExampleExperiment

class ExampleExperiment(ExperimentInterface):
    '''
    A most useful experiment.
    Check <root dir>/peerkeeper/experiments/example_simple/example.py 
    for an example implementation.
    Also, check <root dir>/peerkeeper/dynamic/peerkeeper.py
    to find out how peerkeeper variables work.
    '''

    def num_trackers(self):
        '''Get amount of tracker nodes to allocate'''
        return 2

    def num_peers(self):
        '''get amount of peer nodes to allocate'''
        return 2

    def trackers_use_infiniband(self):
        '''True if trackers must communicate with eachother over infiniband, False otherwise'''
        return False

    def peers_use_infiniband(self):
        '''True if peers must communicate with trackers over infinband, False otherwise'''
        return False

    def trackers_core_affinity(self):
        '''Amount of tracker processes which may be mapped on the same physical node'''
        return 1

    def peers_core_affinity(self):
        '''Amount of peer processes which may be mapped on the same physical node'''
        return 1


    def pre_experiment(self, peerkeeper):
        '''Execution before experiment starts. Executed on the remote once.'''
        print('Hi there! I am executed before the experiment starts!')
        print('According to my data, we will host')

        peerkeeper.register['a_key'] = 'Hello World'
        peerkeeper.register['secret'] = 42
        if peerkeeper.gid == None and peerkeeper.lid == None:
            print('I cannot use gid and lid here yet!')


    def get_peer_run_command(self, peerkeeper):
        '''Get peer run command, executed in All peer nodes'''
        return 'while :; do echo "I am a running peer"; sleep 20; done'


    def experiment_peer(self, peerkeeper):
        '''Execution occuring on ALL peer nodes'''
        print('Hello from peer with gid={}. I am told these hosts exist: {}'.format(peerkeeper.gid, peerkeeper.hosts))
        time.sleep(5)
        print('I (peer {}:{}) slept well. Pre-experiment says "{}" with secret code {}. Goodbye!'.format(
            peerkeeper.gid,
            peerkeeper.lid,
            peerkeeper.register['a_key'],
            peerkeeper.register['secret']))


    def experiment_tracker(self, peerkeeper):
        '''Execution occuring on ALL tracker nodes'''
        print('I am tracker {}:{}, and I will try to modify the register now'.format(peerkeeper.gid, peerkeeper.lid))
        try:
            peerkeeper.register['secret'] = -1
        except Exception as e:
            print('Turns out I (tracker {}) cannot add or change or delete variables after pre_experiment. Goodbye!'.format(peerkeeper.id))
        time.sleep(5)
        print('I (tracker {}:{}) slept well. Goodbye!'.format(
            peerkeeper.gid,
            peerkeeper.lid))

    def post_experiment(self, peerkeeper):
        '''get amount of peer nodes to allocate'''
        print('Experiments are done. Pre-experiment had this secret: {}'.format(peerkeeper.register['secret']))