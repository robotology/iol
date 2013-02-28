#include "SCSPMClassifier.h"
#include <yarp/os/Stamp.h>

#define CMD_TRAIN               VOCAB4('t','r','a','i')
#define CMD_CLASSIFY            VOCAB4('c','l','a','s')
#define CMD_FORGET              VOCAB4('f','o','r','g')

bool SCSPMClassifier::configure(yarp::os::ResourceFinder &rf)
{    


    string moduleName = rf.check("name", Value("SCSPMClassifier"), "module name (string)").asString().c_str();
    setName(moduleName.c_str());

    rpcClassifier.open(("/"+moduleName+"/classify:rpc").c_str());
    imgInput.open(("/"+moduleName+"/img:i").c_str());
    imgOutput.open(("/"+moduleName+"/img:o").c_str());
    scoresInput.open(("/"+moduleName+"/scores:i").c_str());
    handlerPort.open(("/"+moduleName+"/rpc").c_str());

    featureInput.open(("/"+moduleName+"/features:i").c_str());
    featureOutput.open(("/"+moduleName+"/features:o").c_str());

    attach(handlerPort);
    mutex=new Semaphore(1);

    return true ;

}


bool SCSPMClassifier::interruptModule()
{

    imgInput.interrupt();
    imgOutput.interrupt();
    rpcClassifier.interrupt();
    scoresInput.interrupt();
    handlerPort.interrupt();

    return true;
}


bool SCSPMClassifier::close()
{
    imgInput.close();
    imgOutput.close();
    rpcClassifier.close();
    scoresInput.close();
    handlerPort.close();

    delete mutex;
    return true;
}

bool SCSPMClassifier::train(Bottle *locations, Bottle &reply)
{
   if(locations==NULL)
       return false;

    string object_name=locations->get(0).asList()->get(0).asString().c_str();


    // Save Features
    Bottle cmdClass;
    cmdClass.addString("save");
    cmdClass.addString(object_name.c_str());

    Bottle classReply;
    printf("Sending training request: %s\n",cmdClass.toString().c_str());
    rpcClassifier.write(cmdClass,classReply);
    printf("Received reply: %s\n",classReply.toString().c_str());


    // Read Image and Locations
    ImageOf<PixelRgb> *image = imgInput.read(true);
    if(image==NULL)
        return false;

    img= (IplImage*) image->getIplImage();

    Bottle* bb=locations->get(0).asList()->get(1).asList();
    int x_min=bb->get(0).asInt();
    int y_min=bb->get(1).asInt();
    int x_max=bb->get(2).asInt();
    int y_max=bb->get(3).asInt();


    //Crop Image
    cvSetImageROI(img,cvRect(x_min,y_min,x_max,y_max));
    IplImage* croppedImg=cvCreateImage(cvGetSize(img), IPL_DEPTH_8U, img->nChannels);
    cvCopy(img, croppedImg);
    cvResetImageROI(img);

    //Send Image to SC
    ImageOf<PixelBgr>& outim=imgOutput.prepare();
    outim.wrapIplImage(croppedImg);
    imgOutput.write();
    cvReleaseImage(&croppedImg);
    cvResetImageROI(img);

    //Read Coded Feature
    Bottle fea;
    featureInput.read(fea);

    if(fea.size()==0)
        return false;

    //Send Feature to Classifier
     featureOutput.write(fea);


    //Delay 5msec
     Time::delay(0.01);


    //Train Classifier
    Bottle cmdTr;
    cmdTr.addString("train");
    Bottle trReply;
    printf("Sending training request: %s\n",cmdTr.toString().c_str());
    rpcClassifier.write(cmdTr,trReply);
    printf("Received reply: %s\n",trReply.toString().c_str());

    reply.addString("ack");
    return true;
}

void SCSPMClassifier::classify(Bottle *blobs, Bottle &reply)
{

    if(blobs==NULL)
    {
        reply.addList();
        return;
    }

    if(blobs->size()==0)
    {
        reply.addList();
        return;
    }

    //Read Object Classes
    Bottle cmdObjClass;
    cmdObjClass.addString("objList");
    Bottle objList;
    //printf("Sending training request: %s\n",cmdObjClass.toString().c_str());
    rpcClassifier.write(cmdObjClass,objList);
    //printf("Received reply: %s\n",objList.toString().c_str());

    if(objList.size()<=1)
    {
        for(int b=0; b<blobs->size(); b++)
        {
             Bottle &blob_scorelist=reply.addList();
             // name of the blob
             blob_scorelist.addString(blobs->get(b).asList()->get(0).asString().c_str());
             blob_scorelist.addList();
        }
        return;
    }


    // Start Recognition mode
    Bottle cmdClass;
    cmdClass.addString("recognize");

    Bottle classReply;
    //printf("Sending training request: %s\n",cmdClass.toString().c_str());
    rpcClassifier.write(cmdClass,classReply);
    //printf("Received reply: %s\n",classReply.toString().c_str());

    // Read Image
    ImageOf<PixelRgb> *image = imgInput.read(true);
    if(image==NULL)
        return;
    img= (IplImage*) image->getIplImage();


    double t2=Time::now();

    // Classify each blob
    for(int b=0; b<blobs->size(); b++)
    {
        // list of the scores
        Bottle &blob_scorelist=reply.addList();
        // name of the blob
        blob_scorelist.addString(blobs->get(b).asList()->get(0).asString().c_str());

        //list of scores
        Bottle &scores=blob_scorelist.addList();

        //retrieve bounding box
        Bottle* bb=blobs->get(b).asList()->get(1).asList();
        int x_min=(int) bb->get(0).asDouble();
        int y_min=(int) bb->get(1).asDouble();
        int x_max=(int) bb->get(2).asDouble();
        int y_max=(int) bb->get(3).asDouble();

        //Crop Image
        cvSetImageROI(img,cvRect(x_min,y_min,x_max-x_min,y_max-y_min));
        IplImage* croppedImg=cvCreateImage(cvGetSize(img), IPL_DEPTH_8U, img->nChannels);

        cvCopy(img, croppedImg);
        cvResetImageROI(img);



        double t=Time::now();
        //Send Image to SC
        ImageOf<PixelBgr>& outim=imgOutput.prepare();
        outim.wrapIplImage(croppedImg);
        imgOutput.write();
        
        cvReleaseImage(&croppedImg);
        cvResetImageROI(img);

        //Read Coded Feature
        Bottle fea;
        featureInput.read(fea);
        t=Time::now()-t;
        fprintf(stdout, "Coding Time: %g \n", t);

        if(fea.size()==0)
            return;

        //Send Feature to Classifier
         featureOutput.write(fea);

         // Read scores
         Bottle Class_scores;
         scoresInput.read(Class_scores);

        printf("Scores received: %s\n",Class_scores.toString().c_str());


         if(Class_scores.size()==0)
             return;

         // Fill the list of the b-th blob
         Bottle &classifier_score=scores.addList();

         for (int i=0; i<objList.size()-1; i++)
         {
             Bottle *obj=Class_scores.get(i).asList();
             classifier_score.addString(obj->get(0).asString());
             classifier_score.addDouble(obj->get(1).asDouble());
         }

    }
    t2=Time::now()-t2;
    fprintf(stdout, "All Time: %g \n", t2);
}
bool SCSPMClassifier::respond(const Bottle& command, Bottle& reply) 
{
    switch(command.get(0).asVocab())
    {

           case(CMD_TRAIN):
           {
                mutex->wait();
                train(command.get(1).asList(),reply);
   
            
                mutex->post();
                return true;
            }

           case(CMD_CLASSIFY):
           {
                mutex->wait();
                classify(command.get(1).asList(),reply);
                //printf("Sending reply: %s\n",reply.toString().c_str());
                mutex->post();
                return true;
            }
           case(CMD_FORGET):
           {
                mutex->wait();
                string className=command.get(1).asString().c_str();
                Bottle cmdObjClass;
                cmdObjClass.addString("forget");
                cmdObjClass.addString(className.c_str());
                Bottle repClass;
                printf("Sending training request: %s\n",cmdObjClass.toString().c_str());
                rpcClassifier.write(cmdObjClass,repClass);
                printf("Received reply: %s\n",repClass.toString().c_str());
                reply.addString("ack");
                mutex->post();
                return true;
            }

    }

    reply.addString("nack");
    return true;
}


bool SCSPMClassifier::updateModule()
{
    return true;
}



double SCSPMClassifier::getPeriod()
{
    return 0.1;
}
