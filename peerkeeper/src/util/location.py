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


#################### Remote directories ####################
def get_remote_swarmtorrent_dir():
    return st.remote_swarmtorrent_dir

def get_remote_peerkeeper_dir():
    return fs.join(get_remote_metazoo_parent_dir(), 'peerkeeper')


#################### Node directories ####################
# Because we  will use client logging using plan 2, this should change
def get_node_log_dir():
    return '/local/{}/'.format(st.ssh_user_name)