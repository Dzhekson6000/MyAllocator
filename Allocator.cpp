#include "Allocator.h"

using namespace doxyCraft;

CREATE_SINGLETON_CPP(Allocator);


bool Allocator::init()
{
	return true;
}

Allocator::Allocator()
{
	
}

Allocator::~Allocator()
{
	
}


Allocator::MemoryThread* Allocator::getMemoryThread()
{
	_memoryThreadsMutex.lockRead();
	HashMapMemoryThread::iterator it;
	it = _memoryThreads.find(std::this_thread::get_id());
	if( it != _memoryThreads.end())
	{
		_memoryThreadsMutex.unlock();
		return it->second;
	}
	_memoryThreadsMutex.unlock();
	return addMemoryThread();
}

Allocator::MemoryThread* Allocator::addMemoryThread()
{
	_memoryThreadsMutex.lockWrite();
	MemoryThread* ret = new MemoryThread();
	ret->segmentDynamicMemory.setDynamic(true);
	_memoryThreads.insert(std::make_pair(std::this_thread::get_id(), ret));
	_memoryThreadsMutex.unlock();
	return ret;
}

void Allocator::freeThread()
{
	MemoryThread* memoryThread = getMemoryThread();
	
	for( auto                     it: memoryThread->pools )
	{
		delete it.second;
	}
	HashMapMemoryThread::iterator it;
	_memoryThreadsMutex.lockWrite();
	it = _memoryThreads.find(std::this_thread::get_id());
	_memoryThreads.erase(it);
	_memoryThreadsMutex.unlock();
}

void Allocator::freeFreeSegmentThread()
{
	MemoryThread* memoryThread = getMemoryThread();
	
	HashMapPools::iterator cur = memoryThread->pools.begin();
	HashMapPools::iterator end = memoryThread->pools.end();
	
	while( cur != end )
	{
		if( !cur->second->isUse())
		{
			delete cur->second;
			memoryThread->pools.erase(cur);
		}
		++cur;
	}
	
	memoryThread->segmentMemory.freeFreeSegment();
	memoryThread->segmentDynamicMemory.freeFreeSegment();
}






