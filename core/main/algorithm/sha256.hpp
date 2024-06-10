#ifndef SHA256_H_
#define SHA256_H_

#include <cstdint>
#include <cstdio>

#define SHA256_HASH_SIZE 32

struct Sha256Context;

void Sha256Initialise (Sha256Context *);
void Sha256Update (Sha256Context *, void const *, uint32_t);
void Sha256Finalise (Sha256Context *, uint8_t[SHA256_HASH_SIZE]);
void Sha256Calculate (void const *, uint32_t, uint8_t[SHA256_HASH_SIZE]);

#endif // SHA256_H_