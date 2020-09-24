#ifndef _FILES_H_
#define _FILES_H_

#include <stdio.h>

#include <sys/stat.h>

// check if a directory already exists, and if not, creates it
// returns 0 on success, 1 on error
extern unsigned int files_create_dir (const char *dir_path, mode_t mode);

#endif