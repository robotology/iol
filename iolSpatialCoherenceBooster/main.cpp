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
#include <yarp/os/all.h>
#include <yarp/sig/all.h>
#include <yarp/math/Math.h>

using namespace std;
using namespace yarp::os;
using namespace yarp::sig;
using namespace yarp::math;


/**********************************************************/
struct Object
{
    Vector position;
    string name;
    string tag;

    /**********************************************************/
    Object()
    {
        position.resize(3,0.0);
        name=tag="";
    }
};


/**********************************************************/
class Booster : public RFModule
{
    RpcClient rpcMemory;
    deque<Object> prevObjects;
    deque<Object> currObjects;
    double radius;

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
                            rpcMemory.write(cmd,replyProp);

                            // update internal databases
                            if (replyProp.get(0).asVocab()==Vocab::encode("ack"))
                            {
                                if (Bottle *propField=replyProp.get(1).asList())
                                {
                                    if (propField->check("name") && propField->check("position_3d"))
                                    {
                                        Object object;
                                        object.name=propField->find("name").asString().c_str();
                                        propField->find("position_3d").asList()->write(object.position);

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

        deque<Object> oldObjs=prevObjects;
        for (size_t i=0; i<currObjects.size(); i++)
        {
            Object &obj=currObjects[i];
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
                oldObjs.erase(oldObjs.begin()+min_j);
            else
                return false;
        }

        return true;
    }

public:
    /**********************************************************/
    bool configure(ResourceFinder &rf)
    {
        string name=rf.check("name",Value("iolSpatialCoherenceBooster")).asString().c_str();
        radius=rf.check("radius",Value(0.02)).asDouble();

        rpcMemory.open(("/"+name+"/memory:rpc").c_str());
        return true;
    }
    
    /**********************************************************/
    bool interruptModule()
    {
        rpcMemory.interrupt();
        return true;
    }

    /**********************************************************/
    bool close()
    {
        rpcMemory.close();
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

        getObjects();
        if (staticConditions())
        {
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



