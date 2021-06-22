/*
	semi-fixed point number system for angular rate

	RANGE_HI  range=1
	Number format: Q7.13 signed 
	Range: +/- 64 rad/s (+/- 3667 deg/s)
	Resolution: 0.000122 rad/s (0.00699 deg/s)

	RANGE_LO  range=0
	Number format: Q5.15 signed
	Range: +/- 8 rad/s (+/- 458 deg/s)
	Resolution: 0.0000305 rad/s (0.00175 deg/s)


	Usage:

	vec3f_t test_input = {{1.23, 4.56, -33.33}};

	fixpoint_rate_t frp = vec3f_to_fixpoint_rate(test_input); // range is automatically selected based on input
	
	// send frp.raw_u64   (or frp.raw_u8[0] to frp.raw_u8[8]) through CAN or whatever bus

	vec3f_t test_output = fixpoint_rate_to_vec3f(frp);


	flag_x, flag_y, flag_z can be used for any purpose.

	Overflows are currently not detected; constrain the range of inputs.

	TODO:
	If flags are unnecessary, resolution or range can be increased by 100% by going from 20 bits to 21 bits.

*/


#pragma once

#include <stdint.h>
#include <stdlib.h>

#ifndef PACK
	#define PACK __attribute__((packed))
#endif

#ifndef ALWAYS_INLINE
	#define ALWAYS_INLINE static inline __attribute__((always_inline))
#endif

#define RANGE_HI_N_INTEGER_BITS 7
#define RANGE_HI_N_FRACTIONAL_BITS 13

#define RANGE_LO_N_INTEGER_BITS 5
#define RANGE_LO_N_FRACTIONAL_BITS 15
#define RANGE_LO_MAX_INTEGER ((1 << (RANGE_LO_N_INTEGER_BITS + RANGE_LO_N_FRACTIONAL_BITS - 1)) - 1)

typedef union
{
	struct
	{
		float x;
		float y;
		float z;
	};
	float v[3];
} float_3x1_t;

typedef union
{
	struct PACK
	{
		int32_t x : 20;
		int32_t y : 20;
		int32_t z : 20;
		uint8_t range : 1;
		uint8_t flag_x : 1;
		uint8_t flag_y : 1;
		uint8_t flag_z : 1;
	};

	uint64_t raw_u64;
	uint32_t raw_u32[2];
	uint16_t raw_u16[4];
	uint8_t  raw_u8[8];
} fixpoint_rate_t;

typedef union
{
	struct __attribute__((packed))
	{
		int16_t temp;  // temperature in 0.1 degC, i.e. 230 = 23.0
		uint16_t reserved;
		uint32_t reserved2;
	};

	uint64_t raw_u64;

} msg_imu_status_t;

#define vec3f_t float_3x1_t

ALWAYS_INLINE fixpoint_rate_t fff_to_fixpoint_rate(float x, float y, float z)
{
	fixpoint_rate_t ret;

	int32_t ix = x * (float)(1<<RANGE_LO_N_FRACTIONAL_BITS) + ((x<0.0) ? -0.5 : +0.5);
	int32_t iy = y * (float)(1<<RANGE_LO_N_FRACTIONAL_BITS) + ((y<0.0) ? -0.5 : +0.5);
	int32_t iz = z * (float)(1<<RANGE_LO_N_FRACTIONAL_BITS) + ((z<0.0) ? -0.5 : +0.5);


	// over RANGE_LO; reduce resolution, use RANGE_HI
	if(abs(ix) > RANGE_LO_MAX_INTEGER || abs(iy) > RANGE_LO_MAX_INTEGER ||  abs(iz) > RANGE_LO_MAX_INTEGER)
	{
		ix >>= (RANGE_LO_N_FRACTIONAL_BITS - RANGE_HI_N_FRACTIONAL_BITS);
		iy >>= (RANGE_LO_N_FRACTIONAL_BITS - RANGE_HI_N_FRACTIONAL_BITS);
		iz >>= (RANGE_LO_N_FRACTIONAL_BITS - RANGE_HI_N_FRACTIONAL_BITS);

		ret.range = 1;
	}
	else
	{
		ret.range = 0;
	}

	ret.x = ix;
	ret.y = iy;
	ret.z = iz;
	ret.flag_x = 0;
	ret.flag_y = 0;
	ret.flag_z = 0;

	return ret;
}

ALWAYS_INLINE fixpoint_rate_t vec3f_to_fixpoint_rate(vec3f_t vec3)
{
	return fff_to_fixpoint_rate(vec3.x, vec3.y, vec3.z);
}

ALWAYS_INLINE vec3f_t fixpoint_rate_to_vec3f(fixpoint_rate_t fpr)
{
	vec3f_t ret;
	if(fpr.range)
	{
		ret.x = (float)fpr.x / (float)(1<<RANGE_HI_N_FRACTIONAL_BITS);
		ret.y = (float)fpr.y / (float)(1<<RANGE_HI_N_FRACTIONAL_BITS);
		ret.z = (float)fpr.z / (float)(1<<RANGE_HI_N_FRACTIONAL_BITS);
	}
	else
	{
		ret.x = (float)fpr.x / (float)(1<<RANGE_LO_N_FRACTIONAL_BITS);
		ret.y = (float)fpr.y / (float)(1<<RANGE_LO_N_FRACTIONAL_BITS);
		ret.z = (float)fpr.z / (float)(1<<RANGE_LO_N_FRACTIONAL_BITS);
	}

	return ret;
}

