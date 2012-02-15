#include <yarp/os/Network.h>
#include <yarp/os/BufferedPort.h>
#include <yarp/os/RFModule.h>
#include <yarp/os/RateThread.h>
#include <yarp/os/Semaphore.h>
#include <yarp/sig/Image.h>

#include <gsl/gsl_math.h>

#include <cv.h>
#include <highgui.h>

#include <string>
#include <iostream>
#include <fstream>

using namespace std;
using namespace yarp::os;
using namespace yarp::sig;
using namespace cv;

class BlobDetectorThread: public RateThread
{
private:
    ResourceFinder              &rf;

    BufferedPort<Image>         inPort;
    Port                        blobPort;

    int                         gaussian_winsize;

    double                      window_ratio;
    double                      thresh;
    int                         erode_itr;

    Semaphore                   mutex;
    Bottle                      blobs;
    Bottle                      non_blobs;

    int                         offset;
    
    CvBox2D32f                  box;
	CvPoint                     tmpCenter[20], center[20],pt1, pt2;
    double                      theta, perimeter, area;
    int                         numObj;

public:
    BlobDetectorThread(ResourceFinder &_rf)
        :RateThread(5),rf(_rf)
    {
    }
    bool                        details;

    virtual bool threadInit()
    {
        string name=rf.find("name").asString().c_str();

        inPort.open(("/"+name+"/img:i").c_str());

        blobPort.open(("/"+name+"/blobs:o").c_str());

        gaussian_winsize=rf.check("gaussian_winsize",Value(9)).asInt();

        thresh=rf.check("thresh",Value(10.0)).asDouble();
        erode_itr=rf.check("erode_itr",Value(0)).asInt();
        window_ratio=rf.check("window_ratio",Value(0.6)).asDouble();

        offset=rf.check("offset",Value(0)).asInt();
        details = false;
        return true;
    }

    virtual void run()
    {
        Image *img=inPort.read(false);       
        if(img!=NULL)
        {
            IplImage *gray=(IplImage*) img->getIplImage();

            cvSmooth(gray,gray,CV_GAUSSIAN,gaussian_winsize);
            cvThreshold(gray,gray,thresh,255.0,CV_THRESH_BINARY);
            cvEqualizeHist(gray,gray); //normalize brightness and increase contrast.
            cvErode(gray,gray,NULL,8);
            cvDilate(gray,gray,0,3/*erode_itr*/);

            mutex.wait();
            blobs.clear();
            non_blobs.clear();

            int w_offset=cvRound(0.5*gray->width*(1.0-window_ratio));
            int h_offset=cvRound(0.5*gray->height*(1.0-window_ratio));
            for(int row=h_offset; row<gray->height-h_offset; row++)
            {
                uchar *ptr=(uchar*) gray->imageData + row*gray->widthStep;
                for(int col=w_offset; col<gray->width-w_offset; col++)
                {
                    if(ptr[col]==255)
                    {
                        CvConnectedComp comp;
                        cvFloodFill(gray,cvPoint(col,row),cvScalar(255-(blobs.size()+non_blobs.size()+1)),cvScalar(0),cvScalar(0),&comp);

                        if(5<comp.rect.width && comp.rect.width<150 && 5<comp.rect.height && comp.rect.height<150)
                        {
                            Bottle &b=blobs.addList();
                            b.addDouble(comp.rect.x-offset);
                            b.addDouble(comp.rect.y-offset);
                            b.addDouble(comp.rect.x+comp.rect.width+offset);
                            b.addDouble(comp.rect.y+comp.rect.height+offset);
                        }
                        else
                        {
                            Bottle &n=non_blobs.addList();
                            n.addDouble(comp.rect.x-offset);
                            n.addDouble(comp.rect.y-offset);
                            n.addDouble(comp.rect.x+comp.rect.width+offset);
                            n.addDouble(comp.rect.y+comp.rect.height+offset);
                        }
                    }
                }
            }

            if (details)
            {
                processImg(gray); 
            }                       
            
        blobPort.write(*img);
        mutex.post();
        }
    }

    virtual void threadRelease()
    {
        inPort.close();
        blobPort.close();
    }


    bool execReq(const Bottle &command, Bottle &reply)
    {
        if(command.get(0).asVocab()==Vocab::encode("segment"))
        {
            fprintf(stdout,"segment request received\n");
            mutex.wait();
            if (blobs.size()>0)
                reply=blobs;
            else
                reply.addVocab(Vocab::encode("empty"));
            mutex.post();
            fprintf(stdout,"blobs: %s\n",reply.toString().c_str());
            
            return true;
        }

        return false;
    }

    void processImg(IplImage* image)
    {
        for (int i=0; i<blobs.size(); i++)
        {
            CvPoint cog=cvPoint(-1,-1);
            if ((i>=0) && (i<blobs.size()))
            {
                CvPoint tl,br;
                Bottle *item=blobs.get(i).asList();
                if (item==NULL)
                    cout << "ITEM IS NULL" << cog.x << cog.y <<endl;

                tl.x=(int)item->get(0).asDouble() - 10;
                tl.y=(int)item->get(1).asDouble() - 10;
                br.x=(int)item->get(2).asDouble() + 10;
                br.y=(int)item->get(3).asDouble() + 10;

                cog.x=(tl.x+br.x)>>1;
                cog.y=(tl.y+br.y)>>1;
                cvSetImageROI(image, cvRect(tl.x, tl.y, br.x - tl.x, br.y- tl.y));
                getOrientations(image);
                cvResetImageROI(image);
            }
        }
    }

    void getOrientations(IplImage* image)
    {
        float line[4];
        CvMemStorage *stor = cvCreateMemStorage(0);
        CvMemStorage *tmpStor = cvCreateMemStorage(0);
        IplImage *clone = cvCloneImage( image );
        CvSeq *tmpCont = cvCreateSeq(CV_SEQ_ELTYPE_POINT, sizeof(CvSeq), sizeof(CvPoint) , tmpStor);
        CvSeq *cont = cvCreateSeq(CV_SEQ_ELTYPE_POINT, sizeof(CvSeq), sizeof(CvPoint) , stor);
        cvFindContours( clone, tmpStor, &tmpCont, sizeof(CvContour), 
                    CV_RETR_LIST, CV_CHAIN_APPROX_NONE, cvPoint(0,0));
	    cvFindContours( clone, stor, &cont, sizeof(CvContour), 
                    CV_RETR_LIST, CV_CHAIN_APPROX_NONE, cvPoint(0,0));
        cvZero(clone);
        numObj = 0;

        //go first through all contours in order to find if there are some duplications
        for(;tmpCont;tmpCont = tmpCont->h_next){
            numObj ++;  
            CvBox2D32f boxtmp = cvMinAreaRect2(tmpCont, tmpStor); 
            tmpCenter[numObj].x = cvRound(boxtmp.center.x);
	        tmpCenter[numObj].y = cvRound(boxtmp.center.y);
        }
        int inc = 0;
        //check for duplicate center points
        int *index=new int[numObj];
     
        for (int i=1; i<=numObj/2; i++)
        {
            for (int y=numObj/2; y<=numObj; y++)
            {
                if ( abs( tmpCenter[i].x-tmpCenter[y].x) < 50 && i != y)
                {                
                    if ( abs( tmpCenter[i].y-tmpCenter[y].y) < 50 )  
                    {
                        index[inc] = y;
                        inc ++;                        
                    }
                }
            }
        }
        numObj = 0;
        for(;cont;cont = cont->h_next)
        {
            numObj ++;  
            box = cvMinAreaRect2(cont, stor); 
            center[numObj].x = cvRound(box.center.x);
	        center[numObj].y = cvRound(box.center.y);
		    float v = box.size.width;
		    float v1 = box.size.height;
            bool draw = true;
            for (int i= 0; i<inc; i++)
                if (numObj == index[i])
                    draw = false;
            if (draw)
            {        
                perimeter = cvContourPerimeter( cont );
	            area = fabs(cvContourArea( cont, CV_WHOLE_SEQ ));
                cvCircle (image, center[numObj], 3, CV_RGB(0,0,0),3);
                cvFitLine(cont, CV_DIST_L2, 0, 0.01, 0.01, line);
                float t = (v + v1)/2;
                pt1.x = cvRound(line[2] - line[0] *t );
                pt1.y = cvRound(line[3] - line[1] *t );
                pt2.x = cvRound(line[2] + line[0] *t );
                pt2.y = cvRound(line[3] + line[1] *t );

                cvLine( image, pt1, pt2, CV_RGB(255,255,255), 2, CV_AA, 0);
                theta = 0;
                theta = 180 / M_PI * atan2( (double)(pt2.y - pt1.y) , (double)(pt2.x - pt1.x) );
            }
                //cout << "orientation angles " << theta << endl;
        }
        
        cvRelease((void **)&cont); cvRelease((void **)&tmpCont);
		cvClearMemStorage( stor ); cvClearMemStorage( tmpStor );
        cvReleaseMemStorage(&stor);cvReleaseMemStorage(&tmpStor);
        cvReleaseImage(&clone);
        delete[] index;
    }
};


class BlobDetectorModule: public RFModule
{
private:
    BlobDetectorThread          *bdThr;

    Port                        rpcPort;


public:
    BlobDetectorModule()
    {}

    virtual bool configure(ResourceFinder &rf)
    {
        bdThr=new BlobDetectorThread(rf);

        if(!bdThr->start())
        {
            delete bdThr;
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
        bdThr->stop();
        delete bdThr;

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
        reply.clear();
        if (command.get(0).asString()=="details") 
        {
            if (command.get(1).asString()=="on")
            { 
                bdThr->details = true;
                reply.addString("setting details to ON");
                return true;
            }else
            {
                bdThr->details = false;
                reply.addString("setting details to OFF");
                return true;
            }         
            return false;     
        }
        if(bdThr->execReq(command,reply))
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
    rf.setDefaultContext("blobExtractor/conf");
    rf.setDefaultConfigFile("config.ini");
    rf.setDefault("name","blobExtractor");
    rf.configure("ICUB_ROOT",argc,argv);

    BlobDetectorModule mod;

    return mod.runModule(rf);
}




