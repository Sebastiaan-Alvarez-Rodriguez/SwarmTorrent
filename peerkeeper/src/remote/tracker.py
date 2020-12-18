from util.executor import Executor
from remote.util import ip

# # Generates a connectionlist, which client nodes read to decide which host to connect to 
# def gen_connectionlist(config, experiment):
#     # End goal:
#     # <node101>:<clientport1>
#     # <node101>:<clientport2>
#     # <node102>:<clientport1>
#     # <node102>:<clientport2>
#     clientport = 2181
#     serverlist = []
#     for x in range(len(config.nodes) // idr.num_procs_per_node()):
#         addr = ip.node_to_infiniband_ip(config.nodes[x]) if experiment.clients_use_infiniband else 'node{:03d}'.format(config.nodes[x]) 
#         for y in range(idr.num_procs_per_node()):
#             cport = clientport + (idr.num_procs_per_node()+1)*y
#             serverlist.append('{}:{}'.format(addr, cport))
#     return serverlist

def gen_trackerlist(config, experiment):
    # End goal: strings to be passed to a peer program directly: TCP:4:[port]:[ip]
    trackerlist = []
    #TODO: handle differently for higher tracker affinities
    for node in config.nodes:
        addr = ip.node_to_infiniband_ip(node) if experiment.peers_use_infiniband else ip.node_to_ip(node)
        trackerlist.append('{}:{}:{}:{}'.format('TCP', '4', experiment.tracker_port(), addr))


# Starts tracker, returns immediately after starting a thread containing our process
def boot(experiment):
    executor = Executor(experiment.get_tracker_run_command())
    executor.run(shell=True)
    return executor

# Stops tracker instance
def stop(executor):
    return executor.stop()
