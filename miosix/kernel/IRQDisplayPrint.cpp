#include "IRQDisplayPrint.h"
#include "string"
#include "display.h"
#include <memory>

using namespace std;
using namespace mxgui;

namespace miosix {

	IRQDisplayPrint::IRQDisplayPrint()
		: Device(TTY), input_queue()
		, print_lines(), right_margin(5), bottom_margin(5)
		, carriage_return_enabled(false)
	{
	}

	IRQDisplayPrint::~IRQDisplayPrint()
	{

	}

	void IRQDisplayPrint::IRQwrite(const char * to_print)
	{
		string str = to_print;
		input_queue.put(str);
	}

	ssize_t IRQDisplayPrint::writeBlock(const void * buffer, size_t size, off_t where)
	{
		string str = reinterpret_cast<const char*>(buffer);
		str = str.substr(where, size);
		input_queue.put(str);
		return size;
	}

	void IRQDisplayPrint::printIRQ()
	{
		while (true)
		{
			string tmp;
			input_queue.get(tmp);
			if (tmp.size() == 0)
				continue;
			if (carriage_return_fix(tmp))
				continue;
			process_string(tmp);
			check_array_overflow();
			internal_print();
		}
	}

	//returns true if the current string has to be discarded
	bool IRQDisplayPrint::carriage_return_fix(string str)
	{
		if (!carriage_return_enabled && str.compare("\r\n") == 0)
		{
			carriage_return_enabled = true;
			return true;
		}
		if (str.compare("\r\n") == 0)
		{
			return false;
		}
		else
		{
			carriage_return_enabled = false;
			return false;
		}
	}

	void IRQDisplayPrint::check_array_overflow()
	{
		int font_height, display_h;
		{
			DrawingContext dc(Display::instance());
			font_height = dc.getFont().getHeight();
			display_h = dc.getHeight();
		}
		while (font_height * print_lines.size() >= display_h - bottom_margin)
		{
			print_lines.erase(print_lines.begin());
		}
	}

	void IRQDisplayPrint::process_string(string str)
	{
		int display_w;
		auto_ptr<Font> font;
		{
			DrawingContext dc(Display::instance());
			display_w = dc.getWidth();
			font = auto_ptr<Font>(new Font(dc.getFont()));
		}

		vector<string> lines;
		while (str.length() > 0)
		{
			int lenght = font->calculateLength(str.c_str());
			if (lenght < display_w)
			{
				lines.push_back(str);
				break;
			}
			//else
			int len = 0;
			string str_part;
			for (int i = 1; len < display_w - right_margin; i++)
			{
				str_part = str.substr(0, i);
				len = font->calculateLength(str_part.c_str());
			}
			lines.push_back(str_part);
			str = str.substr(str_part.length());
		}


		for (vector<string>::iterator it = lines.begin(); it != lines.end(); ++it)
		{
			print_lines.push_back(*it);
		}
	}

	void IRQDisplayPrint::internal_print()
	{
		DrawingContext dc(Display::instance());
		//dc.clear(0xFFF);
		int cur_y = 0;

		for (vector<string>::iterator it = print_lines.begin(); it != print_lines.end(); ++it)
		{
			string str = *it;
			dc.write(Point(0, cur_y), str.c_str());
			Point up_left(dc.getFont().calculateLength(str.c_str()), cur_y);
			Point down_right(dc.getWidth() -1, cur_y + dc.getFont().getHeight());
			dc.clear(up_left, down_right, 0x0);
			cur_y += dc.getFont().getHeight();
		}
	}
}