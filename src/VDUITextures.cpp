// FindHoveredWindowEx(): is a file dragged from Explorer over an fbo pane? Included first: after
// the using-directives, its log() is ambiguous with ci::log
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
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
	unsigned int fboCount = mVDSession->getFboShaderListSize();
	// the pool used to be fed only by VDUIFbos::Run(), so with the Fbos panel hidden this panel
	// stayed empty; registering is a dedup'd name lookup, cheap enough to do here as well
	for (unsigned int f = 0; f < fboCount; f++) {
		int textureMode = mVDSession->getInputTextureMode(f);
		if (textureMode == VDTextureMode::IMAGE || textureMode == VDTextureMode::MOVIE) {
			mVDSession->registerFboActiveTextureInGlobalPool(f);
		}
	}
	// files dragged from Explorer: highlighted here unless they hover an fbo pane (which then takes
	// them as its input texture); dropped anywhere else they go to this pool
	bool highlightPool = false;
	if (mVDSession->isExternalDragActive()) {
		vec2 dragPos = mVDSession->getExternalDragPos();
		ImGuiWindow* hovered = nullptr;
		ImGuiWindow* hoveredUnderMoving = nullptr;
		ImGui::FindHoveredWindowEx(ImVec2(dragPos.x, dragPos.y), false, &hovered, &hoveredUnderMoving);
		highlightPool = !(hovered && std::string(hovered->RootWindow->Name).find("##fbolbl") != std::string::npos);
	}
	unsigned int poolCount = mVDSession->getLoadedTextureCount();
	if (poolCount == 0) {
		// one window per texture means "nothing at all" when the pool is empty: say so instead
		ImGui::SetNextWindowPos(ImVec2(xPos * uiScale, yPos * uiScale), ImGuiCond_Once);
		ImGui::Begin("Textures##empty", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);
		ImGui::TextUnformatted("No texture loaded yet: image or movie fbos are listed here,");
		ImGui::TextUnformatted("so are files dropped outside the fbo windows.");
		ImGui::End();
		return;
	}
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
			const ivec2 previewSize(mVDParams->getUISmallPreviewW() * kSizeMultiplier * uiScale, mVDParams->getUISmallPreviewH() * kSizeMultiplier * uiScale);
			if (texName == "audio") {
				// the audio texture (64x2 spectrum/wave) means nothing as an image
				// what the audio texture analyses (mic or the playing audio file), and any playing
				// video, whose sound Media Foundation plays without it being analysed
				std::string label = mVDSession->getAudioSourceLabel();
				for (unsigned int f = 0; f < fboCount; f++) {
					if (mVDSession->isMovie(f) && mVDSession->isPlaying(f)) {
						label += "\nvideo playing: " + mVDSession->getInputTextureName(f, 0);
					}
				}
				// fixed ID: the label changes with the source
				label += "##audiosource";
				ImGui::Button(label.c_str(), ImVec2((float)previewSize.x, (float)previewSize.y));
			}
			else {
				ImGui::Image(tex, previewSize);
			}
			// drag onto an fbo pane to make it that fbo's input texture (VDUIFbos.cpp)
			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
				ImGui::SetDragDropPayload("VD_POOL_TEXTURE", &i, sizeof(unsigned int));
				ImGui::Text("Input texture: %s", texName.c_str());
				ImGui::EndDragDropSource();
			}
			// click a texture to assign it (by reference, no reload) to whichever fbo is
			// currently selected - this doesn't change which fbo is selected itself
			if (ImGui::IsItemClicked()) {
				mVDSession->setFboInputTexture(selectedFbo, tex, texName);
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Assign to selected fbo (%d)", selectedFbo);
			// a video's own player lives here, in the pool: play/pause (pauses every other playing
			// video/audio file), loop, volume level
			if (VDVideoSourceRef video = mVDSession->getVideoSource(texName)) {
				sprintf_s(buf, "%s##vplay%d", video->isPlaying() ? "Pause" : "Play", i);
				if (ImGui::Button(buf)) mVDSession->togglePlayPauseSource(texName);
				ImGui::SameLine();
				bool looping = video->isLooping();
				if (looping) ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(230, 160, 0, 255));
				sprintf_s(buf, "%s##vloop%d", looping ? "Loop on" : "Loop off", i);
				if (ImGui::Button(buf)) video->toggleLoop();
				if (looping) ImGui::PopStyleColor(1);
				ImGui::SameLine();
				float volumeLevel = video->getVolumeLevel();
				sprintf_s(buf, "##vvol%d", i);
				ImGui::SetNextItemWidth(60.0f * uiScale);
				if (ImGui::SliderFloat(buf, &volumeLevel, 0.0f, 1.0f, "vol %.2f")) video->setVolumeLevel(volumeLevel);
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Volume (x the highest weight of the fbos showing it)");
			}
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
			if (highlightPool) {
				ImVec2 winMin = ImGui::GetWindowPos();
				ImVec2 winMax(winMin.x + ImGui::GetWindowSize().x, winMin.y + ImGui::GetWindowSize().y);
				ImDrawList* drawList = ImGui::GetForegroundDrawList();
				drawList->AddRect(winMin, winMax, IM_COL32(0, 200, 255, 255), 0.0f, 0, 3.0f);
				drawList->AddText(ImVec2(winMin.x + 6, winMin.y + 24), IM_COL32(0, 200, 255, 255), "Drop: add to texture pool");
			}
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
