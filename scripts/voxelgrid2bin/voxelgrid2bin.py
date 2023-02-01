#!/bin/python3
# Converts json files a voxel grid definition to simple binary files

import argparse
from pathlib import Path
import os
import logging
import config
import json
import numpy as np
import struct



def write_vec3f_byte(file, vec3):
    file.write(struct.pack('4f', *[vec3[0], vec3[1], vec3[2], 0]))

def write_vec3i_byte(file, vec3):
    file.write(struct.pack('4I', *[vec3[0], vec3[1], vec3[2], 0]))


def write_voxel_byte(file, voxel):
    write_vec3f_byte(file, voxel[0])
    write_vec3f_byte(file, voxel[1])
    write_vec3f_byte(file, voxel[2])


if __name__ == "__main__":
    
    parser = argparse.ArgumentParser()
    parser.add_argument('InputFile', type=Path,
                        help='Path of the .json file to be parsed.')
    parser.add_argument('OutputFile', nargs='?', type=Path, default=None,
                        help='Path to export')
    parser.add_argument('-d', '--debug', help="Print lots of debugging statements",
                        action="store_const", dest="loglevel", const=logging.DEBUG,
                        default=logging.WARNING,)
    parser.add_argument('-v', '--verbose', help="Be verbose",
                        action="store_const", dest="loglevel", const=logging.INFO,)
    args = parser.parse_args()    
    logging.basicConfig(level=args.loglevel)

    #get the input file
    jsonFilePath = args.InputFile
    if (not jsonFilePath.suffix == '.json'):
        logging.error("The specified input file is not a .json file.")
        exit()

    config.JSON_FILE_FOLDER = os.path.relpath(os.path.dirname(os.path.abspath(jsonFilePath)))         #doing abspath() to give path to a file when it is in the current folder
    logging.info(f"Relative path of the master json file: {config.JSON_FILE_FOLDER}")
    jsonFilePath = os.path.join(config.JSON_FILE_FOLDER, os.path.basename(jsonFilePath))

    #output path
    binFilePath = args.OutputFile
    if binFilePath is None:
        binFilePath = os.path.splitext(jsonFilePath)[0]+'.bin'
    config.BIN_FILE_FOLDER = os.path.dirname(binFilePath)

    print(jsonFilePath, binFilePath)


    #write data in the output file
    with open(jsonFilePath) as jsonFile:
        outputFile = open(binFilePath,'wb')
        
        data = json.load(jsonFile)

        width  = data["width"]
        height = data["height"]
        depth  = data["depth"]
        
        write_vec3i_byte(outputFile, [width, height, depth])

        for z_plane in data["grid"]:
            for y_row in z_plane:
                for voxel in y_row:
                    write_voxel_byte(outputFile, voxel)

        outputFile.close()


    print(f"Scene written to {binFilePath}.")
