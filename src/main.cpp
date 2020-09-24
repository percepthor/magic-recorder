#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <time.h>
#include <signal.h>

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

	// wait for all frames to be saved
	while (dlist_size ((magic_worker_job_queue->queue))) {
		sleep (1);
	}

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
	while (running || magic_worker_job_queue->queue->size) {
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

#pragma region videos

static void record (
	String *videos_dir,
	String *camera_name,
	int rotation,
	int scale_factor,
	int movement_thresh,
	int fps,
	int max_no_movement_frames
) {

	int no_movement_frames = 0;

	String *camera = str_create ("/dev/%s", camera_name);
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
				frame_width = cap.get (cv::CAP_PROP_FRAME_WIDTH);
				frame_height = cap.get (cv::CAP_PROP_FRAME_HEIGHT);
				break;
		
			default:
				frame_width = cap.get (cv::CAP_PROP_FRAME_WIDTH);
				frame_height = cap.get (cv::CAP_PROP_FRAME_HEIGHT);
				break;
		}

		int cap_fps = cap.get (cv::CAP_PROP_FPS);

		int scaled_width = frame_width / scale_factor;
		int scaled_height = frame_height / scale_factor;
		cv::Mat previous_frame_gray (scaled_height, scaled_width, CV_8U, cv::Scalar (0));
		cv::Mat gray_scale; // (scaled_height, scaled_with, CV_8U, Scalar (0));
		cv::Mat diference (scaled_height, scaled_width, CV_8U, cv::Scalar (0));

		bool movement = false;
		int movement_count = 0;

		cv::Mat *frame;
		cv::Mat resized;
		while (running) {
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

			cv::waitKey (cap_fps);
		}
	}

	else {
		char *status = c_string_create ("Failed to open camera: %s", camera->str);
		if (status) {
			log_error (status);
			free (status);
		}
	}

	str_delete (videos_dir);
	str_delete (camera);
	str_delete (camera_name);

}

#pragma endregion

#pragma region main

static void end (int dummy) {

	running = false;

	magic_worker_end ();

	exit (0);

}

static void print_help (void) {

	printf ("\n");
	printf ("Usage: ./bin/magic-recorder [args]\n");
	printf ("-h       	                Print this help\n");
	printf ("-v [videos dir]            Base path to save video files\n");
	printf ("-c [camera name]           The camera to use\n");
	printf ("-r [rotation]           	The frames rotation\n");
	printf ("-m [movement thresh]       The movmenet threshold to count as movement\n");
	printf ("--max_no_mov [n frames]    The max number of frames to allow without movement\n");
	printf ("--fps [value]              Video writer fps\n");
	printf ("\n");

}

int main (int argc, char **argv) {

	magic_recorder_version_print_full ();

	signal (SIGINT, end);
	signal (SIGTERM, end);

	String *videos_dir = NULL;
	String *camera_name = NULL;
	int rotation = 0;
	int scale_factor = DEFAULT_SCALE_FACTOR;
	int movement_thresh = DEFAULT_MOVEMENT_THRESH;
	int fps = DEFAULT_FPS;
	int max_no_movement_frames = DEFAULT_NO_MOVEMENT_FRAMES;

	if (argc > 1) {
		int j = 0;
		const char *curr_arg = NULL;
		for (int i = 1; i < argc; i++) {
			curr_arg = argv[i];

			if (!strcmp (curr_arg, "-h")) print_help ();

			// videos dir
			else if (!strcmp (curr_arg, "-v")) {
				j = i + 1;
				if (j <= argc) {
					videos_dir = str_new (argv[j]);
					i++;
				}
			}

			// camera
			else if (!strcmp (curr_arg, "-c")) {
				j = i + 1;
				if (j <= argc) {
					camera_name = str_new (argv[j]);
					i++;
				}
			}

			// rotation
			else if (!strcmp (curr_arg, "-r")) {
				j = i + 1;
				if (j <= argc) {
					rotation = atoi (argv[j]);
					i++;
				}
			}

			// movement thresh
			else if (!strcmp (curr_arg, "-m")) {
				j = i + 1;
				if (j <= argc) {
					movement_thresh = atoi (argv[j]);
					i++;
				}
			}

			// max_no_movement_frames
			else if (!strcmp (curr_arg, "--max_no_mov")) {
				j = i + 1;
				if (j <= argc) {
					movement_thresh = atoi (argv[j]);
					i++;
				}
			}

			// fps
			else if (!strcmp (curr_arg, "--fps")) {
				j = i + 1;
				if (j <= argc) {
					fps = atoi (argv[j]);
					i++;
				}
			}

			else {
				char *status = c_string_create ("Unknown argument: %s", curr_arg);
				if (status) {
					log_warning (status);
					free (status);
				}
			}
		}

		if (videos_dir && camera_name) {
			printf ("\n");
			printf ("Videos dir: %s\n", videos_dir->str);
			printf ("Camera: %s\n", camera_name->str);
			printf ("Rotation: %d\n", rotation);
			printf ("Scale factor: %d\n", scale_factor);
			printf ("Movement threshold: %d\n", movement_thresh);
			printf ("Writer's fps: %d\n", fps);
			printf ("Max no movement frames: %d\n", max_no_movement_frames);
			printf ("\n");

			if (!magic_worker_init ()) {
				record (videos_dir, camera_name, rotation, scale_factor, movement_thresh, fps, max_no_movement_frames);
			}
		}

		else {
			printf ("\n");
			log_error ("Videos dir & camera name are required!");
			printf ("\n");
		}
	}

	return 0;

}

#pragma endregion