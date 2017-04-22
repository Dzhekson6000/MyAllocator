#ifndef POOLABSTRACT_H_
#define POOLABSTRACT_H_

#include <cstring>
#include <vector>

#define DEFAULT_COUNT_OBJECT 100

namespace doxyCraft
{
	class PoolAbstract
	{
	public:
		PoolAbstract();
		virtual ~PoolAbstract();
		
		bool isUse();
	protected:
		void* _firstFreeBlock;
		unsigned int       _useBlock;
		std::vector<void*> _memorySegment;
		
		void addPage(size_t sizeBlock);
	};
}

#endif //POOLABSTRACT_H_
