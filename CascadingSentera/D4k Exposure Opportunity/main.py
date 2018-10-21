# Small script to output separate image bands based on an input jpg from the Double4k Camera.

import system_info as info
import normalization as normalize
import configparser
import exifread
import os
import cv2
from PIL import Image, ImageDraw, ImageStat
import numpy as np

# Config checks
SESSION_FOLDER = './sample_data/'


hw_config = info.extract_hw_config(SESSION_FOLDER)
sw_config = info.extract_sw_config(SESSION_FOLDER)

if hw_config['Assembly'] != '21020-02':
    print("ERROR: This script only works on Multispectral Imagers!")
    exit(-1)

# Now, read through the RGB/NDRE images, split the bands, and compute EO.  Save as 32 bit tiffs
for file in os.scandir(os.path.join(SESSION_FOLDER, sw_config['CameraConfig']['Imager'][0]['CamName'])):
    if file.name.endswith(".jpg"):
        normalize.narrow_rgb(file.path)
for file in os.scandir(os.path.join(SESSION_FOLDER, sw_config['CameraConfig']['Imager'][1]['CamName'])):
    if file.name.endswith(".jpg"):
        normalize.ndre(file.path)


# Now that the files are generated, select the same piece of both images
# This was the upper right for this test, since the lighting seemed uniform
rgb_averages = [ 0.0, 0.0, 0.0 ]
count = 0
for file in os.scandir(os.path.join(SESSION_FOLDER, sw_config['CameraConfig']['Imager'][0]['CamName'])):
    if file.name.endswith(".tiff"):
        print(file.path)
        rgb = Image.open(file.path)

        sample = np.array(rgb.getdata()).reshape(rgb.size[0], rgb.size[1])
        rgb_averages[0] += np.average(sample[3720:3820, 145:245])
        rgb.seek(1)
        sample = np.array(rgb.getdata()).reshape(rgb.size[0], rgb.size[1])
        rgb_averages[1] += np.average(sample[3720:3820, 145:245])
        rgb.seek(2)
        sample = np.array(rgb.getdata()).reshape(rgb.size[0], rgb.size[1])
        rgb_averages[2] += np.average(sample[3720:3820, 145:245])
        count += 1

rgb_averages[:] = [ x / count for x in rgb_averages]
print(rgb_averages)

# Now that the files are generated, select the same piece of both images
# This was the upper right for this test, since the lighting seemed uniform
rn_averages = [ 0.0, 0.0 ]
count = 0
for file in os.scandir(os.path.join(SESSION_FOLDER, sw_config['CameraConfig']['Imager'][1]['CamName'])):
    if file.name.endswith(".tiff"):
        print(file.path)
        nir = Image.open(file.path)

        sample = np.array(nir.getdata()).reshape(nir.size[0], nir.size[1])
        rn_averages[0] += np.average(sample[3375:3475, 107:207])
        nir.seek(2)
        sample = np.array(nir.getdata()).reshape(nir.size[0], nir.size[1])
        rn_averages[1] += np.average(sample[3375:3475, 107:207])
        count += 1

rn_averages[:] = [ x / count for x in rn_averages]
print(rn_averages)
