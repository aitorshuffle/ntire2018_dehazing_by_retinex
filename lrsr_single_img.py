# -*- coding: utf-8 -*-
import os
from subprocess import call

import matplotlib

matplotlib.use('Agg')
from sacred import Experiment
ex = Experiment('lrsr')


from skimage import data
from skimage.morphology import disk
from skimage.filters.rank import median
from skimage.io import imread, imsave
from skimage.color import rgb2hsv, hsv2rgb
from skimage import img_as_float, img_as_ubyte


@ex.config
def my_config():
    """
    user-tunnable vars for experiments
    """
    track = 'Indoor'  # Outdoor

    retinex_method = 'lrsr'
    basename = 'hazy1.png'  #needs to be in png format
    fpath_in = os.path.join('./in/', basename)
    fpath_out = os.path.join('./res/', basename)

    N = 1
    n = 341
    k1 = 12
    k2 = k1
    normalize = 0


@ex.automain
def lrsr4dehazing(retinex_method, N, n, k1, k2, normalize, fpath_in, fpath_out):
    if retinex_method.lower() == 'lrsr':
        call_str = ['../lrsr_retinex4dehazing/lrsr', fpath_in, fpath_out, str(N),  str(n), str(k1), str(k2), str(normalize)]
    call(call_str)
	
	## apply median filtering
    img = imread(fpath_out)
    hsv = rgb2hsv(img)
    hsv[:,:,2] = img_as_float(median(img_as_ubyte(hsv[:,:,2]), disk(3)))
    img = hsv2rgb(hsv)
    imsave(fpath_out, img)
	
    call(['pwd'])
    print('Finished running ' + retinex_method + ' as :' + ' '.join(call_str))
    print('in: ' + fpath_in)
    print('out: ' + fpath_out)
