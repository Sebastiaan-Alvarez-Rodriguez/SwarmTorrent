from util.executor import Executor

# Starts tracker, returns immediately after starting a thread containing our process
def boot(experiment):
    executor = Executor(experiment.get_tracker_run_command())
    executor.run(shell=True)
    return executor

# Stops tracker instance
def stop(executor):
    return executor.stop()
