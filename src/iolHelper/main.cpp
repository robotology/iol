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

/** 
\defgroup icub_iolHelper Helper funtions to assist IOL demo
@ingroup icub_interactiveObjectsLearning
 
A container collecting useful functions that help IOL
accomplish its tasks. 

\section proto_sec Protocol 
Available requests queried through the rpc port: 
-# <b>NAME</b>. The format is: [name] "name_sm_0" "name_sm_1" 
   ... This request allows merging the list provided by the
   State Machine within the request (i.e. "name_sm_0"
   "name_sm_1" ...) with further objects names stored within the
   objectsPropertiesCollector database.
   \n The format of the reply is: [nack]/[ack]
   "name_sm_0" "name_sm_1" ... "name_opc_0" "name_opc_1" ...
-# <b>SET NAV LOCATION</b>. The format is: [navs] 
   "location_name" x y theta. This request allows creating a
   location within the database. \n The format of the reply is:
   [nack]/[ack].
-# <b>GET NAV LOCATION</b>. The format is: [navg] 
   "location_name". This request allows retrieving a location
   from the database. \n The format of the reply is:
   [nack]/[ack] x y theta.
-# <b>CLASS</b>. The format is [class] ( (blob_0 (tlx tly brx 
   bry)) (blob_1 (tlx tly brx bry)) ... ). It serves the
   classification request to be forwarded to the external
   classifier.
 
\section lib_sec Libraries 
- YARP libraries

\section parameters_sec Parameters
--name <string> 
- To specify the module's name; all the open ports will be 
  tagged with the prefix /<moduleName>/. If not specified
  \e iolHelper is assumed.
 
--context_extclass <string> 
- To specify the context where to search for memory file for the
  external classification scenario.
 
--memory_extclass <string> 
- To specify the memory file name provided in the
  objectsPropertiesCollector database format for the external
  classification scenario.
 
\section portsc_sec Ports Created
- \e /<moduleName>/rpc to be accessed by SM. 
 
- \e /<moduleName>/opc to be connected to the 
  objectsPropertiesCollector port.
 
- \e /<moduleName>/extclass:o to be connected to the external 
  classifier to forward classification requests.
 
- \e /<moduleName>/extclass:i to be connected to the external 
  classifier to receive classification responses.
 
\section tested_os_sec Tested OS
Linux and Windows.

\author Ugo Pattacini
*/ 

#include <string>
#include <deque>
#include <mutex>
#include <condition_variable>

#include <yarp/os/all.h>

using namespace std;
using namespace yarp::os;


/************************************************************************/
class FakeClassifierService: public PortReader
{
    /************************************************************************/
    bool read(ConnectionReader& connection)
    {
        Bottle cmd,reply;
        cmd.read(connection);
        reply.addVocab32("ack");

        if (cmd.size()>=2)
        {
            if (cmd.get(0).asVocab32()==Vocab32::encode("classify"))
            {
                Bottle *payLoad=cmd.get(1).asList();
                int maxArea=0;
                                
                for (int i=0; i<payLoad->size(); i++)
                {
                    Bottle *item=payLoad->get(i).asList();
                    string tag=item->get(0).asString();
                    Bottle *blob=item->get(1).asList();
                    int tl_x=(int)blob->get(0).asFloat64();
                    int tl_y=(int)blob->get(1).asFloat64();
                    int br_x=(int)blob->get(2).asFloat64();
                    int br_y=(int)blob->get(3).asFloat64();

                    int area=(br_x-tl_x)*(br_y-tl_y);
                    if (area>=maxArea)
                    {
                        reply.clear();
                        Bottle &l1=reply.addList();
                        l1.addString(tag);
                        Bottle &l2=l1.addList().addList();
                        l2.addString("toy");
                        l2.addFloat64(1.0);
                        maxArea=area;
                    }
                }
            }
        }
        
        if (ConnectionWriter *client=connection.getWriter())
            reply.write(*client);

        return true;
    }
};


/************************************************************************/
class iolHelperModule: public RFModule
{    
    RpcClient            opcPort;
    Port                 rpcPort;
    Port                 fakePort;
    Port                 extClassOutPort;
    BufferedPort<Bottle> extClassInPort;

    FakeClassifierService fakeService;

    Bottle blobTags;
    Bottle reply;
    mutex mtx_replyEvent;
    condition_variable cv_replyEvent;
    bool   interrupting;

    deque<pair<int,string> > objects;

public:
    /************************************************************************/
    bool configure(ResourceFinder &rf)
    {
        string name=rf.find("name").asString();
        opcPort.open("/"+name+"/opc");

        rpcPort.open("/"+name+"/rpc");
        attach(rpcPort);

        fakePort.open("/"+name+"/fake");
        fakePort.setReader(fakeService);

        extClassOutPort.open("/"+name+"/extclass:o");
        extClassInPort.open("/"+name+"/extclass:i");

        string context_extclass=rf.find("context_extclass").asString();
        string memory_extclass=rf.find("memory_extclass").asString();

        ResourceFinder memory_rf;
        memory_rf.setDefaultContext(context_extclass);
        memory_rf.setDefaultConfigFile(memory_extclass.c_str());
        memory_rf.configure(0,NULL);

        string dataFile=memory_rf.findFile("from");
        Property dataProp; dataProp.fromConfigFile(dataFile);
        Bottle dataBottle; dataBottle.read(dataProp);
        for (int i=0; i<dataBottle.size(); i++)
        {
            if (Bottle *payLoad=dataBottle.get(i).asList()->get(2).asList())
            {
                if (payLoad->check("extclass_id") && payLoad->check("name"))
                {
                    int id=payLoad->find("extclass_id").asInt32();
                    string name=payLoad->find("name").asString();
                    objects.push_back(pair<int,string>(id,name));
                }
            }
        }

        yInfo("Available objects:");
        for (size_t i=0; i<objects.size(); i++)
            yInfo("#%d: %s",objects[i].first,objects[i].second.c_str());

        interrupting=false;
        return true;
    }

    /************************************************************************/
    bool interruptModule()
    {
        yInfo("interrupting...");
        interrupting=true;
        cv_replyEvent.notify_all();
        return true;
    }

    /************************************************************************/
    bool close()
    {
        opcPort.close();
        rpcPort.close();
        fakePort.close();
        extClassOutPort.close();
        extClassInPort.close();
        return true;
    }

    /************************************************************************/
    bool getNames(Bottle &names)
    {
        names.clear();
        if (opcPort.getOutputCount()>0)
        {
            Bottle opcCmd,opcReply,opcReplyProp;
            opcCmd.addVocab32("ask");
            Bottle &content=opcCmd.addList().addList();
            content.addString("entity");
            content.addString("==");
            content.addString("object");
            opcPort.write(opcCmd,opcReply);
            
            if (opcReply.size()>1)
            {
                if (opcReply.get(0).asVocab32()==Vocab32::encode("ack"))
                {
                    if (Bottle *idField=opcReply.get(1).asList())
                    {
                        if (Bottle *idValues=idField->get(1).asList())
                        {
                            // cycle over items
                            for (int i=0; i<idValues->size(); i++)
                            {
                                int id=idValues->get(i).asInt32();

                                // get the relevant properties
                                // [get] (("id" <num>) ("propSet" ("name")))
                                opcCmd.clear();
                                opcCmd.addVocab32("get");
                                Bottle &content=opcCmd.addList();
                                Bottle &list_bid=content.addList();
                                list_bid.addString("id");
                                list_bid.addInt32(id);
                                Bottle &list_propSet=content.addList();
                                list_propSet.addString("propSet");
                                list_propSet.addList().addString("name");
                                opcPort.write(opcCmd,opcReplyProp);

                                // append the name (if any)
                                if (opcReplyProp.get(0).asVocab32()==Vocab32::encode("ack"))
                                    if (Bottle *propField=opcReplyProp.get(1).asList())
                                        if (propField->check("name"))
                                            names.addString(propField->find("name").asString());
                            }
                        }
                    }
                }
            }

            return true;
        }
        else
            return false;
    }

    /************************************************************************/
    int getLocation(const Bottle &body, Bottle &location)
    {
        int id=-1;
        location.clear();        
        if ((opcPort.getOutputCount()>0) && (body.size()>0))
        {
            Bottle opcCmd,opcReply,opcReplyProp;
            opcCmd.addVocab32("ask");
            Bottle &content=opcCmd.addList();
            Bottle &cond1=content.addList();
            cond1.addString("entity");
            cond1.addString("==");
            cond1.addString("navloc");
            content.addString("&&");
            Bottle &cond2=content.addList();
            cond2.addString("name");
            cond2.addString("==");
            cond2.addString(body.get(0).asString());
            opcPort.write(opcCmd,opcReply);

            if (opcReply.size()>1)
            {
                if (opcReply.get(0).asVocab32()==Vocab32::encode("ack"))
                {
                    if (Bottle *idField=opcReply.get(1).asList())
                    {
                        if (Bottle *idValues=idField->get(1).asList())
                        {
                            if (idValues->size()>0)
                            {                            
                                // consider just the first element
                                id=idValues->get(0).asInt32();

                                // get the relevant properties
                                // [get] (("id" <num>) ("propSet" ("location")))
                                opcCmd.clear();
                                opcCmd.addVocab32("get");
                                Bottle &content=opcCmd.addList();
                                Bottle &list_bid=content.addList();
                                list_bid.addString("id");
                                list_bid.addInt32(id);
                                Bottle &list_propSet=content.addList();
                                list_propSet.addString("propSet");
                                list_propSet.addList().addString("location");
                                opcPort.write(opcCmd,opcReplyProp);

                                // append the name (if any)
                                if (opcReplyProp.get(0).asVocab32()==Vocab32::encode("ack"))
                                    if (Bottle *propField=opcReplyProp.get(1).asList())
                                        if (propField->check("location"))
                                            if (Bottle *loc=propField->find("location").asList())
                                                for (int i=0; i<loc->size(); i++)
                                                    location.addFloat64(loc->get(i).asFloat64());
                            }
                        }
                    }
                }
            }
        }

        return id;
    }

    /************************************************************************/
    bool setLocation(const int id, const Bottle &body)
    {
        if ((opcPort.getOutputCount()>0) && (body.size()>3))
        {
            Bottle opcCmd,opcReply;
            Bottle *pContent=NULL;
            if (id<0)
            {
                opcCmd.addVocab32("add");
                Bottle &content=opcCmd.addList();
                Bottle &list_ent=content.addList();
                list_ent.addString("entity");
                list_ent.addString("navloc");
                Bottle &list_name=content.addList();
                list_name.addString("name");
                list_name.addString(body.get(0).asString());
                pContent=&content;
            }
            else
            {
                opcCmd.addVocab32("set");
                Bottle &content=opcCmd.addList();
                Bottle &list_id=content.addList();
                list_id.addString("id");
                list_id.addInt32(id);
                pContent=&content;
            }

            Bottle &list_loc=pContent->addList();
            list_loc.addString("location");
            Bottle &list_data=list_loc.addList();
            list_data.addFloat64(body.get(1).asFloat64());
            list_data.addFloat64(body.get(2).asFloat64());
            list_data.addFloat64(body.get(3).asFloat64());
            opcPort.write(opcCmd,opcReply);

            return (opcReply.get(0).asVocab32()==Vocab32::encode("ack"));
        }
        else
            return false;
    }

    /************************************************************************/
    Bottle mergeList(const Bottle &b1, const Bottle &b2)
    {
        Bottle ret=b1;
        for (int i=0; i<b2.size(); i++)
        {
            string name=b2.get(i).asString();
            bool toBeAppended=true;

            for (int j=0; j<ret.size(); j++)
            {
                if (name==ret.get(j).asString())
                {
                    toBeAppended=false;
                    break;
                }
            }

            if (toBeAppended)
                ret.addString(name);
        }

        return ret;
    }

    /************************************************************************/
    bool respond(const Bottle &cmd, Bottle &reply)
    {
        yInfo("Received request: %s",cmd.toString().c_str());
        switch (cmd.get(0).asVocab32())
        {
            //-----------------
            case createVocab32('n','a','m','e'):
            {
                Bottle names;
                if (getNames(names))
                {
                    reply.addString("ack");
                    reply.append(mergeList(cmd.tail(),names));
                }
                else
                    reply.addString("nack");

                return true;
            }

            //-----------------
            case createVocab32('n','a','v','g'):
            {
                Bottle location;
                if (getLocation(cmd.tail(),location)>=0)
                {
                    reply.addString("ack");
                    reply.append(location);
                }
                else
                    reply.addString("nack");

                return true;
            }

            //-----------------
            case createVocab32('n','a','v','s'):
            {
                Bottle location;
                Bottle body=cmd.tail();
                int id=getLocation(body,location);
                if (setLocation(id,body))
                    reply.addString("ack");
                else
                    reply.addString("nack");

                return true;
            }

            //-----------------
            case createVocab32('c','l','a','s'):
            {
                if (extClassOutPort.getOutputCount()==0)
                {
                    yWarning("external classifier is not connected => request skipped!");
                    reply.addString("failed");
                    return true;
                }

                blobTags.clear(); 
                Bottle msg;
                msg.addString("classify");

                Bottle *payLoad=cmd.get(1).asList();                
                for (int i=0; i<payLoad->size(); i++)
                {
                    Bottle *item=payLoad->get(i).asList();
                    string tag=item->get(0).asString();
                    Bottle *blob=item->get(1).asList();
                    int tl_x=(int)blob->get(0).asFloat64();
                    int tl_y=(int)blob->get(1).asFloat64();
                    int br_x=(int)blob->get(2).asFloat64();
                    int br_y=(int)blob->get(3).asFloat64();

                    blobTags.addString(tag);
                    msg.addInt32(tl_x);
                    msg.addInt32(tl_y);
                    msg.addInt32(br_x);
                    msg.addInt32(br_y);
                }

                if (blobTags.size()>0)
                {
                    yInfo("Forwarding request: %s",msg.toString().c_str());
                    yInfo("waiting reply...");
                    extClassOutPort.write(msg);
                    unique_lock<mutex> lck(mtx_replyEvent);
                    cv_replyEvent.wait(lck);
                    if (!interrupting)
                    {
                        yInfo("...sending reply");
                        reply=this->reply;
                    }
                    else
                    {
                        yWarning("reply skipped!");
                        reply.addString("failed");
                    }
                }
                else
                {
                    yWarning("empty request!");
                    reply.addString("failed");
                }

                return true;
            }

            //-----------------
            default:
                return RFModule::respond(cmd,reply);
        }

        reply.addString("nack");
        return false;
    }

    /************************************************************************/
    bool updateModule()
    {
        if (Bottle *msg=extClassInPort.read(false))
        {
            yInfo("Received reply: %s",msg->toString().c_str());

            reply.clear();
            for (int i=0; i<msg->size(); i++)
            {
                Bottle &blob=reply.addList();
                blob.addString(blobTags.get(i).asString());
                Bottle &items=blob.addList();
                for (size_t j=0; j<objects.size(); j++)
                {
                    Bottle &item=items.addList();
                    item.addString(objects[j].second);
                    item.addFloat64(objects[j].first==msg->get(i).asInt32()?1.0:0.0);
                }
            }

            yInfo("Reply to be transmitted: %s",reply.toString().c_str());
            cv_replyEvent.notify_all();
        }

        return true;
    }

    /************************************************************************/
    double getPeriod()
    {
        return 0.02;
    }
};


/************************************************************************/
int main(int argc, char *argv[])
{
    ResourceFinder rf;
    rf.setDefault("name","iolHelper");
    rf.setDefault("context_extclass","iolStateMachineHandler");
    rf.setDefault("memory_extclass","memory_extclass.ini");
    rf.configure(argc,argv);

    Network yarp;
    if (!yarp.checkNetwork())
        return 1;

    iolHelperModule module;
    return module.runModule(rf);
}


