#include "crc16.h"

static u16 _crc(u32 n)
{
  u32 i, acc;

  for (n <<= 8, acc = 0, i = 8; i > 0; i--, n <<= 1)
  {
    acc = ((n ^ acc) & 0x8000) ? ((acc << 1) ^ CCITT) : (acc << 1);
  }

  return (u16)(acc);
}

u16 CRC16(u16 crc, u8 *buffer, u32 length)
{
  u32 i, j;

  for (i = 0; i < length; i++)
  {
    j = (crc >> 8) ^ buffer[i];
    crc = (crc << 8) ^ _crc(j);
  }

  return crc;
}


