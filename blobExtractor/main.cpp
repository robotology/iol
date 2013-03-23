#include <yarp/os/Network.h>
#include <yarp/os/BufferedPort.h>
#include <yarp/os/RFModule.h>
#include <yarp/os/RateThread.h>
#include <yarp/os/Semaphore.h>
#include <yarp/os/Stamp.h>

#include <yarp/sig/Image.h>
#include <yarp/sig/Vector.h>

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

    BufferedPort<Image>         port_i_img;
    Port                        port_o_img;
    Port                        port_o_blobs;

    int                         gaussian_winsize;

    double                      window_ratio;
    double                      thresh;
    int                         erode_itr;
    int                         dilate_itr;

    Semaphore                   mutex;
    Semaphore                   contours;
    Bottle                      blobs;
    Bottle                      non_blobs;

    int                         offset;

    yarp::sig::Vector           area, orientation, axe1, axe2;
    int                         numBlobs;
    
    CvPoint                     tmpCenter[500], center[500],pt1, pt2;
    int                         numObj;
    double                      theta;

public:
    BlobDetectorThread(ResourceFinder &_rf)
        :RateThread(5),rf(_rf)
    {
    }
    
    string                        details;

    virtual bool threadInit()
    {
        string name=rf.find("name").asString().c_str();

        port_i_img.open(("/"+name+"/img:i").c_str());
        port_o_img.open(("/"+name+"/img:o").c_str());

        port_o_blobs.open(("/"+name+"/blobs:o").c_str());

        gaussian_winsize=rf.check("gaussian_winsize",Value(9)).asInt();

        thresh=rf.check("thresh",Value(10.0)).asDouble();
        erode_itr=rf.check("erode_itr",Value(8)).asInt();
        dilate_itr=rf.check("dilate_itr",Value(3)).asInt();
        window_ratio=rf.check("window_ratio",Value(0.6)).asDouble();
        
        details=rf.check("details",Value("off")).asString();

        offset=rf.check("offset",Value(0)).asInt();
        //details = false;
        numBlobs = 0;
        orientation.clear();
        axe1.clear();
        axe2.clear();
        area.clear();
        area.resize(500);
        orientation.resize(500);
        axe1.resize(500);
        axe2.resize(500);
        return true;
    }

    virtual void setThreshold(double newThreshold)
    {
        mutex.wait();
        thresh = newThreshold;
        mutex.post();
    }

    virtual void run()
    {
        Image *img=port_i_img.read(false);       
        if(img!=NULL)
        {
            Stamp ts;
            port_i_img.getEnvelope(ts);

            IplImage *gray=(IplImage*) img->getIplImage();

            cvSmooth(gray,gray,CV_GAUSSIAN,gaussian_winsize);
            cvThreshold(gray,gray,thresh,255.0,CV_THRESH_BINARY);
            cvEqualizeHist(gray,gray); //normalize brightness and increase contrast.
            cvErode(gray,gray,NULL,erode_itr);
            cvDilate(gray,gray,0,dilate_itr);

            mutex.wait();
            blobs.clear();
            non_blobs.clear();

            int w_offset=cvRound(0.5*gray->width*(1.0-window_ratio));
            int h_offset=cvRound(0.5*gray->height*(1.0-window_ratio));
            int itr = 0;
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
                            if(details=="on")
                            {
                                if(orientation.size() > 0 )
                                {
                                    b.addDouble(orientation[itr+1]);
                                    b.addInt((int)axe2[itr+1]);
                                    b.addInt((int)axe1[itr+1]);
                                }                         
                            }
                            itr++;
                        }
                        else
                        {
                            Bottle &n=non_blobs.addList();
                            n.addDouble(comp.rect.x-offset);
                            n.addDouble(comp.rect.y-offset);
                            n.addDouble(comp.rect.x+comp.rect.width+offset);
                            n.addDouble(comp.rect.y+comp.rect.height+offset);
                            if(details=="on")
                            {
                                if(orientation.size() > 0 )
                                {
                                    n.addDouble(orientation[itr+1]);
                                    n.addInt((int)axe2[itr+1]);
                                    n.addInt((int)axe1[itr+1]);
                                }                         
                            }
                            itr++;
                        }
                    }
                }
            }

            if (details=="on")
            {
                contours.wait();
                processImg(gray);
                contours.post(); 
            }
            
            port_o_img.setEnvelope(ts);
            port_o_img.write(*img);

            port_o_blobs.setEnvelope(ts);
            port_o_blobs.write(blobs);
            mutex.post();
        }
    }

    virtual void threadRelease()
    {
        port_i_img.close();
        port_o_img.close();
        port_o_blobs.close();
    }


    bool execReq(const Bottle &command, Bottle &reply)
    {
        /*if(command.get(0).asVocab()==Vocab::encode("segment"))
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
        }*/
        if(command.get(0).asVocab()==Vocab::encode("thresh"))
        {
            mutex.wait();
            thresh = command.get(1).asDouble();
            mutex.post();
            reply.addVocab(Vocab::encode("ok"));
            return true;
        }
        if(command.get(0).asVocab()==Vocab::encode("erode"))
        {
            mutex.wait();
            erode_itr = command.get(1).asInt();
            mutex.post();
            reply.addVocab(Vocab::encode("ok"));
            return true;
        }
        if(command.get(0).asVocab()==Vocab::encode("dilate"))
        {
            mutex.wait();
            dilate_itr = command.get(1).asInt();
            mutex.post();
            reply.addVocab(Vocab::encode("ok"));
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

                cog.x=(tl.x + br.x)>>1;
                cog.y=(tl.y + br.y)>>1;
                cvSetImageROI(image, cvRect(tl.x, tl.y, br.x - tl.x, br.y- tl.y));
                //fprintf(stdout,"dgb0\n");
                getOrientations(image);
                cvResetImageROI(image);
                //fprintf(stdout,"dgb1\n");
            }
        }
        numBlobs = 0;
        numObj = 0;
    }

    void getOrientations(IplImage* image)
    {
        float line[4];
        CvMemStorage *stor = cvCreateMemStorage(0);
        CvMemStorage *tmpStor = cvCreateMemStorage(0);
        CvBox2D32f* box;
        CvPoint* PointArray;
        CvPoint2D32f* PointArray2D32f;
        CvPoint center;
        CvSize size;
        //fprintf(stdout,"dgb0.1\n");
        IplImage *clone = cvCloneImage( image );
        CvSeq *tmpCont = cvCreateSeq(CV_SEQ_ELTYPE_POINT, sizeof(CvSeq), sizeof(CvPoint) , tmpStor);
        CvSeq *cont = cvCreateSeq(CV_SEQ_ELTYPE_POINT, sizeof(CvSeq), sizeof(CvPoint) , stor);
        cvFindContours( clone, tmpStor, &tmpCont, sizeof(CvContour), 
                    CV_RETR_LIST, CV_CHAIN_APPROX_NONE, cvPoint(0,0));
	    cvFindContours( clone, stor, &cont, sizeof(CvContour), 
                    CV_RETR_LIST, CV_CHAIN_APPROX_NONE, cvPoint(0,0));
        cvZero(clone);
        //go first through all contours in order to find if there are some duplications
        for(;tmpCont;tmpCont = tmpCont->h_next){
            numObj++;
            CvBox2D32f boxtmp = cvMinAreaRect2(tmpCont, tmpStor); 
            //fprintf(stdout,"dgb0.25  %lf  %lf\n", cvRound(boxtmp.center.x), cvRound(boxtmp.center.y));
            tmpCenter[numObj].x = cvRound(boxtmp.center.x);
	        tmpCenter[numObj].y = cvRound(boxtmp.center.y);
	       	area[numObj] = fabs(cvContourArea( tmpCont, CV_WHOLE_SEQ ));
            //fprintf(stdout,"%d X= %d Y= %d\n",numObj, tmpCenter[numObj].x, tmpCenter[numObj].y); 
        }
        int inc = 0;
        //check for duplicate center points
        yarp::sig::Vector index;
        index.resize(numObj);
        for (int x=1; x<numObj; x++)
        {
        	if (abs( tmpCenter[x].x -tmpCenter[x+1].x ) < 10 )
        		if (abs( tmpCenter[x].y -tmpCenter[x+1].y) < 10)
        		{
        			if ( area[x] < area[x+1] )
        			{
        				index[inc] = x;
        				inc++;
        			}
        			else
        			{
        				index[inc] = x+1;
        				inc++;
        			}
        		} 
        			
        }
        for(;cont;cont = cont->h_next)
        {
            numBlobs++;
            bool draw = true;
            for (int i= 0; i<inc; i++)
                if (numBlobs == index[i])	
                	draw = false;
                    
            int count = cont->total;
            if( count < 6 )
                continue;
            if (draw)
            {        
                // Alloc memory for contour point set.    
                PointArray = (CvPoint*)malloc( count*sizeof(CvPoint) );
                PointArray2D32f= (CvPoint2D32f*)malloc( count*sizeof(CvPoint2D32f) );
                // Alloc memory for ellipse data.
                box = (CvBox2D32f*)malloc(sizeof(CvBox2D32f));
                cvCvtSeqToArray(cont, PointArray, CV_WHOLE_SEQ);
                for(int i=0; i<count; i++)
                {
                    PointArray2D32f[i].x = (float)PointArray[i].x;
                    PointArray2D32f[i].y = (float)PointArray[i].y;
                }
                cvFitEllipse(PointArray2D32f, count, box);
                
                
                if ((box->size.width > 0) && (box->size.width < 300) && (box->size.height > 0) && (box->size.height < 300))
                {
                    center.x = cvRound(box->center.x);
                    center.y = cvRound(box->center.y);
                    size.width = cvRound(box->size.width*0.5);
                    size.height = cvRound(box->size.height*0.5);
                    //box->angle = box->angle;
                    //cvEllipse(image, center, size, box->angle, 0, 360, CV_RGB(255,255,255), 1, CV_AA, 0);
                
                    //orientation[numBlobs] = (box->angle) - 90.0;//box->angle;
                    axe1[numBlobs] = size.width;
                    axe2[numBlobs] = size.height;
                
                    cvFitLine(cont, CV_DIST_L2, 0, 0.01, 0.01, line);
                    float t = (box->size.width + box->size.height)/2;
                    pt1.x = cvRound(line[2] - line[0] *t );
                    pt1.y = cvRound(line[3] - line[1] *t );
                    pt2.x = cvRound(line[2] + line[0] *t );
                    pt2.y = cvRound(line[3] + line[1] *t );
                    cvLine( image, pt1, pt2, CV_RGB(255,255,255), 2, CV_AA, 0);
                    theta = 0;
                    theta = 180 / M_PI * atan2( (double)(pt2.y - pt1.y) , (double)(pt2.x - pt1.x) );
                    
                    if (theta < 0)
                        theta = -theta;
                    else
                        theta = 180 -theta;
                    
                    orientation[numBlobs] = theta;  
                }
                //cout << "orientation angles " << theta << endl;
                // Free memory.          
                free(PointArray);
                free(PointArray2D32f);
                free(box);
                //fprintf(stdout, "NUMBLOBS %d orient %lf   axe1 %d, axe2 %d \n", numBlobs, orientation[numBlobs], (int)axe1[numBlobs], (int)axe2[numBlobs]);
            }
            //fprintf(stdout,"dgb0.8\n");
        }
        // Free memory.
        cvRelease((void **)&cont); cvRelease((void **)&tmpCont);
		cvClearMemStorage( stor ); cvClearMemStorage( tmpStor );
        cvReleaseMemStorage(&stor);cvReleaseMemStorage(&tmpStor);
        cvReleaseImage(&clone);
       // fprintf(stdout,"dgb0.9\n");
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
                bdThr->details = "on";
                reply.addString("setting details to ON");
                return true;
            }else
            {
                bdThr->details = "off";
                reply.addString("setting details to OFF");
                return true;
            }         
            return false;     
        } else if (command.get(0).asString()=="thresh") 
        {
            double newThresh = command.get(1).asDouble();
            if (newThresh<0 || newThresh>255.0)
            {
                reply.addString("Invalid threshold (expecting a value between 0 and 255)");
            	return false;
			}
            reply.addString("Setting threshold");
            bdThr->setThreshold(newThresh);
			return true;
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




