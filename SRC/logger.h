#ifndef LOGGER_H
#define LOGGER_H

/*
 * Standard library imports.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/*
 * Custom MTP imports.
 */
// None

/*****************************************
 * CONSTANTS 
 *****************************************/
// None

/*****************************************
 * STRUCTURES 
 *****************************************/
typedef enum {
    LOG_TO_CONSOLE,
    LOG_TO_FILE
} log_mode_t;

/*****************************************
 * FUNCTION PROTOTYPES 
 *****************************************/
void set_log_mode(log_mode_t mode, const char *file_path);
void close_log_file();
void log_message(const char *format, ...);

#endif
