from util.executor import Executor

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

# Make a torrentfile by peer, returns immediately after starting a thread containing our process
def boot_make(infile, outfile, trackers):
    command = f'./peer make -i {infile} -o {outfile}'
    for tracker in trackers:
        command += f' -t {tracker}'
    executor = Executor(command)
    executor.run(shell=True)
    return executor

# Torrent a file, returns immediately after starting a thread containing our process
def boot_torrent(port, workpath, infile, register):
    command = f'./peer torrent -p {port} -w {workpath} -f {infile}'
    if (register):
        command += ' -r'
    executor = Executor(command)
    executor.run(schell=True)
    return executor

# Stops peer instance
def stop(executor):
    return executor.stop()
