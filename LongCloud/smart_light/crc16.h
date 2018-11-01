#ifndef __CRC_16_H__
#define __CRC_16_H__

#define CCITT     (0x1021)
typedef unsigned short u16;
typedef unsigned int  u32;
typedef unsigned char	u8;

u16 CRC16(u16 crc, u8 *buffer, u32 length);

#endif
