import matplotlib.pyplot as plt
import numpy as np

import result.util.storer as storer
import util.fs as fs
from result.util.reader import Reader
import util.location as loc

def scalability(large, no_show, store_fig, filetype): 
    s1 = []
    s2 = []
    s3 = []
    s4 = []
    for resultfile in fs.ls(loc.get_peerkeeper_results_dir()):
        if resultfile.startswith('scalability'):
            reader = Reader(resultfile)
            file_size = reader.get_file_sizes()[0]
            result = reader.get_result(file_size)
            if resultfile.startswith('scalability1'):
                for key in result:
                    s1.extend(result[key])
            elif resultfile.startswith('scalability2'):
                for key in result:
                    s2.extend(result[key])
            elif resultfile.startswith('scalability3'):
                for key in result:
                    s3.extend(result[key])
            elif resultfile.startswith('scalability4'):
                for key in result:
                    s4.extend(result[key])

    x_labels = ['1', '2', '3', '4']
    colors = ['steelblue', 'firebrick', 'darkgreen', 'mediumslateblue']
    medians = []
    highs = []
    lows = []
    medians.append(np.percentile(s1, 50))
    medians.append(np.percentile(s2, 50))
    medians.append(np.percentile(s3, 50))
    medians.append(np.percentile(s4, 50))
    highs.append(np.percentile(s1, 99))
    highs.append(np.percentile(s2, 99))
    highs.append(np.percentile(s3, 99))
    highs.append(np.percentile(s4, 99))
    lows.append(np.percentile(s1, 1))
    lows.append(np.percentile(s2, 1))
    lows.append(np.percentile(s3, 1))
    lows.append(np.percentile(s4, 1))
    for x, med, low, high, color in zip(x_labels, medians, lows, highs, colors):
        ax.scatter(x, median, color=color, label=x)
        ax.vlines(x, low, high, alpha=0.6, color=color)

    plt.legend()

    if not no_show:
        plt.show()

    # plt.legend()

    if large:
        plt.set_size_inches(10, 8)

    plt.tight_layout()

    if store_fig:
       storer.store('scalability{}'.format(X), filetype, plt)

    if large:
        plt.rcdefaults()

    # if not no_show:
    #     plt.show()
