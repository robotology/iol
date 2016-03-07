/* 
 * Copyright (C) 2016 iCub Facility - Istituto Italiano di Tecnologia
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

#include <cmath>
#include <algorithm>
#include <string>
#include <sstream>
#include <deque>
#include <map>
#include <fstream>
#include <iomanip>

#include <yarp/os/all.h>
#include <yarp/dev/all.h>
#include <yarp/sig/Vector.h>
#include <yarp/sig/Matrix.h>
#include <yarp/math/Math.h>

#include <iCub/optimization/calibReference.h>

#include "src/iolReachingCalibration_IDL.h"

#define ACK     VOCAB3('a','c','k')

using namespace std;
using namespace yarp::os;
using namespace yarp::dev;
using namespace yarp::sig;
using namespace yarp::math;
using namespace iCub::optimization;


/********************************************************/
class Calibrator : public RFModule,
                   public iolReachingCalibration_IDL
{
    struct TableEntry
    {        
        Matrix H;
        bool calibrated;
        CalibReferenceWithMatchedPoints calibrator;
        TableEntry() : H(eye(4,4)), calibrated(false) { }
    };
    map<string,TableEntry> table;

    Vector reachAboveOrientationLeft;
    Vector reachAboveOrientationRight;
    double zOffset;
    int objLocIter;

    ResourceFinder *rf;
    bool testModeOn;
    bool calibOngoing;
    string calibHand;
    string calibEntry;
    Vector calibLoc;

    PolyDriver drvCartLeft;
    PolyDriver drvCartRight;
    
    RpcClient arePort;
    RpcClient opcPort;
    RpcServer rpcPort;

    /********************************************************/
    bool attach(RpcServer &source)
    {
        return this->yarp().attachAsServer(source);
    }

    /********************************************************/
    bool getHandOrientation(ResourceFinder &rf, const string &hand,
                            Vector &o)
    {
        o.resize(4,0.0);
        Bottle &bGroup=rf.findGroup(hand);
        if (!bGroup.isNull())
        {
            if (Bottle *b=bGroup.find("reach_above_orientation").asList())
            {
                size_t len=std::min(o.length(),(size_t)b->size());
                for (size_t i=0; i<len; i++)
                    o[i]=b->get(i).asDouble();
                return true;
            }
        }

        yError()<<"Unable to retrieve \"reach_above_orientation\" for "<<hand;
        return false;
    }

    /*****************************************************/
    bool getObjectLocation(const string &object, Vector &x)
    {
        bool ret=false;
        int ack=Vocab::encode("ack");
        if (opcPort.getOutputCount()>0)
        {
            Bottle cmd,rep;
            cmd.addVocab(Vocab::encode("ask"));
            Bottle &content=cmd.addList();
            Bottle &cond_1=content.addList();
            cond_1.addString("entity");
            cond_1.addString("==");
            cond_1.addString("object");
            content.addString("&&");
            Bottle &cond_2=content.addList();
            cond_2.addString("name");
            cond_2.addString("==");
            cond_2.addString(object);

            opcPort.write(cmd,rep);
            if (rep.size()>1)
            {
                if (rep.get(0).asVocab()==ack)
                {
                    if (Bottle *idField=rep.get(1).asList())
                    {
                        if (Bottle *idValues=idField->get(1).asList())
                        {
                            Bottle cmd;
                            int id=idValues->get(0).asInt();                            
                            cmd.addVocab(Vocab::encode("get"));
                            Bottle &content=cmd.addList();
                            Bottle &list_bid=content.addList();
                            list_bid.addString("id");
                            list_bid.addInt(id);
                            Bottle &list_propSet=content.addList();
                            list_propSet.addString("propSet");
                            list_propSet.addList().addString("position_3d");

                            x.resize(3,0.0); int i;
                            for (i=0; i<objLocIter; i++)
                            {
                                Bottle rep;
                                opcPort.write(cmd,rep);
                                if (rep.get(0).asVocab()==ack)
                                {
                                    if (Bottle *propField=rep.get(1).asList())
                                    {
                                        if (Bottle *b=propField->find("position_3d").asList())
                                        {                                           
                                            for (int i=0; i<b->size(); i++)
                                                x[i]+=b->get(i).asDouble();
                                        }
                                        else
                                            break;
                                    }
                                    else
                                        break;
                                }
                                else
                                    break;

                                Time::delay(0.1);
                            }

                            if (i>=objLocIter)
                            {
                                x/=objLocIter;
                                ret=true;
                            }
                        }
                    }
                }
            }
        }

        if (!ret)
            yError()<<"Unable to retrieve object location";
        return ret; 
    }

    /********************************************************/
    void moveHandUp(const string &hand)
    {
        ICartesianControl *icart;
        Vector reachAboveOrientation;
        if (hand=="left")
        {
            drvCartLeft.view(icart);
            reachAboveOrientation=reachAboveOrientationLeft;
        }
        else
        {
            drvCartRight.view(icart);
            reachAboveOrientation=reachAboveOrientationRight;
        }

        int ctxt;
        icart->storeContext(&ctxt);

        Vector dof(10,1.0);
        dof[0]=dof[1]=dof[2]=0.0;
        icart->setDOF(dof,dof);

        Vector x,o;
        icart->getPose(x,o);
        x[2]+=zOffset;
        icart->goToPose(x,reachAboveOrientation);
        icart->waitMotionDone();

        icart->restoreContext(ctxt);
    }

    /********************************************************/
    Vector getHandLocation(const string &hand)
    {
        ICartesianControl *icart;
        if (hand=="left")
            drvCartLeft.view(icart);
        else
            drvCartRight.view(icart);

        Vector x,o;
        icart->getPose(x,o);
        return x;
    }

    /********************************************************/
    string composeEntry(const string &hand, const string &object)
    {
        return (hand+"-"+object);
    }

    /********************************************************/
    bool calibration_start(const string &hand, const string &object,
                           const string &entry)
    {
        bool reply=false;
        if (arePort.getOutputCount()>0)
        {
            Bottle areCmd,areRep;
            areCmd.addString("look");
            areCmd.addString(object);
            areCmd.addString("wait");
            arePort.write(areCmd,areRep);
            if (areRep.get(0).asVocab()==ACK)
            {
                Vector x;
                if (getObjectLocation(object,x))
                {
                    Bottle areCmd,areRep;
                    areCmd.addString("touch");
                    areCmd.addList().read(x);
                    areCmd.addString(hand);
                    areCmd.addString("still");
                    arePort.write(areCmd,areRep);
                    if (areRep.get(0).asVocab()==ACK)
                    {
                        moveHandUp(hand);

                        Bottle areCmd,areRep;
                        areCmd.addString("hand");
                        areCmd.addString("pretake_hand");
                        areCmd.addString(hand);
                        arePort.write(areCmd,areRep);
                        if (areRep.get(0).asVocab()==ACK)
                        {
                            Bottle areCmd,areRep;
                            areCmd.addString("calib");
                            areCmd.addString("kinematics");
                            areCmd.addString("start");
                            areCmd.addString(hand);
                            arePort.write(areCmd,areRep);
                            if (areRep.get(0).asVocab()==ACK)
                            {
                                calibLoc=x;
                                calibHand=hand;
                                calibEntry=(entry.empty()?composeEntry(hand,object):entry);
                                calibOngoing=true;
                                reply=true;
                            }
                        }
                    }
                }
            }
        }

        return reply;
    }

    /********************************************************/
    bool calibration_stop()
    {
        bool reply=false;
        if (calibOngoing)
        {
            Vector x=getHandLocation(calibHand);
            table[calibEntry].calibrator.addPoints(calibLoc,x);
            table[calibEntry].calibrated=false;

            yInfo()<<"pushing ("<<calibLoc.toString(5,5)<<") ("
                   <<x.toString(5,5)<<")";

            if (arePort.getOutputCount()>0)
            {
                Bottle areCmd,areRep;
                areCmd.addString("calib");
                areCmd.addString("kinematics");
                areCmd.addString("stop");
                areCmd.addString("skip");
                arePort.write(areCmd,areRep);
                if (areRep.get(0).asVocab()==ACK)
                {
                    Bottle areCmd,areRep;
                    areCmd.addString("home");
                    areCmd.addString("all");
                    arePort.write(areCmd,areRep);
                    reply=(areRep.get(0).asVocab()==ACK);
                }
            }

            calibOngoing=false;
        }

        return reply;
    }

    /********************************************************/
    bool calibration_clear(const string &hand, const string &object,
                           const string &entry)
    {
        string entry_name=(entry.empty()?composeEntry(hand,object):entry);
        map<string,TableEntry>::iterator it=table.find(entry_name);
        if (it!=table.end())
        {
            table.erase(it);
            return true;
        }
        else
            return false;
    }

    /********************************************************/
    vector<string> calibration_list()
    {
        vector<string> reply;
        for (map<string,TableEntry>::iterator it=table.begin();
              it!=table.end(); it++)
            reply.push_back(it->first);
        return reply;
    }

    /********************************************************/
    CalibReq get_location(const string &hand, const string &object,
                          const string &entry)
    {
        CalibReq reply("fail",0.0,0.0,0.0);
        string entry_name=(entry.empty()?composeEntry(hand,object):entry);
        map<string,TableEntry>::iterator it=table.find(entry_name);
        if ((it!=table.end()) && (arePort.getOutputCount()>0))
        {
            Bottle areCmd,areRep;
            areCmd.addString("look");
            areCmd.addString(object);
            areCmd.addString("wait");
            arePort.write(areCmd,areRep);
            if (areRep.get(0).asVocab()==ACK)
            {
                Vector x;
                if (getObjectLocation(object,x))
                {
                    reply=CalibReq("fail",x[0],x[1],x[2]);

                    TableEntry &entry=it->second;
                    if (!entry.calibrated)
                    {
                        if (entry.calibrator.getNumPoints()>2)
                        {
                            double err; 
                            entry.calibrator.calibrate(entry.H,err);
                            yInfo()<<"H=\n"<<entry.H.toString(5,5);
                            yInfo()<<"calibration error="<<err;
                            entry.calibrated=true;
                        }
                        else
                            yError()<<"Unable to calibrate: too few points";
                    }

                    if (entry.calibrated)
                    {
                        Vector res=x; res.push_back(1.0); 
                        res=entry.H*res;
                        reply=CalibReq("ok",res[0],res[1],res[2]);
                    }
                }
            }
        }

        return reply;
    }

    /********************************************************/
    bool test_set(const string &entry, const double xi,
                  const double yi, const double zi,
                  const double xo, const double yo,
                  const double zo)
    {
        Vector in(3),out(3);
        in[0]=xi; in[1]=yi; in[2]=xi;
        out[0]=xo; out[1]=yo; out[2]=xo;
        table[entry].calibrator.addPoints(in,out);
        table[entry].calibrated=false;
        return true;
    }

    /********************************************************/
    CalibReq test_get(const string &entry, const double x,
                      const double y, const double z)
    {
        CalibReq reply("fail",x,y,z);
        map<string,TableEntry>::iterator it=table.find(entry);
        if (it!=table.end())
        {
            TableEntry &entry=it->second;
            if (!entry.calibrated)
            {
                if (entry.calibrator.getNumPoints()>2)
                {
                    double err;
                    entry.calibrator.calibrate(entry.H,err);
                    yInfo()<<"H=\n"<<entry.H.toString(5,5);
                    yInfo()<<"calibration error="<<err;
                    entry.calibrated=true;
                }
                else
                    yError()<<"Unable to calibrate: too few points";
            }

            if (entry.calibrated)
            {
                Vector res(4,1.0);
                res[0]=x; res[1]=y; res[2]=z;
                res=entry.H*res;
                reply=CalibReq("ok",res[0],res[1],res[2]);
            }
        }

        return reply;
    }

    /********************************************************/
    bool save()
    {
        string fileName=rf->getHomeContextPath()+"/"+
                        rf->find("calibration-file").asString(); 
        yInfo()<<"Saving calibration in "<<fileName;
        ofstream fout(fileName.c_str());
        if (fout.is_open())
        {
            fout<<"entries (";
            vector<string> entries=calibration_list();
            for (size_t i=0; i<entries.size(); i++)
                fout<<entries[i]<<" ";
            fout<<")"<<endl<<endl;

            for (map<string,TableEntry>::iterator it=table.begin();
                  it!=table.end(); it++)
            {
                fout<<"["<<it->first<<"]"<<endl;
                TableEntry &entry=it->second;
                fout<<"calibrated "<<(entry.calibrated?"true":"false")<<endl;
                fout<<"H ("<<entry.H.toString(5,5," ")<<")"<<endl;
                fout<<"numPoints "<<entry.calibrator.getNumPoints()<<endl;

                deque<Vector> in,out;
                entry.calibrator.getPoints(in,out);
                for (size_t j=0; j<entry.calibrator.getNumPoints(); j++)
                    fout<<"points_"<<j<<" ("
                        <<in[j].toString(5,5)<<") ("
                        <<out[j].toString(5,5)<<")"
                        <<endl;
                fout<<endl;
            }

            fout.close();
            return true;
        }
        else
            return false;
    }

    /********************************************************/
    bool load()
    {
        table.clear();

        ResourceFinder rf;
        rf.setVerbose();
        rf.setDefaultContext(this->rf->getContext().c_str());
        rf.setDefaultConfigFile(this->rf->find("calibration-file").asString().c_str());
        rf.configure(0,NULL);

        if (Bottle *entries=rf.find("entries").asList())
        {
            for (int i=0; i<entries->size(); i++)
            {
                string entry_name=entries->get(i).asString();
                Bottle &bGroup=rf.findGroup(entry_name);
                if (!bGroup.isNull())
                {
                    table[entry_name].calibrated=(bGroup.check("calibrated",Value("false")).asString()=="true");
                    if (Bottle *bH=bGroup.find("H").asList())
                    {
                        if (bH->size()==16)
                        {
                            for (int r=0; r<4; r++)
                                for (int c=0; c<4; c++)
                                    table[entry_name].H(r,c)=bH->get(r+4*c).asDouble();
                        }
                    }

                    int numPoints=bGroup.check("numPoints",Value(0)).asInt();
                    for (int j=0; j<numPoints; j++)
                    {
                        ostringstream tag; tag<<"points_"<<j;
                        Bottle &bPoints=bGroup.findGroup(tag.str());
                        if (bPoints.size()>=3)
                        {
                            Bottle *bIn=bPoints.get(1).asList();
                            Bottle *bOut=bPoints.get(2).asList();
                            if ((bIn!=NULL) && (bOut!=NULL))
                            {
                                Vector in(3,0.0);
                                size_t len_in=std::min(in.length(),(size_t)bIn->size());
                                for (size_t k=0; k<len_in; k++)
                                    in[k]=bIn->get(k).asDouble();

                                Vector out(3,0.0);
                                size_t len_out=std::min(out.length(),(size_t)bOut->size());
                                for (size_t k=0; k<len_out; k++)
                                    out[k]=bOut->get(k).asDouble();

                                table[entry_name].calibrator.addPoints(in,out);
                            }
                        }
                    }
                }
            }
        }

        print_table();
        return true;
    }

    /********************************************************/
    void print_table()
    {
        yInfo()<<"Table Content";
        for (map<string,TableEntry>::iterator it=table.begin();
              it!=table.end(); it++)
        {
            TableEntry &entry=it->second;
            yInfo()<<"["<<it->first<<"]";
            yInfo()<<"calibrated "<<(entry.calibrated?"true":"false");
            yInfo()<<"H=\n"<<entry.H.toString(5,5);
            yInfo()<<"points:";
            deque<Vector> in,out;
            entry.calibrator.getPoints(in,out);
            for (size_t i=0; i<entry.calibrator.getNumPoints(); i++)
                yInfo()<<"("<<in[i].toString(5,5)<<") ("
                       <<out[i].toString(5,5)<<")";
        }
    }

public:
    /********************************************************/
    bool configure(ResourceFinder &rf)
    {
        this->rf=&rf;
        string robot=rf.check("robot",Value("icub")).asString();        
        testModeOn=(rf.check("test-mode",Value("off")).asString()=="on");
        zOffset=rf.check("z-offset",Value(0.0)).asDouble();
        objLocIter=rf.check("object-location-iterations",Value(10)).asInt();

        ResourceFinder areRF;
        areRF.setVerbose();
        areRF.setDefaultContext(rf.find("are-context").asString().c_str());
        areRF.setDefaultConfigFile(rf.find("are-config-file").asString().c_str());
        areRF.configure(0,NULL);

        if (!getHandOrientation(areRF,"left_arm",reachAboveOrientationLeft))
            return false;

        if (!getHandOrientation(areRF,"right_arm",reachAboveOrientationRight))
            return false;

        if (!testModeOn)
        {
            Property option("(device cartesiancontrollerclient)"); 
            option.put("remote",("/"+robot+"/cartesianController/left_arm"));
            option.put("local",("/"+getName("/cartesian/left_arm")).c_str());
            if (!drvCartLeft.open(option))
            {
                yError()<<"Unable to open cartesiancontrollerclient device for left_arm";
                return false;
            }

            option.unput("remote"); option.unput("local");
            option.put("remote",("/"+robot+"/cartesianController/right_arm"));
            option.put("local",("/"+getName("/cartesian/right_arm")).c_str());
            if (!drvCartRight.open(option))
            {
                yError()<<"Unable to open cartesiancontrollerclient device for right_arm";
                drvCartLeft.close();
                return false;
            }
        }

        arePort.open(("/"+getName("/are")).c_str());
        opcPort.open(("/"+getName("/opc")).c_str());
        rpcPort.open(("/"+getName("/rpc")).c_str());
        attach(rpcPort);

        calibOngoing=false;
        load();

        return true;
    }

    /********************************************************/
    bool close()
    {
        rpcPort.close();
        opcPort.close();
        arePort.close();

        if (drvCartLeft.isValid())
            drvCartLeft.close(); 
        if (drvCartRight.isValid())
            drvCartRight.close();

        save();
        return true;
    }

    /********************************************************/
    double getPeriod()
    {
        return 1.0;
    }

    /********************************************************/
    bool updateModule()
    {
        return true;
    }
};


/********************************************************/
int main(int argc, char *argv[])
{
    Network yarp;
    if (!yarp.checkNetwork())
    {
        yError()<<"YARP server not available!";
        return 1;
    }

    Calibrator calib;
    calib.setName("iolReachingCalibration");

    ResourceFinder rf;
    rf.setDefaultContext(calib.getName().c_str());
    rf.setDefault("calibration-file","calibration.ini");
    rf.setDefault("are-context","actionsRenderingEngine");
    rf.setDefault("are-config-file","config.ini");
    rf.configure(argc,argv);

    return calib.runModule(rf);
}


