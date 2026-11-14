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
#include <cstdlib>
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

	// Integer uniform (stored as a float): a slider in whole steps, or an on/off button when the range is 0..1.
	inline bool uniformIntSlider(VDSessionFacadeRef aSession, unsigned int aIndex, float aLo, float aHi,
		const std::string& aLabel = "", float aHue = 0.3f) {
		const std::string name = aLabel.empty() ? aSession->getUniformName(aIndex) : aLabel;
		const float def = aSession->getDefaultUniformValue(aIndex);
		const int value = (int)std::lround(aSession->getUniformValue(aIndex));
		const int lo = (int)std::floor(std::min({ aLo, def, (float)value }));
		const int hi = (int)std::ceil(std::max({ aHi, def, (float)value }));
		char id[48];
		if (hi - lo <= 1) {
			snprintf(id, sizeof(id), "%s##i%u", name.c_str(), aIndex);
			const bool clicked = coloredButton(id, aHue, value > lo);
			if (learnable(aSession, clicked, (int)aIndex)) {
				aSession->setUniformValue(aIndex, value > lo ? (float)lo : (float)hi);
				return true;
			}
			return false;
		}
		std::string fmt;
		for (char c : name) { fmt += c; if (c == '%') fmt += '%'; }
		fmt += " %d";
		snprintf(id, sizeof(id), "##i%u", aIndex);
		int v = value;
		const bool changed = std::abs(value - (int)std::lround(def)) > 0;
		if (changed) ImGui::PushStyleColor(ImGuiCol_Text, changedTextColor());
		ImGui::SetNextItemWidth(-1.0f);
		const bool moved = ImGui::SliderInt(id, &v, lo, hi, fmt.c_str());
		if (changed) ImGui::PopStyleColor(1);
		bool result = false;
		if (learnable(aSession, moved, (int)aIndex)) {
			aSession->setUniformValue(aIndex, (float)v);
			result = true;
		}
		if (!aSession->isMidiLearnMode() && ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
			aSession->setUniformValue(aIndex, def);
			result = true;
		}
		return result;
	}

	// XY pad: a square box with a crosshair at (aX, aY); click or drag inside to move it. Y grows upwards.
	// Returns true while the user moves it. aColor: crosshair color.
	inline bool xyPad(const char* aId, float& aX, float& aY, float aXLo, float aXHi, float aYLo, float aYHi, float aSize, ImU32 aColor) {
		ImDrawList* dl = ImGui::GetWindowDrawList();
		const ImVec2 p0 = ImGui::GetCursorScreenPos();
		const ImVec2 p1(p0.x + aSize, p0.y + aSize);
		ImGui::InvisibleButton(aId, ImVec2(aSize, aSize));
		bool changed = false;
		if (ImGui::IsItemActive() && aXHi > aXLo && aYHi > aYLo) {
			const ImVec2 m = ImGui::GetIO().MousePos;
			const float nx = std::clamp((m.x - p0.x) / aSize, 0.0f, 1.0f);
			const float ny = std::clamp((p1.y - m.y) / aSize, 0.0f, 1.0f);
			const float x = aXLo + nx * (aXHi - aXLo);
			const float y = aYLo + ny * (aYHi - aYLo);
			if (x != aX || y != aY) { aX = x; aY = y; changed = true; }
		}
		dl->AddRectFilled(p0, p1, ImGui::GetColorU32(ImGuiCol_FrameBg));
		const ImU32 faint = ImGui::GetColorU32(ImGuiCol_Border);
		// zero axes, when inside the range
		if (aXLo < 0.0f && aXHi > 0.0f) { const float zx = p0.x + (-aXLo) / (aXHi - aXLo) * aSize; dl->AddLine(ImVec2(zx, p0.y), ImVec2(zx, p1.y), faint); }
		if (aYLo < 0.0f && aYHi > 0.0f) { const float zy = p1.y - (-aYLo) / (aYHi - aYLo) * aSize; dl->AddLine(ImVec2(p0.x, zy), ImVec2(p1.x, zy), faint); }
		const float cx = p0.x + std::clamp((aX - aXLo) / std::max(1e-6f, aXHi - aXLo), 0.0f, 1.0f) * aSize;
		const float cy = p1.y - std::clamp((aY - aYLo) / std::max(1e-6f, aYHi - aYLo), 0.0f, 1.0f) * aSize;
		dl->AddLine(ImVec2(cx, p0.y), ImVec2(cx, p1.y), aColor);
		dl->AddLine(ImVec2(p0.x, cy), ImVec2(p1.x, cy), aColor);
		dl->AddCircleFilled(ImVec2(cx, cy), 3.0f, aColor);
		dl->AddRect(p0, p1, faint);
		return changed;
	}

	// Two float uniforms as an XY pad (iRenderXYX/Y, iMouseX/Y) with their two sliders on its right,
	// which keep MIDI learn, right-click reset and Ctrl+click. Right-click on the pad resets both.
	inline bool uniformXYPad(VDSessionFacadeRef aSession, unsigned int aIndexX, unsigned int aIndexY,
		float aXLo, float aXHi, float aYLo, float aYHi, const std::string& aLabel) {
		float x = aSession->getUniformValue(aIndexX);
		float y = aSession->getUniformValue(aIndexY);
		const float defX = aSession->getDefaultUniformValue(aIndexX);
		const float defY = aSession->getDefaultUniformValue(aIndexY);
		const bool changed = isOffDefault(aSession, aIndexX, aXLo, aXHi) || isOffDefault(aSession, aIndexY, aYLo, aYHi);
		const float size = ImGui::GetFrameHeight() * 2.0f + ImGui::GetStyle().ItemSpacing.y;
		ImGui::PushID((int)aIndexX);
		bool result = xyPad("##xypad", x, y, aXLo, aXHi, aYLo, aYHi, size,
			changed ? ImGui::GetColorU32(changedTextColor()) : IM_COL32(255, 255, 255, 200));
		if (result) {
			aSession->setUniformValue(aIndexX, x);
			aSession->setUniformValue(aIndexY, y);
		}
		if (!aSession->isMidiLearnMode()) {
			if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
				aSession->setUniformValue(aIndexX, defX);
				aSession->setUniformValue(aIndexY, defY);
				result = true;
			}
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
				ImGui::SetTooltip("%s: %.3f, %.3f\ndrag to move, right-click: reset", aLabel.c_str(), x, y);
			}
		}
		ImGui::PopID();
		ImGui::SameLine();
		// narrow windows (the fbo panes): "x 0.027" instead of a clipped "iRenderXY.x 0.02"; the pad's tooltip names it
		const float avail = ImGui::GetContentRegionAvail().x;
		const bool fits = ImGui::CalcTextSize((aLabel + ".x -0.000").c_str()).x + ImGui::GetStyle().FramePadding.x * 2.0f <= avail;
		const std::string lx = fits ? aLabel + ".x" : std::string("x");
		const std::string ly = fits ? aLabel + ".y" : std::string("y");
		ImGui::BeginGroup();
		result |= uniformSlider(aSession, aIndexX, aXLo, aXHi, lx);
		result |= uniformSlider(aSession, aIndexY, aYLo, aYHi, ly);
		ImGui::EndGroup();
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
