#ifndef INIT_RANDOM_SEED_H__
#define INIT_RANDOM_SEED_H__

#ifdef __cplusplus
extern "C" {
#endif

  /*
   * Seed the PRNG from operating-system entropy when available, otherwise
   * from time and PID. Seed uniqueness is not guaranteed.
   *
   * Not suitable for multi-threaded requirements
   */
  void init_random_seed (void);

#ifdef __cplusplus
}
#endif

#endif
