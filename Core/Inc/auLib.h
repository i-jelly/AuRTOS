///////////////////////////////////////////////////////////////////////////
//@Author: 			Xenohoshi,
//@File: 			BaseLib.h
//@Brief:			high performence compute lib for Embeded system(MCU)
///////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __AU_LIB_H__
#define __AU_LIB_H__

#ifdef __GNUC__

#include "stdbool.h"
#include "stdint.h"
#include "stdlib.h"
#include "cmsis_gcc.h"

// swap values XOR
#define libSwap(a, b) (((a) ^= (b)), ((b) ^= (a)), ((a) ^= (b)))

#define __FORCEINLINE __attribute__((always_inline))
#define __AUTO_ALIGN__ __attribute__((aligned))

extern uint8_t __CLZ(uint32_t value);
extern uint32_t __RBIT(uint32_t value);

uint32_t libCountingBitSets(uint32_t value);
bool libIfIntHasOppositeSign(int32_t x, int32_t y);
uint32_t libAbs(int32_t x);
int32_t libMax(int32_t x, int32_t y);
int32_t libMin(int32_t x, int32_t y);
int32_t libSignExtend(int32_t x, uint32_t y);
uint32_t libSetClearBit(bool x, uint8_t mask, uint32_t y);
bool libParityUint32(uint32_t x);
bool libParityUint8(uint8_t x);
uint32_t libCountZeroBitsRight(uint32_t x);
uint32_t libCountZeroBitsLeft(uint32_t x);
#else
#error "THIS LIB REQUIRES GCC COMPILER TO WORK!"
#endif
#endif