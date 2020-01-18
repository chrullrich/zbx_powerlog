/* vim: set ts=4 sw=4 expandtab autoindent fileformat=unix: */
#ifndef __POWERLOG_H__

#include <stdint.h>
#include <time.h>

typedef struct {
	char lockfile[64];
	time_t updated;
    uint32_t interval;
	uint32_t count;
	unsigned short parameters[1];
} pipowermon_shmem;

#endif /* !__POWERLOG_H__ */

