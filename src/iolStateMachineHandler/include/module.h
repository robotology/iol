/* 
 * Copyright (C) 2011 Department of Robotics Brain and Cognitive Sciences - Istituto Italiano di Tecnologia
 * Author: Ugo Pattacini
 * email:  ugo.pattacini@iit.it
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

#ifndef __MODULE_H__
#define __MODULE_H__

#include <string>
#include <deque>
#include <map>

#include <yarp/os/all.h>
#include <yarp/sig/all.h>
#include <iCub/ctrl/filters.h>

#include <opencv2/opencv.hpp>
#include <opencv2/tracking.hpp>

#include "utils.h"
#include "classifierHandling.h"

#define RET_INVALID     -1

using namespace std;
using namespace yarp::os;
using namespace yarp::sig;
using namespace iCub::ctrl;


/**********************************************************/
class Tracker
{
protected:
    enum { idle, init, no_need, tracking };

    string trackerType;
    int trackerState;
    double trackerTmo;
    double trackerTimer;    

    cv::Rect2d trackerResult;
    cv::Ptr<cv::Tracker> tracker;

public:
    Tracker(const string &trackerType_="BOOSTING", const double trackerTmo_=0.0);
    void prepare();
    void latchBBox(const cv::Rect &bbox);
    void track(const Image &img);
    bool is_tracking(cv::Rect &bbox) const;
};


/**********************************************************/
class Manager : public RFModule
{
protected:
    RpcServer           rpcPort;
    RpcServer           rpcHuman;
    RpcClient           rpcClassifier;
    RpcClient           rpcMotor;
    RpcClient           rpcMotorGrasp;
    RpcClient           rpcReachCalib;
    RpcClient           rpcGet3D;
    RpcClient           rpcMotorStop;
    RpcClient           rpcMemory;
    StopCmdPort         rxMotorStop;
    PointedLocationPort pointedLoc;
    MemoryReporter      memoryReporter;

    BufferedPort<Bottle>             blobExtractor;
    BufferedPort<Bottle>             histObjLocPort;
    BufferedPort<Property>           recogTriggerPort;
    BufferedPort<ImageOf<PixelBgr> > imgIn;
    BufferedPort<ImageOf<PixelBgr> > imgOut;
    BufferedPort<ImageOf<PixelBgr> > imgRtLocOut;
    BufferedPort<ImageOf<PixelBgr> > imgTrackOut;
    BufferedPort<ImageOf<PixelBgr> > imgHistogram;
    Port imgClassifier;

    Speaker speaker;
    Attention attention;    
    RtLocalization rtLocalization;
    Exploration exploration;
    MemoryUpdater memoryUpdater;
    ClassifiersDataBase db;
    map<string,int> memoryIds;

    ImageOf<PixelBgr> img;
    ImageOf<PixelBgr> imgRtLoc;
    Semaphore mutexResources;
    Semaphore mutexResourcesMemory;
    Semaphore mutexAttention;
    Semaphore mutexMemoryUpdate;
    
    string name;
    string camera;
    string objectToBeKinCalibrated;
    bool busy;
    bool scheduleLoadMemory;
    bool enableInterrupt;
    bool actionInterrupted;
    bool skipGazeHoming;
    bool doAttention;
    bool trainOnFlipped;
    bool trainBurst;
    bool skipLearningUponSuccess;
    double blobs_detection_timeout;
    double improve_train_period;
    double classification_threshold;
    double blockEyes;

    string tracker_type;
    double tracker_timeout;
    VectorOf<int> tracker_min_blob_size;
    map<string,Tracker> trackersPool;

    map<string,Filter*> histFiltersPool;
    int histFilterLength;
    deque<cv::Scalar> histColorsCode;

    bool    trackStopGood;
    bool    whatGood;    
    cv::Point trackStopLocation;
    cv::Point whatLocation;

    double lastBlobsArrivalTime;
    Bottle lastBlobs;
    Bottle memoryBlobs;
    Bottle memoryScores;
    
    Vector skim_blobs_x_bounds;
    Vector skim_blobs_y_bounds;
    Vector histObjLocation;

    friend class Attention;
    friend class RtLocalization;
    friend class Exploration;
    friend class MemoryUpdater;
    friend class MemoryReporter;

    int       processHumanCmd(const Bottle &cmd, Bottle &b);
    Bottle    skimBlobs(const Bottle &blobs);
    bool      thresBBox(cv::Rect &bbox, const Image &img);
    Bottle    getBlobs();
    cv::Point getBlobCOG(const Bottle &blobs, const int i);
    bool      get3DPosition(const cv::Point &point, Vector &x);
    void      acquireImage(const bool rtlocalization=false);
    void      drawBlobs(const Bottle &blobs, const int i, Bottle *scores=NULL);
    void      rotate(cv::Mat &src, const double angle, cv::Mat &dst);
    void      drawScoresHistogram(const Bottle &blobs, const Bottle &scores, const int i);
    void      loadMemory();
    void      updateClassifierInMemory(Classifier *pClassifier);
    void      updateObjCartPosInMemory(const string &object, const Bottle &blobs, const int i);
    void      triggerRecogInfo(const string &object, const Bottle &blobs, const int i, const string &recogType);
    int       findClosestBlob(const Bottle &blobs, const cv::Point &loc);
    int       findClosestBlob(const Bottle &blobs, const Vector &loc);
    Bottle    classify(const Bottle &blobs, const bool rtlocalization=false);
    void      burst(const string &tag="");
    void      train(const string &object, const Bottle &blobs, const int i);
    void      improve_train(const string &object, const Bottle &blobs, const int i);
    void      home(const string &part="all");
    void      calibTable();
    bool      calibKinStart(const string &object, const string &hand, const int recogBlob);
    void      calibKinStop();
    void      motorHelper(const string &cmd, const string &object);
    void      motorHelper(const string &cmd, const Bottle &blobs, const int i, const Bottle &options=Bottle());
    bool      getCalibratedLocation(const string &object, string &hand, Vector &x);
    bool      interruptableAction(const string &action, deque<string> *param, const string &object, const Bottle &blobs=Bottle(), const int iBlob=RET_INVALID);
    void      point(const string &object);
    void      point(const Bottle &blobs, const int i);
    void      look(const string &object);
    void      look(const Bottle &blobs, const int i, const Bottle &options=Bottle());
    int       recognize(const string &object, Bottle &blobs, Classifier **ppClassifier=NULL);
    int       recognize(Bottle &blobs, Bottle &scores, string &object);
    void      execName(const string &object);
    void      execForget(const string &object);
    void      execWhere(const string &object, const Bottle &blobs, const int recogBlob, Classifier *pClassifier, const string &recogType);
    void      execWhat(const Bottle &blobs, const int pointedBlob, const Bottle &scores, const string &object);
    void      execThis(const string &object, const string &detectedObject, const Bottle &blobs, const int &pointedBlob);
    void      execExplore(const string &object);
    void      execReinforce(const string &object, const Vector &position);
    void      execInterruptableAction(const string &action, const string &object, const Bottle &blobs, const int recogBlob);
    void      switchAttention();
    void      doLocalization();
    bool      get3DPositionFromMemory(const string &object, Vector &position);
    bool      doExploration(const string &object, const Vector &position);
    void      updateMemory();    

public:
    void      interruptMotor();
    void      reinstateMotor(const bool saySorry=true);
    bool      configure(ResourceFinder &rf);
    bool      interruptModule();
    bool      close();
    bool      updateModule();
    bool      respond(const Bottle &command, Bottle &reply);
    double    getPeriod();    
};

#endif

