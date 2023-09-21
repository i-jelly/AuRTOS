/*
 * auOsLowLevel.c
 *
 *  Created on: 2023年4月9日
 *      Author: lanzy
 */

#include "auOsDefines.h"

void ThreadSwitch();
void StartFirstThread();
void PendSV_Handler();

extern void **auOsThread;
extern void *PendSV_Call;
extern void *auOsThreadSPRefresh();

AU_NAKED_BEFORE void ThreadSwitch()
{
	__asm volatile("LDR       R0, =auOsThread");
	__asm volatile("LDR       R12, [R0]");
	__asm volatile("PUSH      {R12, LR}");
	__asm volatile("BL        auOsThreadSPRefresh");
	__asm volatile("POP       {R12, LR}");

	__asm volatile("CMP       R0, #0");
	__asm volatile("IT        EQ");
	__asm volatile("BXEQ      LR");

	__asm volatile("MRS       R0, PSP");

	__asm volatile("TST 	  LR, #0x10");
	__asm volatile("IT        EQ");
	__asm volatile("VSTMDBEQ  R0!, {S16-S31}");
	__asm volatile("STMDB     R0!, {R4-R11,LR}");
	__asm volatile("STR       R0,[R12] ");
	__asm volatile("LDR R0,   =auOsThread");
	__asm volatile("LDR R0,   [R0]");
	__asm volatile("LDR R0,   [R0]");
	__asm volatile("LDMIA     R0!, {R4-R11,LR}");
	__asm volatile("TST       LR,#0x10");
	__asm volatile("IT        EQ");
	__asm volatile("VLDMIAEQ  R0!,{S16-S31}");
	__asm volatile("MSR       PSP,R0");
	__asm volatile("BX        LR");
}

AU_INLINE_BEFORE void StartFirstThread()
{
	__asm volatile("LDR       R0,=PendSV_Call");
	__asm volatile("LDR       R1,=ThreadSwitch");
	__asm volatile("STR       R1,[R0]");
	__asm volatile("MOV       R0,#0");
	__asm volatile("MSR       CONTROL,R0");
	__asm volatile("LDR       R1,=auOsThread");
	__asm volatile("LDR       R1,[R1]");
	__asm volatile("LDR       R0,[R1]");
	__asm volatile("LDMIA     R0!,{R4-R11,LR}");
	__asm volatile("MSR       PSP,R0");
	__asm volatile("BX        LR");
}

AU_INLINE_BEFORE void PendSV_Handler()
{
	__asm volatile("LDR       R0,=PendSV_Call");
	__asm volatile("LDR       R0,[R0]");
	__asm volatile("BX        R0");
}
