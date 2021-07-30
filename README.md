Interactive Objects Learning
============================

[![ZenHub](https://img.shields.io/badge/Shipping_faster_with-ZenHub-435198.svg)](https://zenhub.com)

![gh-pages](https://github.com/robotology/iol/workflows/GitHub%20Pages/badge.svg)

## Installation

##### Dependencies
- [YARP](https://github.com/robotology/yarp) (with `LUA` bindings compiled)
- [iCub](https://github.com/robotology/icub-main)
- [icub-contrib-common](https://github.com/robotology/icub-contrib-common)
- [OpenCV](http://opencv.org/downloads.html) (**`3.3.0` or higher with tracking features enabled**)
    1. Download `OpenCV`: `git clone https://github.com/opencv/opencv.git`.
    2. Checkout the correct branch/tag: e.g. `git checkout 3.3.0`.
    3. Download the external modules: `git clone https://github.com/opencv/opencv_contrib.git`.
    4. Checkout the **same** branch/tag: e.g. `git checkout 3.3.0`.
    5. Configure `OpenCV` by filling in **`OPENCV_EXTRA_MODULES_PATH`** with the path to `opencv_contrib/modules` and then toggling on all possible modules.
    6. Compile `OpenCV`.
- [LUA](http://wiki.icub.org/yarpdoc/yarp_swig.html#yarp_swig_lua)
- [rfsmTools](https://github.com/robotology/rfsmTools)
- [segmentation](https://github.com/robotology/segmentation)
- [Hierarchical Image Representation](https://github.com/robotology/himrep)
- [stereo-vision](https://github.com/robotology/stereo-vision)
- [speech](https://github.com/robotology/speech)

Remember to export the environment variable `LUA_PATH` with paths to lua scripts
located in `iol` directory and put them also in the `PATH`.<br>
Example:
- `export LUA_PATH=";;;$ICUBcontrib_DIR/share/ICUBcontrib/contexts/iol/lua/?.lua"`
- `export PATH=$PATH:$ICUBcontrib_DIR/share/ICUBcontrib/contexts/iol/lua`

## Documentation

Online documentation is available here: [https://robotology.github.io/iol/](https://robotology.github.io/iol).

## Results

A video showing the recognition and interaction capabilities achieved by means
of **IOL** components can be seen [here](https://www.youtube.com/watch?v=ghUFweqm7W8).

## License

Material included here is Copyright of _iCub Facility - Istituto Italiano di
Tecnologia_ and is released under the terms of the GPL v2.0 or later.
See the file LICENSE for details.
