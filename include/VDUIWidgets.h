#pragma once
// Shared ImGui widgets for uniforms and colored buttons (2026-11-14, studies/ui-rework.md):
// one place for the uniform slider (range, MIDI learn, reset, label color) and the
// PushStyleColor x3 pattern the panes used to repeat inline.
#include "cinder/CinderImGui.h"
#include "VDSessionFacade.h"
#include "VDUILearn.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace videodromm
{
namespace ui
{
	// a uniform off its default value: the WebApp's --ui-warning (#ffc409), the same signal in both apps
	inline ImVec4 changedTextColor() { return ImVec4(1.0f, 0.77f, 0.035f, 1.0f); }
	// the uniform selected for MIDI (ISELECTED): red, as before; it wins over "changed"
	inline ImVec4 selectedTextColor() { return (ImVec4)ImColor::HSV(0.0f, 1.0f, 1.0f); }

	// Button colored by hue (0..1): bright when on, dark when off.
	inline bool coloredButton(const char* aLabel, float aHue, bool aOn, const ImVec2& aSize = ImVec2(0, 0)) {
		ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(aHue, aOn ? 0.9f : 0.6f, aOn ? 0.6f : 0.22f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(aHue, 0.7f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(aHue, 0.8f, 0.8f));
		const bool clicked = ImGui::Button(aLabel, aSize);
		ImGui::PopStyleColor(3);
		return clicked;
	}

	// true when the uniform's value differs from its default (relative to its range)
	inline bool isOffDefault(VDSessionFacadeRef aSession, unsigned int aIndex, float aLo, float aHi) {
		const float eps = std::max(1e-4f, std::fabs(aHi - aLo) * 1e-4f);
		return std::fabs(aSession->getUniformValue(aIndex) - aSession->getDefaultUniformValue(aIndex)) > eps;
	}

	// Slider (or drag) for a float uniform, drawn full width with its name and value inside the bar.
	//   aLabel: shown name (default: the uniform's name); aLo/aHi: range; the range widens to include
	//   the default and the current value, so neither is clamped by a wrong stored range.
	//   Ctrl+click types a value (ImGui), right-click resets to the default (outside MIDI learn).
	//   Label color: red when selected for MIDI, yellow when off its default (only checked when
	//   aCheckDefault: an animated uniform moves all the time, its mode button shows that instead).
	// Returns true when the user changed the value (already sent to the session).
	inline bool uniformSlider(VDSessionFacadeRef aSession, unsigned int aIndex, float aLo, float aHi,
		const std::string& aLabel = "", bool aCheckDefault = true, float aDragSpeed = 0.0f, float aWidth = -1.0f) {
		const float def = aSession->getDefaultUniformValue(aIndex);
		float value = aSession->getUniformValue(aIndex);
		const float lo = std::min({ aLo, def, value });
		const float hi = std::max({ aHi, def, value });
		std::string name = aLabel.empty() ? aSession->getUniformName(aIndex) : aLabel;
		// the name goes into the printf-style format: escape '%'
		std::string fmt;
		for (char c : name) { fmt += c; if (c == '%') fmt += '%'; }
		fmt += (std::fabs(hi - lo) >= 100.0f) ? " %.1f" : " %.3f";
		char id[32];
		snprintf(id, sizeof(id), "##u%u", aIndex);

		const bool selected = (unsigned int)aSession->getUniformValue(VDUniforms::ISELECTED) == aIndex;
		const bool changed = aCheckDefault && isOffDefault(aSession, aIndex, lo, hi);
		if (selected || changed) ImGui::PushStyleColor(ImGuiCol_Text, selected ? selectedTextColor() : changedTextColor());
		ImGui::SetNextItemWidth(aWidth);
		// thin, faint grab: the name and value are written in the middle of the bar, a wide grab hid letters
		ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 3.0f);
		ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(1.0f, 1.0f, 1.0f, 0.55f));
		const bool moved = aDragSpeed > 0.0f
			? ImGui::DragFloat(id, &value, aDragSpeed, lo, hi, fmt.c_str())
			: ImGui::SliderFloat(id, &value, lo, hi, fmt.c_str());
		ImGui::PopStyleColor(1);
		ImGui::PopStyleVar(1);
		if (selected || changed) ImGui::PopStyleColor(1);

		bool result = false;
		if (learnable(aSession, moved, (int)aIndex)) {
			aSession->setUniformValue(aIndex, value);
			result = true;
		}
		if (!aSession->isMidiLearnMode()) {
			if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
				aSession->setUniformValue(aIndex, def);
				result = true;
			}
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
				ImGui::SetTooltip("%u %s\ndefault %.3f, range %.3f .. %.3f\nCtrl+click: type a value, right-click: reset",
					aIndex, aSession->getUniformName(aIndex).c_str(), def, lo, hi);
			}
		}
		return result;
	}

	// Animation mode of a uniform as one button: the current mode's letter in its color, a popup to
	// change it. Modes (VDSettings::ANIM_*): 0 none, 1 time, 2 auto, 3 bass, 4 mid, 5 smooth.
	//   aModes: bit mask of the modes offered (bit n = mode n). Choosing "none" resets the value.
	// The colors keep the old six-button columns (a red, f olive, b/m green, s teal); none is grey.
	inline void animModeButton(VDSessionFacadeRef aSession, unsigned int aIndex, unsigned int aModes = 0x3F) {
		static const char* letters[] = { "x", "a", "f", "b", "m", "s" };
		static const char* names[] = { "none (reset to default)", "time", "auto", "bass", "mid", "smooth" };
		static const float hues[] = { 0.0f, 0.0f / 16.0f, 1.0f / 16.0f, 2.0f / 16.0f, 3.0f / 16.0f, 4.0f / 16.0f };
		unsigned int mode = aSession->getAnim(aIndex);
		if (mode > 5) mode = 0;
		char label[32];
		snprintf(label, sizeof(label), "%s##anim%u", letters[mode], aIndex);
		const float w = ImGui::GetFrameHeight();
		bool clicked;
		if (mode == 0) {
			ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.0f, 0.22f));
			clicked = ImGui::Button(label, ImVec2(w, 0));
			ImGui::PopStyleColor(1);
		}
		else {
			clicked = coloredButton(label, hues[mode], true, ImVec2(w, 0));
		}
		char popup[32];
		snprintf(popup, sizeof(popup), "animpopup%u", aIndex);
		if (clicked) ImGui::OpenPopup(popup);
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("Animation: %s (click to change)", names[mode]);
		if (ImGui::BeginPopup(popup)) {
			for (unsigned int m = 1; m <= 6; m++) {
				const unsigned int mm = m % 6; // none last
				if (!(aModes & (1u << mm))) continue;
				char item[48];
				snprintf(item, sizeof(item), "%s  %s", letters[mm], names[mm]);
				if (ImGui::Selectable(item, mm == mode)) {
					aSession->setAnim(aIndex, mm);
					if (mm == 0) aSession->setUniformValue(aIndex, aSession->getDefaultUniformValue(aIndex));
				}
			}
			ImGui::EndPopup();
		}
	}
}
}
