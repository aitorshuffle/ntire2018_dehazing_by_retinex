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
    retinex_method = 'lrsr'
    N=1
    n=341
    track = 'Indoor' #Outdoor
    seed = 1337

@ex.automain
def lrsr4dehazing(retinex_method, N, n, k1, k2, normalize, fpath_in, fpath_out):
    if retinex_method.lower() == 'lrsr':
        call_str = ['../lrsr_retinex4dehazing/lrsr', fpath_in, fpath_out, str(N),  str(n), str(k1), str(k2), str(normalize)]
        # call_str = ['./MSR_original', '-N', to_display_domain_dict[to_display_domain_method], '-g', str(gamma), '-W', wb_method_dict[wb_method], '-A', str(A[0]), '-B', str(A[1]), '-C', str(A[2]), '-l', str(l), '-R', str(R), fpath_in, fpath_out, 'dummy']
		
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
