#ifndef LBP_HPP_
#define LBP_HPP_

//! \author philipp <bytefish[at]gmx[dot]de>
//! \copyright BSD, see LICENSE.

#include <cv.h>
#include <limits>

namespace lbp {

// templated functions
template <typename _Tp>
void OLBP_(const cv::Mat& src, cv::Mat& dst);

template <typename _Tp>
void ELBP_(const cv::Mat& src, cv::Mat& dst, int radius = 1, int neighbors = 8);

template <typename _Tp>
void VARLBP_(const cv::Mat& src, cv::Mat& dst, int radius = 1, int neighbors = 8);

// wrapper functions
void OLBP(const cv::Mat& src, cv::Mat& dst);
void ELBP(const cv::Mat& src, cv::Mat& dst, int radius = 1, int neighbors = 8);
void VARLBP(const cv::Mat& src, cv::Mat& dst, int radius = 1, int neighbors = 8);

// Mat return type functions
cv::Mat OLBP(const cv::Mat& src);
cv::Mat ELBP(const cv::Mat& src, int radius = 1, int neighbors = 8);
cv::Mat VARLBP(const cv::Mat& src, int radius = 1, int neighbors = 8);

}
#endif
