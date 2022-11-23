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


if __name__ == "__main__":
    
    parser = argparse.ArgumentParser()
    parser.add_argument('InputFile', type=Path,
                        help='Path of the .json file to be parsed.')
    parser.add_argument('SupervoxelSize', type=int,
                        help='Size (in each dimension) of each supervoxel')
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
        binFilePath = os.path.splitext(jsonFilePath)[0]+'_majorant.bin'
    config.BIN_FILE_FOLDER = os.path.dirname(binFilePath)

    supervoxelSize = args.SupervoxelSize


    #write data in the output file
    with open(jsonFilePath) as jsonFile:
        outputFile = open(binFilePath,'wb')
        
        data = json.load(jsonFile)

        width  = data["width"]
        height = data["height"]
        depth  = data["depth"]

        if width % supervoxelSize != 0 or height % supervoxelSize != 0 or depth % supervoxelSize != 0:
            logging.error("The specified Supervoxel size is not applicable for a grid with these dimensions.")
            exit()
        
        write_vec3i_byte(outputFile, [width//supervoxelSize, height//supervoxelSize, depth//supervoxelSize])

        grid = data["grid"]
        z_indx = 0
        while z_indx < depth:
            y_indx = 0
            while y_indx < height:
                x_indx = 0
                while x_indx < width:
                    majorant = [0.0, 0.0, 0.0]
                    for i in range(supervoxelSize):
                        for j in range(supervoxelSize):
                            for k in range(supervoxelSize):
                                vxl = grid[z_indx+i][y_indx+j][x_indx+k]
                                sigma_t = [vxl[0][0] + vxl[1][0], vxl[0][1] + vxl[1][1], vxl[0][2] + vxl[1][2]]
                                majorant = [max(majorant[0], sigma_t[0]), max(majorant[1], sigma_t[1]), max(majorant[2], sigma_t[2])]
                    write_vec3f_byte(outputFile, majorant)
                    x_indx += supervoxelSize
                y_indx += supervoxelSize
            z_indx += supervoxelSize


        outputFile.close()


    print(f"Supervoxel Grid written to {binFilePath}.")
