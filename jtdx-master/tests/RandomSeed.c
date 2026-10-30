#ifdef NDEBUG
#undef NDEBUG
#endif
#define _CRT_RAND_S
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

static int scenario, reads;
static unsigned char expected[] = {0x1a, 0x42, 0x00, 0xff};
static ssize_t simulated_read(int source, void *buffer, size_t count)
{
  (void)source;
  ++reads;
  assert(reads <= 8); /* Detect the original EOF/error infinite loop. */
  if (scenario == 1) return 0;
  if (scenario == 2) { errno = EIO; return -1; }
  if (scenario == 3 && reads == 1) { errno = EINTR; return -1; }
  if (scenario == 4 && reads > 1) return 0;
  unsigned offset = (unsigned)(reads - 1 - (scenario == 3));
  assert(count == sizeof expected - offset);
  *(unsigned char *)buffer = expected[offset];
  return 1;
}
#define read simulated_read
#define JTDXF_RANDOM_SEED_TEST
#include "../lib/init_random_seed.c"
#undef read

int main(void)
{
  struct {unsigned before, seed, after;} guarded;
  assert(sizeof guarded.seed == sizeof expected);
  for (scenario = 0; scenario < 5; ++scenario)
    {
      guarded.before = 0x12345678; guarded.seed = 0; guarded.after = 0xabcdef01;
      reads = 0;
      int success = read_seed_bytes(0, &guarded.seed);
      assert(success == (scenario == 0 || scenario == 3));
      assert(guarded.before == 0x12345678 && guarded.after == 0xabcdef01);
      if (success) assert(!memcmp(&guarded.seed, expected, sizeof expected));
    }
#ifdef _WIN32
  /* Exercise the native Windows entropy path too. */
  for (int i = 0; i < 100; ++i) init_random_seed();
#endif
  puts("Random seed: short reads, EOF, errors, EINTR and memory bounds passed");
  return 0;
}
