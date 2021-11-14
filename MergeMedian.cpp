/**
MergeMedian v1.0.0 (c) by Mark Spindler

MergeMedian is licensed under a Creative Commons Attribution 3.0 Unported License.

To view a copy of this license, visit http://creativecommons.org/licenses/by/3.0/.
**/


static const char* const CLASS = "MergeMedian";
static const char* const HELP =	"Returns the median of all connected inputs pixel-by-pixel.\n\n"

								"Version: 1.0.0\n"
								"Author: Mark Spindler\n"
								"Contact: info@mark-spindler.com";

static const char* const bbox_names[] = {"union", "A", "B", 0};

#include "DDImage/Iop.h"
#include "DDImage/NukeWrapper.h"
#include "DDImage/Row.h"
#include "DDImage/Knobs.h"
#include <string>



using namespace DD::Image;



class MergeMedian : public Iop
{
	private:
		int _bbox;
  
	public:
		int minimum_inputs() const {return 2;}
		int maximum_inputs() const {return 100;}

		MergeMedian (Node* node) : Iop (node)
		{
			_bbox = 0;
		}

		~MergeMedian ()
		{}

		const char* input_label (int, char*) const;
		void knobs(Knob_Callback);
		void _validate(bool);
		void _request(int, int, int, int, ChannelMask, int);
		void engine(int, int, int, ChannelMask, Row&);

		const char* Class() const {return CLASS;}
		const char* node_help() const {return HELP;}
		static const Iop::Description description;
};


const char* MergeMedian::input_label(int input, char* buffer) const
{
	if (input == 0)
	{
		return "B";
	}

	else if (input == 1)
	{
		if (inputs() < 3)
			return "A";
		else
			return "A1";
	}

	else
	{
		//create label as string (via stringstream)
		std::stringstream label_stream;
		label_stream << "A" << input;
		std::string label_string = label_stream.str();

		//convert string to char and return it
		char* label_char = new char[label_string.length() + 1];
		strcpy(label_char, label_string.c_str());
		return label_char;
		delete [] label_char;
	}
}


void MergeMedian::knobs(Knob_Callback f)
{
	Enumeration_knob(f, &_bbox, bbox_names, "bbox", "set bbox to");
    Tooltip(f, "Clip one input to match the other if wanted");
}


void MergeMedian::_validate(bool for_real)
{
	copy_info();

	for (int i = 1; i < inputs(); i++)
		merge_info(i);

	switch (_bbox)
	{
		case 0:
			info_.set(input0().info());
			for (int i = 1; i < inputs(); i++)
				info_.merge(input(i)->info());
			break;
		case 1:
			info_.set(input1().info());
			for (int i = 2; i < inputs(); i++)
				info_.merge(input(i)->info());
			break;
		case 2:
			info_.set(input0().info());
			break;
    }
}


void MergeMedian::_request(int x, int y, int r, int t, ChannelMask channels, int count)
{
	for (int i = 0; i < inputs(); i++)
		input(i)->request(x, y, r, t, channels, count);
}


void MergeMedian::engine(int y, int x, int r, ChannelMask channels, Row& outrow)
{
	Row inrow(x, r);
	int input_count = inputs();
	
	foreach (z, channels) 
	{	
		float* outptr = outrow.writable(z) + x;
		float* values = new float[(r - x) * input_count];
		float value;

		//collect the values off all input rows and save them in list "values"; list order: [pixel0 of input0, pixel0 of input1, ..., pixel1 of input0, pixel1 of input1, ...]
		for (int i = 0; i < input_count; i++)
		{
			input(i)->get(y, x, r, z, inrow);
			const float* inptr = inrow[z] + x;
			
			for (int xx = 0; xx < r - x; xx++)
    		{
				values[(input_count * xx) + i] = *inptr++;
    		}
		}

		//get median value of list for each pixel and assign value to out-pointer
		for (int xx = 0; xx < r - x; xx++)
    	{
			std::sort(values + (input_count * xx), values + (input_count * xx) + input_count);

			if (input_count % 2 == 1)
				value = values[(input_count * xx) + (input_count / 2)];
			else
				value = (values[(input_count * xx) + (input_count / 2)] + values[(input_count * xx) + (input_count / 2) - 1]) / 2;
			
			*outptr++ = value;
    	}

		delete[] values;
	}
}


static Iop* MergeMedianCreate(Node* node)
{
	return (new NukeWrapper(new MergeMedian(node)))->channelsRGBoptionalAlpha();
}


const Iop::Description MergeMedian::description (CLASS, "MergeMedian", MergeMedianCreate);