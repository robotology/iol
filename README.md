iol
===

Interactive Objects Learning

## Installation

##### Dependencies
- [YARP](https://github.com/robotology/yarp) (with `LUA` bindings compiled)
- [iCub](https://github.com/robotology/icub-main)
- [icub-contrib-common](https://github.com/robotology/icub-contrib-common)
- [OpenCV](http://opencv.org/downloads.html) (**`3.0.0` or higher with tracking features enabled**)
    1. Download `OpenCV`: `git clone https://github.com/Itseez/opencv.git`.
    2. Checkout the correct branch: e.g. `git checkout 3.0.0`.
    3. Download the external modules: `git clone https://github.com/Itseez/opencv_contrib.git`.
    4. Checkout the **same** branch: e.g. `git checkout 3.0.0`.
    5. Configure `OpenCV` by filling in **`OPENCV_EXTRA_MODULES_PATH`** with the path to `opencv_contrib/modules` and then toggling on **`BUILD_opencv_tracking`**.
    6. Compile `OpenCV`.
- [LUA](http://wiki.icub.org/yarpdoc/yarp_swig.html#yarp_swig_lua)
- [rFSM](https://github.com/kmarkus/rFSM) (just clone it, we don't need to compile)
- [segmentation](https://github.com/robotology/segmentation)
- [Hierarchical Image Representation](https://github.com/robotology/himrep)
- [stereo-vision](https://github.com/robotology/stereo-vision)
- [speech](https://github.com/robotology/speech)
- [Multiple Instance Boosting](https://github.com/robotology/boost-mil) (optional, required to compile [`milClassifier`](https://github.com/robotology/iol/tree/master/src/milClassifier))

Remember to export the environment variable `LUA_PATH` with paths to lua scripts
located in `rFSM` and `iol` directories and put them also in the `PATH`.<br>
Example:
- `export LUA_PATH=";;;/path_to/rFSM/?.lua;$ICUBcontrib_DIR/share/ICUBcontrib/contexts/iol/lua/?.lua"`
- `export PATH=$PATH:/path_to/rFSM/tools;$ICUBcontrib_DIR/share/ICUBcontrib/contexts/iol/lua`

## Documentation

Online documentation is available here: [http://robotology.github.com/iol](http://robotology.github.com/iol).

## Results

A video showing the recognition and interaction capabilities achieved by means
of **IOL** components can be seen [here](https://www.youtube.com/watch?v=ghUFweqm7W8).

## License

Material included here is Copyright of _iCub Facility - Istituto Italiano di
Tecnologia_ and is released under the terms of the GPL v2.0 or later.
See the file LICENSE for details.
