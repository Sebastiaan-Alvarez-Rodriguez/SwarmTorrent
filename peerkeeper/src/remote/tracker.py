from util.executor import Executor

# Starts tracker, returns immediately after starting a thread containing our process
def boot(port):
    command = f'./tracker -p {port}'
    executor = Executor(command)
    executor.run(shell=True)
    return executor

# Stops tracker instance
def stop(executor):
    return executor.stop()
