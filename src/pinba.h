/* Copyright (c) 2007-2009 Antony Dovgal <tony@daylessday.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef PINBA_H
#define PINBA_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

extern "C" {
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <event.h>
#include <math.h>
#include <sys/time.h>
#include <pthread.h>
#include <Judy.h>
#include <event.h>
}

#include "pinba-pb.h"
#include "pinba_config.h"
#include "threadpool.h"
#include "pinba_types.h"

#undef P_SUCCESS
#undef P_FAILURE
#define P_SUCCESS 0
#define P_FAILURE -1


#define P_ERROR        (1<<0L)
#define P_WARNING      (1<<1L)
#define P_NOTICE       (1<<2L)
#define P_DEBUG        (1<<3L)
#define P_DEBUG_DUMP   (1<<4L)

char *pinba_error_ex(int return_error, int type, const char *file, int line, const char *format, ...);

#ifdef PINBA_DEBUG
#define pinba_debug(...) pinba_error_ex(0, P_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#else
#define pinba_debug(...) 
#endif

#define pinba_error(type, ...) pinba_error_ex(0, type, __FILE__, __LINE__, __VA_ARGS__)
#define pinba_error_get(type, ...) pinba_error_ex(1, type, __FILE__, __LINE__, __VA_ARGS__)
extern pinba_daemon *D;

#ifdef __GNUC__
#define LIKELY(x)       __builtin_expect((x),1)
#define UNLIKELY(x)     __builtin_expect((x),0)
#else
#define LIKELY(x)       x
#define UNLIKELY(x)     x
#endif

void *pinba_data_main(void *arg);
void *pinba_collector_main(void *arg);
void *pinba_stats_main(void *arg);
int pinba_collector_init(pinba_daemon_settings settings);
void pinba_collector_shutdown();
int pinba_get_processors_number();

int pinba_process_stats_packet(const unsigned char *buffer, int buffer_len);

void pinba_udp_read_callback_fn(int sock, short event, void *arg);
void pinba_socket_free(pinba_socket *socket);
pinba_socket *pinba_socket_open(char *ip, int listen_port);

void pinba_tag_dtor(pinba_tag *tag);
int pinba_tag_put(const unsigned char *name);
pinba_tag *pinba_tag_get_by_name(const unsigned char *name);
pinba_tag *pinba_tag_get_by_name_next(unsigned char *name);
pinba_tag *pinba_tag_get_by_id(size_t id);
void pinba_tag_delete_by_name(const unsigned char *name);
void pinba_tag_delete_by_id(size_t id);

int pinba_tag_values_put(pinba_timer_record *timer, unsigned int *values, int *values_lens, int values_num);
void pinba_tag_value_delete(size_t timer_id);

void pinba_update_reports_add(const pinba_stats_record *record); 
void pinba_update_reports_delete(const pinba_stats_record *record);
void pinba_update_tag_reports_add(int request_id, const pinba_stats_record *record);
void pinba_update_tag_reports_delete(int request_id, const pinba_stats_record *record);
void pinba_reports_destroy(void);
void pinba_tag_reports_destroy(int force);

/* go over all new records in the pool */
#define pool_traverse_forward(i, pool) \
		for (i = (pool)->out; i != (pool)->in; i = (i == (pool)->size - 1) ? 0 : i + 1)

/* go over all records in the pool */
#define pool_traverse_backward(i, pool) \
			for (i = ((pool)->in > 0) ? (pool)->in - 1 : 0; \
                 i != ((pool)->out ? (pool)->out : ((pool)->in ? ((pool)->size - 1) : 0)); \
                 i = (i == 0) ? ((pool)->size - 1) : i - 1)

#define TMP_POOL(pool) ((pinba_tmp_stats_record *)((pool)->data))
#define DATA_POOL(pool) ((pinba_data_bucket *)((pool)->data))
#define REQ_POOL(pool) ((pinba_stats_record *)((pool)->data))
#define TIMER_POOL(pool) ((pinba_timer_position *)((pool)->data))
#define POOL_DATA(pool) ((void **)((pool)->data))

#define memcpy_static(buf, str, str_len, result_len)	\
do {										\
	if (sizeof(buf) <= (unsigned int)str_len) { 	\
		/* truncate the string */			\
		memcpy(buf, str, sizeof(buf) - 1);	\
		buf[sizeof(buf) - 1] = '\0';		\
		result_len = sizeof(buf) - 1;       \
	} else {								\
		memcpy(buf, str, str_len);			\
		buf[str_len] = '\0';				\
		result_len = str_len;               \
	}										\
} while(0)

#define memcat_static(buf, plus, str, str_len, result_len)	\
do {										\
	register int __n = sizeof(buf);			\
											\
	if ((plus) >= __n) {					\
		break;								\
	}										\
											\
	if ((__n - (plus) - 1) < str_len) {		\
		/* truncate the string */			\
		memcpy(buf + (plus), str, __n - (plus)); \
		buf[__n - 1] = '\0';				\
		result_len = __n - 1;				\
	} else {								\
		memcpy(buf + (plus), str, str_len);	\
		buf[(plus) + str_len] = '\0';		\
		result_len = (plus) + str_len;		\
	}										\
} while(0)

size_t pinba_pool_num_records(pinba_pool *p);
int pinba_pool_init(pinba_pool *p, size_t size, size_t element_size, pool_dtor_func_t dtor);
void pinba_pool_destroy(pinba_pool *p);


/* utility macros */

#define timeval_to_float(tv) (float)tv.tv_sec + (float)tv.tv_usec / 1000000.0

static inline pinba_timeval float_to_timeval(double f) /* {{{ */
{
	pinba_timeval t;
	double fraction, integral;

	fraction = modf(f, &integral);
	t.tv_sec = (int)integral;
	t.tv_usec = (int)(fraction*1000000);
	return t;
}
/* }}} */

#define pinba_pool_is_full(pool) ((pool->in < pool->out) ? pool->size - (pool->out - pool->in) : (pool->in - pool->out)) == (pool->size - 1)

void pinba_data_pool_dtor(void *pool); 
void pinba_temp_pool_dtor(void *pool); 
void pinba_request_pool_dtor(void *pool); 

#ifndef PINBA_ENGINE_HAVE_STRNDUP
char *pinba_strndup(const char *s, unsigned int length);
#define strndup pinba_strndup
#endif

#endif /* PINBA_H */

/* 
 * vim600: sw=4 ts=4 fdm=marker
 */
