import time
import os

from experiments.interface import ExperimentInterface
import util.location as loc
import util.fs as fs

# We suggest all experiments which print anything to the console
# to use below import statement. This forces in-order printing. 
from util.printer import *

def get_experiment():
    '''Pass your defined experiment class in this function so PeerKeeper can find it'''
    return ComparisonExperiment

class ComparisonExperiment(ExperimentInterface):
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

    def file_sizes(self):
        '''File sizes in MB'''
        return [50]

    def tracker_port(self):
        return 2320

    def seeder_port(self):
        return 2321

    def peer_port(self):
        return 2322

    def num_files(self):
        return len(self.file_sizes())

    def get_result_file(self, timestamp):
        return fs.join(loc.get_peerkeeper_results_dir(), 'v2_comparison_{}.csv'.format(timestamp))

    def check_ready(self):
        #TODO: via peerkeeper?
        return len(list(fs.ls(loc.get_swarmtorrent_log_dir()))) == self.num_peers()-1

    def process_logs(self, peerkeeper):
        durations = []
        for log in fs.ls(loc.get_swarmtorrent_log_dir(), full_paths=True):
            logfile = open(log, 'r')
            durations.append(logfile.read())

        resultfile = open(self.get_result_file(peerkeeper.timestamp), 'a')
        for x, duration in enumerate(durations):
            resultfile.write('{} {} {} {}\n'.format(self.file_sizes()[peerkeeper._index], peerkeeper.repeat, x, duration))


    def pre_experiment(self, peerkeeper):
        '''Execution before experiment starts. Executed on the remote once.'''
        peerkeeper._tracker_port = self.tracker_port()
        peerkeeper._num_files = self.num_files()
        fs.mkdir(loc.get_swarmtorrent_log_dir(), exist_ok=True)
        fs.mkdir(loc.get_swarmtorrent_torrentfile_dir(), exist_ok=True)
        resultfile = open(self.get_result_file(peerkeeper.timestamp), 'w+')
        resultfile.write('file_size iteration peer duration\n')



    def get_seeder_make_command(self, peerkeeper):
        outfile = loc.get_swarmtorrent_torrentfile()
        infile = loc.get_initial_file(peerkeeper.index)
        peerloc = fs.join(loc.get_swarmtorrent_dir(), 'peer')
        command = '{} make -i {} -o {}'.format(peerloc, infile, outfile)
        for tracker in peerkeeper._trackers:
            command += ' -t {}'.format(tracker)
        return command


    def get_peer_run_command(self, peerkeeper):
        '''Get peer run command, executed in All peer nodes'''
        register = peerkeeper.gid == 0
        port = self.seeder_port() if register else self.peer_port()
        port += peerkeeper.lid
        # workpath = loc.get_initial_file(peerkeeper._index) if register else loc.get_output_loc()
        workpath = fs.join(loc.get_node_dir(), peerkeeper.lid) if not register else loc.get_node_dir()
        if not register:
            fs.mkdir(workpath, exist_ok=True)
        torrentfile = loc.get_swarmtorrent_torrentfile()
        if not fs.isfile(torrentfile):
            raise RuntimeError('{} does not exist'.format(torrentfile))
        peerloc = fs.join(loc.get_swarmtorrent_dir(), 'peer')
        logfile = fs.join(loc.get_swarmtorrent_log_dir(), '.log{}'.format(peerkeeper.gid))
        command = '{} torrent -p {} -w {} -f {} -l {}'.format(peerloc, port, workpath, torrentfile, logfile)
        if register:
            command += ' -s'
        return command

    def get_tracker_run_command(self, peerkeeper):
        '''Get tracker run command, executed in All tracker nodes'''
        port = self.tracker_port()
        trackerloc = fs.join(loc.get_swarmtorrent_dir(), 'tracker')
        return '{} -p {}'.format(trackerloc, port)

    def experiment_peer(self, peerkeeper):
        '''Execution occuring on ALL peer nodes'''
        while (True):
            time.sleep(1)
            if self.check_ready():
                break

        time.sleep(5)
        status = peerkeeper.executor.stop()
        
        # cleanup files
        if peerkeeper.gid == 0:
            self.process_logs(peerkeeper)
        return status
        

    def experiment_tracker(self, peerkeeper):
        '''Execution occuring on ALL tracker nodes'''
        while (True):
            time.sleep(3)
            if self.check_ready():
                break

        time.sleep(5)
        return peerkeeper.executor.stop()

    def post_experiment(self, peerkeeper):
        '''get amount of peer nodes to allocate'''
        