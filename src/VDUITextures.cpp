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
	int validImages = 0;
	unsigned int selectedFbo = mVDSession->getSelectedFbo();
	float uiScale = mVDUniforms->getUniformValue(mVDUniforms->IUISCALE);
	// one entry per loaded fbo's own input texture (image or audio FFT texture) -
	// this is a gallery across every fbo*.json, not just the currently selected fbo
	for (unsigned int f = 0; f < mVDSession->getFboShaderListSize(); f++) {
		ci::gl::Texture2dRef tex = mVDSession->getFboInputTextureListItem(f, 0);
		if (!tex) continue;
		validImages++;
		ImGui::SetNextWindowSize(ImVec2(mVDParams->getUISmallPreviewW() * uiScale, mVDParams->getPreviewHeight() * uiScale), ImGuiCond_Once);
		ImGui::SetNextWindowPos(ImVec2(xPos * uiScale, yPos * uiScale), ImGuiCond_Once);
		std::string texName = mVDSession->getFboInputTextureName(f);
		unsigned int ms = mVDSession->getFboMsTotal(f);
		sprintf(buf, " %s##s%d", texName.c_str(), f);
		bool isBackingSelectedFbo = (f == selectedFbo);
		if (isBackingSelectedFbo) ImGui::PushStyleColor(ImGuiCol_TitleBg, (ImVec4)ImColor(200, 150, 0, 220));
		ImGui::Begin( buf ); //, NULL, ImVec2(0, 0), ImGui::GetStyle().Alpha, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse);
		{
			ImGui::PushItemWidth(mVDParams->getUISmallPreviewW() * uiScale);
			ImGui::PushID(f);
			ImGui::Image(tex, ivec2(mVDParams->getUISmallPreviewW() * uiScale, mVDParams->getUISmallPreviewH() * uiScale));
			// click a texture to assign it (by reference, no reload) to whichever fbo is
			// currently selected via the "tex" button in the Fbos panel
			if (ImGui::IsItemClicked()) {
				mVDSession->setFboInputTexture(selectedFbo, tex, texName);
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Assign to selected fbo (%d)", selectedFbo);

			ImGui::TextColored(ImColor(155, 50, 255), "%d-", f);
			ImGui::SameLine();
			if (ms > 30) {
				ImGui::TextColored(ImColor(255, 0, 0), "%dms", ms);
			}
			else {
				ImGui::TextColored(ImColor(155, 255, 0), "%dms", ms);
			}
			ImGui::PopID();
			ImGui::PopItemWidth();
		}
		ImGui::End();
		if (isBackingSelectedFbo) ImGui::PopStyleColor(1);
		xPos += mVDParams->getUISmallPreviewW() + mVDParams->getUIMargin();

		if (validImages % 22 == 21)
		{
			xPos = mVDParams->getUIMargin() + mVDParams->getUIXPosCol1();
			yPos -= mVDParams->getPreviewHeight() + mVDParams->getUIMargin();
			if (yPos < mVDParams->getUIYPosRow2() + 200) yPos = mVDParams->getUIYPosRow3();
		}
	}

}
