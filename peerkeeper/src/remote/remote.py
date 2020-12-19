import time
from dynamic.experiment import Experiment
from remote.config import config_construct_tracker, config_construct_peer, Config
from remote.util.syncer import Syncer
import remote.tracker as tracker
import remote.peer as peer
import util.fs as fs
import util.location as loc
from util.printer import *

def run_tracker(debug_mode):
    experiment = Experiment.load()
    repeats = experiment.peerkeeper.repeats
    config = config_construct_tracker(experiment)

    if config.gid == 0:
        print('Network booted. Prime ready')
        with open(loc.get_cfg(), 'w') as file:
            file.write('\n'.join(tracker.gen_trackerlist(config, experiment)))

    syncer = Syncer(config, experiment, 'tracker', debug_mode)

    global_status = True
    for repeat in range(repeats):
        for index in range(experiment.num_files()):
            syncer.sync()
            executor = tracker.boot(experiment)

            status = experiment.experiment_tracker(config, executor, repeat)

            global_status &= status
            if config.gid == 0:
                prints('Tracker iteration {}/{} complete').format(repeat+1, repeats)

            if not status:
                printw('Tracker {} status in iteration {}/{} not good').format(config.gid, repeat, repeats-1)

    syncer.close()
    fs.rm(loc.get_cfg())

    return global_status

def run_peer(debug_mode):
    experiment = Experiment.load()
    repeats = experiment.peerkeeper.repeats
    is_seeder = experiment.peerkeeper.lid == 0

    while not fs.isfile(loc.get_cfg()):
        time.sleep(1)
    with open(loc.get_cfg(), 'r') as file:
        trackers = [line.strip() for line in file.readlines()]

    config = config_construct_peer(experiment, trackers)
    syncer = Syncer(config, experiment, 'peer', debug_mode)

    global_status = True
    for repeat in range(repeats):
        for index in range(experiment.num_files()):
            if debug_mode: print('Peer {} stage PRE_SYNC1'.format(idr.identifier_global()))
            syncer.sync()
            if debug_mode: print('Peer {} stage POST_SYNC1'.format(idr.identifier_global()))
            if is_seeder: 
                # create file to torrent
                os.system('head -c {}MB /dev/urandom > {}'.format(experiment.file_size(index), loc.get_initial_file(index)))

                executor = peer.boot_make(experiment, config, index)
                executor.wait()
                executor = peer.boot_torrent(experiment, config, repeat)
            else:
                time.sleep(10)
                executor = peer.boot_torrent(experiment)

            status = experiment.experiment_peer(config, executor, repeat)
            global_status &= status

            if not status:
                printw('Peer {} status in iteration {}/{} not good').format(config.gid, repeat, repeats-1)


    syncer.close()
    return global_status