#ifndef SEGMENTMEMORY_H_
#define SEGMENTMEMORY_H_

#include <cstring>
#include "Shared/Macros.h"

#define MIN_SIGMENT_SIZE 2048
#define ZOOM_SEGMENT_DYNAMIC 10

namespace doxyCraft
{
	class SegmentMemory
	{
	public:
		struct SegmentStruct
		{
			SegmentStruct* next;
			size_t size;
			size_t maxSizeFreePage;
		};
		//#pragma pack(push, 1)
		struct PageStruct
		{
			PageStruct* prev;
			size_t size;
			bool   isUse;
		};
		//#pragma pack(pop)
		
		SegmentMemory();
		virtual ~SegmentMemory();
		
		void* alloc(size_t size);
		void* reloc(void* ptr, size_t newSize);
		static void free(void* ptr);
		
		unsigned int freeFreeSegment();
		void freeAllSegment();
				
		void printMemory() const;
		void printSegmentMemory(void* ptr) const;
		void printPageMemory(PageStruct* ptr) const;
	SYNTHESIZE(bool, _isDynamic, Dynamic);
	private:
		SegmentStruct* _firstSegment;
		static size_t _sizePageStruct;
		static size_t _sizeSegmentStruct;
		
		SegmentStruct* getSegment(size_t size);
		SegmentStruct* addSegment(size_t size);
		static void calcMaxSizeSegment(SegmentStruct* ptr);
		static SegmentStruct* getSegmentOfPage(PageStruct* ptr);
		
		static PageStruct* getFreePageOfSegment(SegmentStruct* ptr, size_t size);
		
		static inline SegmentStruct* getSS(void* ptr);
		static inline PageStruct* getPS(void* ptr);
		static inline PageStruct* getNextPS(PageStruct* ptr);
		
	};
}

#endif //SEGMENTMEMORY_H_
