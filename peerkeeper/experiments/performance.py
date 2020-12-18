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

    def get_result_file(self):
        return fs.join(loc.get_peerkeeper_results_dir(), 'performance_experiment.csv')

    def check_ready(self):
        #TODO: via peerkeeper?
        return len(list(fs.ls(loc.get_swarmtorrent_log_dir()))) == num_peers()-1

    def process_logs(self, peerkeeper):
        durations = []
        for log in fs.ls(loc.get_swarmtorrent_log_dir()):
            logfile = open(log, 'r')
            duration.append(logfile.read())

        resultfile = open(get_result_file(), 'a')
        for x, duration in enumerate(durations):
            # TODO: init filesize
            resultfile.write('{} {} {} {}\n'.format(filesize, peerkeeper.repeat, x, duration))


    def pre_experiment(self, peerkeeper):
        '''Execution before experiment starts. Executed on the remote once.'''
        #TODO: via peerkeeper?
        fs.mkdir(loc.get_swarmtorrent_log_dir(), exist_ok=True)
        fs.mkdir(loc.get_swarmtorrent_torrentfile_dir(), exist_ok=True)
        resultfile = open(get_result_file(), 'w+')
        resultfile.write('file_size iteration peer duration\n')

    def get_initial_seeder_run_command(self, peerkeeper):
        #TODO: generate infiles?
        #TODO: find tracker ips and generate required strings
        outfile = loc.get_swarmtorrent_torrentfile()
        command = f'./peer make -i {infile} -o {outfile}'
        for tracker in trackers:
            command += f' -t {tracker}'


    def get_peer_run_command(self, peerkeeper):
        '''Get peer run command, executed in All peer nodes'''
        register = peerkeeper.lid == 0
        port = 2322 if register else 2321
        #TODO: via peerkeeper?
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

    def experiment_peer(self, peerkeeper):
        '''Execution occuring on ALL peer nodes'''
        while (True):
            time.sleep(30)
            if check_ready():
                break

        status = peerkeeper.executor.stop()

        # TODO: use for different file sizes? What about initial seeders?
        #         run_time = metazoo.register['time']
        # time.sleep(run_time) #Client remains active for a while
        # nr_ratios = len(self.get_read_ratios())
        # for ratio in range(1, nr_ratios):
        #     metazoo.executor.cmd = self.get_run_command(metazoo, ratio)
        #     metazoo.executor.reboot()
        #     time.sleep(run_time)

        # metazoo.executor.stop()
        
        # cleanup files
        if peerkeeper.lid == 0:
            process_logs()
            fs.rm(loc.get_swarmtorrent_log_dir())
            fs.rm(loc.get_swarmtorrent_torrentfile())
            fs.rm(loc.get_initial_file_dir())
        else: 
            fs.rm(loc.get_output_loc()) 
        return status
        

    def experiment_tracker(self, peerkeeper):
        '''Execution occuring on ALL tracker nodes'''
        while (True):
            time.sleep(30)
            if check_ready():
                break
        return peerkeeper.executor.stop()

    def post_experiment(self, peerkeeper):
        '''get amount of peer nodes to allocate'''
        