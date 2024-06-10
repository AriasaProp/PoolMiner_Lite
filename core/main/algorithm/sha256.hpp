#ifndef SHA256_H_
#define SHA256_H_

#include <cstdint>
#include <cstdio>

#define SHA256_HASH_SIZE 32

namespace Sha256 {

struct Context {
  uint32_t curlen;
  uint64_t length;
  uint32_t state[8];
  uint8_t buf[64];
};

void Initialise (Context *);
void Update (Context *, void const *, uint32_t);
void Finalise (Context *, uint8_t[SHA256_HASH_SIZE]);
void Calculate (void const *, uint32_t, uint8_t[SHA256_HASH_SIZE]);

}
#endif // SHA256_H_