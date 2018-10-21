import os
import yaml


""" The name of the info folder in the session folder"""
INFO_NAME = 'system.info'


def extract_hw_config(session_folder):
    """ Extracts the hardware config portion of a system.info file
        This is returned as a nested dictionary of entries """
    with open(os.path.join(session_folder, INFO_NAME)) as f:
        return extract_yaml(f, "settings.Hardware")


def extract_sw_config(session_folder):
    """ Extracts the software config portion of a system.info file
        This is returned as a nested dictionary of entries """
    with open(os.path.join(session_folder, INFO_NAME)) as f:
        return extract_yaml(f, "settings.UserConfig")


def extract_yaml(file, class_name):
    """ Using the given class name, this will extract the yaml file from the system.info file"""
    config_string = ""
    config = False
    for line in file:
        # Exclude the class string, since it is only used in Java
        if class_name in line:
            config = True
        elif line.isspace():
            config = False
        elif config:
            config_string += line

    config_dict = yaml.load(config_string)
    return config_dict

