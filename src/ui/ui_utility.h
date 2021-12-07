#pragma once
#include <vector>
#include <string>

namespace emoc{

	// text center help function
	void TextCenter(std::string text);

	// tooltip help function
	void HelpMarker(const char* desc);

	// init function for different lists
	void InitAlgorithmList(std::vector<char*>& algorithm_names);
	void InitProlbemList(std::vector<char*>& problem_names);
	void InitDisplayList(std::vector<char*>& display_names);
	void InitPlotMetricList(std::vector<char*>& plot_metric_names);

	// the splitter function is just for test now, it maybe remove in further commits
	bool Splitter(bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size = -1.0f);

	void DisplayAlgorithmParameters(const std::string& algorithm);				// display algorithm's parameters

}