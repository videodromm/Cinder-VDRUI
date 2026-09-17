#include "VDUIWarps.h"

#if ! defined( CINDER_MSW )
#define sprintf_s(buffer, format, ...) snprintf((buffer), sizeof(buffer), (format), ##__VA_ARGS__)
#endif

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

		// small rendered-preview overview of every warp, right in this same panel - each warp
		// still has its own window below (title, resize sliders, per-fbo input buttons); this is
		// just a quick-glance summary so all warps are visible together without hunting for each
		// one's separate floating window
		ImGui::Separator();
		int warpCount = mVDSession->getWarpCount();
		for (int w = 0; w < warpCount; w++) {
			ci::gl::TextureRef previewTex = mVDSession->getWarpPreviewTexture(w);
			if (!previewTex) continue;
			if (w > 0 && (w % 4 != 0)) ImGui::SameLine();
			unsigned int fboIndex = mVDSession->getWarpAFboIndex(w);
			std::string label = (fboIndex >= mVDSession->getFboShaderListSize()) ? "Post/Fx" : mVDSession->getFboShaderName(fboIndex);
			ImGui::PushID(w);
			ImGui::BeginGroup();
			ImGui::Image(previewTex, ivec2(mVDParams->getUISmallPreviewW() * uiScale, mVDParams->getUISmallPreviewH() * uiScale));
			ImGui::TextUnformatted(label.c_str());
			ImGui::EndGroup();
			ImGui::PopID();
		}
	}
	ImGui::End();

	yPos = mVDParams->getUIYPosRow3();
	// a window's own close [x] (via ImGui::Begin's p_open) actually removes the warp - deferred
	// until after the loop below so erasing mid-iteration can't shift indices out from under it
	int warpToRemove = -1;
	for (int w = 0; w < mVDSession->getWarpCount(); w++) {

		xPos = mVDParams->getUIMargin() + mVDParams->getUIXPosCol3() + ((mVDParams->getUILargePreviewW() + mVDParams->getUIMargin()) * (w));//+1

		ImGui::SetNextWindowSize(ImVec2(mVDParams->getUILargePreviewW() * uiScale, 150.0f * uiScale), ImGuiCond_Once);
		ImGui::SetNextWindowPos(ImVec2(xPos * uiScale, yPos * uiScale), ImGuiCond_Once);


		// title is the fbo currently feeding this warp (or "Post/Fx" if it's showing the full
		// composite instead), not the warp's own (largely meaningless, auto-generated) name -
		// makes it obvious at a glance what each warp shows
		unsigned int titleFboIndex = mVDSession->getWarpAFboIndex(w);
		std::string warpInputName = (titleFboIndex >= mVDSession->getFboShaderListSize()) ? "Post/Fx" : mVDSession->getFboShaderName(titleFboIndex);
		sprintf_s(buf, "%s##sh%d", warpInputName.c_str(), w);
		bool warpOpen = true;
		ImGui::Begin(buf, &warpOpen, ImGuiWindowFlags_NoSavedSettings);
		if (!warpOpen) warpToRemove = w;
		{

			int hue = 0;

			sprintf_s(buf, "w %d", mVDSession->getWarpWidth(w));
			ImGui::TextColored(ImColor(150, 220, 0), buf);
			ImGui::SameLine();
			sprintf_s(buf, "h %d", mVDSession->getWarpHeight(w));
			ImGui::TextColored(ImColor(150, 220, 0), buf);
			int ww = mVDSession->getWarpWidth(w);
			sprintf_s(buf, "WarpWidth##ww%d", w);
			if (ImGui::SliderInt(buf, &ww, 0.0f, mVDParams->getFboWidth()*2))
			{
				//mVDSession->setWarpWidth(w, ww);
			}
			
			int wh = mVDSession->getWarpHeight(w);
			sprintf_s(buf, "WarpHeight##wh%d", w);
			if (ImGui::SliderInt(buf, &wh, 0.0f, mVDParams->getFboHeight()*2))
			{
				//mVDSession->setWarpHeight(w, wh);
			}


			ImGui::PushItemWidth(mVDParams->getPreviewFboWidth() * uiScale);
			ImGui::PushID(w);
			unsigned int fboa = mVDSession->getWarpAFboIndex(w);
			// any value that isn't a real fbo index (the sentinel Warp::NO_FBO_INDEX, or simply
			// out of range - e.g. a fbo that no longer exists) means "no specific fbo chosen",
			// i.e. show the full composite - see VDSession::renderWarpsToFbo()
			bool showingComposite = (fboa >= mVDSession->getFboShaderListSize());

			// this warp's own rendered result (after the actual warp/perspective transform is
			// applied), not the flat, un-warped fbo/mixette preview shown before
			ci::gl::TextureRef previewTex = mVDSession->getWarpPreviewTexture(w);
			if (previewTex) ImGui::Image(previewTex, ivec2(mVDParams->getPreviewFboWidth() * uiScale, mVDParams->getPreviewFboHeight() * uiScale));
			//if (ImGui::IsItemHovered()) ImGui::SetTooltip(mVDSession->getWarpName(w).c_str());

			// "Post/Fx" - the full weighted composite of every active fboshader, instead of one
			// specific fbo's raw output. This is also the default for a fresh warp (or one saved
			// before per-warp fbo selection existed), so mixing weighted fboshaders together
			// still looks right by default - only override it when a single fbo is deliberately
			// wanted, isolated from the mix.
			if (showingComposite) {
				ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 1.0f, 1.0f));
			}
			else {
				ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.1f, 0.1f));
			}
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.7f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
			sprintf_s(buf, "Post/Fx##wiapfx%d", w);
			if (ImGui::Button(buf)) {
				mVDSession->setWarpAFboIndex(w, (unsigned int)-1);
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show the full weighted composite instead of one fbo");
			ImGui::PopStyleColor(3);
			ImGui::SameLine();

			// loop on the fbos A
			for (unsigned int a = 0; a < mVDSession->getFboShaderListSize(); a++) {
				if (a > 0 && (a % 6 != 0)) ImGui::SameLine();
				if (!showingComposite && fboa == a) {
					ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(a / 16.0f, 1.0f, 1.0f));
				}
				else {
					ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(a / 16.0f, 0.1f, 0.1f));
				}
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(a / 16.0f, 0.7f, 0.7f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(a / 16.0f, 0.8f, 0.8f));

				sprintf_s(buf, "%d##wia%d%d", a, w, a);
				if (ImGui::Button(buf)) {
					mVDSession->setWarpAFboIndex(w, a);
				};
				sprintf_s(buf, "Set input fbo A to %s", mVDSession->getFboShaderName(a).c_str());
				if (ImGui::IsItemHovered()) ImGui::SetTooltip(buf);
				ImGui::PopStyleColor(3);
			}
			ImGui::PopID();
			ImGui::PopItemWidth();
		
		}

		ImGui::End();
	}

	if (warpToRemove >= 0) {
		mVDSession->removeWarp((unsigned int)warpToRemove);
	}
}
