#ifndef _MAGIC_VERSION_H_
#define _MAGIC_VERSION_H_

#define MAGIC_RECORDER_VERSION                    	"0.1"
#define MAGIC_RECORDER_VERSION_NAME               	"Version 0.1"
#define MAGIC_RECORDER_VERSION_DATE			     	"24/09/2020"
#define MAGIC_RECORDER_VERSION_TIME			     	"08:10 CST"
#define MAGIC_RECORDER_VERSION_AUTHOR				"Erick Salas"

// print full magic recorder version information 
extern void magic_recorder_version_print_full (void);

// print the version id
extern void magic_recorder_version_print_version_id (void);

// print the version name
extern void magic_recorder_version_print_version_name (void);

#endif