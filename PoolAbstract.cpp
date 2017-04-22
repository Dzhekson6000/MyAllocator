#include "PoolAbstract.h"
#include <string>


using namespace doxyCraft;

PoolAbstract::PoolAbstract():_useBlock(0)
{
	_memorySegment.resize(10);
}

PoolAbstract::~PoolAbstract()
{
	for( auto segment:_memorySegment )
	{
		::free(segment);
	}
}

bool PoolAbstract::isUse()
{
	return _useBlock != 0;
}

void PoolAbstract::addPage(size_t sizeBlock)
{
	size_t sizePage = sizeBlock*DEFAULT_COUNT_OBJECT;
	void* page = malloc(sizePage);
	_memorySegment.push_back(page);
	void* currentBlock = (uint8_t*)page + sizePage - sizeBlock;
	if( _firstFreeBlock )
	{
		currentBlock = _firstFreeBlock;
	}
	else
	{
		currentBlock = nullptr;
	}
	
	void* nextBlock;
	for( size_t i = sizeBlock; i < sizePage; )
	{
		nextBlock = currentBlock;
		currentBlock = (uint8_t*)currentBlock - sizeBlock;
		*(void**)currentBlock = nextBlock;
		i += sizeBlock;
	}
	_firstFreeBlock = page;
	
}

