///////////////////////////////////////////////////////////////////////////
//@Author: 			Xenohoshi,
//@File: 			BaseLib.c
//@Brief:			high performence compute lib for Embeded system(MCU)
//@Changelog:
//		@v1.4:	date 2020.12. fix bug in sign extend
//		@v1.3:	date 2020.10. add opposite sign and sign extend
//		@v1.2.1: date 2020.9. fix Parity compute, f**k bugs.
//		@v1.2: date 2020.9. add Parity compute.
//		@v1.1: date 2019.6. bug found in Set/Clear bit by mask, fallback
//		@v1.0: date 2019.6. fix some errors, it should now work fine
//		@V0.6: date 2019.6. add Set/Clear bit by mask
//		@v0.5: date 2019.3. rework bit count, change from
//				LUT(better for SIMD, NOT MCU) to loop
//		@v0.4: date 2019.3. add bit count
//		@v0.3: date 2019.3. add leading zeros and ending zeros
//		@v0.2.1: date 2019.1 fix errors in abs
//		@v0.2: date 2018.9. add abs, min, max
//		@v0.1.1: date 2018.03. change bit swap from func to macro
//		@v0.1: date 2018.03. Init, add bit swap
///////////////////////////////////////////////////////////////////////////
#include "auLib.h"

/// @brief counting bits set in value, O(n), do NOT use when CPU supports SIMD instructions
/// @param value value to compute
/// @return how much bits are set in value(0b0010 = 1)
uint32_t libCountingBitSets(uint32_t value)
{
	uint32_t retval;
	for (retval = 0; value; retval++)
	{
		value &= value - 1;
	}
	return retval;
}

/// @brief compute two int and return if they have different sign
/// @param x val 1
/// @param y val 2
/// @return true if val1 and val2 have different sign
bool libIfIntHasOppositeSign(int32_t x, int32_t y)
{
	return ((x ^ y) < 0);
}

/// @brief compute abs(x) no branching
/// @param x val
/// @return abs(x)
uint32_t libAbs(int32_t x)
{
	uint32_t mask = x >> sizeof(int32_t) * 8 - 1;
	return (x + mask) ^ mask;
}

/// @brief compute max(x,y) no branching
/// @param x val1
/// @param y val2
/// @return max(x,y)
int32_t libMax(int32_t x, int32_t y)
{
	return x ^ ((x ^ y) & -(x < y));
}

/// @brief compute min(x,y) no branching
/// @param x val1
/// @param y val2
/// @return min(x,y)
int32_t libMin(int32_t x, int32_t y)
{
	return y ^ ((x ^ y) & -(x < y));
}

/// @brief convert a non standard width signed variable to int32. x(7bits) to x(32bits)
/// @param x val to extand
/// @param y val eff bits
/// @return extended x
int32_t libSignExtend(int32_t x, uint32_t y)
{
	int32_t const mask = 0x01 << (y - 1);
	// x = x & ((0x01 << y) - 1); // bits in x above eff bits should be all zero
	return (x ^ mask) - mask;
}

/// @brief set/clear bit by mask in y no branching
/// @param mask
/// @param y
/// @return edited value
uint32_t libSetClearBit(bool x, uint8_t mask, uint32_t y)
{
	y ^= (-x ^ y) & mask;
	return y;
}

/// @brief count how much bit set in x
/// @param x value to count
/// @return
bool libParityUint32(uint32_t x)
{
	x ^= x >> 16;
	x ^= x >> 8;
	x ^= x >> 4;
	x &= 0x0F;
	return (0x6996 >> x) & 1;
}

/// @brief count how much bit set in x
/// @param x value to count
/// @return
bool libParityUint8(uint8_t x)
{
	x ^= x >> 4;
	x &= 0x0F;
	return (0x6996 >> x) & 1;
}

/// @brief count ending zeros in x
/// @param x
/// @return sum of zeros
uint32_t libCountZeroBitsRight(uint32_t x)
{
	return __CLZ(__RBIT(x));
}

/// @brief count leading zeros in x
/// @param x
/// @return sum of zeros
uint32_t libCountZeroBitsLeft(uint32_t x)
{
	return __CLZ(x);
}