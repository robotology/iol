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

#include <yarp/os/Network.h>
#include <yarp/os/BufferedPort.h>
#include <yarp/os/RFModule.h>
#include <yarp/os/RateThread.h>
#include <yarp/os/Semaphore.h>
#include <yarp/os/Time.h>
#include <yarp/os/Stamp.h>

#include <yarp/sig/Image.h>
#include <yarp/sig/Vector.h>

#include <cv.h>
#include <highgui.h>

#include <string>
#include <list>

#include <iostream>
#include <fstream>


using namespace std;
using namespace yarp::os;
using namespace yarp::sig;


#define IDLE 0
#define TRACKING 1
#define POINTING 2


class PointingThread: public RateThread, public BufferedPort<Bottle>
{
private:
    ResourceFinder              &rf;

    BufferedPort<Image>         inPort;
    Port                        outPort;
    Port                        pointPort;

    double                      internal_time;
    double                      tracking_thresh;
    double                      pointing_thresh;

    list<Vector>                positions;
    Vector                      lastPoint;

    Semaphore                   mutex;

    int                         state;

public:
    PointingThread(ResourceFinder &_rf)
        :RateThread(5),rf(_rf)
    {
    }

    virtual bool threadInit()
    {
        string name=rf.find("name").asString().c_str();
        tracking_thresh=rf.check("tracking_threshold",Value(0.5)).asDouble();
        pointing_thresh=rf.check("pointing_threshold",Value(2.5)).asDouble();


        inPort.open(("/"+name+"/img:i").c_str());
        outPort.open(("/"+name+"/img:o").c_str());
        pointPort.open(("/"+name+"/point:o").c_str());

        this->useCallback();
        this->open(("/"+name+"/blobs:i").c_str());

        internal_time=0.0;

        positions.clear();
        lastPoint.resize(2,0.0);

        state=IDLE;

        return true;
    }

    virtual void run()
    {
        Image *img=inPort.read(false);
        if(img!=NULL)
        {
            if(positions.size()>0)
            {
                mutex.wait();
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
                            cvCircle(img->getIplImage(),cvPoint(cvRound(positions.back()[0]),cvRound(positions.back()[1])),5,cvScalar(255),5);
                        break;
                    }

                    case(POINTING):
                    {
                        cvCircle(img->getIplImage(),cvPoint(cvRound(positions.back()[0]),cvRound(positions.back()[1])),5,cvScalar(0,0,255),5);
                        cvCircle(img->getIplImage(),cvPoint(cvRound(positions.back()[0]),cvRound(positions.back()[1])),10,cvScalar(0,255),5);

                        if ((positions.back()[0]!=lastPoint[0]) || (positions.back()[1]!=lastPoint[1]))
                        {
                            pointPort.write(positions.back());
                            lastPoint=positions.back();
                        }

                        break;
                    }

                }
                mutex.post();
            }
            outPort.write(*img);
        }
    }

    virtual void onRead(Bottle &bot)
    {
        double curr_time=Time::now();

        mutex.wait();


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

            mutex.post();


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
    PointingThread          *thr;

    Port                     rpcPort;


public:
    virtual bool configure(ResourceFinder &rf)
    {
        thr=new PointingThread(rf);

        if(!thr->start())
        {
            delete thr;
            return false;
        }

        string name=rf.find("name").asString().c_str();
        rpcPort.open(("/"+name+"/rpc").c_str());
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
        return -1;

    ResourceFinder rf;
    rf.setVerbose(true);
    rf.setDefault("name","blobSelector");
    rf.configure("ICUB_ROOT",argc,argv);

    DetectorModule mod;

    return mod.runModule(rf);
}




