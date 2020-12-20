import util.fs as fs

class Reader(object):
    '''Object to read result data from a file
    Expects a file with format: file_size iteration peer_id duration'''
    def __init__(self, resultfile):
        if not fs.isfile(resultfile):
            raise RuntimeError('Cannot read result data from path "{}"'.format(resultfile))

        '''
        dictionary from file_size to a dictionary from peer_id to [durations]
        e.g. 50 --> [0, 1, 2, 3, 4], 0 --> [123, 414, 551, 203]
        '''
        self.result = dict()
        with open(resultfile, 'r') as file:
            lines = file.readlines()
            for line in lines[1:]:
                splitted = line.split()
                file_size = splitted[0]
                peer_id = splitted[2]
                duration = splitted[3]
                if file_size not in result:
                    self.result[file_size] = dict()
                if peer_id not in self.result[file_size]:
                    self.result[file_size][peer_id] = []
                self.result[file_size][peer_id].append(duration)


    def get_amount_file_sizes(self):
        return len(self.results)

    def get_file_sizes(self):
        return self.results.keys()

    # Returns dictionary from peer_id to a list of durations
    def get_result(self, key):
        return self.results[key]


