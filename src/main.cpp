#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <time.h>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "types/types.h"
#include "types/string.h"

#include "threads/bsem.h"
#include "threads/jobs.h"
#include "threads/thread.h"

#include "utils/utils.h"
#include "utils/log.h"

#include "magic.h"
#include "version.h"

#define BUFFER_SIZE			1024

static bool running = true;

static char video_name[BUFFER_SIZE] = { 0 };
static cv::VideoWriter writer;

#pragma region movement

static int check_movement (cv::Mat frame) {

	int contador = 0;
	int valor = 0;
	uint8_t *data = (uint8_t *) frame.data;

	for (int y = 0; y < frame.rows; y++) {
		for (int x = 0; x < frame.cols; x++) {
			valor = *data++;
			if (valor) {
				contador++;
			}
		}
	}

	return contador;

}

#pragma endregion

#pragma region worker

static void *magic_worker_thread (void *null_ptr);

static JobQueue *magic_worker_job_queue = NULL;

static unsigned int magic_worker_init (void) {

	unsigned int retval = 1;

	magic_worker_job_queue = job_queue_create ();
	if (magic_worker_job_queue) {
		pthread_t thread_id = 0;
		if (!thread_create_detachable (&thread_id, magic_worker_thread, NULL)) {
			retval = 0;
		}
	}

	return retval;

}

static unsigned int magic_worker_end (void) {

	bsem_post (magic_worker_job_queue->has_jobs);

	sleep (1);

	job_queue_delete (magic_worker_job_queue);

	return 0;

}

static unsigned int magic_worker_push (cv::Mat *frame) {

	return job_queue_push (
		magic_worker_job_queue,
		job_create (NULL, frame)
	);

}

static void *magic_worker_thread (void *null_ptr) {

	sleep (2);			// wait until everything has started

	log_success ("Magic WORKER thread has started!");

	thread_set_name ("magic-worker");

	Job *job = NULL;
	cv::Mat *frame = NULL;
	while (running) {
		bsem_wait (magic_worker_job_queue->has_jobs);
		
		// we are safe to analyze the frames & generate embeddings
		job = job_queue_pull (magic_worker_job_queue);
		if (job) {
			printf ("magic_worker_thread () - New job!\n");
			frame = (cv::Mat *) job->args;

			// save the frame
			writer.write (*frame);
			
			// delete the frame
			frame->release ();
			delete (frame);

			job_delete (job);
		}
	}

	log_success ("Magic worker thread has exited!");

	return NULL;

}

#pragma endregion

int main (int argc, char **argv) {

	magic_recorder_version_print_full ();

	// TODO: get arguments
	const char *camera_name = NULL;
	int rotation = 0;
	int scale_factor = 0;
	int movement_thresh = 0;
	int fps = 0;

	unsigned int no_movement_frames = 0;
	unsigned int max_no_movement_frames = DEFAULT_NO_MOVEMENT_FRAMES;

	String *videos_dir = NULL;
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

		int scaled_width = frame_width / scale_factor;
		int scaled_height = frame_height / scale_factor;
		cv::Mat previous_frame_gray (scaled_height, scaled_width, CV_8U, cv::Scalar (0));
		cv::Mat gray_scale; // (scaled_height, scaled_with, CV_8U, Scalar (0));
		cv::Mat diference (scaled_height, scaled_width, CV_8U, cv::Scalar (0));

		bool movement = false;
		int movement_count = 0;

		cv::Mat *frame;
		cv::Mat resized;
		while (true) {
			frame = new cv::Mat ();
			cap >> *frame;

			if (!frame->empty ()) {
				switch (rotation) {
					case 1: cv::rotate (*frame, *frame, cv::ROTATE_90_CLOCKWISE); break;
					case 2: cv::rotate (*frame, *frame, cv::ROTATE_90_COUNTERCLOCKWISE); break;
					case 3: cv::rotate (*frame, *frame, cv::ROTATE_180); break;
			
					default: break;
				}

				// check for movement in frame
				cv::resize (*frame, resized, cv::Size (scaled_width, scaled_height));
				cv::cvtColor (resized, gray_scale, cv::COLOR_BGR2GRAY);
				// fastNlMeansDenoising (gray_scale, gray_scale, 3.0, 3, 3);
				cv::absdiff (gray_scale, previous_frame_gray, diference);
				cv::threshold (diference, diference, 45, 255, cv::THRESH_BINARY);

				movement_count = check_movement (diference);
				printf ("Movement: %d\n", movement);

				if (!movement) {
					if (movement_count > movement_thresh) {
						// create files to save video
						printf ("\n");
						log_success ("First movement -> creating video...");
						printf ("\n");

						snprintf (video_name, BUFFER_SIZE, "%s/%s/%ld.avi", videos_dir->str, camera->str, time (NULL));
						// MJPG -> fourcc ('D','I','V','X')
						writer = cv::VideoWriter (video_name, cv::VideoWriter::fourcc ('M','J','P','G'), fps, cv::Size (frame_width, frame_height));
						if (writer.isOpened ()) {
							char *status = c_string_create ("Opened %s", video_name);
							if (status) {
								log_debug (status);
								free (status);
							}

							// writer.write (frame);
							magic_worker_push (frame);
						}

						else {
							char *status = c_string_create ("Failed to open video writer %s", video_name);
							if (status) {
								log_error (status);
								free (status);
							}
						}
						
						no_movement_frames = 0;
						movement = true;
					}
				}

				else {
					// check if there is still movement
					if (movement_count > movement_thresh) {
						// writer.write (frame);
						magic_worker_push (frame);
						no_movement_frames = 0;
					}

					else {
						no_movement_frames += 1;
						if (no_movement_frames >= max_no_movement_frames) {
							log_warning ("Max no movement frames reached! Stopping current video...");
							movement = false;
							// FIXME: sync with worker to correctly release the writer
							writer.release ();
						}
					}
				}

				previous_frame_gray = gray_scale.clone ();
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