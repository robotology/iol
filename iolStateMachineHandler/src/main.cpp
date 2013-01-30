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
\defgroup icub_iolStateMachineHandler State Machine Handler 
@ingroup icub_interactiveObjectsLearning 
 
The module managing all the components of the 
\ref icub_iolStateMachineHandler application. 
 
\section intro_sec Description 
This module is responsible for coordinating all the components 
that form the overall \ref icub_iolStateMachineHandler 
application. To this end, it receives input from the human 
operator and then forwards proper requests to the classifier, 
the blob detector, the motion caputer, the motor layer to let 
the robot achieve the goal. 
 
The commands sent as bottles to the module port 
/<modName>/human:i are the following: 
 
(notation: [.] identifies a vocab, <.> specifies a double,
"." specifies a string) 
 
<b>HOME</b> 
format: [home]
action: brings the robot back to its resting state.
 
<b>CALIB_TABLE</b> 
format: [cata]
action: let the robot discover the table height.

<b>CALIB_KINEMATICS</b> 
This command is splitted in two consecutive sub-commands: 
 
format subcmd1: [caki] [start] [left]/[right] "object" 
action: the robot reaches the object "object" with the 
specified hand and waits for the interaction with human based on 
force control in order to store the kinematic offsets corresponding
to the given object.
 
format subcmd2: [caki] [stop] 
action: terminate the calibration phase.

<b>TRACK</b> 
format: [track] [start]/[stop]
action: let the robot track any moving object.

<b>NAME</b> 
format: [name] "object"
action: let the robot learn a name to be associated with the
object that was tracked right before.

<b>FORGET</b> 
format: [forget] "object"
action: remove the object from the internal memory.
The special key "all" is used to purge the whole memory.

<b>WHERE</b> 
format: [where] "object"
action: ask the robot to point at the given object.
If no_object/wrong is recognized then the robot enters the
learning phase, where further commands are envisaged: i.e. [ack],
[nack], [skip], ...

<b>WHAT</b> 
format: [what]
action: ask the robot to say the name of the pointed object.
In case of a mistake the robot enters the learning phase where
further commands are envisaged: i.e. [ack], [nack], [skip],
[name], ...

<b>MOTOR_COMMANDS</b> 
format: [take]/[push]/[touch]/[hold]/[drop] "object"
action: ask the robot to perform some motor commands on the
given object.

<b>ATTENTION</b> 
format: [attention] [start]/[stop]
action: switch on/off the attention system.

\section lib_sec Libraries 
- YARP libraries. 
 
\section portsc_sec Ports Created 
- \e /<modName>/img:i receives the image acquired from the 
  camera previously specified through the command-line
  parameters.
 
- \e /<modName>/img:o streams out the image containing 
  recognized object. The image is updated whenever an action is
  required to be executed.
 
- \e /<modName>/imgLoc:o streams out the images for real-time 
  objects localization.
 
- \e /<modName>/imgClassifier:o used to pass images to the 
  classifier.
 
- \e /<modName>/human:rpc receives requests for actions 
  execution.
 
- \e /<modName>/blobs:rpc used to forward requests to the blob 
  detector for image segmentation.
 
- \e /<modName>/classify:rpc sends out requests for object 
  classification.
 
- \e /<modName>/motor:rpc sends out motor commands. 
 
- \e /<modName>/motor_stop:rpc used to interrupt/restore motor 
  capabilities.
 
- \e /<modName>/motor_stop:i receives the [icub-stop] trigger 
  from the verbal commands interpreter.
 
- \e /<modName>/point:i receives the latest pointed location 
  within the image from the motion capture module.
 
- \e /<modName>/speak:o streams out the robot's sentences that 
  need to be spoken.
 
- \e /<modName>/memory:rpc used to communicate with the objects 
  properties collector.
 
\section parameters_sec Parameters 
--name \e name
- specify the module stem-name, which is
  \e iolStateMachineHandler by default. The stem-name is used as
  prefix for all open ports.
 
--rt_localization_period \e period
- specify the period (given in [ms]) of the thread devoted to 
  real-time objects localization. The default value is 30 ms.
 
--memory_update_period \e period
- specify the period (given in [ms]) of the thread devoted to 
  updating the objects properties database. The default value is
  60 ms.
 
--camera \e [left|right] 
- specify the camera used to localized object. The default 
  camera is "left".
 
\section tested_os_sec Tested OS
Windows, Linux

\author Ugo Pattacini
*/ 

#include <yarp/os/Network.h>

#include "module.h"

using namespace yarp::os;


/**********************************************************/
int main(int argc, char *argv[])
{
    Network yarp;
    if (!yarp.checkNetwork())
        return -1;

    ResourceFinder rf;
    rf.setVerbose(true);
    rf.setDefault("name","iolStateMachineHandler");
    rf.setDefault("rt_localization_period","30");
    rf.setDefault("memory_update_period","60");
    rf.setDefault("camera","left");
    rf.configure("ICUB_ROOT",argc,argv);

    Manager manager;
    return manager.runModule(rf);
}



