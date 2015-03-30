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

#include <iostream>
#include <string>
#include <algorithm>

#include <highgui.h>
#include <cv.h>

#include <yarp/os/all.h>
#include <yarp/sig/all.h>

using namespace std;
using namespace yarp::os;
using namespace yarp::sig;


class Classifier : public RFModule
{
    Mutex mutex;
    Port scoresInput;
    Port featureInput;
    Port featureOutput;
    Port imgOutput;

    RpcServer rpcPort;
    RpcClient rpcClassifier;
    RpcClient opcPort;

    BufferedPort<ImageOf<PixelRgb> > imgInput;
    BufferedPort<ImageOf<PixelRgb> > imgSIFTInput;
    BufferedPort<ImageOf<PixelRgb> > imgSIFTOutput;

    bool sync;
    bool doTrain;
    bool burst;

    vector<Bottle> trainingFeature;
    vector<Bottle> negativeFeature;
    string currObject;

    bool train(Bottle *locations, Bottle &reply);
    void classify(Bottle *blobs, Bottle &reply);
    bool getOPCList(Bottle &names);
    bool updateObjDatabase();

public:
    bool configure(ResourceFinder &rf);
    bool interruptModule();
    bool close();
    bool respond(const Bottle& command, Bottle& reply);
    double getPeriod();
    bool updateModule();
};
