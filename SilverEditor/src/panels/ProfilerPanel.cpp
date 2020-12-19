#include "core_editor.h"

#include "ProfilerPanel.h"
#include "gui.h"

#include "engine.h"

namespace sv {

	static constexpr u32 HISTORY_COUNT = 200u;

	struct TimeHistory {
		Time times[HISTORY_COUNT];
		u32 pos = 0u;

		void add(Time time) {
			times[pos++] = time;
			if (pos == HISTORY_COUNT) pos = 0u;
		}

		Time get(u32 index) const {
			return times[(pos + index) % HISTORY_COUNT];
		}
	};

	static TimeHistory g_Frame;
	static TimeHistory g_Update;
	static TimeHistory g_Render;
	static TimeHistory g_Animations;

	Time g_PlotLimit = 0.f;

	f32 displayPlot(void* data, i32 index)
	{
		const TimeHistory& history = *reinterpret_cast<const TimeHistory*>(data);
		return history.get(index);
	}

	void plotHisotory(TimeHistory& history, const char* name, Time prefereedTime)
	{
		Time t = history.get(HISTORY_COUNT - 1u) * 0.25f;
		t = t + history.get(HISTORY_COUNT - 2u) * 0.2f;
		t = t + history.get(HISTORY_COUNT - 3u) * 0.15f;
		t = t + history.get(HISTORY_COUNT - 4u) * 0.1f;
		t = t + history.get(HISTORY_COUNT - 5u) * 0.1f;
		t = t + history.get(HISTORY_COUNT - 6u) * 0.05f;
		t = t + history.get(HISTORY_COUNT - 7u) * 0.05f;
		t = t + history.get(HISTORY_COUNT - 8u) * 0.05f;
		t = t + history.get(HISTORY_COUNT - 9u) * 0.05f;

		std::string overlay = name;
		overlay += ": ";
		overlay += std::to_string(t.GetMillisecondsUInt()) + "ms";

		// Compute color
		ImVec4 imColor{ 0.f, 0.f, 0.f, 1.f };
		{
			Time max = prefereedTime + prefereedTime * 0.2f;
			Time min = prefereedTime - prefereedTime * 0.2f;

			f32 norm = (std::max(std::min(t, max), min) - min) / (max - min);

			
			if (norm < 0.25f) {
				imColor.y = 1.f;
			}
			else if (norm < 0.5f) {
				imColor.z = 0.5f;
				imColor.y = 0.5f;
			}
			else if (norm < 0.75f) {
				imColor.x = 0.5f;
			}
			else if (norm < 1.f) {
				imColor.x = 1.f;
			}
		}

		ImGui::PushStyleColor(ImGuiCol_PlotLines, imColor);
		ImGui::PushStyleColor(ImGuiCol_PlotLinesHovered, imColor);

		ImGui::PlotLines("Frame Lines", displayPlot, &history, HISTORY_COUNT, 0, overlay.c_str(), 0.f, g_PlotLimit, { ImGui::GetWindowContentRegionWidth(), 100.f });
		
		ImGui::PopStyleColor(2);
	}

	ProfilerPanel::ProfilerPanel()
	{}

	bool ProfilerPanel::onDisplay()
	{
		g_Frame.add((Time) SV_PROFILER_CHRONO_GET("CPU_Frame"));
		g_Update.add((Time) SV_PROFILER_CHRONO_GET("CPU_Update"));
		g_Render.add((Time) SV_PROFILER_CHRONO_GET("CPU_Render"));
		g_Animations.add((Time) SV_PROFILER_CHRONO_GET("CPU_Animations"));

		// Compute plot limit
		{
			Time maxFrame = float_min;

			foreach(i, HISTORY_COUNT) {
				
				if (maxFrame < g_Frame.times[i]) {
					maxFrame = g_Frame.times[i];
				}
			}

			g_PlotLimit = maxFrame;
		}

		ImGui::Text("CPU");
		
		plotHisotory(g_Frame, "Frame", 1.f / 60.f);
		plotHisotory(g_Update, "Update", 1.f / 30.f);
		plotHisotory(g_Render, "Render", 1.f / 30.f);
		plotHisotory(g_Animations, "Animations", 1.f / 10.f);
		

		return true;
	}

	void ProfilerPanel::onClose()
	{

	}
}