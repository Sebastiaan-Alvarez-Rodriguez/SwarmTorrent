import matplotlib.pyplot as plt
import numpy as np

import result.util.storer as storer
import util.fs as fs
from result.util.reader import Reader
import util.location as loc

def performance(large, no_show, store_fig, filetype): 
    reader = Reader(fs.join(loc.get_peerkeeper_results_dir(), 'performance_experiment.csv'))

    for key in reader.get_file_sizes():
        # dictionary from peer_ids to list of durations
        result = reader.get_result(key)

        for peer_id in result:
            median = np.percentile(result[peer_id], 50)
            high = np.percentile(result[peer_id], 75)
            low = np.percentile(result[peer_id], 25)
            plt.plot(peer_id, median, label=key)
            plt.vlines(peer_id, low, high, alpha=0.6, label=key)

    if large:
        fig.set_size_inches(10, 8)

    fig.tight_layout()

    if store_fig:
       storer.store('performance', filetype, plt)

    if large:
        plt.rcdefaults()

    if not no_show:
        plt.show()
