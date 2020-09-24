#include <stdio.h>

#include "version.h"

// print full magic recorder version information 
void magic_recorder_version_print_full (void) {

    printf ("\n\nMagic Recorder Version: %s\n", MAGIC_RECORDER_VERSION_NAME);
    printf ("Release Date & time: %s - %s\n", MAGIC_RECORDER_VERSION_DATE, MAGIC_RECORDER_VERSION_TIME);
    printf ("Author: %s\n\n", MAGIC_RECORDER_VERSION_AUTHOR);

}

// print the version id
void magic_recorder_version_print_version_id (void) {

    printf ("\n\nMagic Recorder Version ID: %s\n", MAGIC_RECORDER_VERSION);

}

// print the version name
void magic_recorder_version_print_version_name (void) {

    printf ("\n\nMagic Recorder Version: %s\n", MAGIC_RECORDER_VERSION_NAME);

}