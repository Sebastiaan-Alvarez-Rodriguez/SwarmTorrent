import matplotlib.pyplot as plt
import numpy as np

import result.util.storer as storer
import util.fs as fs
from result.util.reader import Reader
import util.location as loc

def performance(large, no_show, store_fig, filetype): 
    reader = Reader(fs.join(loc.get_peerkeeper_results_dir(), 'performance_experiment.csv'))

    colors = ['steelblue', 'firebrick', 'darkgreen']
    ax = plt.subplot(111)
    for color, key in zip(colors, reversed(list(reader.get_file_sizes()))):
        # dictionary from peer_ids to list of durations
        result = reader.get_result(key)

        durations = []
        for peer_id in result:
            durations.extend(result[peer_id])

        medians = []
        highs = []
        lows = []
        for peer_id in result:
            medians.append(np.percentile(result[peer_id], 50))
            highs.append(np.percentile(result[peer_id], 99))
            lows.append(np.percentile(result[peer_id], 1))
        ax.scatter(list(result.keys()), medians, color=color, label=key)
        ax.vlines(list(result.keys()), lows, highs, alpha=0.6, color=color)

    box = ax.get_position()
    ax.set_position([box.x0, box.y0, box.width*0.8, box.height])
    ax.legend(loc='center left', bbox_to_anchor=(1, 0.5), title='file sizes', fancybox=True)
    ax.set_xlabel('chronological order of finished peers')
    ax.set_ylabel('time in seconds')
    ax.set_title('Performance Results')

    if not no_show:
        plt.show()

    
    # if large:
    #     plt.set_size_inches(10, 8)

    if store_fig:
       storer.store('performance', filetype, plt)

    if large:
        plt.rcdefaults()

    # if not no_show:
    #     plt.show()
