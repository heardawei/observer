#ifndef __EXTRA_LOG_H__
#define __EXTRA_LOG_H__

#include <stddef.h>
#include <stdbool.h>

bool extra_filter(const char *buff);
ssize_t extra_log(const char *buff, size_t count);

#endif /* __EXTRA_LOG_H__ */