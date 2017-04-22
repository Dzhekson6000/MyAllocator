#include "SegmentMemory.h"
#include "Shared/Utility/Logger/Logger.h"
#include <sstream>

using namespace doxyCraft;

size_t SegmentMemory::_sizePageStruct=sizeof(PageStruct);

size_t SegmentMemory::_sizeSegmentStruct=sizeof(SegmentStruct);

SegmentMemory::SegmentMemory():_isDynamic(false),
		_firstSegment(nullptr)
{

}

SegmentMemory::~SegmentMemory()
{
	freeAllSegment();
}

SegmentMemory::SegmentStruct* SegmentMemory::addSegment(size_t size)
{
	size_t sizeSegment=size>=MIN_SIGMENT_SIZE ? size : MIN_SIGMENT_SIZE;
	if(_isDynamic)
	{
		sizeSegment=sizeSegment*ZOOM_SEGMENT_DYNAMIC;
	}
	size_t fullSizeSegment=sizeSegment+_sizeSegmentStruct+_sizePageStruct;
	uint8_t* segment=(uint8_t*)malloc(fullSizeSegment);

	getSS(segment)->size=fullSizeSegment;
	getSS(segment)->maxSizeFreePage=sizeSegment;

	PageStruct* pageStruct=getPS(segment+_sizeSegmentStruct);
	pageStruct->isUse=false;
	pageStruct->prev=nullptr;
	pageStruct->size=sizeSegment;

	if(_firstSegment)
	{
		getSS(segment)->next=_firstSegment;
	}
	else
	{
		getSS(segment)->next=nullptr;
	}
	_firstSegment=getSS(segment);
	return _firstSegment;
}

void* SegmentMemory::alloc(size_t size)
{
	SegmentStruct* segment=getSegment(size);
	uint8_t* endSegment=(uint8_t*)segment+segment->size;

	PageStruct* page=getFreePageOfSegment(segment, size);

	if(page->size-size>_sizePageStruct)
	{
		PageStruct* nextPage=getNextPS(page);
		size_t newSizeNextPage=0;

		if(_isDynamic && page->size/2>_sizePageStruct+size)
		{
			size_t left=page->size/2;
			PageStruct* prevPage=page;

			newSizeNextPage=page->size-left-(_sizePageStruct+size);
			page=getPS((uint8_t*)page+_sizePageStruct+left);

			prevPage->size=left;
			page->prev=prevPage;
		}
		else
		{
			newSizeNextPage=page->size-size;
		}

		if(newSizeNextPage>_sizePageStruct)
		{
			void* newPage=(uint8_t*)page+(_sizePageStruct+size);
			getPS(newPage)->isUse=false;
			getPS(newPage)->size=newSizeNextPage-_sizePageStruct;
			getPS(newPage)->prev=page;

			if((uint8_t*)nextPage!=endSegment)
			{
				nextPage->prev=getPS(newPage);
			}
		}
		else
		{
			size+=newSizeNextPage;
			if((uint8_t*)nextPage!=endSegment)
			{
				nextPage->prev=getPS(page);
			}
		}

		page->isUse=true;
		page->size=size;

		calcMaxSizeSegment(segment);
	}
	else
	{
		page->isUse=true;
		if(page->size==segment->maxSizeFreePage)
		{
			calcMaxSizeSegment(segment);
		}
	}
	return (void*)((uint8_t*)page+_sizePageStruct);
}

void* SegmentMemory::reloc(void* ptr, size_t newSize)
{
	PageStruct* page=getPS((uint8_t*)ptr-_sizePageStruct);
	SegmentStruct* segment=getSegmentOfPage(page);
	uint8_t* endSegment=(uint8_t*)segment+segment->size;

	PageStruct* nextPage=getNextPS(page);

	if((uint8_t*)nextPage!=endSegment &&
	   !nextPage->isUse &&
	   page->size+_sizePageStruct+nextPage->size>newSize)
	{
		size_t oldSize=nextPage->size;
		size_t sizeNewPage=page->size+_sizePageStruct+nextPage->size;
		PageStruct* nextNextPage=getNextPS(nextPage);

		if(sizeNewPage-newSize>_sizePageStruct)
		{
			nextPage=getPS((uint8_t*)page+_sizePageStruct+newSize);
			page->size=newSize;

			nextPage->prev=page;
			nextPage->size=sizeNewPage-newSize-_sizePageStruct;
			nextPage->isUse=false;
		}
		else
		{
			page->size=sizeNewPage;
			nextPage=page;
		}

		if((uint8_t*)nextNextPage!=endSegment)
		{
			nextNextPage->prev=nextPage;
		}

		if(segment->maxSizeFreePage==oldSize)
		{
			calcMaxSizeSegment(segment);
		}
	}
	else if(newSize > page->size)
	{
		void* newPtr=alloc(newSize);
		memcpy(newPtr, ptr, page->size);
		free(ptr);
		page=getPS((uint8_t*)newPtr-_sizePageStruct);
	}

	return (void*)((uint8_t*)page+_sizePageStruct);
}

void SegmentMemory::free(void* ptr)
{
	PageStruct* page=getPS((uint8_t*)ptr-_sizePageStruct);
	SegmentStruct* segment=getSegmentOfPage(page);
	uint8_t* endSegment=(uint8_t*)segment+segment->size;

	page->isUse=false;

	PageStruct* nextPage=getNextPS(page);
	if((uint8_t*)nextPage!=endSegment && !nextPage->isUse)
	{
		PageStruct* nextNextPage=getNextPS(nextPage);
		page->size+=_sizePageStruct+nextPage->size;
		if((uint8_t*)nextNextPage!=endSegment)
		{
			nextNextPage->prev=page;
		}
	}

	PageStruct* prev=page->prev;
	if(prev!=nullptr && !prev->isUse)
	{
		nextPage=getNextPS(page);
		prev->size+=_sizePageStruct+page->size;
		nextPage->prev=prev;
		page=prev;
	}

	if(page->size>segment->maxSizeFreePage)
	{
		calcMaxSizeSegment(segment);
	}
}

void SegmentMemory::calcMaxSizeSegment(SegmentStruct* ptr)
{
	size_t maxSizeFreePage=0;
	uint8_t* endSegment=(uint8_t*)ptr+ptr->size;
	for(PageStruct* page=getPS((uint8_t*)ptr+_sizeSegmentStruct);
		(uint8_t*)page!=endSegment;
		page=getNextPS(page))
	{
		if(!page->isUse && page->size>maxSizeFreePage)
		{
			maxSizeFreePage=page->size;
		}
	}
	ptr->maxSizeFreePage=maxSizeFreePage;
}


SegmentMemory::SegmentStruct* SegmentMemory::getSegment(size_t size)
{
	SegmentStruct* ret=nullptr;
	for(ret=_firstSegment;
		ret!=nullptr;
		ret=ret->next)
	{
		if(ret->maxSizeFreePage>=size)
		{
			break;
		}
	}

	if(!ret)
	{
		ret=addSegment(size);
	}
	return ret;
}

SegmentMemory::SegmentStruct* SegmentMemory::getSegmentOfPage(PageStruct* ptr)
{
	PageStruct* corentPage=ptr;
	for(; corentPage->prev!=nullptr;
		  corentPage=corentPage->prev
			);

	return getSS((uint8_t*)corentPage-_sizeSegmentStruct);
}

SegmentMemory::PageStruct* SegmentMemory::getFreePageOfSegment(SegmentStruct* ptr, size_t size)
{
	void* endSegment=(uint8_t*)ptr+ptr->size;
	PageStruct* ret=nullptr;
	for(ret=getPS((uint8_t*)ptr+_sizeSegmentStruct);
		ret!=endSegment;
		ret=getNextPS(ret))
	{
		if(!ret->isUse && ret->size>=size)
		{
			break;
		}
	}
	return ret;
}


SegmentMemory::SegmentStruct* SegmentMemory::getSS(void* ptr)
{
	return (SegmentStruct*)ptr;
}

SegmentMemory::PageStruct* SegmentMemory::getPS(void* ptr)
{
	return (PageStruct*)ptr;
}


SegmentMemory::PageStruct* SegmentMemory::getNextPS(PageStruct* ptr)
{
	return getPS((uint8_t*)ptr+(_sizePageStruct+ptr->size));
}


void SegmentMemory::printMemory() const
{
	for(SegmentStruct* segment=_firstSegment;
		segment!=nullptr;
		segment=segment->next
			)
	{
		printSegmentMemory(segment);
	}
}

void SegmentMemory::printSegmentMemory(void* ptr) const
{
	std::stringstream log;
	log<<"Segment["<<ptr<<"]"<< getSS(ptr)->maxSizeFreePage << "/"<< getSS(ptr)->size;
	String loge = String(log.str().c_str());
	Logger::getInstance()<<Log("SegmentMemory", loge);

	void* endSegment=(uint8_t*)ptr+getSS(ptr)->size;
	for(void* page=(uint8_t*)ptr+_sizeSegmentStruct;
		page!=endSegment;
		page=(uint8_t*)page + _sizePageStruct+getPS(page)->size)
	{
		printPageMemory(getPS(page));
	}
}

void SegmentMemory::printPageMemory(PageStruct* ptr) const
{
	std::stringstream log;
	log<<"["<<(void*)ptr<<"]("<<ptr->isUse<<")"<<ptr->size;
	Logger::getInstance()<<Log("SegmentMemory", String(log.str().c_str()));
}


unsigned int SegmentMemory::freeFreeSegment()
{
	unsigned int ret = 0;
	SegmentStruct* currentSegment=_firstSegment;
	SegmentStruct* prevSegment=nullptr;
	while(currentSegment)
	{
		if(currentSegment->maxSizeFreePage == currentSegment->size - _sizeSegmentStruct - _sizePageStruct)
		{
			if(prevSegment)
			{
				prevSegment->next = currentSegment->next;
			}
			else
			{
				_firstSegment=currentSegment->next;
			}
			prevSegment=currentSegment;
			::free((void*)currentSegment);
			currentSegment=prevSegment->next;
			continue;
		}
		prevSegment=currentSegment;
		currentSegment=currentSegment->next;
		++ret;
	}
	return ret;
}

void SegmentMemory::freeAllSegment()
{
	while(_firstSegment)
	{
		auto ptr = _firstSegment;
		_firstSegment = _firstSegment->next;
		delete ptr;
	}
	_firstSegment = nullptr;
}


