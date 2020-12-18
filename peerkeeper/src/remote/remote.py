import time
from dynamic.experiment import Experiment
from remote.config import config_construct, Config
from remote.util.syncer import Syncer
import remote.tracker as tracker
import remote.peer as peer
import util.fs as fs
import util.location as loc
from util.printer import *

def run_tracker(debug_mode):
    experiment = Experiment.load()
    repeats = experiment.peerkeeper.repeats
    config = config_construct(experiment, True)

    if config.gid == 0:
        print('Network booted. Prime ready')

    syncer = Syncer(config, experiment, 'tracker', debug_mode)

    global_status = True
    for repeat in range(repeats):
        syncer.sync()
        executor = tracker.boot(experiment)

        status = experiment.experiment_tracker(config, executor, repeat)

        global_status &= status
        if config.gid == 0:
            prints('Tracker iteration {}/{} complete').format(repeat+i, repeats)

        if not status:
            printw('Tracker {} status in iteration {}/{} not good').format(config.gid, repeat, repeats-1)

    syncer.close()

    return global_status

def run_peer(debug_mode):
    experiment = Experiment.load()
    repeats = experiment.peerkeeper.repeats
    config = config_construct(experiment, False)
    syncer = Syncer(config, experiment, 'peer', debug_mode)
    is_seeder = experiment.peerkeeper.lid == 0

    # Required for initial seeder
    infile = loc.get_initial_file_dir()
    outfile = loc.get_output_loc()
    trackers = experiment.peerkeeper.trackers()

    global_status = True
    for repeat in range(repeats):
        if debug_mode: print('Peer {} stage PRE_SYNC1'.format(idr.identifier_global()))
        syncer.sync()
        if debug_mode: print('Peer {} stage POST_SYNC1'.format(idr.identifier_global()))
        if is_seeder: #TODO: wait for tracker ready?
            #TODO: create file to torrent --> somewhere in experiments
            executor = peer.boot_make(infile, outfile, trackers)
            executor.wait()
            #TODO: set timestamp?
            executor = peer.boot_torrent(experiment)
        else: #TODO: more/ different? Perhaps if timestamp != default, go go go?
            time.sleep(10)
            executor = peer.boot_torrent(experiment)

        status = experiment.experiment_peer(config, executor, repeat)
        global_status &= status

        if not status:
            printw('Peer {} status in iteration {}/{} not good').format(config.gid, repeat, repeats-1)


    syncer.close()
    return global_status