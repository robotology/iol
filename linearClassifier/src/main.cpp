
#include <yarp/dev/Drivers.h>
#include "linearClassifierModule.h"


int main(int argc, char * argv[])
{

   Network yarp;
   linearClassifierModule linearClassifierModule; 

   ResourceFinder rf;
   rf.setVerbose(true);
   rf.setDefaultConfigFile("linearClassifier.ini"); 
   rf.setDefaultContext("onTheFlyRecognition/conf");
   rf.configure("ICUB_ROOT", argc, argv);
 

   linearClassifierModule.runModule(rf);

    return 0;
}
