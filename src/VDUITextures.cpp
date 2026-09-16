#include "VDUITextures.h"

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
	for (unsigned int i = 0; i < poolCount; i++) {
		ci::gl::Texture2dRef tex = mVDSession->getLoadedTexture(i);
		if (!tex) continue;
		// twice the size of the underlying VDParams preview dimensions, scoped to this panel only
		const float kSizeMultiplier = 2.0f;
		ImGui::SetNextWindowSize(ImVec2(mVDParams->getUISmallPreviewW() * kSizeMultiplier * uiScale, mVDParams->getPreviewHeight() * kSizeMultiplier * uiScale), ImGuiCond_Once);
		ImGui::SetNextWindowPos(ImVec2(xPos * uiScale, yPos * uiScale), ImGuiCond_Once);
		std::string texName = mVDSession->getLoadedTextureName(i);
		sprintf(buf, " %s##s%d", texName.c_str(), i);
		ImGui::Begin( buf ); //, NULL, ImVec2(0, 0), ImGui::GetStyle().Alpha, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse);
		{
			ImGui::PushItemWidth(mVDParams->getUISmallPreviewW() * kSizeMultiplier * uiScale);
			ImGui::PushID(i);
			ImGui::Image(tex, ivec2(mVDParams->getUISmallPreviewW() * kSizeMultiplier * uiScale, mVDParams->getUISmallPreviewH() * kSizeMultiplier * uiScale));
			// click a texture to assign it (by reference, no reload) to whichever fbo is
			// currently selected via the "tex" button in the Fbos panel
			if (ImGui::IsItemClicked()) {
				mVDSession->setFboInputTexture(selectedFbo, tex, texName);
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Assign to selected fbo (%d)", selectedFbo);
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
