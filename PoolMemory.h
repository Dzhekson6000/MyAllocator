#ifndef POOLMEMORY_H_
#define POOLMEMORY_H_

#include "PoolAbstract.h"

namespace doxyCraft
{
	template<class T>
	class PoolMemory:public PoolAbstract
	{
	public:
		PoolMemory()
		{
			_sizeBlock=sizeof(T)>=sizeof(void*) ? sizeof(T) : sizeof(void*);
			addPage(_sizeBlock);
		}

		~PoolMemory()
		{
		}

		T* alloc()
		{
			if(!*(void**)_firstFreeBlock)
			{
				addPage(_sizeBlock);
			}
			T* ret=(T*)_firstFreeBlock;
			_firstFreeBlock=*(void**)_firstFreeBlock;
			_firstFreeBlock=_firstFreeBlock;
			++_useBlock;
			return ret;
		}

		void free(T* ptr)
		{
			*(void**)ptr=_firstFreeBlock;
			_firstFreeBlock=ptr;
			--_useBlock;
		}
	private:
		size_t _sizeBlock;
	};
}

#endif //POOLMEMORY_H_
