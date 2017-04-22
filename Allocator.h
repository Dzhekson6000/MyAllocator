#ifndef ALLOCATOR_H_
#define ALLOCATOR_H_

#include <map>
#include <thread>
#include "PoolMemory.h"
#include "SegmentMemory.h"
#include "Shared/Thread/RWLock.h"
#include "Shared/Macros.h"

#define ALLOC Allocator::getInstance()->alloc
#define FREE Allocator::getInstance()->free
#define ALLOC_ARRAY Allocator::getInstance()->allocArray
#define RELOC_ARRAY Allocator::getInstance()->relocArray
#define FREE_ARRAY Allocator::getInstance()->freeArray

#define FREE_THREAD Allocator::getInstance()->freeThread
#define FREE_SEGMET_THREAD Allocator::getInstance()->freeFreeSegmentThread

#define AL_GLOBAL Allocator::Type::GLOBAL
#define AL_STATIC Allocator::Type::STATIC
#define AL_DYNAMIC Allocator::Type::DYNAMIC

namespace doxyCraft
{

	class Allocator
	{
	public:
		enum class Type
		{
			GLOBAL,
			STATIC,
			DYNAMIC
		};

		virtual ~Allocator();

		template<class T, Allocator::Type typeAlloc=Allocator::Type::STATIC, class ... Args>
		T* alloc(Args ... args)
		{
			T* p=nullptr;
			switch(typeAlloc)
			{
				case Allocator::Type::STATIC:
				{
					PoolMemory<T>* pool = getPoolStatic<T>();
					p = pool->alloc();
					p = new(p) T(args...);
				}
					break;
				case Allocator::Type::GLOBAL:
					p=(T*)getMemoryThread()->segmentMemory.alloc(sizeof(T));
					break;

			}


			return p;
		}

		template<class T, Allocator::Type typeAlloc=Allocator::Type::STATIC>
		void free(T* ptr)
		{
			switch(typeAlloc)
			{
				case Allocator::Type::STATIC:
				{
					PoolMemory<T>* pool = getPoolStatic<T>();
					pool->free(ptr);
				}
					break;
				case Allocator::Type::GLOBAL:
					SegmentMemory::free(ptr);
					break;

			}
		}

		template<class T, Allocator::Type typeAlloc=Allocator::Type::STATIC>
		T* allocArray(unsigned int n)
		{
			T* ret=nullptr;
			switch(typeAlloc)
			{
				case Allocator::Type::STATIC:
					ret=(T*)getMemoryThread()->segmentMemory.alloc(sizeof(T)*n);
					break;
				case Allocator::Type::DYNAMIC:
					ret=(T*)getMemoryThread()->segmentDynamicMemory.alloc(sizeof(T)*n);
					break;
				case Allocator::Type::GLOBAL:
					ret=(T*)getMemoryThread()->segmentMemory.alloc(sizeof(T)*n);
					break;

			}
			return ret;
		}

		template<class T, Allocator::Type typeAlloc=Allocator::Type::STATIC>
		T* relocArray(T* ptr, unsigned int n)
		{
			T* ret=nullptr;
			switch(typeAlloc)
			{
				case Allocator::Type::STATIC:
					ret=(T*)getMemoryThread()->segmentMemory.reloc(ptr, sizeof(T)*n);
					break;
				case Allocator::Type::DYNAMIC:
					ret=(T*)getMemoryThread()->segmentDynamicMemory.reloc(ptr, sizeof(T)*n);
					break;
				case Allocator::Type::GLOBAL:
					ret=(T*)getMemoryThread()->segmentMemory.reloc(ptr, sizeof(T)*n);
					break;

			}
			return ret;
		};

		template<class T>
		void freeArray(T* ptr)
		{
			SegmentMemory::free(ptr);
		}

		void freeThread();
		void freeFreeSegmentThread();

		void printSegmentMemory()
		{
			MemoryThread* memoryThread = getMemoryThread();
			memoryThread->segmentDynamicMemory.printMemory();
		}
	
	
	CREATE_SINGLETON(Allocator);
	private:

		typedef std::map<size_t, PoolAbstract*> HashMapPools;

		struct MemoryThread
		{
			HashMapPools pools;
			SegmentMemory segmentMemory;
			SegmentMemory segmentDynamicMemory;
		};

		Allocator();
		static Allocator* _allocator;

		typedef std::map<std::thread::id, MemoryThread*> HashMapMemoryThread;
		HashMapMemoryThread _memoryThreads;
		RWLock _memoryThreadsMutex;

		MemoryThread* getMemoryThread();
		MemoryThread* addMemoryThread();

		template<class T>
		PoolMemory<T>* getPoolStatic()
		{
			MemoryThread* memoryThread = getMemoryThread();
			HashMapPools::iterator it;
			it=memoryThread->pools.find(sizeof(T));
			if(it!=memoryThread->pools.end())
			{
				return (PoolMemory<T>*)it->second;
			}
			return addPoolStatic<T>();
		}

		template<class T>
		PoolMemory<T>* addPoolStatic()
		{
			MemoryThread* memoryThread = getMemoryThread();
			PoolMemory<T>* ret=new PoolMemory<T>();
			memoryThread->pools.insert(std::make_pair(sizeof(T), ret));
			return ret;
		}
	};
}

#endif //ALLOCATOR_H_
