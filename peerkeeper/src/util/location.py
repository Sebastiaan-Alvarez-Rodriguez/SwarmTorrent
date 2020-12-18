# This file contains all relevant paths for PeerKeeper to function
# Here, we chose for a function-call approach instead of
# a global object, as we don't have to maintain state here.

import util.fs as fs
from settings.settings import settings_instance as st

#################### PeerKeeper directories ####################
def get_peerkeeper_experiment_dir():
    return fs.join(fs.abspath(), 'experiments')

def get_peerkeeper_results_dir():
    return fs.join(fs.abspath(), 'results')

def get_peerkeeper_graphs_dir():
    return fs.join(fs.abspath(), 'graphs')

#################### SwarmTorrent directories ####################
def get_swarmtorrent_dir():
    return fs.dirname(fs.dirname(fs.abspath()))

def get_swarmtorrent_test_dir():
    return fs.join(get_swarmtorrent_dir(), 'test')

def get_swarmtorrent_torrentfile_dir():
    return fs.join(get_swarmtorrent_test_dir(), 'tfs')

def get_swarmtorrent_torrentfile():
    return fs.join(get_swarmtorrent_torrentfile_dir(), 'tf.out')

def get_swarmtorrent_log_dir():
    return fs.join(get_swarmtorrent_test_dir(), 'logs')

#################### Remote directories ####################
def get_remote_swarmtorrent_dir():
    return st.remote_swarmtorrent_dir

def get_remote_peerkeeper_dir():
    return fs.join(get_remote_metazoo_parent_dir(), 'peerkeeper')

#################### Node directories ####################
def get_node_dir()
    return '/local/{}/'.format(st.ssh_user_name) 

def get_initial_file_dir():
    return fs.join(get_node_dir(), 'initial.out')

def get_output_loc():
    return fs.join(get_node_dir(), 'file.out')
