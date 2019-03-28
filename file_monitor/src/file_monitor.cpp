#include <unistd.h>
#include <pthread.h>

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <vector>

#include "file_monitor.h"
#include "stap_file_monitor.h"
#include "charset_conv.h"
#include "debug_print.h"
#include "util-string.h"
#include "common_utils.h"
#include "extra_log.h"

#ifdef DEBUG_STATISTICS
static time_t debug_time = 0;
static size_t n_file_event_alloc = 0;
static size_t n_file_event_free = 0;
#endif /* DEBUG_STATISTICS */

#define PUBLIC_LIB __attribute__ ((visibility ("default")))

#define FGETS_MAX_SIZE 10240
#define INDEX_FILE_MAX 10


#define STAP_DEFAULT_EXE "SYSTEMTAP_STAPIO=./stapio ./staprun"
#define STAP_DEFAULT_ARGS "../lib/edr_stap_file_monitor.ko -R"


struct FileMonitor
{
public:
    bool running;

    void *p_user_data;
    event_callback_t event_cb;

    std::vector<std::string> monitor_paths;
    pthread_rwlock_t lock;

    stap_monitor *p_stap_handle;
};

/* monitor paths library */
#ifdef DEBUG_MONITOR_PATHS
#include <stdio.h>
static void monitor_paths_debug(FileMonitor *p_file_monitor)
{
    printf("-------------%s-------------\n", __func__);
    pthread_rwlock_rdlock(&p_file_monitor->lock);
    for (std::vector<std::string>::iterator itr=p_file_monitor->monitor_paths.begin();
            itr!=p_file_monitor->monitor_paths.end(); itr++)
    {
        printf("%s\n", (*itr).data());
    }
    pthread_rwlock_unlock(&p_file_monitor->lock);
    printf("-------------%s-------------\n", __func__);
}
#endif /* DEBUG_MONITOR_PATHS */

static int monitor_paths_insert(FileMonitor *p_file_monitor, const char *path)
{
    bool is_exists = false;
    std::string format_path = path_format(path);
    pthread_rwlock_wrlock(&p_file_monitor->lock);
    for (std::vector<std::string>::iterator itr=p_file_monitor->monitor_paths.begin();
            itr!=p_file_monitor->monitor_paths.end(); itr++)
    {
        if (itr->compare(format_path) == 0)
        {
            is_exists = true;
        }
    }
    if (is_exists == false)
    {
        p_file_monitor->monitor_paths.push_back(format_path);
    }
    pthread_rwlock_unlock(&p_file_monitor->lock);
#ifdef DEBUG_MONITOR_PATHS
    monitor_paths_debug(p_file_monitor);
#endif /* DEBUG_MONITOR_PATHS */
    return is_exists ? 0 : 1;
}

static int monitor_paths_erase(FileMonitor *p_file_monitor, const char *path)
{
    bool is_erase= false;
    std::string format_path = path_format(path);
    pthread_rwlock_wrlock(&p_file_monitor->lock);
    for (std::vector<std::string>::iterator itr=p_file_monitor->monitor_paths.begin();
            itr!=p_file_monitor->monitor_paths.end(); )
    {
        if (itr->compare(format_path) == 0)
        {
            itr = p_file_monitor->monitor_paths.erase(itr);
            is_erase = true;
        }
        else {
            itr++;
        }
    }
    pthread_rwlock_unlock(&p_file_monitor->lock);
#ifdef DEBUG_MONITOR_PATHS
    monitor_paths_debug(p_file_monitor);
#endif /* DEBUG_MONITOR_PATHS */
    return is_erase ? 1 : 0;
}

static bool monitor_paths_contain(FileMonitor *p_file_monitor, const char *path)
{
    bool ret = false;
    std::string format_path = path_format(path);
    pthread_rwlock_rdlock(&p_file_monitor->lock);
    for (std::vector<std::string>::iterator itr=p_file_monitor->monitor_paths.begin();
            itr!=p_file_monitor->monitor_paths.end(); itr++)
    {
        std::string &s = *itr;
        if (s.size() > format_path.size())
            continue;
        if (format_path.compare(0, s.size(), s) == 0)
        {
            ret = true;
            break;
        }
    }
    pthread_rwlock_unlock(&p_file_monitor->lock);
    return ret;
}

PUBLIC_LIB void free_file_event(file_event_t *p_event)
{
#ifdef DEBUG_STATISTICS
    assert(p_event);
    n_file_event_free++;
#endif /* DEBUG_STATISTICS */

    if(p_event)
    {
        if(p_event->fullpath)
        {
            free((char *)p_event->fullpath);
        }
        if(p_event->args)
        {
            free((char *)p_event->args);
        }
        free(p_event);
    }
}

static void stap_file_event_cb_impl(const void *event, void *p_user_data)
{
    file_event_t *p_event = (file_event_t *)event;
    FileMonitor *p_file_monitor = (FileMonitor *)p_user_data;

    // if(p_event == NULL)
    // {
    //     return ;
    // }

    // if(p_event->fullpath == NULL)
    // {
    //     free_file_event(p_event);
    //     p_event = NULL;
    //     return ;
    // }

#ifdef DEBUG_STATISTICS
    if(p_event->time >= debug_time + 60)
    {
        printf("n_file_event_alloc size:[%lu]\n"
                    "n_file_event_free  size:[%lu]\n",
                    n_file_event_alloc, n_file_event_free);
        debug_time = p_event->time;
    }
#endif /* DEBUG_STATISTICS */

    // debug_print("stap_file_event_cb_impl --> %s\n", p_event->fullpath);

    p_file_monitor->event_cb(p_event, p_file_monitor->p_user_data);
    p_event = NULL;
}


static struct events
{
    /* decided by systemtap.stp "NAME", normally equivalent to api name */
    const char *p;
    event_type_t i;
} file_events[] = {
    {"open",    FILE_EVENT_OPEN},
    {"openat",  FILE_EVENT_OPEN},
    {"unlink",  FILE_EVENT_UNLINK},
    {"rename",  FILE_EVENT_RENAME},
    {"chmod",   FILE_EVENT_CHMOD},
    {"chown",   FILE_EVENT_CHOWN},
    {"mkdir",   FILE_EVENT_MKDIR},
    {"rmdir",   FILE_EVENT_RMDIR},
    {"write",   FILE_EVENT_WRITE},
    {"read",    FILE_EVENT_READ},
    {"close",   FILE_EVENT_CLOSE},
    {"creat",   FILE_EVENT_CREATE}
};

static event_type_t stap_monitor_on_file_event_event(const char *action_str)
{
    size_t i;
    for(i=0; i<sizeof(file_events)/sizeof(file_events[0]); i++)
    {
        if(strcmp(action_str, file_events[i].p) == 0)
        {
            return file_events[i].i;
        }
    }
    return FILE_EVENT_OTHER;
}

enum
{
    INDEX_TIMESTAMP = 0,
    INDEX_PPID      = 1,
    INDEX_PID       = 2,
    INDEX_ACTION    = 3,
    INDEX_FULL_PATH = 4,
    INDEX_ARGS      = 5,
    INDEX_MAX
};

static std::string path_transform(const char *p_path)
{
    std::string fullpath = path_format(p_path);

    /*
     * unicode string(like:\u4F60) transform to utf-8
     * TODO: fix a bug
     */
    if(fullpath.find("\\u") != std::string::npos)
    {
        std::string utf8path = unicode_escape_string_to_utf8_via_wchar(fullpath);
        if(utf8path.size())
        {
            return utf8path;
        }
        else
        {
            fprintf(stderr, "unicode_escape_string_to_utf8_via_wchar(%s) error:%d\n", fullpath.c_str(), errno);
        }
    }
    return fullpath;
}

static void stap_monitor_on_file_event(char **toks, int num_toks, stap_interface_t *p_itf)
{
    if(num_toks < INDEX_FULL_PATH + 1)
    {
        fprintf(stderr, "[%s] expect %d toks, actual:%d\n", __func__, INDEX_FULL_PATH + 1, num_toks);
        return ;
    }

    if(p_itf->p_callback == NULL)
    {
        return ;
    }
    file_monitor_t *p_file_monitor = (file_monitor_t *)p_itf->p_user_data;
    std::string fullpath = path_transform(toks[INDEX_FULL_PATH]);
    if(p_file_monitor)
    {
        if(!monitor_paths_contain(p_file_monitor, fullpath.c_str()))
        {
            return;
        }
    }

#ifdef DEBUG_STATISTICS
    n_file_event_alloc++;
#endif /* DEBUG_STATISTICS */

    file_event_t *p_event = (file_event_t *)calloc(1, sizeof(file_event_t));
    assert(p_event);

    p_event->time = (time_t)atol(toks[INDEX_TIMESTAMP]);
    p_event->ppid = atoi(toks[INDEX_PPID]);
    p_event->pid = atoi(toks[INDEX_PID]);
    p_event->event = stap_monitor_on_file_event_event(toks[INDEX_ACTION]);
    p_event->fullpath = strndup(fullpath.c_str(), fullpath.size());

    if(num_toks == INDEX_MAX)
    {
        p_event->args = toks[INDEX_ARGS];
        toks[INDEX_ARGS] = NULL;
    }
    if(p_event->event == FILE_EVENT_OTHER)
    {
        goto stap_monitor_on_file_event_error;
    }

    p_itf->p_callback(p_event, p_itf->p_user_data);
    return ;

stap_monitor_on_file_event_error:
    free_file_event(p_event);
    return ;
}

static void on_stap_print_line(const char *line, stap_interface_t *p_itf)
{
    int num_toks = 0;
    char **toks = str_split(line, " ", MAX_TOKS, &num_toks, 0);
    if(!toks || !num_toks)
    {
        fprintf(stderr, "%s: str_split parse[%s] failed\n", __func__, line);
        return ;
    }

    stap_monitor_on_file_event(toks, num_toks, p_itf);

    if(toks && num_toks)
        str_split_free(&toks, num_toks);
    return ;
}

static int file_monitor_looooop(stap_monitor *p_stap_handle)
{
    int result = -1;

    fd_set current_set;
    fd_set monitor_set;
    FD_ZERO(&monitor_set);

    char *stap_buffer = (char*)calloc(FGETS_MAX_SIZE, sizeof(char));

    int stap_fd = fileno(p_stap_handle->rfp);
    FD_SET(stap_fd, &monitor_set);

    result = 0;

    while(true)
    {
        if(!p_stap_handle->running)
        {
            result = 0;
            break;
        }

        current_set = monitor_set;
        select(stap_fd + 1, &current_set, NULL, NULL, NULL);

        if (FD_ISSET(stap_fd, &current_set))
        {
            if(fgets(stap_buffer, FGETS_MAX_SIZE, p_stap_handle->rfp))
            {
                if(!extra_filter(stap_buffer))
                {
                    extra_log(stap_buffer, strlen(stap_buffer));
                    on_stap_print_line(stap_buffer, &p_stap_handle->itf);
                }
            }
            else
            {
                fprintf(stderr, "ERROR: kernel module error, fgets errno:%d\n", errno);
                result = errno;
                break;
            }
        }
    }

    free(stap_buffer);
    stap_buffer = NULL;

    return result;
}

PUBLIC_LIB file_monitor_t* init_file_monitor(event_callback_t callback, void *p_user_data,
            size_t option_count, const char *(*pp_options)[2])
{
    if (NULL == pp_options || NULL == callback)
        return NULL;

    FileMonitor *p_file_monitor = new FileMonitor;
    assert(p_file_monitor);

    pthread_rwlock_init(&p_file_monitor->lock, NULL);
    p_file_monitor->p_user_data = p_user_data;
    p_file_monitor->event_cb = callback;
    p_file_monitor->running = false;
    p_file_monitor->p_stap_handle = NULL;

    for (size_t i = 0; i < option_count; i++)
    {
        if (strcmp(pp_options[i][0], "monitor_directory") == 0)
        {
            p_file_monitor->monitor_paths.push_back(path_format(pp_options[i][1]));
            debug_print("%s monitor_directory add [%s]\n", __func__, path_format(pp_options[i][1]).c_str());
        }
    }

    debug_print("%s success\n", __func__);
    return p_file_monitor;
}

/* If success function is blocked */
PUBLIC_LIB bool run_file_monitor(file_monitor_t *p_file_monitor)
{
    bool result = false;

    if(p_file_monitor == NULL)
    {
        return result;
    }

    if(p_file_monitor->p_stap_handle == NULL)
    {
        stap_interface_t itf;
        itf.p_callback = stap_file_event_cb_impl;
        itf.p_user_data = p_file_monitor;

        p_file_monitor->p_stap_handle = init_stap_monitor(STAP_DEFAULT_EXE, STAP_DEFAULT_ARGS, &itf);
        if(p_file_monitor->p_stap_handle == NULL)
        {
            return result;
        }
    }
    p_file_monitor->running = true;
    result = file_monitor_looooop(p_file_monitor->p_stap_handle);
    return result;
}

PUBLIC_LIB bool add_dir_to_watcher(file_monitor_t *p_file_monitor, const char *p_directory)
{
    if (p_file_monitor && p_directory)
    {
        monitor_paths_insert(p_file_monitor, p_directory);
        debug_print("add [%s] to monitor done!\n", p_directory);
        return true;
    }
    return false;
}

PUBLIC_LIB bool remove_dir_from_watcher(file_monitor_t *p_file_monitor,const char *p_directory)
{
    if (p_file_monitor && p_directory)
    {
        monitor_paths_erase(p_file_monitor, p_directory);
        debug_print("remove [%s] to monitor done!\n", p_directory);
        return true;
    }
    return false;
}

PUBLIC_LIB bool add_to_white_list(file_monitor_t * p_file_monitor,const char * p_path)
{
    if (p_file_monitor && p_path)
    {
        // TODO
        debug_print("add [%s] to white list done!\n", p_path);
        return true;
    }
    return false;
}

PUBLIC_LIB bool remove_from_white_list(file_monitor_t *p_file_monitor, const char *p_path)
{
    if (p_file_monitor && p_path)
    {
        // TODO
        debug_print("remove [%s] from white list done!\n", p_path);
        return true;
    }
    return false;
}

PUBLIC_LIB void stop_file_monitor(file_monitor_t *p_file_monitor)
{
    if (p_file_monitor)
    {
		p_file_monitor->running = false;

        if (p_file_monitor->p_stap_handle)
        {
            fin_stap_monitor(p_file_monitor->p_stap_handle);
            p_file_monitor->p_stap_handle = NULL;
        }
    }
}

PUBLIC_LIB void destroy_file_monitor(file_monitor_t *p_file_monitor)
{

    if (p_file_monitor)
    {
        stop_file_monitor(p_file_monitor);
        pthread_rwlock_destroy(&p_file_monitor->lock);

        delete p_file_monitor;
    }
}
