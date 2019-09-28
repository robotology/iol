/* 
 * Copyright (C) 2011 Department of Robotics Brain and Cognitive Sciences - Istituto Italiano di Tecnologia
 * Author: Carlo Ciliberto
 * email:  carlo.ciliberto@iit.it
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
 \defgroup icub_blobSelector Blob Selector
 
 Module that uses the information from the motionCut module to select the recognized blob of interest.
 \ref icub_blobSelector application.

 \section intro_sec Description
 This module is responsible for selecting the blob of interest from a human pointing action. 
 based upon the \ref icub_motionCUT module. To this end, it receives
 motion independent blob input from the motionCUT and select the x and y position of the pointing

 The commands sent as bottles to the module port
 /<modName>/rpc are the following:

 None available at the moment.

 \section lib_sec Libraries
 - YARP libraries, OpenCV.

 \section portsc_sec Ports Created
 - \e /<modName>/img:i receives the image acquired from the
 lumaChroma module previously specified through the command-line
 parameters.

 - \e /<modName>/img:o streams out an image containing the motion information of the pointing.

 - \e /<modName>/point:o streams out the x and y position of the pointing.

 - \e /<modName>/blobs:i receives a yarp list containing all the motion blob information from the motionCUT module.


 \section parameters_sec Parameters
 --name \e name
 - specify the module stem-name, which is
 \e blobSelector by default. The stem-name is used as
 prefix for all open ports.

 --tracking_threshold \e value
 - specify the threshold value for the tracking. The default value is 0.5 seconds.

 --pointing_threshold \e value
 - specify the threshold value for the pointing. The default value is 2.5 seconds.


 \section tested_os_sec Tested OS
 Windows, Linux, Mac OS

 \author Vadim Tikhanoff
 */

#include <mutex>

#include <yarp/os/Network.h>
#include <yarp/os/BufferedPort.h>
#include <yarp/os/RFModule.h>
#include <yarp/os/PeriodicThread.h>
#include <yarp/os/Time.h>
#include <yarp/os/Stamp.h>

#include <yarp/sig/Image.h>
#include <yarp/sig/Vector.h>

#include <yarp/cv/Cv.h>

#include <opencv2/opencv.hpp>

#include <string>
#include <list>
#include <cmath>

#include <iostream>
#include <fstream>


using namespace std;
using namespace yarp::os;
using namespace yarp::sig;
using namespace yarp::cv;


#define IDLE 0
#define TRACKING 1
#define POINTING 2


class PointingThread: public PeriodicThread, public BufferedPort<Bottle>
{
private:
    ResourceFinder                  &rf;
    BufferedPort<ImageOf<PixelRgb>> inPort;
    BufferedPort<ImageOf<PixelRgb>> outPort;
    BufferedPort<Vector>            pointPort;
                                    
    double                          internal_time;
    double                          tracking_thresh;
    double                          pointing_thresh;
                                    
    list<Vector>                    positions;
    Vector                          lastPoint;
                                    
    mutex                           mtx;
    int                             state;

public:
    PointingThread(ResourceFinder &_rf)
        : PeriodicThread(0.005),rf(_rf) { }

    virtual bool threadInit()
    {
        string name=rf.find("name").asString();
        tracking_thresh=rf.check("tracking_threshold",Value(0.5)).asDouble();
        pointing_thresh=rf.check("pointing_threshold",Value(2.5)).asDouble();

        inPort.open("/"+name+"/img:i");
        outPort.open("/"+name+"/img:o");
        pointPort.open("/"+name+"/point:o");

        this->useCallback();
        this->open("/"+name+"/blobs:i");

        internal_time=0.0;

        positions.clear();
        lastPoint.resize(2,0.0);

        state=IDLE;
        return true;
    }

    virtual void run()
    {
        lock_guard<mutex> lg(mtx);
        if(ImageOf<PixelRgb> *imgIn=inPort.read(false))
        {
            ImageOf<PixelRgb> &imgOut=outPort.prepare();
            imgOut=*imgIn;
            cv::Mat imgOutMat=toCvMat(imgOut);

            if(positions.size()>0)
            {
                switch(state)
                {
                    case(IDLE):
                    {
                        break;
                    }
                    case(TRACKING):
                    {
                        if(Time::now()-internal_time>tracking_thresh)
                            state=POINTING;
                        else
                            cv::circle(imgOutMat,cv::Point((int)round(positions.back()[0]),
                                                           (int)round(positions.back()[1])),5,cv::Scalar(255),5);
                        break;
                    }
                    case(POINTING):
                    {
                        cv::circle(imgOutMat,cv::Point((int)round(positions.back()[0]),
                                                       (int)round(positions.back()[1])),5,cv::Scalar(0,0,255),5);
                        cv::circle(imgOutMat,cv::Point((int)round(positions.back()[0]),
                                                       (int)round(positions.back()[1])),10,cv::Scalar(0,255),5);

                        if ((positions.back()[0]!=lastPoint[0]) || (positions.back()[1]!=lastPoint[1]))
                        {
                            lastPoint=positions.back();
                            pointPort.prepare()=lastPoint;
                            pointPort.writeStrict();
                        }
                        break;
                    }
                }
            }
            imgOut=fromCvMat<PixelRgb>(imgOutMat);
            outPort.writeStrict();
        }
    }

    virtual void onRead(Bottle &bot)
    {
        lock_guard<mutex> lg(mtx);
        double curr_time=Time::now();

        if(bot.size()>0)
        {
            switch(state)
            {
                case(IDLE):
                {
                    positions.clear();

                    Bottle *blob=bot.get(0).asList();
                    Vector v(2);
                    v[0]=blob->get(0).asInt();
                    v[1]=blob->get(1).asInt();

                    state=TRACKING;
                    internal_time=curr_time;
                    break;
                }

                case(TRACKING):
                {
                    positions.clear();

                    Bottle *blob=bot.get(0).asList();
                    Vector v(2);
                    v[0]=blob->get(0).asInt();
                    v[1]=blob->get(1).asInt();

                    positions.push_back(v);
                    break;
                }

                case(POINTING):
                {
                    if(curr_time-internal_time>pointing_thresh)
                    {
                        state=TRACKING;
                        internal_time=curr_time;
                    }
                    break;
                }
            }
        }
    }

    virtual void threadRelease()
    {
        inPort.close();
        outPort.close();
        pointPort.close();
        this->close();
    }

    bool execReq(const Bottle &command, Bottle &reply)
    {
        return false;
    }
};


class DetectorModule: public RFModule
{
private:
    PointingThread *thr;
    Port            rpcPort;

public:
    virtual bool configure(ResourceFinder &rf)
    {
        thr=new PointingThread(rf);

        if(!thr->start())
        {
            delete thr;
            return false;
        }

        string name=rf.find("name").asString();
        rpcPort.open("/"+name+"/rpc");
        attach(rpcPort);

        return true;
    }

    virtual bool interruptModule()
    {
        rpcPort.interrupt();

        return true;
    }

    virtual bool close()
    {
        thr->stop();
        delete thr;

        rpcPort.close();

        return true;
    }

    virtual double getPeriod()
    {
        return 0.1;
    }

    virtual bool updateModule()
    {
        return true;
    }

    virtual bool respond(const Bottle &command, Bottle &reply)
    {
        if(thr->execReq(command,reply))
            return true;
        else
            return RFModule::respond(command,reply);
    }
};


int main(int argc, char *argv[])
{
    Network yarp;

    if (!yarp.checkNetwork())
        return 1;

    ResourceFinder rf;
    rf.setVerbose(true);
    rf.setDefault("name","blobSelector");
    rf.configure(argc,argv);

    DetectorModule mod;

    return mod.runModule(rf);
}

