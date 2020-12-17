import time
import remote.tracker as tracker
import remote.peer as peer
import util.fs as fs
import util.location as loc
from util.printer import *

#TODO: provide configs?

def run_tracker():
    executor = tracker.boot()

def run_seeder():
    executor = peer.boot_make()
    executor = peer.boot_torrent()

def run_leecher():
    executor = peer.boot_torrent()

