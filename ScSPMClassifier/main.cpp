
#include <yarp/dev/Drivers.h>
#include "SCSPMClassifier.h"


int main(int argc, char * argv[])
{

   Network yarp;
   SCSPMClassifier SCSPMClassifier; 

   ResourceFinder rf;
   rf.setVerbose(true);
   rf.setDefaultConfigFile("SCSPMClassifier.ini"); 
   rf.setDefaultContext("iolStateMachineHandler/conf");
   rf.configure("ICUB_ROOT", argc, argv);
 

   SCSPMClassifier.runModule(rf);

    return 0;
}
