#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring> // memset
#include <iomanip> // stdprecision
#include <string>
#include <regex>
#include <vector>
#include "sndfile.h"

using namespace std;

string read_file(const char *filename)
{
	string s = "-1";
	ifstream infile(filename);
	if (infile)
	{
		ostringstream oss;
		oss << infile.rdbuf();
		s = oss.str();
	}
	else
	{
		cerr << "Couldn't open file '" << filename << "'" << endl;
	}
	return s;
}

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		std::cerr << R"(Syntax: csv2wav input.csv output.wav
)";

		return -1;
	}

	char *infilename = argv[1];
	char *outfilename = argv[2];

	vector<float> float_data;
	std::string str;
	str = read_file(infilename);

	// Tokenize str to floats, recording min and max values
	regex re("[,;\\s ]+");
	sregex_token_iterator p(str.begin(), str.end(), re, -1);
	sregex_token_iterator end;
	float minval = numeric_limits<float>::max(), maxval = numeric_limits<float>::min();
	while (p != end)
	{
		float x = stof(*p);
		if (x < minval) minval = x;
		if (x > maxval) maxval = x;
		float_data.push_back(x);
		p++;
	}

	int samples = float_data.size();

	// Check range and normalize
	bool range_error = false;
	if (minval < -1.0f) range_error = true;
	if (maxval >= 1.0f) range_error = true;
	if (range_error)
	{
		cout << "Expected float range for float 32 WAV file in e.g. Audacity is [-1,1), however:" << endl;
		if (minval < -1.0f)
		{
			cout << "- minimum value is " << minval << " which is < -1 " << endl;
		}
		if (maxval >= 1.0f)
		{
			cout << "- maximum value is " << maxval << " which is >= 1 " << endl;
		}
		float normalization_factor = (1.0f - numeric_limits<float>::epsilon()) / max(abs(minval), abs(maxval));
		cout << setprecision(10);
		cout << "- I will normalize by a factor of " << normalization_factor << endl;
		cout << setprecision(0);
		for (int i = 0; i < samples; ++i)
		{
			float_data[i] *= normalization_factor;
		}
	}

	// Audacity seem to only accept WAV float files in the range [-1,1)
	cout << "\nFirst 10 samples:" << endl;
	for (int i = 0, count = min((size_t)10, float_data.size()); i < count; ++i)
	{
		cout << float_data[i] << " ";
	}
	cout << endl;

	// Output 32-bit float mono WAV file
	cout << "\nOutputting " << samples << " samples to '" << outfilename << "'..." << flush;
	SF_INFO info;
	memset(&info, 0, sizeof(SF_INFO));
	info.channels = 1;
	info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_32;
	info.frames = samples;
	info.samplerate = 48000;
	info.sections = 1;
	SNDFILE *output = sf_open(outfilename, SFM_WRITE, &info);
	sf_write_float(output, float_data.data(), samples);
	sf_write_sync(output);
	sf_close(output);
	cout << " done." << endl;

	return 0;
}
