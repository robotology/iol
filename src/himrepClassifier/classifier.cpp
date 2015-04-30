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

#include "classifier.h"

#define CMD_TRAIN       VOCAB4('t','r','a','i')
#define CMD_CLASSIFY    VOCAB4('c','l','a','s')
#define CMD_FORGET      VOCAB4('f','o','r','g')
#define CMD_BURST       VOCAB4('b','u','r','s')
#define CMD_LIST        VOCAB4('l','i','s','t')


bool Classifier::configure(yarp::os::ResourceFinder &rf)
{
    string moduleName = rf.check("name",Value("himrepClassifier"),"module name (string)").asString().c_str();
    setName(moduleName.c_str());

    rpcClassifier.open(("/"+moduleName+"/classify:rpc").c_str());
    imgInput.open(("/"+moduleName+"/img:i").c_str());
    imgSIFTInput.open(("/"+moduleName+"/SIFTimg:i").c_str());
    imgOutput.open(("/"+moduleName+"/img:o").c_str());
    scoresInput.open(("/"+moduleName+"/scores:i").c_str());
    rpcPort.open(("/"+moduleName+"/rpc").c_str());

    featureInput.open(("/"+moduleName+"/features:i").c_str());
    featureOutput.open(("/"+moduleName+"/features:o").c_str());

    imgSIFTOutput.open(("/"+moduleName+"/SIFTimg:o").c_str());
    opcPort.open(("/"+moduleName+"/opc").c_str());

    attach(rpcPort);

    sync=true;
    doTrain=true;
    burst=false;

    return true;
}


bool Classifier::interruptModule()
{
    imgInput.interrupt();
    imgOutput.interrupt();
    rpcClassifier.interrupt();
    scoresInput.interrupt();
    rpcPort.interrupt();
    imgSIFTInput.interrupt();
    imgSIFTOutput.interrupt();
    opcPort.interrupt();
    return true;
}


bool Classifier::close()
{
    imgInput.close();
    imgOutput.close();
    rpcClassifier.close();
    scoresInput.close();
    rpcPort.close();
    imgSIFTInput.close();
    imgSIFTOutput.close();
    opcPort.close();
    return true;
}


bool Classifier::getOPCList(Bottle &names)
{
    names.clear();
    if (opcPort.getOutputCount()>0)
    {
        Bottle opcCmd,opcReply,opcReplyProp;
        opcCmd.addVocab(Vocab::encode("ask"));
        Bottle &content=opcCmd.addList().addList();
        content.addString("entity");
        content.addString("==");
        content.addString("object");
        opcPort.write(opcCmd,opcReply);

        if (opcReply.size()>1)
        {
            if (opcReply.get(0).asVocab()==Vocab::encode("ack"))
            {
                if (Bottle *idField=opcReply.get(1).asList())
                {
                    if (Bottle *idValues=idField->get(1).asList())
                    {
                        // cycle over items
                        for (int i=0; i<idValues->size(); i++)
                        {
                            int id=idValues->get(i).asInt();

                            // get the relevant properties
                            // [get] (("id" <num>) ("propSet" ("name")))
                            opcCmd.clear();
                            opcCmd.addVocab(Vocab::encode("get"));
                            Bottle &content=opcCmd.addList();
                            Bottle &list_bid=content.addList();
                            list_bid.addString("id");
                            list_bid.addInt(id);
                            Bottle &list_propSet=content.addList();
                            list_propSet.addString("propSet");
                            list_propSet.addList().addString("name");
                            opcPort.write(opcCmd,opcReplyProp);

                            // append the name (if any)
                            if (opcReplyProp.get(0).asVocab()==Vocab::encode("ack"))
                                if (Bottle *propField=opcReplyProp.get(1).asList())
                                    if (propField->check("name"))
                                        names.addString(propField->find("name").asString().c_str());
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


bool Classifier::updateObjDatabase()
{
    LockGuard lg(mutex);
    if ((opcPort.getOutputCount()==0) || (rpcClassifier.getOutputCount()==0))
        return false;

    Bottle opcObjList;
    // Retrieve OPC Object List
    bool success=getOPCList(opcObjList);
    if (!success)
    {
        printf("Error in communicating with OPC \n");
        return false;
    }

    // Retrieve LinearClassifier Object List
    Bottle cmdObjClass;
    cmdObjClass.addString("objList");
    Bottle objList;
    rpcClassifier.write(cmdObjClass,objList);
    for (int k=0; k<objList.size(); k++)
    {
        string currObj=objList.get(k).asString().c_str();
        if ((currObj.compare("ack")==0) || (currObj.compare("background")==0))
            continue;

        bool found=false;
        // check if the object is stored in the opc memory
        for (int i=0; i<opcObjList.size(); i++)
        {
            string opcObj=opcObjList.get(i).asString().c_str();
            if (currObj.compare(opcObj)==0)
            {
                found=true;
                break;
            }
        }

        // if the object is not stored in memory delete it from the LinearClassifier DB
        if (!found)
        {
            printf("****** Deleting %s ..... \n",currObj.c_str());
            cmdObjClass.clear();
            cmdObjClass.addString("forget");
            cmdObjClass.addString(currObj.c_str());
            Bottle repClass;
            rpcClassifier.write(cmdObjClass,repClass);
            printf("****** Deleted %s ..... \n",currObj.c_str());
        }

    }

    Bottle cmdTr,trReply;
    cmdTr.addString("train");
    rpcClassifier.write(cmdTr,trReply);
    printf("****** Retrained ..... \n");

    return true;
}


bool Classifier::train(Bottle *locations, Bottle &reply)
{
   if (locations==NULL)
       return false;

    string object_name=locations->get(0).asList()->get(0).asString().c_str();
    if (burst)
        currObject=object_name.c_str();

    // save features
    if (doTrain)
    {
        Bottle cmdClass;
        cmdClass.addString("save");
        cmdClass.addString(object_name.c_str());

        Bottle classReply;
        printf("Sending training request: %s\n",cmdClass.toString().c_str());
        rpcClassifier.write(cmdClass,classReply);
        printf("Received reply: %s\n",classReply.toString().c_str());
    }

    // read image and locations
    ImageOf<PixelRgb> *image=imgInput.read();
    if (image==NULL)
        return false;

    IplImage *img=(IplImage*)image->getIplImage();

    Bottle* bb=locations->get(0).asList()->get(1).asList();
    int x_min=bb->get(0).asInt();
    int y_min=bb->get(1).asInt();
    int x_max=bb->get(2).asInt();
    int y_max=bb->get(3).asInt();

    if (x_min>5)
        x_min-=5;

    if (y_min>5)
        y_min-=5;

    if ((x_max+5)<img->width)
        x_max+=5;

    if ((y_max+5)<img->height)
        y_max+=5;

    int blobW=x_max-x_min;
    int blobH=y_max-y_min;

    // crop image
    ImageOf<PixelRgb> croppedImg;
    croppedImg.resize(blobW,blobH);
    cvSetImageROI(img,cvRect(x_min,y_min,blobW,blobH));
    cvCopy(img,(IplImage*)croppedImg.getIplImage());
    cvResetImageROI(img);

    // send image to SC
    imgOutput.write(croppedImg);

    // read coded features
    Bottle fea;
    featureInput.read(fea);
    if (fea.size()==0)
        return false;

    // send feature to classifier
    if (burst)
        trainingFeature.push_back(fea);
    else
        featureOutput.write(fea);

    // train classifier
    if (doTrain)
    {
        Bottle cmdTr;
        cmdTr.addString("train");
        Bottle trReply;
        printf("Sending training request: %s\n",cmdTr.toString().c_str());
        rpcClassifier.write(cmdTr,trReply);
        printf("Received reply: %s\n",trReply.toString().c_str());
    }

    reply.addString("ack");
    return true;
}


void Classifier::classify(Bottle *blobs, Bottle &reply)
{
    if (blobs==NULL)
    {
        reply.addList();
        return;
    }

    if (blobs->size()==0)
    {
        reply.addList();
        return;
    }

    if ((imgInput.getInputCount()==0)     || (imgSIFTInput.getInputCount()==0) ||
        (featureInput.getInputCount()==0) || (scoresInput.getInputCount()==0)  ||
        (rpcClassifier.getOutputCount()==0))
    {
        reply.addList();
        return;
    }

    // read object classes
    Bottle cmdObjClass,objList;
    cmdObjClass.addString("objList");
    rpcClassifier.write(cmdObjClass,objList);

    if (objList.size()<=1)
    {
        for (int b=0; b<blobs->size(); b++)
        {
            Bottle &blob_scorelist=reply.addList();
            blob_scorelist.addString(blobs->get(b).asList()->get(0).asString().c_str());
            blob_scorelist.addList();
        }

        return;
    }

    // start recognition mode
    Bottle cmdClass,classReply;
    cmdClass.addString("recognize");
    rpcClassifier.write(cmdClass,classReply);

    // read image
    ImageOf<PixelRgb> *image=imgInput.read();
    if (image==NULL)
        return;

    IplImage *imgC=(IplImage*)image->getIplImage();

    // classify each blob
    printf("Start classification\n");
    for (int b=0; b<blobs->size(); b++)
    {
        // list of the scores
        Bottle &blob_scorelist=reply.addList();
        // name of the blob
        blob_scorelist.addString(blobs->get(b).asList()->get(0).asString().c_str());
        // list of scores
        Bottle &scores=blob_scorelist.addList();
        // retrieve bounding box
        Bottle *bb=blobs->get(b).asList()->get(1).asList();
        int x_min=(int)bb->get(0).asDouble();
        int y_min=(int)bb->get(1).asDouble();
        int x_max=(int)bb->get(2).asDouble();
        int y_max=(int)bb->get(3).asDouble();

        if (x_min>5)
           x_min-=5;

        if (y_min>5)
           y_min-=5;

        if ((x_max+5)<imgC->width)
           x_max+=5;

        if ((y_max+5)<imgC->height)
           y_max+=5;

        // crop image
        ImageOf<PixelRgb> croppedImg;
        croppedImg.resize(x_max-x_min,y_max-y_min);
        cvSetImageROI(imgC,cvRect(x_min,y_min,x_max-x_min,y_max-y_min));
        cvCopy(imgC,(IplImage*)croppedImg.getIplImage());
        cvResetImageROI(imgC);

        // send image to SC
        imgOutput.write(croppedImg);

        // read coded features
        Bottle fea;
        featureInput.read(fea);
        ImageOf<PixelRgb> *imgSift=imgSIFTInput.read();
        if ((fea.size()==0) || (imgSift==NULL))
            return;

        x_max=std::min(x_min+imgSift->width(),image->width()-1);
        y_max=std::min(y_min+imgSift->height(),image->height()-1);
        IplImage *iplSift=(IplImage*)imgSift->getIplImage();

        cvSetImageROI(iplSift,cvRect(0,0,x_max-x_min,y_max-y_min));
        cvSetImageROI(imgC,cvRect(x_min,y_min,x_max-x_min,y_max-y_min));
        cvCopy(iplSift,imgC);
        cvResetImageROI(imgC);

        // send feature to classifier
        featureOutput.write(fea);

        // read scores
        Bottle class_scores;
        scoresInput.read(class_scores);
        if (class_scores.size()==0)
            return;

        // fill the list of the i-th blob
        printf("Scores received: ");
        for (int i=0; i<objList.size()-1; i++)
        {
            Bottle *obj=class_scores.get(i).asList();
            if (obj->get(0).asString()=="background")
               continue;

            Bottle &currObj_score=scores.addList();
            currObj_score.addString(obj->get(0).asString().c_str());
            double normalizedVal=((obj->get(1).asDouble())+1.0)/2.0;
            currObj_score.addDouble(normalizedVal);
            printf("(%s %g) ",obj->get(0).asString().c_str(),normalizedVal);
        }

        printf("\n");
    }

    if (imgSIFTOutput.getOutputCount()>0)
    {
        imgSIFTOutput.prepare()=*image;
        imgSIFTOutput.write();
    }
}


bool Classifier::respond(const Bottle& command, Bottle& reply)
{
    LockGuard lg(mutex);
    switch(command.get(0).asVocab())
    {
        case CMD_TRAIN:
        {
            if (!burst)
                doTrain=true;
            train(command.get(1).asList(),reply);
            return true;
        }

        case CMD_CLASSIFY:
        {
            classify(command.get(1).asList(),reply);
            return true;
        }

        case CMD_FORGET:
        {
            string className=command.get(1).asString().c_str();
            Bottle cmdObjClass;
            cmdObjClass.addString("forget");
            cmdObjClass.addString(className.c_str());
            Bottle repClass;
            printf("Sending training request: %s\n",cmdObjClass.toString().c_str());
            rpcClassifier.write(cmdObjClass,repClass);
            printf("Received reply: %s\n",repClass.toString().c_str());
            reply.addString("ack");
            return true;
        }

        case CMD_BURST:
        {
            string cmd=command.get(1).asString().c_str();
            if (cmd=="star")
            {
                trainingFeature.clear();
                burst=true;
                doTrain=false;
            }
            else
            {
                burst=false;
                doTrain=false;
                Bottle cmdClass;
                cmdClass.addString("save");
                cmdClass.addString(currObject.c_str());
                Bottle classReply;
                rpcClassifier.write(cmdClass,classReply);

                for (size_t i=0; i<trainingFeature.size(); i++)
                    featureOutput.write(trainingFeature[i]);

                trainingFeature.clear();

                Bottle cmdTr;
                cmdTr.addString("train");
                Bottle trReply;
                rpcClassifier.write(cmdTr,trReply);
            }

            reply.addString("ack");
            return true;
        }

        case CMD_LIST:
        {
            Bottle cmdList,replyList;
            cmdList.addString("objList");
            printf("Sending list request: %s\n",cmdList.toString().c_str());
            rpcClassifier.write(cmdList,replyList);
            printf("Received reply: %s\n",replyList.toString().c_str());
            reply.addString("ack");
            reply.addList().append(replyList);
            return true;
        }
    }

    reply.addString("nack");
    return true;
}


double Classifier::getPeriod()
{
    return 1.0;
}


bool Classifier::updateModule()
{
    if (sync)
    {
        printf("Trying to start Synchronization with OPC... \n");
        if (updateObjDatabase())
        {
            printf("Synchronization with OPC Completed... \n");
            sync=false;
        }
    }

    return true;
}
