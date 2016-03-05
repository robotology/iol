# Copyright: (C) 2016 iCub Facility - Istituto Italiano di Tecnologia
# Authors: Ugo Pattacini
# CopyPolicy: Released under the terms of the GNU GPL v2.0.
#
# idl.thrift

/**
* CalibReq
*
* IDL structure to send/receive points.
*/
struct CalibReq
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
   * Retrieve the calibrated object location.
   * @param hand can be "left" or "right".
   * @param object selects the object.
   * @param entry forces to specify the entry
   *              name in the calibration map.
   * @return the requested point in \ref CalibReq format.
   */
   CalibReq get_location(1:string hand, 2:string object, 3:string entry="");

   /**
   * Test for defining an input location.
   * @param entry selects the entry in the table.
   * @param xi the input point x-coordinate.
   * @param yi the input point y-coordinate.
   * @param zi the input point z-coordinate.
   * @param xo the output point x-coordinate.
   * @param yo the output point y-coordinate.
   * @param zo the output point z-coordinate.
   * @return true/false on success/failure.
   */
   bool test_set(1:string entry, 2:double xi, 3:double yi, 4:double zi,
                                 5:double xo, 6:double yo, 7:double zo);

   /**
   * Test for retrieving location.
   * @param entry selects the entry in the table.
   * @param x the input point x-coordinate.
   * @param y the input point y-coordinate.
   * @param z the input point z-coordinate.
   * @return the requested point in \ref CalibReq format.
   */
   CalibReq test_get(1:string entry, 2:double x, 3:double y, 4:double z);

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
