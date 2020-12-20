import matplotlib.pyplot as plt

import util.location as loc
import util.fs as fs

# Returns a generator for extensions matplotlib can store figures in
# Looks like ('pdf', 'svg', ...)
def supported_filetypes():
    generator = (x for x in plt.figure().canvas.get_supported_filetypes())
    plt.clf()
    plt.close()
    return generator

# Returns True if matplotlib supports filetype, False otherwise
def filetype_is_supported(extension):
    return str(extension).strip().lower() in supported_filetypes()

# Stores given <plotlike> in dir <dirname>/<filename>.<filetype>, passing kwargs to <plotlike>.savefig()
def store(filename, filetype, plotlike, **kwargs):
    fs.mkdir(loc.get_peerkeeper_graphs_dir(), exist_ok=True)
    plotlike.savefig(fs.join(loc.get_peerkeeper_graphs_dir(), '{}.{}'.format(filename, filetype)), **kwargs)