/*
 * osMalloc.c
 *
 *  Created on: Apr 14, 2023
 *      Author: lanzy
 */

#include "auOsMalloc.h"
#include "auOsDefines.h"
/**
 * @brief 初始化内存池
 * @param MemAddr 作为内存池数组的指针
 * @param MemSize 数组大小(字节)
 * @return 无
 */
void auMallocCoreInit(void *MemAddr, auRegLen MemSize)
{

	auOsMallocHeaderTypeDef *Header;
	auRegLen offset;
	auByte_pt ptr;

	Header = (auOsMallocHeaderTypeDef *)MemAddr;

	offset = sizeof(auOsMallocHeaderTypeDef);
	while (0 != ((offset + osCacheLineAlign) % osCacheLineAlign))
	{
		offset++;
	}

	ptr = (auByte_pt)MemAddr;
	Header->StartSize_Ptr = (auRegLen *)&ptr[offset];
	Header->EntrySize_Ptr = Header->StartSize_Ptr;
	Header->StartSize_Ptr[0] = MemSize - offset - osCacheLineAlign;
	Header->StartSize_Ptr[1] = 0;
	Header->EndAddr = (void *)&ptr[MemSize];
}

/**
 * @brief 向指定内存池申请内存
 * @param MemAddr 内存池地址
 * @param Size 要申请的内存大小
 * @return 申请到的内存指针 如果为NULL则为失败
 */
void *auMalloc(void *MemoryAddr, auUsize_t Size)
{
	auOsMallocHeaderTypeDef *Header;
	auUsize_t tSize;
	auUsize_t *tBlock_Ptr;
	auUsize_t *EntryBlock_Ptr;
	auUsize_t *NextBlock_Ptr;
	auUsize_t *NewBlock_Ptr;
	auByte_t *r;

	if (Size == 0U)
	{
		return NULL;
	}

	Header = (auOsMallocHeaderTypeDef *)MemoryAddr;

	if (osMemLocateAddrBegin > (auUint32_t)MemoryAddr)
	{
		OsErrorHandler();
	}

	EntryBlock_Ptr = Header->EntrySize_Ptr;

	// 将大小转化成以osCacheLineAlign字节单位
	if (0 != (Size % osCacheLineAlign))
	{
		Size = ((Size + osCacheLineAlign) / osCacheLineAlign) * osCacheLineAlign;
	}

	tBlock_Ptr = EntryBlock_Ptr;
	do
	{
		tSize = tBlock_Ptr[0]; // 读取tBlock_Ptr指向的块大小

		if ((tSize & auOsMemkOccupyMsk) != 0U)
		{
			// 已占用 不做操作
		}
		else
		{
			// 未占用

			if (tSize >= Size)
			{
				// 大小足够 尝试分隔

				// 如果要分隔 需要额外多出 2x32bit 字节的空间 (1个用来保存大小信息 另一个用于分配)

				if ((tSize - Size) < 2U)
				{
					// 大小不足以分割 全部用于分配

					tBlock_Ptr[0] = tSize | auOsMemkOccupyMsk; // 只需要直接标记就好

					// 获取块分配的地址
					r = &((auByte_t *)tBlock_Ptr)[osCacheLineAlign];

					// 开始更新EntryBlock_Ptr
					EntryBlock_Ptr = (auUsize_t *)&((auByte_t *)tBlock_Ptr)[(tSize + 1) << 3];
					if (Header->EndAddr == EntryBlock_Ptr)
					{
						// 现在的指针超过了内存范围(EndMemoryAddr用于界定范围, 只要内存没有被破坏, 就一定能指到这个)
						EntryBlock_Ptr = Header->StartSize_Ptr; // 指向开头 重新开始
					}
					Header->EntrySize_Ptr = EntryBlock_Ptr; // 更新EntryBlock_Ptr
					return (void *)r;
				}
				else
				{
					// 可分割

					// 获取下一块的指针
					NextBlock_Ptr = (auUsize_t *)&((auByte_t *)tBlock_Ptr)[(tSize + 1) << 3];

					// 获取新块指针(这个块要即将从旧块中分隔出来)
					NewBlock_Ptr = (auUsize_t *)&((auByte_t *)tBlock_Ptr)[(Size + 1) << 3];

					// 块大小信息更新
					NewBlock_Ptr[0] = tSize - Size - 1U;
					NewBlock_Ptr[1] = Size;
					tBlock_Ptr[0] = Size | auOsMemkOccupyMsk;

					if (((void *)NextBlock_Ptr) != Header->EndAddr)
					{
						// NextBlock_Ptr为有效块的指针
						NextBlock_Ptr[1] = tSize - Size - 1U; // 更新大小信息
					}

					Header->EntrySize_Ptr = NewBlock_Ptr;

					// 获取块分配的地址
					r = &((auByte_t *)tBlock_Ptr)[osCacheLineAlign];

					return (void *)r;
				}
			}
		}

		tBlock_Ptr = (auUsize_t *)&((auByte_t *)tBlock_Ptr)[((tSize & (~auOsMemkOccupyMsk)) + 1) << 3];
		if (Header->EndAddr == tBlock_Ptr)
		{
			// 现在的指针超过了内存范围(EndMemoryAddr用于界定范围, 只要内存没有被破坏, 就一定能指到这个)
			tBlock_Ptr = Header->StartSize_Ptr; // 指向开头 重新开始
		}

	} while (tBlock_Ptr != EntryBlock_Ptr); // 如果回到开始的块 表示操作结束

	return NULL;
}

/**
 * @brief 将内存释放回内存池
 * @param MemAddr 内存池地址
 * @param Ptr 要释放的内存指针
 * @return 无
 */
void auFree(void *MemAddr, void *Ptr)
{

	auOsMallocHeaderTypeDef *Header;
	auRegLen tSize;
	auRegLen *Last_Ptr;
	auRegLen *EntrySize_Ptr;
	auRegLen *After_Ptr;
	auRegLen *After_After_Ptr;
	auRegLen Msk;
	auByte_pt ptr;

	if (Ptr == NULL)
	{
		return;
	}

	Msk = ((auRegLen)0x1) << (sizeof(auRegLen) * 8 - 1);
	Header = (auOsMallocHeaderTypeDef *)MemAddr;
	EntrySize_Ptr = (auRegLen *)((auByte_pt)Ptr - (auByte_pt)osCacheLineAlign);
	tSize = EntrySize_Ptr[0] & (~Msk);

	if (EntrySize_Ptr[1] == 0)
	{
		// 没有上一块内存

		ptr = (auByte_pt)EntrySize_Ptr;
		After_Ptr = (auRegLen *)&ptr[tSize + osCacheLineAlign];

		if ((void *)After_Ptr != Header->EndAddr)
		{
			// 有下一个

			ptr = (auByte_pt)After_Ptr;
			After_After_Ptr = (auRegLen *)&ptr[(After_Ptr[0] & (~Msk)) + osCacheLineAlign];

			if (After_Ptr[0] & Msk)
			{
				// 下一个被占用

				EntrySize_Ptr[0] &= (~Msk); // 取消标记
			}
			else
			{
				// 都无占用

				if (After_Ptr == Header->EntrySize_Ptr)
				{
					// 下次Malloc是即将要合并的位置

					Header->EntrySize_Ptr = EntrySize_Ptr;
				}
				EntrySize_Ptr[0] = tSize + After_Ptr[0] + osCacheLineAlign; // 取消标记 并且更新大小
				if ((void *)After_After_Ptr != Header->EndAddr)
				{
					After_After_Ptr[1] = EntrySize_Ptr[0];
				}
			}
		}
		else
		{
			// 无下一个

			EntrySize_Ptr[0] &= (~Msk); // 取消标记
		}
	}
	else
	{
		// 有上一块内存

		Last_Ptr = (auRegLen *)((auByte_pt)EntrySize_Ptr - (auByte_pt)(EntrySize_Ptr[1] + (auRegLen)osCacheLineAlign));
		ptr = (auByte_pt)EntrySize_Ptr;
		After_Ptr = (auRegLen *)&ptr[tSize + osCacheLineAlign];

		if ((void *)After_Ptr != Header->EndAddr)
		{
			// 有下一个

			ptr = (auByte_pt)After_Ptr;
			After_After_Ptr = (auRegLen *)&ptr[(After_Ptr[0] & (~Msk)) + osCacheLineAlign];

			if ((Last_Ptr[0] & Msk) && (After_Ptr[0] & Msk))
			{
				// 都为已占用

				EntrySize_Ptr[0] &= (~Msk); // 取消标记
			}
			else if (Last_Ptr[0] & Msk)
			{
				// 仅上一个占用 合并下一个

				if (After_Ptr == Header->EntrySize_Ptr)
				{
					// 下次Malloc是即将要合并的位置

					Header->EntrySize_Ptr = EntrySize_Ptr;
				}
				EntrySize_Ptr[0] = tSize + After_Ptr[0] + osCacheLineAlign; // 取消标记 并且更新大小
				if ((void *)After_After_Ptr != Header->EndAddr)
				{
					After_After_Ptr[1] = EntrySize_Ptr[0];
				}
			}
			else if (After_Ptr[0] & Msk)
			{
				// 仅下一个占用 合并到上一个

				if (EntrySize_Ptr == Header->EntrySize_Ptr)
				{
					// 下次Malloc是即将要合并的位置

					Header->EntrySize_Ptr = Last_Ptr;
				}
				Last_Ptr[0] = Last_Ptr[0] + tSize + osCacheLineAlign; // 并且更新大小
				After_Ptr[1] = Last_Ptr[0];
			}
			else
			{
				// 都无占用 全合并

				if ((EntrySize_Ptr == Header->EntrySize_Ptr) || (After_Ptr == Header->EntrySize_Ptr))
				{
					// 下次Malloc是即将要合并的位置

					Header->EntrySize_Ptr = Last_Ptr;
				}
				Last_Ptr[0] = Last_Ptr[0] + tSize + After_Ptr[0] + osCacheLineAlign * 2; // 并且更新大小
				if ((void *)After_After_Ptr != Header->EndAddr)
				{
					After_After_Ptr[1] = Last_Ptr[0];
				}
			}
		}
		else
		{
			// 无下一个

			if (Last_Ptr[0] & Msk)
			{
				// 上一个被占用

				EntrySize_Ptr[0] &= (~Msk); // 取消标记
			}
			else
			{
				// 都无占用

				if (EntrySize_Ptr == Header->EntrySize_Ptr)
				{
					// 下次Malloc是即将要合并的位置

					Header->EntrySize_Ptr = Last_Ptr;
				}
				Last_Ptr[0] = Last_Ptr[0] + tSize + osCacheLineAlign; // 并且更新大小
			}
		}
	}
}

/**
 * @brief 获取内存池状态
 * @param MemAddr 内存池地址
 * @param info 用于存放信息的指针
 * @return 无
 */
void auGetOsMallocInfo(void *MemAddr, auMallocCoreDef *info)
{

	auOsMallocHeaderTypeDef *Header;
	auRegLen *EntrySize_Ptr;
	auRegLen *After_Ptr;
	// auRegLen* After_After_Ptr;
	auRegLen *Last_Ptr;
	auRegLen Msk;
	auRegLen tSize;
	auByte_pt ptr;

	Msk = ((auRegLen)0x1) << (sizeof(auRegLen) * 8 - 1);
	Header = (auOsMallocHeaderTypeDef *)MemAddr;

	info->UseSize = 0;
	info->FreeSize = 0;
	info->OccupySize = 0;
	info->NoOccupySize = 0;

	Last_Ptr = Header->StartSize_Ptr;

	tSize = Last_Ptr[0] & (~Msk);
	if (Last_Ptr[0] & Msk)
	{
		info->UseSize += tSize;
		info->OccupySize += tSize + osCacheLineAlign;
	}
	else
	{
		info->FreeSize += tSize;
		info->NoOccupySize += tSize + osCacheLineAlign;
	}

	if (Last_Ptr[1] != 0)
	{

		info->Result = -1;
		info->ErrPtr = Last_Ptr;
		return;
	}

	ptr = (auByte_pt)Last_Ptr;
	EntrySize_Ptr = (auRegLen *)&ptr[tSize + osCacheLineAlign];

	while (EntrySize_Ptr != Header->EndAddr)
	{
		if ((void *)EntrySize_Ptr > Header->EndAddr)
		{
			info->Result = -2;
			info->ErrPtr = EntrySize_Ptr;
			return;
		}

		tSize = EntrySize_Ptr[0] & (~Msk);

		if (EntrySize_Ptr[1] == 0)
		{
			// 没有上一块内存

			ptr = (auByte_pt)EntrySize_Ptr;
			After_Ptr = (auRegLen *)&ptr[tSize + osCacheLineAlign];

			if ((void *)After_Ptr != Header->EndAddr)
			{
				// 有下一个

				ptr = (auByte_pt)After_Ptr;
				// After_After_Ptr=(auRegLen*)&ptr[(After_Ptr[0]&(~Msk))+osCacheLineAlign];

				if (After_Ptr[1] != tSize)
				{
					info->Result = -5;
					info->ErrPtr = EntrySize_Ptr;
					return;
				}
			}
			else
			{
				// 无下一个
			}
		}
		else
		{
			// 有上一块内存

			Last_Ptr = (auRegLen *)((auByte_pt)EntrySize_Ptr - (auByte_pt)(EntrySize_Ptr[1] + (auRegLen)osCacheLineAlign));
			ptr = (auByte_pt)EntrySize_Ptr;
			After_Ptr = (auRegLen *)&ptr[tSize + osCacheLineAlign];

			if ((void *)After_Ptr != Header->EndAddr)
			{
				// 有下一个

				ptr = (auByte_pt)After_Ptr;
				// After_After_Ptr=(auRegLen*)&ptr[(After_Ptr[0]&(~Msk))+osCacheLineAlign];

				if (After_Ptr[1] != tSize)
				{
					info->Result = -5;
					info->ErrPtr = EntrySize_Ptr;
					return;
				}

				if ((Last_Ptr[0] & (~Msk)) != EntrySize_Ptr[1])
				{
					info->Result = -6;
					info->ErrPtr = EntrySize_Ptr;
					return;
				}
			}
			else
			{
				// 无下一个

				if ((Last_Ptr[0] & (~Msk)) != EntrySize_Ptr[1])
				{
					info->Result = -6;
					info->ErrPtr = EntrySize_Ptr;
					return;
				}
			}
		}

		if (EntrySize_Ptr[0] & Msk)
		{
			info->UseSize += tSize;
			info->OccupySize += tSize + osCacheLineAlign;
		}
		else
		{
			if ((Last_Ptr[0] & Msk) == 0)
			{
				// 都为未占用
				info->Result = -3;
				info->ErrPtr = Last_Ptr;
				return;
			}

			info->FreeSize += tSize;
			info->NoOccupySize += tSize + osCacheLineAlign;
		}

		if ((Last_Ptr[0] & (~Msk)) != EntrySize_Ptr[1])
		{
			info->Result = -4;
			info->ErrPtr = Last_Ptr;
			return;
		}

		Last_Ptr = EntrySize_Ptr;

		ptr = (auByte_pt)Last_Ptr;
		EntrySize_Ptr = (auRegLen *)&ptr[tSize + osCacheLineAlign];
	}

	info->Result = 0;
}
