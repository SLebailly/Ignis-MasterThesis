#!/bin/python3
# Converts json files a voxel grid definition to simple binary files

import argparse
from pathlib import Path
import os
import logging
import config
import json
import numpy as np
from random import random


if __name__ == "__main__":
    
    parser = argparse.ArgumentParser()
    parser.add_argument('OutputFile', type=Path,
                        help='Path to export')
    parser.add_argument('size', type=int,
                        help='size of the grid (in each dimension')
    parser.add_argument('-d', '--debug', help="Print lots of debugging statements",
                        action="store_const", dest="loglevel", const=logging.DEBUG,
                        default=logging.WARNING,)
    parser.add_argument('-v', '--verbose', help="Be verbose",
                        action="store_const", dest="loglevel", const=logging.INFO,)
    args = parser.parse_args()    
    logging.basicConfig(level=args.loglevel)

    #get the input file
    jsonFilePath = args.OutputFile
    if (not jsonFilePath.suffix == '.json'):
        logging.error("The specified output file is not a .json file.")
        exit()

    config.JSON_FILE_FOLDER = os.path.relpath(os.path.dirname(os.path.abspath(jsonFilePath)))         #doing abspath() to give path to a file when it is in the current folder
    logging.info(f"Relative path of the master json file: {config.JSON_FILE_FOLDER}")
    jsonFilePath = os.path.join(config.JSON_FILE_FOLDER, os.path.basename(jsonFilePath))

    size = args.size
        
    data = {
        "width": args.size,
        "height": args.size,
        "depth": args.size,
        "grid": []
    }

    for x in range(0, size):
        plane = []
        for y in range(0, size):
            row = []
            for z in range(0, size):
                sigma_s = [random(), random(), random()]
                sigma_a = [random(), random(), random()]
                row.append([sigma_a, sigma_s])
            plane.append(row)
        data["grid"].append(plane)
        
    jsonObject = json.dumps(data, indent=4)

    #write data in the output file
    with open(jsonFilePath,'w') as jsonFile:       
        jsonFile.write(jsonObject)
        jsonFile.close()


    print(f"Scene written to {jsonFilePath}.")
