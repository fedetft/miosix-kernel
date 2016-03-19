#include "../filesystem/devfs/devfs.h"
#include "queue.h"
#include <vector>
#include "kernel.h"

using namespace std;

namespace miosix {
	
	class IRQDisplayPrint : public Device
	{
	public:
		IRQDisplayPrint();
		~IRQDisplayPrint();
		
		void IRQwrite(const char *str);
		ssize_t writeBlock(const void *buffer, size_t size, off_t where);

		void printIRQ();
	private:
		Queue<string, 20> input_queue;
		vector<string> print_lines;

		int right_margin;
		int bottom_margin;
		bool carriage_return_enabled;

		bool carriage_return_fix(string str);
		void process_string(string str);
		void check_array_overflow();
		void internal_print();
	};
};
