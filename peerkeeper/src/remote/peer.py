from util.executor import Executor

# Make a torrentfile by peer, returns immediately after starting a thread containing our process
def boot_make(experiment, config, index):
    command = experiment.get_seeder_make_command(config, index)
    executor = Executor(command)
    executor.run(shell=True)
    return executor

# Torrent a file, returns immediately after starting a thread containing our process
def boot_torrent(experiment, config, repeat):
    command = experiment.get_peer_run_command(config, repeat)
    executor = Executor(command)
    executor.run(shell=True)
    return executor

# Stops peer instance
def stop(executor):
    return executor.stop()
