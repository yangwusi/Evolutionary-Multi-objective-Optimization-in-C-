#include "ui/experiment_panel.h"

#include <iostream>
#include <thread>

#include "emoc_app.h"
#include "imgui.h"
#include "ui/ui_utility.h"
#include "ui/uipanel_manager.h"

namespace emoc {

	static double CalculateIndicator(const std::vector<double>& indicator_history, const std::vector<bool>& is_indicator_record, bool &is_valid)
	{
		double res = 0.0;
		int count = 0;
		for (int i = 0; i < indicator_history.size(); i++)
		{
			if (is_indicator_record[i])
			{
				res += indicator_history[i];
				count++;
			}
		}
		if (count > 0) is_valid = true;
		return res / (double)count;
	}


	ExperimentPanel::ExperimentPanel():
		algorithm_index(0),
		problem_index(0),
		display_index(0)
	{

		InitDisplayList(display_names);
		InitAlgorithmList(algorithm_names);
		InitProlbemList(problem_names);
	}

	ExperimentPanel::~ExperimentPanel()
	{

	}

	void ExperimentPanel::Render()
	{
		// normal way to set table columns
		//ImGui::TableSetupColumn("Run #", ImGuiTableColumnFlags_None);
		//ImGui::TableSetupColumn("Algorithm", ImGuiTableColumnFlags_None);
		//ImGui::TableSetupColumn("Problem", ImGuiTableColumnFlags_None);
		//ImGui::TableSetupColumn("N", ImGuiTableColumnFlags_None);
		//ImGui::TableSetupColumn("M", ImGuiTableColumnFlags_None);
		//ImGui::TableSetupColumn("D", ImGuiTableColumnFlags_None);
		//ImGui::TableSetupColumn("Evaluation", ImGuiTableColumnFlags_None);
		//ImGui::TableSetupColumn("Runtime", ImGuiTableColumnFlags_None);
		//ImGui::TableHeadersRow();
		
		// update EMOC experiment module running state
		bool is_finish = EMOCManager::Instance()->GetExperimentFinish();
		bool is_pause = EMOCManager::Instance()->GetExperimentPause();

		// Experiment Module Algorithm and Problem Selection Window
		{
			ImGui::Begin("Algorithm and Problem Selection##Experiment");

			// get some basic window property
			float window_width = ImGui::GetWindowSize().x;
			float window_height = ImGui::GetWindowSize().y;
			const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
			int list_height = (int)(0.28f * window_height / TEXT_BASE_HEIGHT);		// adaptive list height in number of items
			list_height = list_height > 3 ? list_height : 3;						// minimal height of list

			// Algorithm selection part
			TextCenter("Algorithm Selection");
			ImGui::Dummy(ImVec2(0.0f, 2.0f));
			ImGui::SetNextItemWidth(-FLT_MIN);
			bool is_value_changed = ImGui::ListBox("##AlgorithmExperimentListbox", &algorithm_index, algorithm_names.data(), algorithm_names.size(), list_height);
			if (is_value_changed && selected_algorithm_map.count(algorithm_names[algorithm_index]) == 0)
			{
				selected_algorithm_map[algorithm_names[algorithm_index]] = 1;
				selected_algorithms.push_back(algorithm_names[algorithm_index]);
			}
			ImGui::Dummy(ImVec2(0.0f, 20.0f));


			// Problem selection part
			ImGui::Separator();
			TextCenter("Problem Selection");
			ImGui::Dummy(ImVec2(0.0f, 2.0f));
			ImGui::SetNextItemWidth(-FLT_MIN);
			is_value_changed = ImGui::ListBox("##ProblemExperimentListbox", &problem_index, problem_names.data(), problem_names.size(), list_height);
			if (is_value_changed && selected_problem_map.count(problem_names[problem_index]) == 0)
			{
				selected_problem_map[problem_names[problem_index]] = 1;
				selected_problems.push_back(problem_names[problem_index]);

				// add some default problem settings
				Ns.push_back(100);
				Ms.push_back(2);
				Ds.push_back(30);
				Evaluations.push_back(25000);
			}
			ImGui::Dummy(ImVec2(0.0f, 20.0f));

			// set number of run and save interval inputs position
			float current_posY = ImGui::GetCursorPosY();
			float max_text_width = ImGui::CalcTextSize("Number of runs").x;
			float remain_width = window_width - max_text_width;
			float input_pos = max_text_width + 0.28f * remain_width;
			float height_pos = (window_height * 0.902f - TEXT_BASE_HEIGHT * 3.0f);
			height_pos = height_pos > current_posY ? height_pos : current_posY + 10.0f;

			ImGui::SetCursorPosY(height_pos);
			ImGui::PushItemWidth(0.71f * remain_width);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Number of Runs");
			ImGui::SameLine(); ImGui::SetCursorPosX(input_pos);
			ImGui::InputInt("##Runs", &run_num, 0);

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Save Interval");
			ImGui::SameLine(); HelpMarker("Population file is saved per 'Save Interval' generations.\n"
				"If 'Save Interval' > max generation number, it will save\n"
				" the first and last generation as default. ");
			ImGui::SameLine(); ImGui::SetCursorPosX(input_pos);
			ImGui::InputInt("##SaveInterval", &save_interval, 0);
			ImGui::PopItemWidth();


			// set start button position
			current_posY = ImGui::GetCursorPosY();
			height_pos = window_height * 0.902f > current_posY ? window_height * 0.902f : current_posY + 10.0f;
			ImGui::SetCursorPos(ImVec2(0.025f * window_width, height_pos));

			// When algorithm is running, we diable 'Start' button.
			if (!is_finish) ImGui::BeginDisabled();
			if (ImGui::Button("Start##Experiment", ImVec2(window_width * 0.95f, window_height * 0.08f)))
			{
				EMOCManager::Instance()->SetIsExperiment(true);
				// set the algorithms and problems display in table
				table_algorithms  = selected_algorithms;
				table_problems	  = selected_problems;
				table_Ns          = Ns;
				table_Ds		  = Ds;
				table_Ms		  = Ms;
				table_Evaluations = Evaluations;



				std::cout << Ns.size() << "\n";
				ConstructTasks();
				if (experiment_tasks.size() > 0)
				{
					{
						std::lock_guard<std::mutex> locker(EMOCLock::multithread_data_mutex);
						EMOCManager::Instance()->SetMultiThreadDataState(false);
					} 

					std::thread algorithm_thread(&EMOCManager::ExperimentModuleRun, EMOCManager::Instance(), experiment_tasks, thread_num);
					algorithm_thread.detach();
				}

			}
			if (!is_finish) ImGui::EndDisabled();



			ImGui::End();
		}

		// Experiment Module Parameter Setting Window
		{
			ImGui::Begin("Parameter Setting##Experiment");

			TextCenter("Selected Algorithm");
			ImGui::Dummy(ImVec2(0.0f, 2.0f));
			for (int i = 0; i < selected_algorithms.size(); i++)
				DisplaySelectedAlgorithm(i);
			ImGui::Dummy(ImVec2(0.0f, 20.0f));
			ImGui::Separator();


			TextCenter("Problem Selection");
			// set selected problem display position
			float window_width = ImGui::GetWindowSize().x;
			float window_height = ImGui::GetWindowSize().y;
			float max_text_width = ImGui::CalcTextSize("Evaluation").x;
			float item_width = (window_width - max_text_width) * 0.81f;
			float item_pos = max_text_width + 0.18f * (window_width - max_text_width);

			ImGui::Dummy(ImVec2(0.0f, 2.0f));
			for (int i = 0; i < selected_problems.size(); i++)
			{
				DisplaySelectedProblem(i, item_width, item_pos);
			}
			ImGui::End();
		}

		// Experiment Moduel Control Window
		{
			ImGui::Begin("Control##Experiment");
			float window_width = ImGui::GetWindowSize().x;
			float window_height = ImGui::GetWindowSize().y;
			float text_width = ImGui::CalcTextSize("1000000 evaluations").x;
			float remain_width = window_width - text_width;
			float remain_height = (window_height - 150.0f) > 0.0f ? window_height - 150.0f : 0.0f;
			ImGui::Dummy(ImVec2(0.0f, remain_height * 0.5f));	// for vertical center


			// a simple progress bar
			//static float progress = 0.0f;
			//progress = (float)current_evaluation / (float)max_evaluation;
			//if (progress > 1.0f) progress = 1.0f;
			//ImGui::SetNextItemWidth(remain_width * 0.95f);
			//ImGui::ProgressBar(progress, ImVec2(0.f, 0.f));
			//ImGui::SameLine(); ImGui::Dummy(ImVec2(2.0f, 0.0f)); ImGui::SameLine(); ImGui::Text("%d evaluations", current_evaluation);
			//ImGui::Dummy(ImVec2(0.0f, 10.0f));


			// put the button at the appropriate position
			float button_pos = remain_width * 0.95f > 320.0f ? (remain_width * 0.95 - 320.0) * 0.5f : 0.0f;
			ImGui::SetCursorPosX(button_pos);

			// When algorithm (is not running) or (running but not paused), we diable 'Continue' button.
			if (is_finish || (!is_finish && !is_pause)) ImGui::BeginDisabled();
			if (ImGui::Button("Continue##Experiment", ImVec2(100, 60)))
			{
				std::cout << "Continue!\n";
				std::lock_guard<std::mutex> locker(EMOCLock::experiment_pause_mutex);
				EMOCManager::Instance()->SetExperimentPause(false);
				std::cout << "After click continue button, the pause value is: " << EMOCManager::Instance()->GetExperimentPause() << "\n";
				EMOCLock::experiment_pause_cond.notify_all();
			}
			ImGui::SameLine(); ImGui::Dummy(ImVec2(10.0f, 0.0f)); ImGui::SameLine();
			if (is_finish || (!is_finish && !is_pause)) ImGui::EndDisabled();

			// When algorithm (is not running) or (is running but paused), we diable 'Pause' and 'Stop' button.
			if (is_finish || (!is_finish && is_pause)) ImGui::BeginDisabled();
			if (ImGui::Button("Pause##Experiment", ImVec2(100, 60)))
			{
				std::cout << "Pause\n";
				std::lock_guard<std::mutex> locker(EMOCLock::experiment_pause_mutex);
				EMOCManager::Instance()->SetExperimentPause(true);
			}
			ImGui::SameLine(); ImGui::Dummy(ImVec2(10.0f, 0.0f)); ImGui::SameLine();

			if (ImGui::Button("Stop##Experiment", ImVec2(100, 60)))
			{
				std::cout << "Stop!\n";
				std::lock_guard<std::mutex> locker(EMOCLock::experiment_finish_mutex);
				EMOCManager::Instance()->SetExperimentFinish(true);
				std::cout << "After click continue button, the finish value is: " << EMOCManager::Instance()->GetExperimentFinish() << "\n";
			}
			ImGui::SameLine(); ImGui::Dummy(ImVec2(10.0f, 0.0f)); ImGui::SameLine(); \
				if (is_finish || (!is_finish && is_pause)) ImGui::EndDisabled();

			ImGui::End();
		}

		// Experiment Module Result Infomation Window
		{
			ImGui::Begin("Result Infomation##Experiment");
			const float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
			const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
			static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable;

			std::vector<std::string> columns;
			TextCenter("Result Info Display");
			ImGui::Dummy(ImVec2(0.0f, 5.0f));
			static bool is_displayN = true; ImGui::Checkbox("N##Experiment", &is_displayN); ImGui::SameLine();
			static bool is_displayM = true; ImGui::Checkbox("M##Experiment", &is_displayM); ImGui::SameLine();
			static bool is_displayD = true; ImGui::Checkbox("D##Experiment", &is_displayD); ImGui::SameLine();
			static bool is_displayEvaluation = true; ImGui::Checkbox("Evaluation##Experiment", &is_displayEvaluation); ImGui::SameLine();
			ImGui::SetNextItemWidth(ImGui::CalcTextSize("Runtimexxx").x);
			ImGui::Combo("##DisplayExperiment", &display_index, display_names.data(), display_names.size());


			// set this frame's columns
			if (is_displayN) columns.push_back("N");
			if (is_displayM) columns.push_back("M");
			if (is_displayD) columns.push_back("D");
			if (is_displayEvaluation) columns.push_back("Evaluation");

			// When using ScrollX or ScrollY we need to specify a size for our table container!
			// Otherwise by default the table will fit all available space, like a BeginChild() call.
			ImVec2 outer_size = ImVec2(0.0f, ImGui::GetContentRegionAvail().y - 45.0f);
			int column_count = table_algorithms.size() + columns.size() + 1;
			if (ImGui::BeginTable("ResultTable", column_count, flags, outer_size))
			{
				ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible

				// Instead of calling TableHeadersRow() we'll submit custom headers ourselves
				ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
				
				// first column is an empty header
				ImGui::TableHeader("    ");
				for (int c = 0; c < columns.size(); c++)
				{
					ImGui::TableSetColumnIndex(c+1);
					const char* column_name = columns[c].c_str(); 
					ImGui::TableHeader(column_name);
				}
				for (int c = 0; c < table_algorithms.size(); c++)
				{
					ImGui::TableSetColumnIndex(c + columns.size()+1);
					const char* column_name = table_algorithms[c].c_str();
					ImGui::TableHeader(column_name);
				}


				int rows = table_problems.size();
				for (int row = 0; row < rows; row++)
				{
					ImGui::TableNextRow();
					//const EMOCSingleThreadResult& res = EMOCManager::Instance()->GetSingleThreadResult(row);
					for (int c = 0; c < column_count; c++)
					{
						ImGui::TableSetColumnIndex(c);
						if (c == 0)
						{
							ImU32 cell_bg_color = ImGui::GetColorU32(ImVec4(0.18f, 0.18f, 0.32f, 1.00f));
							ImGui::Text(table_problems[row].c_str());
							ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, cell_bg_color);
						}
						else if (c > 0 && c < columns.size() + 1)
						{
							int col_index = c - 1;
							DisplayTableProblemProperty(columns[col_index], row);
						}
						else
						{
							// update table's data when data is ready
							if (EMOCManager::Instance()->GetMultiThreadDataState())
							{
								int paramter_index = row * table_algorithms.size() + c - columns.size() - 1;
								const EMOCMultiThreadResult& res = EMOCManager::Instance()->GetMultiThreadResult(paramter_index);
								DisplayTableResult(res, display_names[display_index]);
							}
						}
					}
				}

				ImGui::EndTable();
			}
			ImGui::SetCursorPosX(ImGui::GetWindowSize().x - 260.0f);
			if (ImGui::Button("Open Plot Window", ImVec2(250.0f, 40.0f)))
			{
				UIPanelManager::Instance()->SetUIPanelState(UIPanel::TestPanel);
			}
			ImGui::End();
		}
	}

	void ExperimentPanel::DisplayTableResult(const EMOCMultiThreadResult& res, const std::string& para)
	{
		double display = 0.0;
		bool is_valid = false;
		if (para == "Runtime")
		{
			display = CalculateIndicator(res.runtime_history, res.is_runtime_record, is_valid);
		}
		else if (para == "IGD")
		{
			display = CalculateIndicator(res.igd_history, res.is_igd_record, is_valid);
		}
		else if (para == "HV")
		{
			display = CalculateIndicator(res.hv_history, res.is_hv_record, is_valid);
		}

		if(is_valid) ImGui::Text(std::to_string(display).c_str());
		
	}

	void ExperimentPanel::DisplayTableProblemProperty(const std::string& col_name, int row)
	{
		if (col_name == "N")
			ImGui::Text("%d", table_Ns[row]);
		else if (col_name == "M")
			ImGui::Text("%d", table_Ms[row]);
		else if (col_name == "D")
			ImGui::Text("%d", table_Ds[row]);
		else if (col_name == "Evaluation")
			ImGui::Text("%d", table_Evaluations[row]);
	}

	void ExperimentPanel::DisplaySelectedAlgorithm(int index)
	{
		const std::string& algorithm = selected_algorithms[index];
		std::string header_name = algorithm + "##" + std::to_string(index);
		bool is_open = false;
		if (is_open = ImGui::CollapsingHeader(header_name.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
		{
			DisplayMovePopup(index, true);
			DisplayAlgorithmParameters(algorithm);
		}
		if (!is_open) DisplayMovePopup(index, true);
	}

	void ExperimentPanel::DisplaySelectedProblem(int index, int item_width, int item_pos)
	{
		char name[256];
		const std::string& problem = selected_problems[index];
		std::string header_name = problem + "##" + std::to_string(index);
		bool is_open = false;
		if (is_open = ImGui::CollapsingHeader(header_name.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
		{
			DisplayMovePopup(index, false);
			ImGui::PushItemWidth(item_width);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("D"); ImGui::SameLine();
			ImGui::SetCursorPosX(item_pos);
			sprintf(name, "##DExperiment%d", index);
			ImGui::InputInt(name, &Ds[index], 0);

			ImGui::AlignTextToFramePadding();
			ImGui::Text("M"); ImGui::SameLine();
			ImGui::SetCursorPosX(item_pos);
			sprintf(name, "##MExperiment%d", index);
			ImGui::InputInt(name, &Ms[index], 0);

			ImGui::AlignTextToFramePadding();
			ImGui::Text("N"); ImGui::SameLine();
			ImGui::SetCursorPosX(item_pos);
			sprintf(name, "##NExperiment%d", index);
			ImGui::InputInt(name, &Ns[index], 0);

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Evaluation"); ImGui::SameLine();
			ImGui::SetCursorPosX(item_pos);
			sprintf(name, "##EvaluationExperiment%d", index);
			ImGui::InputInt(name, &Evaluations[index], 0);
			ImGui::PopItemWidth();
		}
		if (!is_open) DisplayMovePopup(index, false);
	}

	void ExperimentPanel::DisplayMovePopup(int index, bool is_algorithm_popup)
	{
		if (ImGui::BeginPopupContextItem())
		{
			ImGui::Button("Move Up     ");
			ImGui::Button("Move Down");
			ImGui::Button("Delete          ");

			ImGui::EndPopup();
		}
	}

	void ExperimentPanel::ConstructTasks()
	{
		experiment_tasks.clear();
		for (int i = 0; i < selected_problems.size(); i++)
		{
			std::string problem = selected_problems[i];
			int N = Ns[i];
			int M = Ms[i];
			int D = Ds[i];
			int Evaluation = Evaluations[i];

			for (int j = 0; j < selected_algorithms.size(); j++)
			{
				std::string algorithm = selected_algorithms[j];

				EMOCParameters para;
				para.is_plot = false;
				para.algorithm_name = algorithm;
				para.problem_name = problem;
				para.population_num = N;
				para.objective_num = M;
				para.decision_num = D;
				para.max_evaluation = Evaluation;
				para.runs_num = run_num;
				para.output_interval = save_interval;
				experiment_tasks.push_back(para);
			}
		}
	}

}