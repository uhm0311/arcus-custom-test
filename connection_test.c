#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

#include "libmemcached/memcached.h"

#define USE_REPLICATION 1

#if USE_REPLICATION
static const char *zkadmin_addr = "jam2in-s003:2181,jam2in-s004:2181,jam2in-s005:2181";
#else
static const char *zkadmin_addr = "jam2in-m002:2181";
#endif
static const char *service_code = "test";

#define MY_POOL_SIZE 10000
#define MY_SLEEP_SEC 1000000000

memcached_st *global_mc = NULL;

int
main(int argc __attribute__((unused)), char** argv __attribute__((unused)))
{
  int ret = 0;
  int x;
  int min_size = MY_POOL_SIZE;
  int max_size = MY_POOL_SIZE;
  pthread_t tstat;

  memcached_pool_st *pool = NULL;
  arcus_return_t arc = ARCUS_ERROR;

  do {
    global_mc = memcached_create(NULL);
    if (!global_mc) {
      fprintf(stderr, "memcached_create failed\n");
      ret = 1;
      break;
    }

    pool = memcached_pool_create(global_mc, min_size, max_size);
    if (!pool) {
      fprintf(stderr, "memcached_pool_create failed\n");
      ret = 1;
      break;
    }

    arc = arcus_pool_connect(pool, zkadmin_addr, service_code);
    if (arc != ARCUS_SUCCESS) {
      fprintf(stderr, "arcus_connect() failed, reason=%s\n", arcus_strerror(arc));
      ret = 1;
      break;
    }

    for (uint64_t i = 0; i < MY_SLEEP_SEC; i++) {
      sleep(1);
    }
  } while (0);

  if (pool) {
    if (arc == ARCUS_SUCCESS) {
      arcus_pool_close(pool);
    }

    memcached_pool_destroy(pool);
  }

  if (global_mc) {
    memcached_free(global_mc);
  }

  return ret;
}