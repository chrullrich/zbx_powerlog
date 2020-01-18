CPPFLAGS = -I${ZBX_INCLUDE}
LDFLAGS = -lrt -rpath /usr/local/lib -shared
CFLAGS = -fPIC -std=gnu11 -Wall -Werror

ZBX_INCLUDE = ../zabbix-4.0.0/include
ZBX_CONFIG_H = ${ZBX_INCLUDE}/config.h

.ifdef DEBUG
CFLAGS += -g
.else
CFLAGS += -O2
.endif

all: libzbx_powerlog.so

clean:
	-rm -f zbx_powerlog.o libzbx_powerlog.so

${ZBX_CONFIG_H}: ${ZBX_CONFIG_H}.in
	cp $> $@

libzbx_powerlog.so: zbx_powerlog.c ${ZBX_INCLUDE}/config.h
	${CC} ${CPPFLAGS} ${CFLAGS} -o zbx_powerlog.o -c zbx_powerlog.c
	${CC} ${LDFLAGS} -o libzbx_powerlog.so zbx_powerlog.o -lrt

