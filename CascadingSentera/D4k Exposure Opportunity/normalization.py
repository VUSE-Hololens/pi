# Utilities for normalization RGB/NIR cameras to each other
# For now, this only works with the Double4k Multispectral
import exifread as exif
from PIL import Image, TiffImagePlugin
import numpy as np

# Constants for various corrections
RED = 0
GRN = 1
BLU = 2

# Color correct matrix in the form:
# R2r, R2g, R2b
# G2r, G2g, G2b
# B2r, B2g, B2b

rgb_n_coef = np.transpose(np.float32([
    [ 1.150, -0.110, -0.034],
    [-0.329,  1.420, -0.199],
    [-0.061, -0.182,  1.377],
]))

# NDRE has a 2.7 correction factor due to the normalized constants being used
ndre_n_coef = np.transpose(np.float32([
    [ 1.000,  0.000, -0.956],
    [ 0.000,  0.000,  0.000],
    [-0.341,  0.000,  2.426],
])) * 2.7


def narrow_rgb(image):
    correct(image, "nrgb", rgb_n_coef)


def ndre(image):
    correct(image, "ndre", ndre_n_coef)


def correct(image, type, coef):
    with open(image, 'rb') as img_file:
        tags = exif.process_file(img_file, strict=False, details=False)
        exposure = tags["EXIF ExposureTime"].values[0].num / float(tags["EXIF ExposureTime"].values[0].den)
        gain = tags["EXIF ISOSpeedRatings"].values[0] / 100.0

        print("{} : Exp: {} Gain: {}".format(image, exposure, gain))
        normalization = 1.0 / (exposure * gain)

        original = Image.open(img_file)
        img = np.float32(original)

    width = img.shape[0]
    height = img.shape[1]
    depth = img.shape[2]

    # Apply color correction by reshaping, multiplying, and undoing the reshape
    img_corr = np.reshape(img, (width * height, depth))
    np.dot(img_corr, coef, img_corr)
    img_corr = np.reshape(img_corr, (width, height, depth))

    # Apply exposure normalization
    img_corr *= normalization

    print("Corr Max/Avg: {} / {}".format(np.amax(img_corr, (0, 1)), np.average(img_corr, (0, 1))))

    images = [
        Image.fromarray(img_corr[:, :, RED]),
        Image.fromarray(img_corr[:, :, GRN]),
        Image.fromarray(img_corr[:, :, BLU]),
    ]

    with TiffImagePlugin.AppendingTiffWriter(image + "." + type + ".corr.tiff", True) as tf:
        for tiff in images:
            tiff.save(tf)
            tf.newFrame()
