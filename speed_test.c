#define POOL_SIZE 100
#define SERVER_COUNT 10
#define LOOP 4
#define USE_REPLICATION 0
#define RANDOM_RGROUPNAME 0
#define PRINT_POOL 0
#define WARM_UP 0
#define DO_REPOPULATE 0
#define HASH_COLLISION 0
#define DO_UPDATE_SERVERLIST 1
#define GROUPNAME_SIZE 100

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

#include "libmemcached/memcached.h"

typedef struct memcached_server_info_st
{
#ifdef ENABLE_REPLICATION
  char *groupname;
#endif
  char *hostname;
  unsigned short port;
#ifdef ENABLE_REPLICATION
  bool master;
#endif
  bool exist;
} memcached_server_info_st;

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

memcached_st *global_mc = NULL;
char **groupnames = NULL;

int servercount = 0;
memcached_server_info_st *serverinfo = NULL;

static char *rand_string(char *str, size_t size)
{
  const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  if (size)
  {
    --size;
    for (size_t n = 0; n < size; n++)
    {
      int key = rand() % (int)(sizeof charset - 1);
      str[n] = charset[key];
    }
    str[size] = '\0';
  }
  return str;
}

char *rand_string_alloc(size_t size)
{
  char *s = (char *)malloc(size + 1);
  if (s)
  {
    rand_string(s, size);
  }
  return s;
}

void free_groupnames()
{
  if (groupnames)
  {
    for (int i = 0; i < servercount; i++)
    {
      free(groupnames[i]);
    }
    free(groupnames);
  }
}

void init_master_serverlist()
{
  if (serverinfo) {
    free(serverinfo);
  }
  servercount = SERVER_COUNT;
  serverinfo = (memcached_server_info_st *)(malloc(sizeof(memcached_server_info_st) * servercount));

  free_groupnames();
  groupnames = (char **)malloc(sizeof(char *) * servercount);

  for (int i = 0; i < servercount; i++)
  {
#if RANDOM_RGROUPNAME
    groupnames[i] = rand_string_alloc(GROUPNAME_SIZE);
#else
    groupnames[i] = (char *)malloc(sizeof(char) * GROUPNAME_SIZE);
    snprintf(groupnames[i], GROUPNAME_SIZE, "group %d", i);
#endif
  }

  for (int i = 0; i < servercount; i += 2)
  {
    serverinfo[i].groupname = groupnames[i];
    serverinfo[i].master = true;
    serverinfo[i + 1].groupname = groupnames[i];
    serverinfo[i + 1].master = false;
    
    serverinfo[i].hostname = "localhost";
    serverinfo[i + 1].hostname = "localhost";
#if HASH_COLLISION
    serverinfo[i].port = i % 65536;
    serverinfo[i + 1].port = (i + 1) % 65536;
#else
    serverinfo[i].port = rand() % 65536;
    serverinfo[i + 1].port = rand() % 65536;
#endif
    serverinfo[i].exist = false;
    serverinfo[i + 1].exist = false;
  }
  memcached_update_cachelist(global_mc, serverinfo, servercount, NULL);
}

double diffTimeval(struct timeval startTime, struct timeval endTime)
{
  return (endTime.tv_sec - startTime.tv_sec) + ((endTime.tv_usec - startTime.tv_usec) / 1000000.0);
}

int main(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
  int ret = 0;
  int min_size = POOL_SIZE;
  int max_size = POOL_SIZE;

  memcached_pool_st *pool = NULL;

  struct timeval startTime, endTime;
  double elapsed = 0;

  srand((unsigned int)time(NULL));

  do
  {
    global_mc = memcached_create(NULL);
    if (!global_mc)
    {
      fprintf(stderr, "memcached_create failed\n");
      ret = 1;
      break;
    }
#if HASH_COLLISION
    memcached_behavior_set(global_mc, MEMCACHED_BEHAVIOR_SORT_HOSTS, 1);
    memcached_behavior_set(global_mc, MEMCACHED_BEHAVIOR_KETAMA_WEIGHTED, 1);
    memcached_behavior_set(global_mc, MEMCACHED_BEHAVIOR_DISTRIBUTION, MEMCACHED_DISTRIBUTION_CONSISTENT_KETAMA_SPY);
    memcached_behavior_set_key_hash(global_mc, MEMCACHED_HASH_MD5);
#endif
#if USE_REPLICATION
    memcached_behavior_set(global_mc, MEMCACHED_BEHAVIOR_SORT_HOSTS, 1);
    memcached_behavior_set(global_mc, MEMCACHED_BEHAVIOR_KETAMA_WEIGHTED, 1);
    memcached_behavior_set(global_mc, MEMCACHED_BEHAVIOR_DISTRIBUTION, MEMCACHED_DISTRIBUTION_CONSISTENT_KETAMA_SPY);
    memcached_behavior_set_key_hash(global_mc, MEMCACHED_HASH_MD5);
    memcached_enable_replication(global_mc);
#endif

    pool = memcached_pool_create(global_mc, min_size, max_size);
    if (!pool)
    {
      fprintf(stderr, "memcached_pool_create failed\n");
      ret = 1;
      break;
    }

#if WARM_UP
    init_master_serverlist();
    memcached_pool_repopulate(pool);
#endif

#if DO_REPOPULATE
    for (int i = 0; i < LOOP; i++)
    {
      init_master_serverlist();

      gettimeofday(&startTime, NULL);
      memcached_pool_repopulate(pool);
      gettimeofday(&endTime, NULL);

#if PRINT_POOL
      memcached_pool_print(pool);
#endif
      elapsed += diffTimeval(startTime, endTime);
    }
    printf("memcached_pool_repopulate: %lfs\n", elapsed / LOOP);
    elapsed = 0;
#endif

#if DO_UPDATE_SERVERLIST
    for (int i = 0; i < LOOP; i++)
    {
      init_master_serverlist();

      gettimeofday(&startTime, NULL);
      memcached_pool_update_cachelist(pool, serverinfo, servercount, false);
      gettimeofday(&endTime, NULL);

#if PRINT_POOL
      memcached_pool_print(pool);
#endif
      elapsed += diffTimeval(startTime, endTime);
    }
    printf("memcached_pool_update_serverlist: %lfs\n", elapsed / LOOP);
#endif
  } while (0);

  if (RANDOM_RGROUPNAME)
  {
    free_groupnames();
  }

  if (serverinfo)
  {
    free(serverinfo);
  }

  if (pool)
  {
    memcached_pool_destroy(pool);
  }

  if (global_mc)
  {
    memcached_free(global_mc);
  }

  return ret;
}