#include "init_random_seed.h"

#define _CRT_RAND_S
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

/* basic PRNG to use for improving the basic seed selection */
static unsigned lcg (uint64_t seed)
{
  if (0 ==seed)
    {
      seed = UINT64_C(104729);
    }
  else
    {
      seed %= UINT64_C(4294967296);
    }
  seed = (seed * UINT64_C(279470273)) % UINT64_C(4294967291);
  return seed % UINT64_MAX;
}

#if !defined(_WIN32) || defined(JTDXF_RANDOM_SEED_TEST)
/* read() advances in bytes; a short read, EOF or error must never spin or
 * write beyond the seed. Interrupted reads are safe to retry. */
static int read_seed_bytes(int source, unsigned *seed)
{
  size_t length = 0;
  while (length < sizeof *seed)
    {
      ssize_t result = read(source, (unsigned char *)seed + length, sizeof *seed - length);
      if (result < 0 && errno == EINTR) continue;
      if (result <= 0) return 0;
      length += (size_t)result;
    }
  return 1;
}
#endif

/* Generate a good PRNG seed value */
void init_random_seed(void)
{
  unsigned seed = 0u;
  int have_seed = 0;

#ifdef _WIN32
  /* Windows CRT text reads can interpret random 0x1a as EOF. Use its native
   * entropy API instead of treating a POSIX device path as a Windows file. */
  have_seed = rand_s(&seed) == 0;
#else
  int random_source = open("/dev/urandom", O_RDONLY);
  if (random_source >= 0)
    {
      have_seed = read_seed_bytes(random_source, &seed);
      close(random_source);
    }
#endif
  if (!have_seed)
    {
      // fallback to combining the time and PID in a fairly random way
      pid_t pid = getpid ();
      struct timeval tv;
      gettimeofday (&tv, NULL);
      seed = (unsigned)(((unsigned)pid << 16)
        ^ (unsigned)pid
        ^ (unsigned)tv.tv_sec
        ^ (unsigned)tv.tv_usec);
      seed = lcg (seed);
    }
  srand (seed);
}

#ifdef TEST
#include <stdio.h>

int main (int argc, char * argv[])
{
  init_random_seed ();
  int i, j;
  int r[10][4];
  for (i = 0; i < 10; ++i)
    {
      for (j = 0; j < 4; ++j)
        {
          printf ("%10d ", rand ());
        }
      printf ("\n");
    }
  return 0;
}
#endif
