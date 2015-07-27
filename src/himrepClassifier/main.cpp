/*
 * Copyright (C) 2013 iCub Facility - Istituto Italiano di Tecnologia
 * Author: Sean Ryan Fanello
 * email:  sean.fanello@iit.it
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU General Public License, version 2 or any
 * later version published by the Free Software Foundation.
 *
 * A copy of the license can be found at
 * http://www.robotcub.org/icub/license/gpl.txt
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
*/

/**
\defgroup icub_himrepClassifier himrepClassifier

The module wraps around classifiers and coders making use of
hierarchical image representation which are provided in the
<a href="https://github.com/robotology/himrep">himrep</a> repository.

\section intro_sec Description
This module is responsible for the communication with
coders (i.e. <a href="https://github.com/robotology/himrep/tree/master/modules/sparseCoder">sparseCoder</a>
or <a href="https://github.com/robotology/himrep/tree/master/modules/caffeCoder">caffeCoder</a>)
and the classifier (i.e. <a href="https://github.com/robotology/himrep/tree/master/modules/linearClassifierModule">linearClassifierModule</a>)
for learning and classify feature vectors. Input features are passed, the output
are the scores of the SVM machines.

The commands sent as bottles to the module port
/himrepClassifier/rpc are the following:

(notation: [.] identifies a vocab, <.> specifies a double,
"." specifies a string)

<b>TRAIN</b> \n
format: [train] \n
action: starts the training process by waiting for an image and a bounding box
where the object is located.

<b>CLASSIFY</b> \n
format: [classify] \n
action: starts the classification process by waiting for an image and a bounding
box where the object is located, it returns the output scores in the form "class" "score".

<b>FORGET</b> \n
format: [forget] "class" \n
action: forgets the "class", deleting all the feature vectors in the database.
If "class"="all" all the classes are forgotten.

<b>BURST</b> \n
format: [burst] [start | stop] \n
action: If [start] it starts the training process by waiting for a stream of
images and bounding boxes where the object is located. If [stop] it stops the
current burst session and automatically trains the new class model.

<b>LIST</b> \n
format: [list] \n
reply: "ack" ("name_1" "name_2" ... "name_n") \n
action: returns the list of known classes names.

<b>CHANGE NAME</b> \n
format: [chname] "old_name" "new_name" \n
reply: "ack"/"nack" \n
action: change the current name of a known object with a new name.

\section lib_sec Libraries
- YARP libraries.

- OpenCV 2.2

\section portsc_sec Ports Created

- \e /himrepClassifier/rpc receives rpc requests for training
   and recognition.

- \e /himrepClassifier/img:i receives an input image.

- \e /himrepClassifier/img:o outputs the image to the coder.

- \e /himrepClassifier/classify:rpc RPC port that communicates
   with the classifier module.

- \e /himrepClassifier/SIFTimg:i reads the image with the
   extracted local descriptors from the coder.

- \e /himrepClassifier/SIFTimg:o outputs the image with the
   extracted local descriptors.

- \e /himrepClassifier/scores:i reads the classification scores
   from the classifier.

- \e /himrepClassifier/features:i reads the hierarchical image
   representation from the coder.

- \e /himrepClassifier/features:o outpus the hierarchical image
   representation to the classifier.

- \e /himrepClassifier/opc communication with the Object Property
  Collector.

\section parameters_sec Parameters

None.

\section tested_os_sec Tested OS
Linux, Windows 7

\author Sean Ryan Fanello
*/

#include "classifier.h"

int main(int argc, char *argv[])
{
   Network yarp;
   if (!yarp.checkNetwork())
       return 1;

   ResourceFinder rf;
   rf.setVerbose(true);
   rf.configure(argc,argv);

   Classifier classifier;
   return classifier.runModule(rf);
}
