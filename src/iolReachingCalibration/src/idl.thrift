# Copyright: (C) 2016 iCub Facility - Istituto Italiano di Tecnologia
# Authors: Ugo Pattacini
# CopyPolicy: Released under the terms of the GNU GPL v2.0.
#
# idl.thrift

struct Matrix { }
(
   yarp.name="yarp::sig::Matrix"
   yarp.includefile="yarp/sig/Matrix.h"
)

/**
* CalibPointReq
*
* IDL structure to send/receive points.
*/
struct CalibPointReq
{
   /**
   * contain [ok]/[fail] on success/failure.
   */
   1:string result;

   /**
   * the x-coordinate.
   */
   2:double x;

   /**
   * the y-coordinate.
   */
   3:double y;

   /**
   * the z-coordinate.
   */
   4:double z;
}

/**
* CalibMatrixReq
*
* IDL structure to ask for calibration matrix.
*/
struct CalibMatrixReq
{
   /**
   * contain [ok]/[fail] on success/failure.
   */
   1:string result;

   /**
   * the calibration matrix.
   */
   2:Matrix H;
}

/**
* iolReachingCalibration_IDL
*
* IDL Interface to \ref IOL Reaching Calibration services.
*/
service iolReachingCalibration_IDL
{
   /**
   * Initiate the calibration.
   * @param hand can be "left" or "right".
   * @param object selects the object.
   * @param entry forces to specify the entry
   *              name in the calibration map.
   * @return true/false on success/failure.
   */
   bool calibration_start(1:string hand, 2:string object, 3:string entry="");

   /**
   * Finish the calibration.
   * @return true/false on success/failure.
   */
   bool calibration_stop();

   /**
   * Clear calibration
   * @param hand can be "left" or "right".
   * @param object selects the object.
   * @param entry forces to specify the entry
   *              name in the calibration map.
   * @return true/false on success/failure.
   */
   bool calibration_clear(1:string hand, 2:string object, 3:string entry="");

   /**
   * List available calibrations.
   * @return the list of available calibrations.
   */
   list<string> calibration_list();

   /**
   * Retrieve the calibrated object location.\n
   * The robot will bring the object in foveation
   * before computing the calibration.
   * @param hand can be "left" or "right".
   * @param object selects the object.
   * @param entry forces to specify the entry
   *              name in the calibration map.
   * @return the requested point in \ref CalibPointReq format.
   */
   CalibPointReq get_location(1:string hand, 2:string object, 3:string entry="");

   /**
   * Retrieve the calibrated object location.\n
   * No foveation is performed before computing the
   * calibrated values.
   * @param entry selects the entry in the table.
   * @param x the input point x-coordinate.
   * @param y the input point y-coordinate.
   * @param z the input point z-coordinate.
   * @param invert if true invert the input/output map.
   * @return the requested point in \ref CalibPointReq format.
   */
   CalibPointReq get_location_nolook(1:string entry, 2:double x, 3:double y, 4:double z, 5:bool invert=false);

   /**
   * Retrieve the calibration matrix.
   * @param entry selects the entry in the table.
   * @return the requested calibration matrix in \ref CalibMatrixReq format.
   */
   CalibMatrixReq get_matrix(1:string entry);

   /**
   * Add an input-ouput pair to the location map.
   * @param entry selects the entry in the table.
   * @param xi the input point x-coordinate.
   * @param yi the input point y-coordinate.
   * @param zi the input point z-coordinate.
   * @param xo the output point x-coordinate.
   * @param yo the output point y-coordinate.
   * @param zo the output point z-coordinate.
   * @return true/false on success/failure.
   */
   bool add_pair(1:string entry, 2:double xi, 3:double yi, 4:double zi,
                 5:double xo, 6:double yo, 7:double zo);

   /**
   * Save calibration on file.
   * @return true/false on success/failure.
   */
   bool save();

   /**
   * Load calibration from file.
   * @return true/false on success/failure.
   */
   bool load();
}
