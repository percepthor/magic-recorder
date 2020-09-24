#include <stdlib.h>
#include <stdio.h>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "types/types.h"
#include "types/string.h"

#include "utils/utils.h"
#include "utils/log.h"

#include "magic.h"
#include "version.h"

int main (int argc, char **argv) {

	magic_recorder_version_print_full ();

	// TODO: get arguments
	const char *camera_name = NULL;
	int rotation = 0;
	int scale_factor = 0;
	int movement_thresh = 0;

	String *camera = NULL;
	cv::VideoCapture cap (camera->str);

	if (cap.isOpened ()) {
		// set device properties
		cap.set (cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc ('M', 'J', 'P', 'G' ));
		cap.set (cv::CAP_PROP_FPS, 30);
		cap.set (cv::CAP_PROP_FRAME_WIDTH, 3840);
		cap.set (cv::CAP_PROP_FRAME_HEIGHT, 2160);
		
		cap.set (cv::CAP_PROP_AUTO_EXPOSURE, 1);
		cap.set (cv::CAP_PROP_BRIGHTNESS, 40);    
		cap.set (cv::CAP_PROP_CONTRAST, 10);
		cap.set (cv::CAP_PROP_SATURATION, 80);
		cap.set (cv::CAP_PROP_GAIN, 90);
		cap.set (cv::CAP_PROP_EXPOSURE, 155);
		cap.set (cv::CAP_PROP_SHARPNESS, 0);

		// default resolution of the frame is obtained
		// the default resolution is system dependent
		int frame_height = 0;
		int frame_width = 0;
		switch (rotation) {
			case 1:
				frame_height = cap.get (cv::CAP_PROP_FRAME_WIDTH);
				frame_width = cap.get (cv::CAP_PROP_FRAME_HEIGHT);
				break;
			case 2:
				frame_height = cap.get (cv::CAP_PROP_FRAME_WIDTH);
				frame_width = cap.get (cv::CAP_PROP_FRAME_HEIGHT);
				break;
			case 3:
				frame_width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
				frame_height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
				break;
		
			default:
				frame_width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
				frame_height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
				break;
		}

		int scaled_with = frame_width / scale_factor;
		int scaled_height = frame_height / scale_factor;
		cv::Mat previous_frame_gray (scaled_height, scaled_with, CV_8U, cv::Scalar (0));
		cv::Mat grayScale; // (scaled_height, scaled_with, CV_8U, Scalar (0));
		cv::Mat diference (scaled_height, scaled_with, CV_8U, cv::Scalar (0));

		cv::Mat frame;
		while (true) {
			cap >> frame;

			if (!frame.empty ()) {
				switch (rotation) {
					case 1: cv::rotate (frame, frame, cv::ROTATE_90_CLOCKWISE); break;
            		case 2: cv::rotate (frame, frame, cv::ROTATE_90_COUNTERCLOCKWISE); break;
            		case 3: cv::rotate (frame, frame, cv::ROTATE_180); break;
            
            		default: break;
				}

				// check for movement in frame
				// TODO:
			}

			else {
				log_warning ("Empty frame!");
			}

			// TODO: handle this based on capture real fps
			cv::waitKey (30);
		}
	}

	else {
		char *status = c_string_create ("Failed to open camera: %s", camera->str);
		if (status) {
			log_error (status);
			free (status);
		}
	}

	return 0;

}