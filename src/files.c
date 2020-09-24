#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define _XOPEN_SOURCE 700
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <unistd.h>

#include "types/types.h"
#include "types/string.h"

#include "utils/utils.h"
#include "utils/log.h"

// check if a directory already exists, and if not, creates it
// returns 0 on success, 1 on error
unsigned int files_create_dir (const char *dir_path, mode_t mode) {

	unsigned int retval = 1;

	if (dir_path) {
		struct stat st = { 0 };
		int ret = stat (dir_path, &st);
		switch (ret) {
			case -1: {
				if (!mkdir (dir_path, mode)) {
					retval = 0;		// success
				}

				else {
					char *s = c_string_create ("Failed to create dir %s!", dir_path);
					if (s) {
                        log_error (s);
						free (s);
					}
				}
			} break;
			case 0: {
				char *s = c_string_create ("Dir %s already exists!", dir_path);
				if (s) {
                    log_warning (s);
					free (s);
				}
			} break;

			default: break;
		}
	}

	return retval;

}