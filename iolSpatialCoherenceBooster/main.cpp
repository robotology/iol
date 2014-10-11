/* 
 * Copyright (C) 2014 iCub Facility - Istituto Italiano di Tecnologia
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

/** 
\defgroup icub_iolSpatialCoherenceBooster Spatial Coherence Booster 
 
The module responsible for improving the internal knowledge of 
objects based on spatial coherence check. 
 
\section intro_sec Description 
When the object locations are detected as stable (i.e. no 
movements are present in the scene), a spatial coherence check 
is performed with the aim to improve the object recognition. 
 
\section lib_sec Libraries 
- YARP libraries. 
 
\section portsc_sec Ports Created 
- \e /<modName>/manager:rpc used to communicate with \ref 
  icub_iolStateMachineHandler.
 
- \e /<modName>/human:rpc used to send reinforce commands to 
  \ref icub_iolStateMachineHandler.
 
- \e /<modName>/memory:rpc used to communicate with the objects 
  properties collector.
 
- \e /<modName>/label:i receives asynchronous object labels from
  \ref icub_iolStateMachineHandler.
 
\section parameters_sec Parameters 
--name \e name
- specify the module stem-name, which is
  \e iolSpatialCoherenceBooster by default. The stem-name is
  used as prefix for all open ports.
 
--period \e T 
- specify the period of the thread in seconds (default = 0.25 
  s).
 
--radius \e R 
- specify the radius (in meters) used to check the spatial 
  consistency of the objects (default = 0.02 m).
 
--mismatches \e N 
- specify the number of mismatches to be detected for an object 
  to start off retraining (default = 5).

\section tested_os_sec Tested OS
Windows, Linux

\author Ugo Pattacini
*/ 

#include <iostream>
#include <iomanip>
#include <string>
#include <deque>
#include <algorithm>

#include <yarp/os/all.h>
#include <yarp/sig/all.h>
#include <yarp/math/Math.h>

using namespace std;
using namespace yarp::os;
using namespace yarp::sig;
using namespace yarp::math;


/**********************************************************/
struct iolObject
{
    Vector position;
    string name;
    string label;
    int triggerLearnCnt;

    /**********************************************************/
    iolObject()
    {
        position.resize(3,0.0);
        name=label="";
        triggerLearnCnt=0;
    }

    /**********************************************************/
    void propagate(const iolObject &obj)
    {
        label=obj.label;
        position=obj.position;
        triggerLearnCnt=obj.triggerLearnCnt;
    }
};


/**********************************************************/
class Booster : public RFModule, public PortReader
{
    RpcClient rpcMemory;
    RpcClient rpcManager;
    RpcClient rpcHuman;
    Port labelPort;
    
    deque<iolObject> prevObjects;
    deque<iolObject> currObjects;
    Mutex mutex;

    double period;
    double radius;
    int mismatches;

    /**********************************************************/
    bool read(ConnectionReader &connection)
    {
        LockGuard lg(mutex);

        // format: ("label" "object-name") ("position_3d" (<x> <y> <z>)) ("type" "creation"|"recognition")
        Bottle msg;
        if (!msg.read(connection))
            return false;

        cout<<"received msg: "<<msg.toString().c_str()<<endl;
        if (prevObjects.size()==0)
        {
            cout<<"empty database"<<endl;
            return true;
        }

        if (msg.check("type",Value("creation")).asString()=="creation")
        {
            cout<<"object creation => skipped"<<endl;
            return true;
        }

        Vector position(3);
        string label=msg.find("label").asString().c_str();
        if (Bottle *bPos=msg.find("position_3d").asList())
        {
            if (bPos->size()<(int)position.length())
                return true;
            else for (size_t i=0; i<position.length(); i++)
                position[i]=bPos->get(i).asDouble();
        }

        double dist_min=1e9; int min_i=0;
        for (size_t i=0; i<prevObjects.size(); i++)
        {
            double dist=norm(position-prevObjects[i].position);
            cout<<prevObjects[i].name<<" in ("
                <<prevObjects[i].position.toString(3,3).c_str()
                <<") => dist="<<dist<<endl;

            if (dist<dist_min)
            {
                dist_min=dist;
                min_i=i;
            }
        }

        cout<<"min_dist="<<dist_min;
        if (dist_min<radius)
        {
            prevObjects[min_i].label=label;
            cout<<" < radius="<<radius;
            cout<<" => label \""<<label<<"\" injected";
        }
        else
            cout<<" >= radius="<<radius;
        cout<<endl;

        return true;
    }

    /**********************************************************/
    void getObjects()
    {
        currObjects.clear();

        // ask for all the objects stored in memory
        Bottle cmd,reply,replyProp;
        cmd.addVocab(Vocab::encode("ask"));
        Bottle &content=cmd.addList().addList();
        content.addString("entity");
        content.addString("==");
        content.addString("object");
        rpcMemory.write(cmd,reply);

        if (reply.size()>1)
        {
            if (reply.get(0).asVocab()==Vocab::encode("ack"))
            {
                if (Bottle *idField=reply.get(1).asList())
                {
                    if (Bottle *idValues=idField->get(1).asList())
                    {
                        // cycle over objects
                        for (int i=0; i<idValues->size(); i++)
                        {
                            int id=idValues->get(i).asInt();

                            // get the relevant properties
                            // [get] (("id" <num>) ("propSet" ("name" "position_3d")))
                            cmd.clear(); replyProp.clear();
                            cmd.addVocab(Vocab::encode("get"));
                            Bottle &content=cmd.addList();
                            Bottle &list_bid=content.addList();
                            list_bid.addString("id");
                            list_bid.addInt(id);
                            Bottle &list_propSet=content.addList();
                            list_propSet.addString("propSet");
                            Bottle &list_items=list_propSet.addList();
                            list_items.addString("name");
                            list_items.addString("position_3d");
                            rpcMemory.write(cmd,replyProp);

                            // update internal databases
                            if (replyProp.get(0).asVocab()==Vocab::encode("ack"))
                            {
                                if (Bottle *propField=replyProp.get(1).asList())
                                {
                                    if (propField->check("name") && propField->check("position_3d"))
                                    {
                                        iolObject object;
                                        object.name=propField->find("name").asString().c_str();
                                        if (Bottle *bPos=propField->find("position_3d").asList())
                                            for (size_t i=0; i<object.position.length(); i++)
                                                object.position[i]=bPos->get(i).asDouble();

                                        currObjects.push_back(object);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    /**********************************************************/
    bool staticConditions()
    {
        if ((currObjects.size()!=prevObjects.size()) || currObjects.size()==0)
            return false;

        deque<iolObject> oldObjs=prevObjects;
        for (size_t i=0; i<currObjects.size(); i++)
        {
            iolObject &obj=currObjects[i];
            double dist_min=1e9;
            int min_j=0;

            for (size_t j=0; j<oldObjs.size(); j++)
            {
                double dist=norm(obj.position-oldObjs[j].position);
                if (dist<dist_min)
                {
                    dist_min=dist;
                    min_j=j;
                }
            }

            if (dist_min<radius)
            {
                // propagate info from the past
                obj.propagate(oldObjs[min_j]);
                oldObjs.erase(oldObjs.begin()+min_j);
            }
            else
                return false;
        }

        return true;
    }

    /**********************************************************/
    iolObject* findObjects()
    {
        int triggerLearnIdx=0;
        int triggerLearnCnt=0;
        for (size_t i=0; i<currObjects.size(); i++)
        {
            iolObject &obj=currObjects[i];
            if (!obj.label.empty() && (obj.label!=obj.name))
            {
                if (++obj.triggerLearnCnt>mismatches)
                {
                    if (obj.triggerLearnCnt>triggerLearnCnt)
                    {
                        triggerLearnIdx=i;
                        triggerLearnCnt=obj.triggerLearnCnt;
                    }
                }                
            }
        }

        return (triggerLearnCnt>0?&currObjects[triggerLearnIdx]:NULL);
    }

public:
    /**********************************************************/
    bool configure(ResourceFinder &rf)
    {
        string name=rf.check("name",Value("iolSpatialCoherenceBooster")).asString().c_str();
        period=rf.check("period",Value(0.25)).asDouble();
        radius=rf.check("radius",Value(0.02)).asDouble();
        mismatches=rf.check("mismatches",Value(5)).asInt();

        rpcMemory.open(("/"+name+"/memory:rpc").c_str());
        rpcManager.open(("/"+name+"/manager:rpc").c_str());
        rpcHuman.open(("/"+name+"/human:rpc").c_str());
        labelPort.open(("/"+name+"/label:i").c_str());
        labelPort.setReader(*this);

        return true;
    }
    
    /**********************************************************/
    bool interruptModule()
    {
        rpcMemory.interrupt();
        rpcManager.interrupt();
        rpcHuman.interrupt();
        labelPort.interrupt();
        return true;
    }

    /**********************************************************/
    bool close()
    {
        rpcMemory.close();
        rpcManager.close();
        rpcHuman.close();
        labelPort.close();
        return true;
    }

    /**********************************************************/
    double getPeriod()
    {
        return period;
    }

    /**********************************************************/
    bool updateModule()
    {
        if ((rpcMemory.getOutputCount()==0)  ||
            (rpcManager.getOutputCount()==0) ||
            (rpcHuman.getOutputCount()==0))
            return true;

        Bottle cmd,reply;
        cmd.addString("status");
        rpcManager.write(cmd,reply);
        if ((reply.get(0).asString()=="ack") && (reply.get(1).asString()=="busy"))
            return true;

        LockGuard lg(mutex);

        getObjects();
        if (staticConditions())
        {
            if (iolObject *obj=findObjects())
            {
                cmd.clear();
                cmd.addString("reinforce");
                cmd.addString(obj->label.c_str());
                cmd.addList().read(obj->position);
                rpcHuman.write(cmd,reply);
                obj->triggerLearnCnt=0;
                cout<<"trigger reinforcement: "<<cmd.toString().c_str()<<endl;
            }
        }

        prevObjects=currObjects;
        return true;
    }
};


/**********************************************************/
int main(int argc, char *argv[])
{
    Network yarp;
    if (!yarp.checkNetwork())
    {
        cout<<"YARP network is unavailable!"<<endl;
        return -1;
    }

    ResourceFinder rf;
    rf.configure(argc,argv);

    Booster booster;
    return booster.runModule(rf);
}



