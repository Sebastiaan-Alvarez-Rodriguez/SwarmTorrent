import time
from dynamic.experiment import Experiment
from remote.config import config_construct_tracker, config_construct_peer, Config
from remote.util.syncer import Syncer
import remote.tracker as tracker
import remote.peer as peer
import remote.util.identifier as idr
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
        for index in range(experiment.peerkeeper.num_files):
            syncer.sync()
            
            executor = tracker.boot(experiment)

            syncer.sync()
            status = experiment.experiment_tracker(config, executor, repeat)

            global_status &= status
            if config.gid == 0:
                prints('File size {}/{} complete'.format(index+1, experiment.peerkeeper.num_files)) 

            if not status:
                printw('Tracker {} status in iteration {}/{} not good'.format(config.gid, repeat, repeats-1))
                
        if config.gid == 0:
            prints('Tracker iteration {}/{} complete'.format(repeat+1, repeats))

    syncer.close()
    if config.gid == 0:
        fs.rm(loc.get_cfg())

    return global_status

def run_peer(debug_mode):
    experiment = Experiment.load()
    repeats = experiment.peerkeeper.repeats
    is_seeder = idr.identifier_global() == 0

    while not fs.isfile(loc.get_cfg()):
        time.sleep(1)
    with open(loc.get_cfg(), 'r') as file:
        trackers = [line.strip() for line in file.readlines()]

    config = config_construct_peer(experiment, trackers)
    syncer = Syncer(config, experiment, 'peer', debug_mode)

    global_status = True
    fs.mkdir(loc.get_node_dir(), exist_ok=True)
    for repeat in range(repeats):
        for index in range(experiment.peerkeeper.num_files):
            if debug_mode: print('Peer {} stage PRE_SYNC1'.format(idr.identifier_global()))
            syncer.sync()
            if debug_mode: print('Peer {} stage POST_SYNC1'.format(idr.identifier_global()))
            if is_seeder: 
                # cleanup previous iteration
                fs.rm(loc.get_swarmtorrent_log_dir(), ignore_errors=True)
                fs.mkdir(loc.get_swarmtorrent_log_dir())
                fs.rm(loc.get_swarmtorrent_torrentfile(), ignore_errors=True)
                if debug_mode: print('Peer {} is seeder'.format(idr.identifier_global()))
                # create file to torrent
                os.system('head -c {}MB /dev/urandom > {}'.format(experiment.file_size(index), loc.get_initial_file(index)))
                if not fs.isfile(loc.get_initial_file(index)):
                    raise RuntimeError('{} does not exist'.format(loc.get_initial_file(index)))

                # waiting for tracker to boot
                time.sleep(5)
                executor = peer.boot_make(experiment, config, index)
                executor.wait()
                syncer.sync()
                if debug_mode: print('Peer {} made torrent'.format(idr.identifier_global()))
                executor = peer.boot_torrent(experiment, config, repeat)
            else:
                if debug_mode: print('Peer {} is not seeder'.format(idr.identifier_global()))
                syncer.sync()
                time.sleep(5)
                printw('Booting peer')
                executor = peer.boot_torrent(experiment, config , repeat)

            status = experiment.experiment_peer(config, executor, repeat)
            global_status &= status

            if not status:
                printw('Peer {} status in iteration {}/{} not good'.format(config.gid, repeat, repeats-1))


    syncer.close()
    if idr.identifier_local() == 0:
        fs.rm(loc.get_node_dir())
    return global_status