#include "sha256.hpp"
#include <memory.h>

#define ror(value, bits) (((value) >> (bits)) | ((value) << (32 - (bits))))

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

// The K array
static const uint32_t K[64] = {0x428a2f98UL, 0x71374491UL, 0xb5c0fbcfUL, 0xe9b5dba5UL, 0x3956c25bUL, 0x59f111f1UL, 0x923f82a4UL, 0xab1c5ed5UL, 0xd807aa98UL, 0x12835b01UL, 0x243185beUL, 0x550c7dc3UL, 0x72be5d74UL, 0x80deb1feUL, 0x9bdc06a7UL, 0xc19bf174UL, 0xe49b69c1UL, 0xefbe4786UL, 0x0fc19dc6UL, 0x240ca1ccUL, 0x2de92c6fUL, 0x4a7484aaUL, 0x5cb0a9dcUL, 0x76f988daUL, 0x983e5152UL, 0xa831c66dUL, 0xb00327c8UL, 0xbf597fc7UL, 0xc6e00bf3UL, 0xd5a79147UL, 0x06ca6351UL, 0x14292967UL, 0x27b70a85UL, 0x2e1b2138UL, 0x4d2c6dfcUL, 0x53380d13UL, 0x650a7354UL, 0x766a0abbUL, 0x81c2c92eUL, 0x92722c85UL, 0xa2bfe8a1UL, 0xa81a664bUL, 0xc24b8b70UL, 0xc76c51a3UL, 0xd192e819UL, 0xd6990624UL, 0xf40e3585UL, 0x106aa070UL, 0x19a4c116UL, 0x1e376c08UL, 0x2748774cUL, 0x34b0bcb5UL, 0x391c0cb3UL, 0x4ed8aa4aUL, 0x5b9cca4fUL, 0x682e6ff3UL, 0x748f82eeUL, 0x78a5636fUL, 0x84c87814UL, 0x8cc70208UL, 0x90befffaUL, 0xa4506cebUL, 0xbef9a3f7UL, 0xc67178f2UL};

#define BLOCK_SIZE 64

// Various logical functions
#define Ch(x, y, z) (z ^ (x & (y ^ z)))
#define Maj(x, y, z) (((x | y) & z) | (x & y))
#define S(x, n) ror ((x), (n))
#define R(x, n) (((x)&0xFFFFFFFFUL) >> (n))
#define Sigma0(x) (S (x, 2) ^ S (x, 13) ^ S (x, 22))
#define Sigma1(x) (S (x, 6) ^ S (x, 11) ^ S (x, 25))
#define Gamma0(x) (S (x, 7) ^ S (x, 18) ^ R (x, 3))
#define Gamma1(x) (S (x, 17) ^ S (x, 19) ^ R (x, 10))

#define Sha256Round(a, b, c, d, e, f, g, h, i)      \
  t0 = h + Sigma1 (e) + Ch (e, f, g) + K[i] + W[i]; \
  t1 = Sigma0 (a) + Maj (a, b, c);                  \
  d += t0;                                          \
  h = t0 + t1;

static void TransformFunction (Sha256::Context *c, uint8_t const *Buffer) {
  uint32_t S[8];
  uint32_t W[64];
  uint32_t t0;
  uint32_t t1;
  uint32_t t;
  size_t i;

  // Copy state into S
  for (i = 0; i < 8; ++i) {
    S[i] = c->state[i];
  }

  // Copy the state into 512-bits into W[0..15]
  for (i = 0; i < 16; ++i) {
    W[i] = *(Buffer + (i*4));
    W[i] <<= 8;
    W[i] |= *(Buffer + (i*4) + 1);
    W[i] <<= 8;
    W[i] |= *(Buffer + (i*4) + 2);
    W[i] <<= 8;
    W[i] |= *(Buffer + (i*4) + 3);
  }

  // Fill W[16..63]
  for (i = 16; i < 64; ++i) {
    W[i] = Gamma1 (W[i - 2]) + W[i - 7] + Gamma0 (W[i - 15]) + W[i - 16];
  }

  // Compress
  for (i = 0; i < 64; ++i) {
    Sha256Round (S[0], S[1], S[2], S[3], S[4], S[5], S[6], S[7], i);
    t = S[7];
    S[7] = S[6];
    S[6] = S[5];
    S[5] = S[4];
    S[4] = S[3];
    S[3] = S[2];
    S[2] = S[1];
    S[1] = S[0];
    S[0] = t;
  }

  // Feedback
  for (i = 0; i < 8; ++i) {
    c->state[i] = c->state[i] + S[i];
  }
}

void Sha256::Initialise (Sha256::Context *c) {
  c->curlen = 0;
  c->length = 0;
  c->state[0] = 0x6A09E667UL;
  c->state[1] = 0xBB67AE85UL;
  c->state[2] = 0x3C6EF372UL;
  c->state[3] = 0xA54FF53AUL;
  c->state[4] = 0x510E527FUL;
  c->state[5] = 0x9B05688CUL;
  c->state[6] = 0x1F83D9ABUL;
  c->state[7] = 0x5BE0CD19UL;
}

void Sha256::Update (Sha256::Context *c, void const *Buffer, uint32_t BufferSize) {
  uint32_t n;

  if (c->curlen > sizeof (c->buf)) {
    return;
  }

  while (BufferSize > 0) {
    if (c->curlen == 0 && BufferSize >= BLOCK_SIZE) {
      TransformFunction (c, (uint8_t *)Buffer);
      c->length += BLOCK_SIZE * 8;
      Buffer = (uint8_t *)Buffer + BLOCK_SIZE;
      BufferSize -= BLOCK_SIZE;
    } else {
      n = MIN (BufferSize, (BLOCK_SIZE - c->curlen));
      memcpy (c->buf + c->curlen, Buffer, (size_t)n);
      c->curlen += n;
      Buffer = (uint8_t *)Buffer + n;
      BufferSize -= n;
      if (c->curlen == BLOCK_SIZE) {
        TransformFunction (c, c->buf);
        c->length += 8 * BLOCK_SIZE;
        c->curlen = 0;
      }
    }
  }
}

void Sha256::Finalise (Sha256::Context *c, uint8_t Digest[SHA256_HASH_SIZE]) {
  int i;

  if (c->curlen >= sizeof (c->buf)) {
    return;
  }

  // Increase the length of the message
  c->length += c->curlen * 8;

  // Append the '1' bit
  c->buf[c->curlen++] = (uint8_t)0x80;

  // if the length is currently above 56 bytes we append zeros
  // then compress.  Then we can fall back to padding zeros and length
  // encoding like normal.
  if (c->curlen > 56) {
    while (c->curlen < 64) {
      c->buf[c->curlen++] = (uint8_t)0;
    }
    TransformFunction (c, c->buf);
    c->curlen = 0;
  }

  // Pad up to 56 bytes of zeroes
  while (c->curlen < 56) {
    c->buf[c->curlen++] = (uint8_t)0;
  }

  // Store length
  c->buf[56] = (uint8_t)((c->length >> 56) & 255);
  c->buf[57] = (uint8_t)((c->length >> 48) & 255);
  c->buf[58] = (uint8_t)((c->length >> 40) & 255);
  c->buf[59] = (uint8_t)((c->length >> 32) & 255);
  c->buf[60] = (uint8_t)((c->length >> 24) & 255);
  c->buf[61] = (uint8_t)((c->length >> 16) & 255);
  c->buf[62] = (uint8_t)((c->length >>  8) & 255);
  c->buf[63] = (uint8_t)( c->length        & 255);
  TransformFunction (c, c->buf);

  // Copy output
  for (i = 0; i < 8; ++i) {
    *(Digest + (4 * i))     = (uint8_t)((c->state[i] >> 24) & 255);
    *(Digest + (4 * i) + 1) = (uint8_t)((c->state[i] >> 16) & 255);
    *(Digest + (4 * i) + 2) = (uint8_t)((c->state[i] >> 8) & 255);
    *(Digest + (4 * i) + 3) = (uint8_t)(c->state[i] & 255);
  }
}

void Sha256::Calculate (void const *Buffer, uint32_t BufferSize, uint8_t Digest[SHA256_HASH_SIZE]) {
  Sha256::Context *c = new Sha256::Context;
  Sha256::Initialise (c);
  Sha256::Update (c, Buffer, BufferSize);
  Sha256::Finalise (c, Digest);
}
