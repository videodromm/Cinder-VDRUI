#include "VDUITextures.h"

#if ! defined( CINDER_MSW )
#define sprintf_s(buffer, format, ...) snprintf((buffer), sizeof(buffer), (format), ##__VA_ARGS__)
#endif

using namespace videodromm;

VDUITextures::VDUITextures(VDUniformsRef aVDUniforms, VDSessionFacadeRef aVDSession) {
	mVDUniforms = aVDUniforms;
	mVDSession = aVDSession;
	// Params
	mVDParams = VDParams::create();
}
VDUITextures::~VDUITextures() {

}

void VDUITextures::Run(const char* title) {

	xPos = mVDParams->getUIMargin() + mVDParams->getUIXPosCol1();
	yPos = mVDParams->getUIYPosRow3();
	unsigned int selectedFbo = mVDSession->getSelectedFbo();
	float uiScale = mVDUniforms->getUniformValue(mVDUniforms->IUISCALE);
	// one entry per uniquely-named loaded texture, deduplicated - not one per (fbo, slot); several
	// fbos can (and often do) load the same file, and any fbo can pick any texture here, not just
	// the ones it loaded itself. See VDMix::registerLoadedTexture()/VDUIFbos.cpp's per-frame sync.
	unsigned int poolCount = mVDSession->getLoadedTextureCount();
	unsigned int fboCount = mVDSession->getFboShaderListSize();
	for (unsigned int i = 0; i < poolCount; i++) {
		ci::gl::Texture2dRef tex = mVDSession->getLoadedTexture(i);
		if (!tex) continue;
		// twice the size of the underlying VDParams preview dimensions, scoped to this panel only
		const float kSizeMultiplier = 2.0f;
		ImGui::SetNextWindowSize(ImVec2(mVDParams->getUISmallPreviewW() * kSizeMultiplier * uiScale, mVDParams->getPreviewHeight() * kSizeMultiplier * uiScale), ImGuiCond_Once);
		ImGui::SetNextWindowPos(ImVec2(xPos * uiScale, yPos * uiScale), ImGuiCond_Once);
		std::string texName = mVDSession->getLoadedTextureName(i);
		sprintf_s(buf, " %s##s%d", texName.c_str(), i);
		ImGui::Begin( buf ); //, NULL, ImVec2(0, 0), ImGui::GetStyle().Alpha, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse);
		{
			ImGui::PushItemWidth(mVDParams->getUISmallPreviewW() * kSizeMultiplier * uiScale);
			ImGui::PushID(i);
			ImGui::Image(tex, ivec2(mVDParams->getUISmallPreviewW() * kSizeMultiplier * uiScale, mVDParams->getUISmallPreviewH() * kSizeMultiplier * uiScale));
			// click a texture to assign it (by reference, no reload) to whichever fbo is
			// currently selected - this doesn't change which fbo is selected itself
			if (ImGui::IsItemClicked()) {
				mVDSession->setFboInputTexture(selectedFbo, tex, texName);
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Assign to selected fbo (%d)", selectedFbo);
			// one button per fbo to assign this texture directly to that fbo, instead of only
			// ever the currently-selected one - clicking one also makes that fbo the selected
			// one (highlighted here in orange), so the image click above then targets it too;
			// selection otherwise still also follows whichever fbo's own window last had focus
			// (VDUIFbos.cpp)
			for (unsigned int f = 0; f < fboCount; f++) {
				if (f > 0 && (f % 6 != 0)) ImGui::SameLine();
				bool isSelected = (f == selectedFbo);
				ImGui::PushStyleColor(ImGuiCol_Button, isSelected ? (ImVec4)ImColor(230, 160, 0, 255) : (ImVec4)ImColor::HSV(f / 16.0f, 0.4f, 0.4f));
				sprintf_s(buf, "%d##texassign%d_%d", f, i, f);
				if (ImGui::Button(buf)) {
					mVDSession->setFboInputTexture(f, tex, texName);
					mVDSession->setSelectedFbo(f);
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Assign to fbo %d", f);
				ImGui::PopStyleColor(1);
			}
			ImGui::PopID();
			ImGui::PopItemWidth();
		}
		ImGui::End();
		xPos += mVDParams->getUISmallPreviewW() * kSizeMultiplier + mVDParams->getUIMargin();

		// windows are twice as wide/tall as the base preview size, so half as many fit per row
		if (i % 11 == 10)
		{
			xPos = mVDParams->getUIMargin() + mVDParams->getUIXPosCol1();
			yPos -= mVDParams->getPreviewHeight() * kSizeMultiplier + mVDParams->getUIMargin();
			if (yPos < mVDParams->getUIYPosRow2() + 200) yPos = mVDParams->getUIYPosRow3();
		}
	}

}
