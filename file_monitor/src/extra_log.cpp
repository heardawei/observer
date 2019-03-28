
#ifdef EXTRA_LOG

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#define EXTRA_LOG_FILE "/tmp/file_monitor.log"

static int _fd = -1;
static size_t fsize = 0;
static size_t fnum = 0;
static char fname[1024] = { 0 };

bool extra_filter(const char *buff)
{
    if(fname[0] && strstr(buff, fname))
    {
        return true;
    }
    return false;
}

ssize_t extra_log(const char *buff, size_t count)
{
    if(_fd == -1)
    {
        snprintf(fname, sizeof(fname), "%s.%lu", EXTRA_LOG_FILE, fnum);
        _fd = open(fname, O_CREAT | O_TRUNC | O_WRONLY, 0777);
        if(_fd == -1)
        {
            return 0;
        }
        fsize = 0;
        fnum++;
    }
    ssize_t nwrite = write(_fd, buff, count);
    if(nwrite > 0)
    {
        if((fsize += nwrite) > 1024 * 1024 * 10)
        {
            close(_fd);
            _fd = -1;
            fname[0] = '\0';
        }
    }

    return nwrite;
}

#else /* EXTRA_LOG */
#include <sys/types.h>
#include <stdbool.h>

bool extra_filter(const char *buff)
{
    return false;
}

ssize_t extra_log(const char *buff, size_t count)
{
    return 0;
}

#endif /* EXTRA_LOG */