#ifndef _MODULE_INTERFACE_
#define _MODULE_INTERFACE_

#ifdef __cplusplus
extern "C"
{
#endif

#include <inttypes.h>
#include <stdbool.h>

#include <time.h>

typedef enum
{
    FILE_EVENT_OPEN = 1,
    FILE_EVENT_UNLINK,
    FILE_EVENT_RENAME,
    FILE_EVENT_CHMOD ,
    FILE_EVENT_CHOWN ,
    FILE_EVENT_MKDIR ,
    FILE_EVENT_RMDIR ,
    FILE_EVENT_WRITE ,
    FILE_EVENT_READ  ,
    FILE_EVENT_CLOSE ,
    FILE_EVENT_CREATE,
    FILE_EVENT_OTHER
} event_type_t;

typedef struct
{
    event_type_t event;
    int         pid;
    int         ppid;
    time_t      time;
    // absolute
    const char *fullpath;
    const char *args;
} file_event_t;

typedef struct FileMonitor file_monitor_t;

typedef bool (*event_callback_t)(file_event_t *p_event, void *p_usr_data);

void free_file_event(file_event_t *p_event);

file_monitor_t* init_file_monitor(event_callback_t callback, void *p_user_data,
    size_t option_count, const char *(*pp_options)[2]);

/* If success function is blocked */
bool run_file_monitor(file_monitor_t *p_file_monitor);

void stop_file_monitor(file_monitor_t *p_file_monitor);

void destroy_file_monitor(file_monitor_t *p_file_monitor);



bool add_dir_to_watcher(file_monitor_t *p_file_monitor, const char *p_directory);
bool remove_dir_from_watcher(file_monitor_t *p_file_monitor, const char *p_directory);
bool add_to_white_list(file_monitor_t *p_file_monitor, const char *p_path);
bool remove_from_white_list(file_monitor_t *p_file_monitor, const char *p_path);

#ifdef __cplusplus
}
#endif
#endif
