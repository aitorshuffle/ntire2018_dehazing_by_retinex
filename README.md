# Requirements
* Python packages: sacred, skimage
* C++ compiler

# Execution instructions

* Download the code

```
$ git clone https://github.com/aitorshuffle/ntire2018_dehazing_by_retinex.git
$ cd ntire2018_dehazing_by_retinex
```

* Compile the C++ executable that performs the LRSR-based actual dehazing by inverting the input image, applying the retinex operation and inverting back the result:

```
$ cd lrsr_retinex4dehazing
$ make
```
Note that a precompiled executable built in a linux machine is also provided. In order to compile your own executable, please delete the lrsr file first.

* Place the hazy image in the ```in/``` directory

* Run the method. The input image can be specified by changing the 'basename' variable from within the python script or by calling it as follows::

```
$ python lrsr_single_img.py with basename=hazy1.png
```

* The output images are now located in the ```res/``` directory with the same name as the corresponding inputs.