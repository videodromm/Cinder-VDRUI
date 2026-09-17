#include "VDUIWarps.h"

using namespace videodromm;

VDUIWarps::VDUIWarps(VDSettingsRef aVDSettings, VDSessionFacadeRef aVDSession, VDUniformsRef aVDUniforms) {
	mVDSettings = aVDSettings;
	mVDSession = aVDSession;
	mVDUniforms = aVDUniforms;
	// Params
	mVDParams = VDParams::create();
}

void VDUIWarps::Run(const char* title) {
	//static int currentNode = 0;

	float uiScale = mVDUniforms->getUniformValue(mVDUniforms->IUISCALE);
	// moved here from VDUI.cpp's top-level toggle bar - creating a warp belongs with the rest of
	// the warp controls, not the panel-visibility buttons
	ImGui::SetNextWindowPos(ImVec2(mVDParams->getUIMargin() * uiScale, mVDParams->getUIYPosRow3() * uiScale), ImGuiCond_Once);
	ImGui::Begin(title, NULL, ImGuiWindowFlags_NoSavedSettings);
	{
		if (ImGui::Button("Warp++")) {
			mVDSession->createWarp();
		}
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Create a new warp");
	}
	ImGui::End();

	yPos = mVDParams->getUIYPosRow3();
	for (int w = 0; w < mVDSession->getWarpCount(); w++) {

		xPos = mVDParams->getUIMargin() + mVDParams->getUIXPosCol3() + ((mVDParams->getUILargePreviewW() + mVDParams->getUIMargin()) * (w));//+1

		float uiScale = mVDUniforms->getUniformValue(mVDUniforms->IUISCALE);
		ImGui::SetNextWindowSize(ImVec2(mVDParams->getUILargePreviewW() * uiScale, 150.0f * uiScale), ImGuiCond_Once);
		ImGui::SetNextWindowPos(ImVec2(xPos * uiScale, yPos * uiScale), ImGuiCond_Once);


		// title is the fbo currently feeding this warp, not the warp's own (largely meaningless,
		// auto-generated) name - makes it obvious at a glance which fboshader each warp shows
		sprintf(buf, "%s##sh%d", mVDSession->getFboShaderName(mVDSession->getWarpAFboIndex(w)).c_str(), w);
		ImGui::Begin(buf, NULL, ImGuiWindowFlags_NoSavedSettings);
		{

			int hue = 0;

			sprintf(buf, "w %d", mVDSession->getWarpWidth(w));
			ImGui::TextColored(ImColor(150, 220, 0), buf);
			ImGui::SameLine();
			sprintf(buf, "h %d", mVDSession->getWarpHeight(w));
			ImGui::TextColored(ImColor(150, 220, 0), buf);
			int ww = mVDSession->getWarpWidth(w);
			sprintf(buf, "WarpWidth##ww%d", w);
			if (ImGui::SliderInt(buf, &ww, 0.0f, mVDParams->getFboWidth()*2))
			{
				//mVDSession->setWarpWidth(w, ww);
			}
			
			int wh = mVDSession->getWarpHeight(w);
			sprintf(buf, "WarpHeight##wh%d", w);
			if (ImGui::SliderInt(buf, &wh, 0.0f, mVDParams->getFboHeight()*2))
			{
				//mVDSession->setWarpHeight(w, wh);
			}


			ImGui::PushItemWidth(mVDParams->getPreviewFboWidth() * uiScale);
			ImGui::PushID(w);
			int fboa = mVDSession->getWarpAFboIndex(w);

			if (mVDSession->buildFboRenderedTexture(fboa)) ImGui::Image(mVDSession->buildFboRenderedTexture(fboa), ivec2(mVDParams->getPreviewFboWidth() * uiScale, mVDParams->getPreviewFboHeight() * uiScale));
			//if (ImGui::IsItemHovered()) ImGui::SetTooltip(mVDSession->getWarpName(w).c_str());
			// loop on the fbos A
			for (unsigned int a = 0; a < mVDSession->getFboShaderListSize(); a++) {
				if (a > 0 && (a % 6 != 0)) ImGui::SameLine();
				if (fboa == a) {
					ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(a / 16.0f, 1.0f, 1.0f));
				}
				else {
					ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(a / 16.0f, 0.1f, 0.1f));
				}
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(a / 16.0f, 0.7f, 0.7f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(a / 16.0f, 0.8f, 0.8f));

				sprintf(buf, "%d##wia%d%d", a, w, a);
				if (ImGui::Button(buf)) {
					mVDSession->setWarpAFboIndex(w, a);
				};
				sprintf(buf, "Set input fbo A to %s", mVDSession->getFboShaderName(a).c_str());
				if (ImGui::IsItemHovered()) ImGui::SetTooltip(buf);
				ImGui::PopStyleColor(3);
			}
			ImGui::PopID();
			ImGui::PopItemWidth();
		
		}

		ImGui::End();
	}


}
