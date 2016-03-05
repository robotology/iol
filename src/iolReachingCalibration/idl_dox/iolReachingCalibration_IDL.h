// This is an automatically-generated file.
// It could get re-generated if the ALLOW_IDL_GENERATION flag is on.

#ifndef YARP_THRIFT_GENERATOR_iolReachingCalibration_IDL
#define YARP_THRIFT_GENERATOR_iolReachingCalibration_IDL

#include <yarp/os/Wire.h>
#include <yarp/os/idl/WireTypes.h>
#include <src/CalibReq.h>

class iolReachingCalibration_IDL;


/**
 * iolReachingCalibration_IDL
 * IDL Interface to \ref IOL Reaching Calibration services.
 */
class iolReachingCalibration_IDL : public yarp::os::Wire {
public:
  iolReachingCalibration_IDL();
  /**
   * Initiate the calibration.
   * @param hand can be "left" or "right".
   * @param object selects the object.
   * @param entry forces to specify the entry
   *              name in the calibration map.
   * @return true/false on success/failure.
   */
  virtual bool calibration_start(const std::string& hand, const std::string& object, const std::string& entry = "");
  /**
   * Finish the calibration.
   * @return true/false on success/failure.
   */
  virtual bool calibration_stop();
  /**
   * Clear calibration
   * @param hand can be "left" or "right".
   * @param object selects the object.
   * @param entry forces to specify the entry
   *              name in the calibration map.
   * @return true/false on success/failure.
   */
  virtual bool calibration_clear(const std::string& hand, const std::string& object, const std::string& entry = "");
  /**
   * List available calibrations.
   * @return the list of available calibrations.
   */
  virtual std::vector<std::string>  calibration_list();
  /**
   * Retrieve the calibrated object location.
   * @param hand can be "left" or "right".
   * @param object selects the object.
   * @param entry forces to specify the entry
   *              name in the calibration map.
   * @return the requested point in \ref CalibReq format.
   */
  virtual CalibReq get_location(const std::string& hand, const std::string& object, const std::string& entry = "");
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
  virtual bool test_set(const std::string& entry, const double xi, const double yi, const double zi, const double xo, const double yo, const double zo);
  /**
   * Test for retrieving location.
   * @param entry selects the entry in the table.
   * @param x the input point x-coordinate.
   * @param y the input point y-coordinate.
   * @param z the input point z-coordinate.
   * @return the requested point in \ref CalibReq format.
   */
  virtual CalibReq test_get(const std::string& entry, const double x, const double y, const double z);
  /**
   * Save calibration on file.
   * @return true/false on success/failure.
   */
  virtual bool save();
  /**
   * Load calibration from file.
   * @return true/false on success/failure.
   */
  virtual bool load();
  virtual bool read(yarp::os::ConnectionReader& connection);
  virtual std::vector<std::string> help(const std::string& functionName="--all");
};

#endif
