#include "logger.h"

static log_mode_t current_log_mode = LOG_TO_CONSOLE;
static FILE *log_file = NULL;

void set_log_mode(log_mode_t mode, const char *file_path) 
{
    current_log_mode = mode;

    if(mode == LOG_TO_FILE) 
    {
        log_file = fopen(file_path, "w");

        if(log_file == NULL) 
        {
            perror("Failed to open log file");
            exit(EXIT_FAILURE);
        }
    } 
    
    else if(log_file != NULL) 
    {
        fclose(log_file);
        log_file = NULL;
    }
}

void close_log_file() 
{
    if(log_file != NULL) 
    {
        fclose(log_file);
        log_file = NULL;
    }
}

void log_message(const char *format, ...) 
{
    va_list args;
    va_start(args, format);
    if(current_log_mode == LOG_TO_CONSOLE) 
    {
        vprintf(format, args);
    } 
    
    else if(current_log_mode == LOG_TO_FILE && log_file != NULL) 
    {
        vfprintf(log_file, format, args);
        fflush(log_file);
    }

    va_end(args);
}
