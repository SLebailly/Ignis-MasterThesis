#!/bin/python3
# Converts PBRT Volume definition to a uniform voxel grid in json

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
                        help='Path of the .pbrt file to be parsed.')
    parser.add_argument('OutputFile', type=Path,
                        help='Path to export')
    parser.add_argument('sigma_a_r', help="absorption cross-section")
    parser.add_argument('sigma_a_g', help="absorption cross-section")
    parser.add_argument('sigma_a_b', help="absorption cross-section")
    parser.add_argument('sigma_s_r', help="scattering cross-section")
    parser.add_argument('sigma_s_g', help="scattering cross-section")
    parser.add_argument('sigma_s_b', help="scattering cross-section")
    parser.add_argument('scalar', help="scalar for density", type=float)
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

    #get the output file
    jsonOutFilePath = args.OutputFile
    if (not jsonOutFilePath.suffix == '.json'):
        logging.error("The specified output file is not a .json file.")
        exit()

    config.JSON_FILE_FOLDER = os.path.relpath(os.path.dirname(os.path.abspath(jsonOutFilePath)))         #doing abspath() to give path to a file when it is in the current folder
    logging.info(f"Relative path of the master json file: {config.JSON_FILE_FOLDER}")
    jsonOutFilePath = os.path.join(config.JSON_FILE_FOLDER, os.path.basename(jsonOutFilePath))


    sigma_a = [float(args.sigma_a_r), float(args.sigma_a_g), float(args.sigma_a_b)]
    sigma_s = [float(args.sigma_s_r), float(args.sigma_s_g), float(args.sigma_s_b)]

    if (len(sigma_a) != 3):
        print("Incorrect length of Sigma a")
    elif (len(sigma_s) != 3):
        print("Incorrect length of Sigma s")
    else:
        #write data in the output file
        with open(jsonFilePath) as jsonFile:            
            input = json.load(jsonFile)

            nx = input["nx"]
            ny = input["ny"]
            nz = input["nz"]
                    
            data = {
                "width": nx,
                "height": ny,
                "depth": nz,
                "grid": []
            }

            grid = input["density"]
            scalar = args.scalar

            if (len(grid) != nx * ny * nz):
                print("Incorrect grid dimensions")
                print("Expected", nx * ny * nz)
                print("got", len(grid))
            else:
                for x in range(0, nx):
                    plane = []
                    for y in range(0, ny):
                        row = []
                        for z in range(0, nz):
                            index = x + nx * (z * ny + y)
                            density = grid[index] * scalar
                            mu_s =  [density * sigma_s[0], density * sigma_s[1], density * sigma_s[2]]
                            mu_a =  [density * sigma_a[0], density * sigma_a[1], density * sigma_a[2]]
                            e = [0.0, 0.0, 0.0] #no emission
                            row.append([mu_a, mu_s, e])
                        plane.append(row)
                    data["grid"].append(plane)
                    
                jsonObject = json.dumps(data, indent=4)

                #write data in the output file
                with open(jsonOutFilePath,'w') as outputFile:       
                    outputFile.write(jsonObject)
                    outputFile.close()


                print(f"Scene written to {jsonOutFilePath}.")
