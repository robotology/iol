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
 
\section tested_os_sec Tested OS
Windows, Linux

\author Ugo Pattacini
*/ 

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
    VectorOf<int> blob;
    string name;
    string label;
    int triggerLearnCnt;

    /**********************************************************/
    iolObject()
    {
        position.resize(3,0.0);
        blob.resize(4,0);
        name=label="";
        triggerLearnCnt=0;
    }
};


/**********************************************************/
class Booster : public RFModule, public PortReader
{
    RpcClient rpcMemory;
    Port labelPort;
    
    deque<iolObject> prevObjects;
    deque<iolObject> currObjects;
    Mutex mutex;

    double radius;
    string camera;

    /**********************************************************/
    bool read(ConnectionReader &connection)
    {
        LockGuard lg(mutex);

        Bottle msg; // format: ("label" "string") ("position_3d" (<x> <y> <z>))
        if (!msg.read(connection))
            return false;        

        Vector position;
        string label=msg.find("label").asString().c_str();
        msg.find("position_3d").asList()->write(position);

        double dist_min=1e9; int min_i=0;
        for (size_t i=0; i<prevObjects.size(); i++)
        {
            double dist=norm(position-prevObjects[i].position);
            if (dist<dist_min)
            {
                dist_min=dist;
                min_i=i;
            }
        }

        if (dist_min<radius)
            prevObjects[min_i].label=label; // inject the label

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
                            string blobTag("position_2d_"+camera);

                            // get the relevant properties
                            // [get] (("id" <num>) ("propSet" ("name" "position_3d" "position_2d_${camera}")))
                            cmd.clear();
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
                            list_items.addString(blobTag.c_str());
                            rpcMemory.write(cmd,replyProp);

                            // update internal databases
                            if (replyProp.get(0).asVocab()==Vocab::encode("ack"))
                            {
                                if (Bottle *propField=replyProp.get(1).asList())
                                {
                                    if (propField->check("name") &&
                                        propField->check("position_3d") &&
                                        propField->check(blobTag.c_str()))
                                    {
                                        iolObject object;
                                        object.name=propField->find("name").asString().c_str();
                                        propField->find("position_3d").asList()->write(object.position);
                                        propField->find(blobTag.c_str()).asList()->write(object.blob);

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
                // propagate info from the past (label, position_3d)
                obj.label=oldObjs[min_j].label;
                obj.position=oldObjs[min_j].position;
                oldObjs.erase(oldObjs.begin()+min_j);
            }
            else
                return false;
        }

        return true;
    }

    /**********************************************************/
    iolObject* findObjectsForLearning()
    {
        int triggerLearnIdx=0;
        int triggerLearnCnt=0;
        for (size_t i=0; i<currObjects.size(); i++)
        {
            iolObject &obj=currObjects[i];
            if (!obj.label.empty() && (obj.label!=obj.name))
            {
                if (++obj.triggerLearnCnt>10)
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
        radius=rf.check("radius",Value(0.02)).asDouble();
        camera=rf.check("camera",Value("left")).asString().c_str();
        if ((camera!="left") && (camera!="right"))
            camera="left";

        rpcMemory.open(("/"+name+"/memory:rpc").c_str());
        labelPort.open(("/"+name+"/label:i").c_str());
        labelPort.setReader(*this);

        return true;
    }
    
    /**********************************************************/
    bool interruptModule()
    {
        rpcMemory.interrupt();
        labelPort.interrupt();
        return true;
    }

    /**********************************************************/
    bool close()
    {
        rpcMemory.close();
        labelPort.close();
        return true;
    }

    /**********************************************************/
    double getPeriod()
    {
        return 0.2;
    }

    /**********************************************************/
    bool updateModule()
    {
        if (rpcMemory.getOutputCount()==0)
            return true;

        LockGuard lg(mutex);

        getObjects();
        if (staticConditions())
        {
            iolObject *obj=findObjectsForLearning();
            obj->triggerLearnCnt=0;
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
        return -1;

    ResourceFinder rf;
    rf.configure(argc,argv);

    Booster booster;
    return booster.runModule(rf);
}



