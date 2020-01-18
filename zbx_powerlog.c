/* vim: set ts=4 sw=4 expandtab autoindent fileformat=unix: */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>
#include <time.h>
#include <stdbool.h>
#include <stdint.h>
#include <netinet/in.h>     /* ntohl(); I'm lazy. */

#include "sysinc.h"
#include "module.h"

#include "powerlog.h"

#define SHM_NAME "/pipowermon"

int powerlog_register_value(AGENT_REQUEST*, AGENT_RESULT*);

static ZBX_METRIC items[] = {
	{ "modbus.register.value", CF_HAVEPARAMS, powerlog_register_value, "Test1" },
	{ NULL }
};

int item_timeout = 0;
int shm_fd = -1;
int lock_fd = -1;
pipowermon_shmem *shm;

int zbx_module_api_version(void)
{
	return ZBX_MODULE_API_VERSION_ONE;
}

int zbx_module_init(void)
{
	shm_fd = shm_open(SHM_NAME, O_RDONLY|O_CREAT, S_IRUSR|S_IWUSR);
	if (shm_fd == -1) {
		return ZBX_MODULE_FAIL;
	}

	shm = (pipowermon_shmem*)mmap(NULL, 
                                 sizeof(pipowermon_shmem),
                                 PROT_READ,
                                 MAP_SHARED,
                                 shm_fd,
                                 0);

	if (shm == (void*)-1 || shm->lockfile[0] == 0) {
		return ZBX_MODULE_FAIL;
	}

    lock_fd = open(shm->lockfile, O_RDWR);
    if (lock_fd == -1) {
        return ZBX_MODULE_FAIL;
    }

    return ZBX_MODULE_OK;
}

int zbx_module_uninit(void)
{
    if (shm_fd != -1) {
        close(shm_fd);
    }
    close(lock_fd);

	munmap((void*)shm, sizeof(pipowermon_shmem));
	return ZBX_MODULE_OK;
}

ZBX_METRIC *zbx_module_item_list(void)
{
	return items;
}

void zbx_module_item_timeout(int timeout)
{
	item_timeout = timeout;
}

/* TODO: Timeout */
static void powerlog_lock(int timeout)
{
	flock(lock_fd, LOCK_EX);
}

static void powerlog_unlock(int timeout)
{
	flock(lock_fd, LOCK_UN);
}

/* Maximum age of measurement is twice the poll interval. */
static bool is_updated()
{
    return ((shm->updated != 0)
            && (shm->updated - time(NULL)) < (shm->interval * 2));
}

int powerlog_register_value(AGENT_REQUEST *req, AGENT_RESULT *res)
{
	int status = SYSINFO_RET_FAIL;

	if (is_updated() && req->nparam == 2) {
        unsigned long index = strtoul(get_rparam(req, 0), NULL, 10);
        const char* type = get_rparam(req, 1);
		powerlog_lock(item_timeout);
        if (strcasecmp(type, "int32") == 0) {
            uint32_t reg = 0;
            uint16_t* raw = &shm->parameters[index];
            reg = ntohs(*raw) << 16;
            reg |= ntohs(*++raw);
            SET_UI64_RESULT(res, reg);
            status = SYSINFO_RET_OK;
        } else if (strcasecmp(type, "float32") == 0) {
            SET_DBL_RESULT(res, *(float*)(&shm->parameters[index]));
            status = SYSINFO_RET_OK;
        }
		powerlog_unlock(item_timeout);
	}

	return status;
}

