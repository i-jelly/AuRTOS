/*
 * auOsDefines.h
 *
 *  Created on: 2023年4月9日
 *      Author: lanzy
 */
#pragma once
#ifndef INC_AUOSDEFINES_H_
#define INC_AUOSDEFINES_H_

#include "auOsConfig.h"
#include "auOsTypes.h"
// 此项根据芯片选定
#include "stm32h7xx_hal.h"
#include "core_cm7.h"

// type and structs, enums
typedef struct
{
	void (*IRQCallback)(void *);
	void *v;
} IRQCallbackTypeDef;

// 通用函数指针，参数为void*
typedef void (*RunPtr)(void *);
// GCC attritubes
// 打包数据，以cache line宽度对齐
#define AU_ALIGNED_AFTER __attribute__((aligned(osCacheLineAlign)))

#define AU_NAKED_BEFORE __attribute__((naked))
// 展开
#define AU_INLINE_BEFORE __attribute__((always_inline))

#define AU_SET_LOCATION_AFTER(x) __attribute__((section(x)))
// 放在data ram的数据，bootstrap程序若没有初始化此区域需要手动补
#define AU_RAM_DATA_AFTER AU_SET_LOCATION_AFTER(".dtcmram")
// 放在instru ram的数据，同上
#define AU_RAM_FUNC_AFTER AU_SET_LOCATION_AFTER(".itcmram")
// 以上两块内存区域与一般SRAM内存区别在于与Cortex内核耦合更紧密，普通SRAM内存主频只有内核频率的一半，而*tcmram频率与内核相同
// 一般存在于M4/M7内核中，M3内核无此内存
//  macros
//  包装触发中断
// 此操作使中断标志位置1，内核会在其余中断结束后处理此中断
#define TrigScheduler()                      \
	do                                       \
	{                                        \
		SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; \
	} while (0)

// 使用内核指令保证操作原子性
// 未验证，目前没用到
#define WriteOnce(data, address)                            \
	do                                                      \
	{                                                       \
		__LDREXW(address);                                  \
		(*((volatile typeof(data) *)(&(address))) = (data)) \
	} while (__STREXW(address, data))

// 系统错误处理函数
#define OsErrorHandler() while (1)
// 内存块在内存池中占用情况,
#define auOsMemkOccupyMsk (((auRegLen)0x1U) << (sizeof(auRegLen) * 8 - 1))

#endif /* INC_AUOSDEFINES_H_ */