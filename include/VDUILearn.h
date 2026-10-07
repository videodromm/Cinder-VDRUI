#pragma once
#include "cinder/CinderImGui.h"
#include "VDSessionFacade.h"
#include <string>

namespace videodromm
{
	// MIDI learn on a uniform widget: wrap the widget call,
	//   if (learnable(mVDSession, ImGui::SliderFloat(...), uniformIndex)) { ... }
	// Outside learn mode it returns the widget's own result. In learn mode (Midi panel) the
	// widget is outlined (green: already bound, blue: not bound, yellow: armed), a click arms
	// its uniform for the next note / CC / aftertouch, and value changes are ignored.
	inline bool learnable(VDSessionFacadeRef aSession, bool aChanged, int aUniform) {
		if (aUniform < 0 || !aSession->isMidiLearnMode()) return aChanged;
		const bool armed = aSession->getMidiLearnTarget() == aUniform;
		const std::string bound = aSession->getMidiBindingLabel(aUniform);
		ImU32 color = armed ? IM_COL32(255, 200, 0, 255) : (bound.empty() ? IM_COL32(0, 170, 255, 200) : IM_COL32(0, 230, 120, 220));
		ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), color, 0.0f, 0, armed ? 3.0f : 1.5f);
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
			aSession->armMidiLearn(armed ? -1 : aUniform);
		}
		if (ImGui::IsItemHovered()) {
			if (armed) ImGui::SetTooltip("Armed: move a control (note, CC, aftertouch) to bind uniform %d", aUniform);
			else if (bound.empty()) ImGui::SetTooltip("Click to arm MIDI learn for uniform %d", aUniform);
			else ImGui::SetTooltip("Uniform %d bound to %s\nClick to arm and bind another control", aUniform, bound.c_str());
		}
		return false;
	}
}
