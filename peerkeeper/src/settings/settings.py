# This file contains code to generate a small config file,
# containing the only stateful data of MetaZoo.
# 
# The data stored here should be things we do not want to ask
# the user every time they use MetaZoo


import configparser

import util.fs as fs
from util.printer import *
import util.ui as ui

# Gets path to config storage location
def get_settings_file():
    return fs.join(fs.abspath(), 'peerkeeper.cfg')

def ask_ssh_key_name():
    return ui.ask_string('Please type the name of your SSH entry (e.g. "dpsDAS5LU")', confirm=True)

def ask_ssh_user_name():
    return ui.ask_string('What is your username on the remote machine (e.g. "dpsDAS5LU")?')

def ask_remote_swarmtorrent_dir():
    return ui.ask_string('''
In which directory to install SwarmTorrent on the remote machine (e.g. /some/path)?
Note: SwarmTorrent is installed in that path. So in our example, remote would get /some/path/SwarmTorrent/
Note: This directory you choose MUST be accessible by all nodes we might spawn on the remote. Recommendation: /var/scratch/<some_dir>
''')

# Ask user questions to generate a config
def gen_config(config_loc):
    write_config(config_loc, ask_ssh_key_name(), ask_ssh_user_name(), ask_remote_swarmtorrent_dir())

# Persist a configuration file
def write_config(config_loc, key_name, user, swarmtorrent_dir):
    parser = configparser.ConfigParser()
    parser['SSH'] = {
        'key_name': key_name,
        'user': user,
        'swarmtorrent_dir': swarmtorrent_dir
    }
    with open(config_loc, 'w') as file:
        parser.write(file)

# Change an amount of user settings
def change_settings():
    if not fs.exists(get_metazoo_settings_file()):
        gen_config(get_metazoo_settings_file())
        return
    l = ['key_name', 'user', 'swarmtorrent_dir']
    while True:
        idx = ui.ask_pick('Which setting to change?', l)
        cur = [s.ssh_key_name, s.ssh_user_name, s.remote_swarmtorrent_dir]
        print('\nCurrent value: "{}"'.format(cur[idx]))
        if idx == 0:
            s.ssh_key_name = ask_ssh_key_name
        elif idx == 1:
            s.ssh_user_name = ask_ssh_user_name()
        elif idx == 2:
            s.ask_remote_metazoo_dir = ask_remote_swarmtorrent_dir()
        s.persist()
        if ui.ask_bool('Done?'):
            return

# Check if all required data is present in the config
def validate_settings(config_loc):
    d = dict()
    d['SSH'] = {'key_name', 'user', 'swarmtorrent_dir'}
    
    parser = configparser.ConfigParser()
    parser.read(config_loc)
    for key in d:
        if not key in parser:
            raise RuntimeError('Missing section "{}"'.format(key))
        else:
            for subkey in d[key]:
                if not subkey in parser[key]:
                    raise RuntimeError('Missing key "{}" in section "{}"'.format(subkey, key))


class SettingsConfig(object):    
    '''
    Simple object to quickly interact with stored settings.
    This way, we don't have to read in the config every time,
    or pass it along a large amount of times.
    Below, we define a global instance.
    '''
    def __init__(self):
        loc = get_metazoo_settings_file()
        if not fs.exists(loc):
            gen_config(loc)
        else:
            validate_settings(loc)
        self.parser = configparser.ConfigParser()
        self.parser.read(loc)


    # SSH key to use when communicating with remote
    @property
    def ssh_key_name(self):
        return self.parser['SSH']['key_name']

    # Username on remote
    @property
    def ssh_user_name(self):
        return self.parser['SSH']['user']

    # Path to the desired location to store swarmtorrent on the remote
    @property
    def remote_swarmtorrent_dir(self):
        return self.parser['SSH']['swarmtorrent_dir']

    # Persist current settings
    def persist():
        with open(config_loc, 'w') as file:
            parser.write(file)


# Import settings_instance if you wish to read settings
settings_instance = SettingsConfig()
