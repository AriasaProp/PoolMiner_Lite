#include "hashing.hpp"
#include <cstring>

#define _rotl(value, bits) (((value) >> (bits)) | ((value) << (32 - (bits))))

hashing::hashing () {}
hashing::~hashing () {}
void hashing::xorSalsa8(const size_t di, const size_t xi) {
  uint32_t x00 = (X[di +  0] ^= X[xi +  0]);
  uint32_t x01 = (X[di +  1] ^= X[xi +  1]);
  uint32_t x02 = (X[di +  2] ^= X[xi +  2]);
  uint32_t x03 = (X[di +  3] ^= X[xi +  3]);
  uint32_t x04 = (X[di +  4] ^= X[xi +  4]);
  uint32_t x05 = (X[di +  5] ^= X[xi +  5]);
  uint32_t x06 = (X[di +  6] ^= X[xi +  6]);
  uint32_t x07 = (X[di +  7] ^= X[xi +  7]);
  uint32_t x08 = (X[di +  8] ^= X[xi +  8]);
  uint32_t x09 = (X[di +  9] ^= X[xi +  9]);
  uint32_t x10 = (X[di + 10] ^= X[xi + 10]);
  uint32_t x11 = (X[di + 11] ^= X[xi + 11]);
  uint32_t x12 = (X[di + 12] ^= X[xi + 12]);
  uint32_t x13 = (X[di + 13] ^= X[xi + 13]);
  uint32_t x14 = (X[di + 14] ^= X[xi + 14]);
  uint32_t x15 = (X[di + 15] ^= X[xi + 15]);
  for (int i = 0; i < 8; i += 2) {
      x04 ^= _rotl(x00+x12, 7);  x08 ^= _rotl(x04+x00, 9);
      x12 ^= _rotl(x08+x04,13);  x00 ^= _rotl(x12+x08,18);
      x09 ^= _rotl(x05+x01, 7);  x13 ^= _rotl(x09+x05, 9);
      x01 ^= _rotl(x13+x09,13);  x05 ^= _rotl(x01+x13,18);
      x14 ^= _rotl(x10+x06, 7);  x02 ^= _rotl(x14+x10, 9);
      x06 ^= _rotl(x02+x14,13);  x10 ^= _rotl(x06+x02,18);
      x03 ^= _rotl(x15+x11, 7);  x07 ^= _rotl(x03+x15, 9);
      x11 ^= _rotl(x07+x03,13);  x15 ^= _rotl(x11+x07,18);
      x01 ^= _rotl(x00+x03, 7);  x02 ^= _rotl(x01+x00, 9);
      x03 ^= _rotl(x02+x01,13);  x00 ^= _rotl(x03+x02,18);
      x06 ^= _rotl(x05+x04, 7);  x07 ^= _rotl(x06+x05, 9);
      x04 ^= _rotl(x07+x06,13);  x05 ^= _rotl(x04+x07,18);
      x11 ^= _rotl(x10+x09, 7);  x08 ^= _rotl(x11+x10, 9);
      x09 ^= _rotl(x08+x11,13);  x10 ^= _rotl(x09+x08,18);
      x12 ^= _rotl(x15+x14, 7);  x13 ^= _rotl(x12+x15, 9);
      x14 ^= _rotl(x13+x12,13);  x15 ^= _rotl(x14+x13,18);
  }
  X[di +  0] += x00;
  X[di +  1] += x01;
  X[di +  2] += x02;
  X[di +  3] += x03;
  X[di +  4] += x04;
  X[di +  5] += x05;
  X[di +  6] += x06;
  X[di +  7] += x07;
  X[di +  8] += x08;
  X[di +  9] += x09;
  X[di + 10] += x10;
  X[di + 11] += x11;
  X[di + 12] += x12;
  X[di + 13] += x13;
  X[di + 14] += x14;
  X[di + 15] += x15;
}
void hashing::xorSalsa8 () {
  xs[0] = (X[0] ^= X[16]);
  xs[1] = (X[1] ^= X[17]);
  xs[2] = (X[2] ^= X[18]);
  xs[3] = (X[3] ^= X[19]);
  xs[4] = (X[4] ^= X[20]);
  xs[5] = (X[5] ^= X[21]);
  xs[6] = (X[6] ^= X[22]);
  xs[7] = (X[7] ^= X[23]);
  xs[8] = (X[8] ^= X[24]);
  xs[9] = (X[9] ^= X[25]);
  xs[10] = (X[10] ^= X[26]);
  xs[11] = (X[11] ^= X[27]);
  xs[12] = (X[12] ^= X[28]);
  xs[13] = (X[13] ^= X[29]);
  xs[14] = (X[14] ^= X[30]);
  xs[15] = (X[15] ^= X[31]);
  for (l = 0; l < 4; l++) { // 8/2
    xs[4] ^= _rotl (xs[0] + xs[12], 7);
    xs[8] ^= _rotl (xs[4] + xs[0], 9);
    xs[12] ^= _rotl (xs[8] + xs[4], 13);
    xs[0] ^= _rotl (xs[12] + xs[8], 18);
    xs[9] ^= _rotl (xs[5] + xs[1], 7);
    xs[13] ^= _rotl (xs[9] + xs[5], 9);
    xs[1] ^= _rotl (xs[13] + xs[9], 13);
    xs[5] ^= _rotl (xs[1] + xs[13], 18);
    xs[14] ^= _rotl (xs[10] + xs[6], 7);
    xs[2] ^= _rotl (xs[14] + xs[10], 9);
    xs[6] ^= _rotl (xs[2] + xs[14], 13);
    xs[10] ^= _rotl (xs[6] + xs[2], 18);
    xs[3] ^= _rotl (xs[15] + xs[11], 7);
    xs[7] ^= _rotl (xs[3] + xs[15], 9);
    xs[11] ^= _rotl (xs[7] + xs[3], 13);
    xs[15] ^= _rotl (xs[11] + xs[7], 18);
    xs[1] ^= _rotl (xs[0] + xs[3], 7);
    xs[2] ^= _rotl (xs[1] + xs[0], 9);
    xs[3] ^= _rotl (xs[2] + xs[1], 13);
    xs[0] ^= _rotl (xs[3] + xs[2], 18);
    xs[6] ^= _rotl (xs[5] + xs[4], 7);
    xs[7] ^= _rotl (xs[6] + xs[5], 9);
    xs[4] ^= _rotl (xs[7] + xs[6], 13);
    xs[5] ^= _rotl (xs[4] + xs[7], 18);
    xs[11] ^= _rotl (xs[10] + xs[9], 7);
    xs[8] ^= _rotl (xs[11] + xs[10], 9);
    xs[9] ^= _rotl (xs[8] + xs[11], 13);
    xs[10] ^= _rotl (xs[9] + xs[8], 18);
    xs[12] ^= _rotl (xs[15] + xs[14], 7);
    xs[13] ^= _rotl (xs[12] + xs[15], 9);
    xs[14] ^= _rotl (xs[13] + xs[12], 13);
    xs[15] ^= _rotl (xs[14] + xs[13], 18);
  }
  X[0] += xs[0];
  X[1] += xs[1];
  X[2] += xs[2];
  X[3] += xs[3];
  X[4] += xs[4];
  X[5] += xs[5];
  X[6] += xs[6];
  X[7] += xs[7];
  X[8] += xs[8];
  X[9] += xs[9];
  X[10] += xs[10];
  X[11] += xs[11];
  X[12] += xs[12];
  X[13] += xs[13];
  X[14] += xs[14];
  X[15] += xs[15];

  xs[0] = (X[16] ^= X[0]);
  xs[1] = (X[17] ^= X[1]);
  xs[2] = (X[18] ^= X[2]);
  xs[3] = (X[19] ^= X[3]);
  xs[4] = (X[20] ^= X[4]);
  xs[5] = (X[21] ^= X[5]);
  xs[6] = (X[22] ^= X[6]);
  xs[7] = (X[23] ^= X[7]);
  xs[8] = (X[24] ^= X[8]);
  xs[9] = (X[25] ^= X[9]);
  xs[10] = (X[26] ^= X[10]);
  xs[11] = (X[27] ^= X[11]);
  xs[12] = (X[28] ^= X[12]);
  xs[13] = (X[29] ^= X[13]);
  xs[14] = (X[30] ^= X[14]);
  xs[15] = (X[31] ^= X[15]);
  for (l = 0; l < 4; l++) { // 8/2
    xs[4] ^= _rotl (xs[0] + xs[12], 7);
    xs[8] ^= _rotl (xs[4] + xs[0], 9);
    xs[12] ^= _rotl (xs[8] + xs[4], 13);
    xs[0] ^= _rotl (xs[12] + xs[8], 18);
    xs[9] ^= _rotl (xs[5] + xs[1], 7);
    xs[13] ^= _rotl (xs[9] + xs[5], 9);
    xs[1] ^= _rotl (xs[13] + xs[9], 13);
    xs[5] ^= _rotl (xs[1] + xs[13], 18);
    xs[14] ^= _rotl (xs[10] + xs[6], 7);
    xs[2] ^= _rotl (xs[14] + xs[10], 9);
    xs[6] ^= _rotl (xs[2] + xs[14], 13);
    xs[10] ^= _rotl (xs[6] + xs[2], 18);
    xs[3] ^= _rotl (xs[15] + xs[11], 7);
    xs[7] ^= _rotl (xs[3] + xs[15], 9);
    xs[11] ^= _rotl (xs[7] + xs[3], 13);
    xs[15] ^= _rotl (xs[11] + xs[7], 18);
    xs[1] ^= _rotl (xs[0] + xs[3], 7);
    xs[2] ^= _rotl (xs[1] + xs[0], 9);
    xs[3] ^= _rotl (xs[2] + xs[1], 13);
    xs[0] ^= _rotl (xs[3] + xs[2], 18);
    xs[6] ^= _rotl (xs[5] + xs[4], 7);
    xs[7] ^= _rotl (xs[6] + xs[5], 9);
    xs[4] ^= _rotl (xs[7] + xs[6], 13);
    xs[5] ^= _rotl (xs[4] + xs[7], 18);
    xs[11] ^= _rotl (xs[10] + xs[9], 7);
    xs[8] ^= _rotl (xs[11] + xs[10], 9);
    xs[9] ^= _rotl (xs[8] + xs[11], 13);
    xs[10] ^= _rotl (xs[9] + xs[8], 18);
    xs[12] ^= _rotl (xs[15] + xs[14], 7);
    xs[13] ^= _rotl (xs[12] + xs[15], 9);
    xs[14] ^= _rotl (xs[13] + xs[12], 13);
    xs[15] ^= _rotl (xs[14] + xs[13], 18);
  }
  X[16] += xs[0];
  X[17] += xs[1];
  X[18] += xs[2];
  X[19] += xs[3];
  X[20] += xs[4];
  X[21] += xs[5];
  X[22] += xs[6];
  X[23] += xs[7];
  X[24] += xs[8];
  X[25] += xs[9];
  X[26] += xs[10];
  X[27] += xs[11];
  X[28] += xs[12];
  X[29] += xs[13];
  X[30] += xs[14];
  X[31] += xs[15];
}
void hashing::hash (const uint8_t *header) {
  memcpy (B, header, 80);
  uint8_t temp = B[76];
  B[76] = B[79];
  B[79] = temp;
  temp = B[77];
  B[77] = B[78];
  B[78] = temp;
  innerHash();
}
void hashing::hash (const uint8_t *header, const uint32_t nonce) {
  memcpy (B, header, 76);
  uint8_t *tempN = (uint8_t*)&nonce;
  B[76] = tempN[3];
  B[77] = tempN[2];
  B[78] = tempN[1];
  B[79] = tempN[0];
  innerHash();
}
void hashing::innerHash() {
  Sha256Initialise (&context);
  memset (B + 80, 0, 3);

  for (i = 0; i < 4; i++) {
    B[83] = i + 1;
    Sha256Update (&context, B, 84);
    Sha256Finalise (&context, (uint8_t*)(X + (i * 8)));
  }

  for (i = 0; i < 32768; i += 32) {
    memcpy (V+i, X, 32);
/*
    xorSalsa8 (0,16);
    xorSalsa8 (16,0);
*/
    xorSalsa8 ();
  }

  for (i = 0; i < 1024; i++) {
    k = (X[16] & 0x3ff) << 5;
    for (j = 0; j < 32; j++) {
      X[j] ^= V[k + j];
    }
/*
    xorSalsa8 (0,16);
    xorSalsa8 (16,0);
*/
    xorSalsa8 ();
  }
  memcpy (B, X, 128);
  memset (B+128, 0, 3);
  B[131] = 1;
  Sha256Update (&context, B, 132);
  Sha256Finalise (&context, H);
}

void hashN (const uint8_t *header, uint8_t H[SHA256_HASH_SIZE]) {
  uint8_t B[132];
  uint32_t X[32];
  uint32_t V[32768];
  uint32_t xs[16];

  Sha256Context context;
  size_t i, j, k, l;
  memcpy (B, header, 80);
  Sha256Initialise (&context);
  memset (B + 80, 0, 3);

  for (i = 0; i < 4; i++) {
    B[83] = i + 1;
    Sha256Update (&context, B, 84);
    Sha256Finalise (&context, H);
    memcpy (X + (i * 8), H, 32);
  }

  for (i = 0; i < 32768; i += 32) {
    memcpy (V + i, X, 32);
    // xorSalsa8
    {
      xs[0] = (X[0] ^= X[16]);
      xs[1] = (X[1] ^= X[17]);
      xs[2] = (X[2] ^= X[18]);
      xs[3] = (X[3] ^= X[19]);
      xs[4] = (X[4] ^= X[20]);
      xs[5] = (X[5] ^= X[21]);
      xs[6] = (X[6] ^= X[22]);
      xs[7] = (X[7] ^= X[23]);
      xs[8] = (X[8] ^= X[24]);
      xs[9] = (X[9] ^= X[25]);
      xs[10] = (X[10] ^= X[26]);
      xs[11] = (X[11] ^= X[27]);
      xs[12] = (X[12] ^= X[28]);
      xs[13] = (X[13] ^= X[29]);
      xs[14] = (X[14] ^= X[30]);
      xs[15] = (X[15] ^= X[31]);
      for (l = 0; l < 4; l++) { // 8/2
        xs[4] ^= _rotl (xs[0] + xs[12], 7);
        xs[8] ^= _rotl (xs[4] + xs[0], 9);
        xs[12] ^= _rotl (xs[8] + xs[4], 13);
        xs[0] ^= _rotl (xs[12] + xs[8], 18);
        xs[9] ^= _rotl (xs[5] + xs[1], 7);
        xs[13] ^= _rotl (xs[9] + xs[5], 9);
        xs[1] ^= _rotl (xs[13] + xs[9], 13);
        xs[5] ^= _rotl (xs[1] + xs[13], 18);
        xs[14] ^= _rotl (xs[10] + xs[6], 7);
        xs[2] ^= _rotl (xs[14] + xs[10], 9);
        xs[6] ^= _rotl (xs[2] + xs[14], 13);
        xs[10] ^= _rotl (xs[6] + xs[2], 18);
        xs[3] ^= _rotl (xs[15] + xs[11], 7);
        xs[7] ^= _rotl (xs[3] + xs[15], 9);
        xs[11] ^= _rotl (xs[7] + xs[3], 13);
        xs[15] ^= _rotl (xs[11] + xs[7], 18);
        xs[1] ^= _rotl (xs[0] + xs[3], 7);
        xs[2] ^= _rotl (xs[1] + xs[0], 9);
        xs[3] ^= _rotl (xs[2] + xs[1], 13);
        xs[0] ^= _rotl (xs[3] + xs[2], 18);
        xs[6] ^= _rotl (xs[5] + xs[4], 7);
        xs[7] ^= _rotl (xs[6] + xs[5], 9);
        xs[4] ^= _rotl (xs[7] + xs[6], 13);
        xs[5] ^= _rotl (xs[4] + xs[7], 18);
        xs[11] ^= _rotl (xs[10] + xs[9], 7);
        xs[8] ^= _rotl (xs[11] + xs[10], 9);
        xs[9] ^= _rotl (xs[8] + xs[11], 13);
        xs[10] ^= _rotl (xs[9] + xs[8], 18);
        xs[12] ^= _rotl (xs[15] + xs[14], 7);
        xs[13] ^= _rotl (xs[12] + xs[15], 9);
        xs[14] ^= _rotl (xs[13] + xs[12], 13);
        xs[15] ^= _rotl (xs[14] + xs[13], 18);
      }
      X[0] += xs[0];
      X[1] += xs[1];
      X[2] += xs[2];
      X[3] += xs[3];
      X[4] += xs[4];
      X[5] += xs[5];
      X[6] += xs[6];
      X[7] += xs[7];
      X[8] += xs[8];
      X[9] += xs[9];
      X[10] += xs[10];
      X[11] += xs[11];
      X[12] += xs[12];
      X[13] += xs[13];
      X[14] += xs[14];
      X[15] += xs[15];

      xs[0] = (X[16] ^= X[0]);
      xs[1] = (X[17] ^= X[1]);
      xs[2] = (X[18] ^= X[2]);
      xs[3] = (X[19] ^= X[3]);
      xs[4] = (X[20] ^= X[4]);
      xs[5] = (X[21] ^= X[5]);
      xs[6] = (X[22] ^= X[6]);
      xs[7] = (X[23] ^= X[7]);
      xs[8] = (X[24] ^= X[8]);
      xs[9] = (X[25] ^= X[9]);
      xs[10] = (X[26] ^= X[10]);
      xs[11] = (X[27] ^= X[11]);
      xs[12] = (X[28] ^= X[12]);
      xs[13] = (X[29] ^= X[13]);
      xs[14] = (X[30] ^= X[14]);
      xs[15] = (X[31] ^= X[15]);
      for (l = 0; l < 4; l++) { // 8/2
        xs[4] ^= _rotl (xs[0] + xs[12], 7);
        xs[8] ^= _rotl (xs[4] + xs[0], 9);
        xs[12] ^= _rotl (xs[8] + xs[4], 13);
        xs[0] ^= _rotl (xs[12] + xs[8], 18);
        xs[9] ^= _rotl (xs[5] + xs[1], 7);
        xs[13] ^= _rotl (xs[9] + xs[5], 9);
        xs[1] ^= _rotl (xs[13] + xs[9], 13);
        xs[5] ^= _rotl (xs[1] + xs[13], 18);
        xs[14] ^= _rotl (xs[10] + xs[6], 7);
        xs[2] ^= _rotl (xs[14] + xs[10], 9);
        xs[6] ^= _rotl (xs[2] + xs[14], 13);
        xs[10] ^= _rotl (xs[6] + xs[2], 18);
        xs[3] ^= _rotl (xs[15] + xs[11], 7);
        xs[7] ^= _rotl (xs[3] + xs[15], 9);
        xs[11] ^= _rotl (xs[7] + xs[3], 13);
        xs[15] ^= _rotl (xs[11] + xs[7], 18);
        xs[1] ^= _rotl (xs[0] + xs[3], 7);
        xs[2] ^= _rotl (xs[1] + xs[0], 9);
        xs[3] ^= _rotl (xs[2] + xs[1], 13);
        xs[0] ^= _rotl (xs[3] + xs[2], 18);
        xs[6] ^= _rotl (xs[5] + xs[4], 7);
        xs[7] ^= _rotl (xs[6] + xs[5], 9);
        xs[4] ^= _rotl (xs[7] + xs[6], 13);
        xs[5] ^= _rotl (xs[4] + xs[7], 18);
        xs[11] ^= _rotl (xs[10] + xs[9], 7);
        xs[8] ^= _rotl (xs[11] + xs[10], 9);
        xs[9] ^= _rotl (xs[8] + xs[11], 13);
        xs[10] ^= _rotl (xs[9] + xs[8], 18);
        xs[12] ^= _rotl (xs[15] + xs[14], 7);
        xs[13] ^= _rotl (xs[12] + xs[15], 9);
        xs[14] ^= _rotl (xs[13] + xs[12], 13);
        xs[15] ^= _rotl (xs[14] + xs[13], 18);
      }
      X[16] += xs[0];
      X[17] += xs[1];
      X[18] += xs[2];
      X[19] += xs[3];
      X[20] += xs[4];
      X[21] += xs[5];
      X[22] += xs[6];
      X[23] += xs[7];
      X[24] += xs[8];
      X[25] += xs[9];
      X[26] += xs[10];
      X[27] += xs[11];
      X[28] += xs[12];
      X[29] += xs[13];
      X[30] += xs[14];
      X[31] += xs[15];
    }
  }

  for (i = 0; i < 1024; i++) {
    k = (X[16] & 0x3ff) << 5;
    for (j = 0; j < 32; j++) {
      X[j] ^= V[k + j];
    }
    // xorSalsa8
    {
      xs[0] = (X[0] ^= X[16]);
      xs[1] = (X[1] ^= X[17]);
      xs[2] = (X[2] ^= X[18]);
      xs[3] = (X[3] ^= X[19]);
      xs[4] = (X[4] ^= X[20]);
      xs[5] = (X[5] ^= X[21]);
      xs[6] = (X[6] ^= X[22]);
      xs[7] = (X[7] ^= X[23]);
      xs[8] = (X[8] ^= X[24]);
      xs[9] = (X[9] ^= X[25]);
      xs[10] = (X[10] ^= X[26]);
      xs[11] = (X[11] ^= X[27]);
      xs[12] = (X[12] ^= X[28]);
      xs[13] = (X[13] ^= X[29]);
      xs[14] = (X[14] ^= X[30]);
      xs[15] = (X[15] ^= X[31]);
      for (l = 0; l < 4; l++) { // 8/2
        xs[4] ^= _rotl (xs[0] + xs[12], 7);
        xs[8] ^= _rotl (xs[4] + xs[0], 9);
        xs[12] ^= _rotl (xs[8] + xs[4], 13);
        xs[0] ^= _rotl (xs[12] + xs[8], 18);
        xs[9] ^= _rotl (xs[5] + xs[1], 7);
        xs[13] ^= _rotl (xs[9] + xs[5], 9);
        xs[1] ^= _rotl (xs[13] + xs[9], 13);
        xs[5] ^= _rotl (xs[1] + xs[13], 18);
        xs[14] ^= _rotl (xs[10] + xs[6], 7);
        xs[2] ^= _rotl (xs[14] + xs[10], 9);
        xs[6] ^= _rotl (xs[2] + xs[14], 13);
        xs[10] ^= _rotl (xs[6] + xs[2], 18);
        xs[3] ^= _rotl (xs[15] + xs[11], 7);
        xs[7] ^= _rotl (xs[3] + xs[15], 9);
        xs[11] ^= _rotl (xs[7] + xs[3], 13);
        xs[15] ^= _rotl (xs[11] + xs[7], 18);
        xs[1] ^= _rotl (xs[0] + xs[3], 7);
        xs[2] ^= _rotl (xs[1] + xs[0], 9);
        xs[3] ^= _rotl (xs[2] + xs[1], 13);
        xs[0] ^= _rotl (xs[3] + xs[2], 18);
        xs[6] ^= _rotl (xs[5] + xs[4], 7);
        xs[7] ^= _rotl (xs[6] + xs[5], 9);
        xs[4] ^= _rotl (xs[7] + xs[6], 13);
        xs[5] ^= _rotl (xs[4] + xs[7], 18);
        xs[11] ^= _rotl (xs[10] + xs[9], 7);
        xs[8] ^= _rotl (xs[11] + xs[10], 9);
        xs[9] ^= _rotl (xs[8] + xs[11], 13);
        xs[10] ^= _rotl (xs[9] + xs[8], 18);
        xs[12] ^= _rotl (xs[15] + xs[14], 7);
        xs[13] ^= _rotl (xs[12] + xs[15], 9);
        xs[14] ^= _rotl (xs[13] + xs[12], 13);
        xs[15] ^= _rotl (xs[14] + xs[13], 18);
      }
      X[0] += xs[0];
      X[1] += xs[1];
      X[2] += xs[2];
      X[3] += xs[3];
      X[4] += xs[4];
      X[5] += xs[5];
      X[6] += xs[6];
      X[7] += xs[7];
      X[8] += xs[8];
      X[9] += xs[9];
      X[10] += xs[10];
      X[11] += xs[11];
      X[12] += xs[12];
      X[13] += xs[13];
      X[14] += xs[14];
      X[15] += xs[15];

      xs[0] = (X[16] ^= X[0]);
      xs[1] = (X[17] ^= X[1]);
      xs[2] = (X[18] ^= X[2]);
      xs[3] = (X[19] ^= X[3]);
      xs[4] = (X[20] ^= X[4]);
      xs[5] = (X[21] ^= X[5]);
      xs[6] = (X[22] ^= X[6]);
      xs[7] = (X[23] ^= X[7]);
      xs[8] = (X[24] ^= X[8]);
      xs[9] = (X[25] ^= X[9]);
      xs[10] = (X[26] ^= X[10]);
      xs[11] = (X[27] ^= X[11]);
      xs[12] = (X[28] ^= X[12]);
      xs[13] = (X[29] ^= X[13]);
      xs[14] = (X[30] ^= X[14]);
      xs[15] = (X[31] ^= X[15]);
      for (l = 0; l < 4; l++) { // 8/2
        xs[4] ^= _rotl (xs[0] + xs[12], 7);
        xs[8] ^= _rotl (xs[4] + xs[0], 9);
        xs[12] ^= _rotl (xs[8] + xs[4], 13);
        xs[0] ^= _rotl (xs[12] + xs[8], 18);
        xs[9] ^= _rotl (xs[5] + xs[1], 7);
        xs[13] ^= _rotl (xs[9] + xs[5], 9);
        xs[1] ^= _rotl (xs[13] + xs[9], 13);
        xs[5] ^= _rotl (xs[1] + xs[13], 18);
        xs[14] ^= _rotl (xs[10] + xs[6], 7);
        xs[2] ^= _rotl (xs[14] + xs[10], 9);
        xs[6] ^= _rotl (xs[2] + xs[14], 13);
        xs[10] ^= _rotl (xs[6] + xs[2], 18);
        xs[3] ^= _rotl (xs[15] + xs[11], 7);
        xs[7] ^= _rotl (xs[3] + xs[15], 9);
        xs[11] ^= _rotl (xs[7] + xs[3], 13);
        xs[15] ^= _rotl (xs[11] + xs[7], 18);
        xs[1] ^= _rotl (xs[0] + xs[3], 7);
        xs[2] ^= _rotl (xs[1] + xs[0], 9);
        xs[3] ^= _rotl (xs[2] + xs[1], 13);
        xs[0] ^= _rotl (xs[3] + xs[2], 18);
        xs[6] ^= _rotl (xs[5] + xs[4], 7);
        xs[7] ^= _rotl (xs[6] + xs[5], 9);
        xs[4] ^= _rotl (xs[7] + xs[6], 13);
        xs[5] ^= _rotl (xs[4] + xs[7], 18);
        xs[11] ^= _rotl (xs[10] + xs[9], 7);
        xs[8] ^= _rotl (xs[11] + xs[10], 9);
        xs[9] ^= _rotl (xs[8] + xs[11], 13);
        xs[10] ^= _rotl (xs[9] + xs[8], 18);
        xs[12] ^= _rotl (xs[15] + xs[14], 7);
        xs[13] ^= _rotl (xs[12] + xs[15], 9);
        xs[14] ^= _rotl (xs[13] + xs[12], 13);
        xs[15] ^= _rotl (xs[14] + xs[13], 18);
      }
      X[16] += xs[0];
      X[17] += xs[1];
      X[18] += xs[2];
      X[19] += xs[3];
      X[20] += xs[4];
      X[21] += xs[5];
      X[22] += xs[6];
      X[23] += xs[7];
      X[24] += xs[8];
      X[25] += xs[9];
      X[26] += xs[10];
      X[27] += xs[11];
      X[28] += xs[12];
      X[29] += xs[13];
      X[30] += xs[14];
      X[31] += xs[15];
    }
  }
  memcpy (B, X, 128);
  B[131] = 1;
  Sha256Update (&context, B, 132);
  Sha256Finalise (&context, H);
}
