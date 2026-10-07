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
		// same width and preview size as the fbo panes (VDUIFbos.cpp). The width is a constraint:
		// these windows keep their size in imgui.ini, which would win over a size set once
		const float paneWidth = mVDParams->getUILargePreviewW() * uiScale;
		ImGui::SetNextWindowSize(ImVec2(paneWidth, mVDParams->getUILargePreviewH() * 1.4f * uiScale), ImGuiCond_Once);
		ImGui::SetNextWindowSizeConstraints(ImVec2(paneWidth, 0.0f), ImVec2(paneWidth, FLT_MAX));
		ImGui::SetNextWindowPos(ImVec2(xPos * uiScale, yPos * uiScale), ImGuiCond_Once);
		std::string texName = mVDSession->getLoadedTextureName(i);
		sprintf_s(buf, " %s##s%d", texName.c_str(), i);
		ImGui::Begin( buf ); //, NULL, ImVec2(0, 0), ImGui::GetStyle().Alpha, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse);
		{
			const ivec2 previewSize(mVDParams->getPreviewFboWidth() * uiScale, mVDParams->getPreviewFboHeight() * uiScale);
			ImGui::PushItemWidth((float)previewSize.x);
			ImGui::PushID(i);
			if (texName == "audio") {
				// the audio texture (64x2 spectrum/wave) means nothing as an image: a button (click =
				// assign, drag = onto an fbo pane), the spectrum and the analysed source below
				ImGui::Button("audio##audiosource", ImVec2((float)previewSize.x, ImGui::GetFrameHeight()));
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
			// the selected fbo: the last fbo pane clicked (highlighted in purple there)
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Click: assign to the selected fbo (%d, purple)\nDrag: onto any fbo pane", selectedFbo);
			if (texName == "audio") {
				// what the audio texture analyses (mic or the playing audio file), and any playing
				// video, whose sound Media Foundation plays without it being analysed
				ImGui::TextUnformatted(mVDSession->getAudioSourceLabel().c_str());
				for (unsigned int f = 0; f < fboCount; f++) {
					if (mVDSession->isMovie(f) && mVDSession->isPlaying(f)) {
						ImGui::Text("video playing: %s", mVDSession->getInputTextureName(f, 0).c_str());
					}
				}
				// spectrum, max volume history (red above 240), gain (AX) with reset
				static ImVector<float> timeValues;
				if (timeValues.empty()) { timeValues.resize(40); memset(&timeValues.front(), 0, timeValues.size() * sizeof(float)); }
				static int timeValues_offset = 0;
				static double tRefresh_time = 0.0;
				const float maxVolume = mVDSession->getUniformValue(mVDUniforms->IMAXVOLUME);
				if (ImGui::GetTime() > tRefresh_time) {
					tRefresh_time = ImGui::GetTime() + 1.0 / 20.0;
					timeValues[timeValues_offset] = maxVolume;
					timeValues_offset = (timeValues_offset + 1) % timeValues.size();
				}
				ImGui::PlotHistogram("##fftH", mVDSession->getFreqs(), mVDSession->getFFTWindowSize(), 0, NULL, 0.0f, 255.0f, ImVec2((float)previewSize.x * 0.5f, 30));
				ImGui::SameLine();
				if (maxVolume > 240.0f) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
				ImGui::PlotLines("##fftV", &timeValues.front(), (int)timeValues.size(), timeValues_offset, toString(int(maxVolume)).c_str(), 0.0f, 255.0f, ImVec2((float)previewSize.x * 0.5f - ImGui::GetStyle().ItemSpacing.x, 30));
				if (maxVolume > 240.0f) ImGui::PopStyleColor();
				ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(13.0f / 16.0f, 1.0f, 0.5f));
				if (ImGui::Button("x##audioxreset")) mVDSession->setUniformValue(mVDUniforms->IAUDIOX, 1.0f);
				ImGui::PopStyleColor(1);
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Reset gain to 1");
				ImGui::SameLine();
				float multx = mVDSession->getUniformValue(mVDUniforms->IAUDIOX);
				ImGui::SetNextItemWidth((float)previewSize.x - ImGui::GetItemRectSize().x - ImGui::GetStyle().ItemSpacing.x * 2.0f - 20.0f);
				if (ImGui::SliderFloat("AX##audiox", &multx, 0.01f, 7.0f)) mVDSession->setUniformValue(mVDUniforms->IAUDIOX, multx);
			}
			// a video's own player lives here, in the pool: play/pause (pauses every other playing
			// video/audio file), loop, volume level
			if (VDVideoSourceRef video = mVDSession->getVideoSource(texName)) {
				// green while playing, black while paused
				const bool playing = video->isPlaying();
				ImGui::PushStyleColor(ImGuiCol_Button, playing ? (ImVec4)ImColor(0, 170, 60, 255) : (ImVec4)ImColor(0, 0, 0, 255));
				sprintf_s(buf, "%s##vplay%d", playing ? "Pause" : "Play", i);
				if (ImGui::Button(buf)) mVDSession->togglePlayPauseSource(texName);
				ImGui::PopStyleColor(1);
				ImGui::SameLine();
				bool looping = video->isLooping();
				// green when looping, black when not
				ImGui::PushStyleColor(ImGuiCol_Button, looping ? (ImVec4)ImColor(0, 170, 60, 255) : (ImVec4)ImColor(0, 0, 0, 255));
				sprintf_s(buf, "Loop##vloop%d", i);
				if (ImGui::Button(buf)) video->toggleLoop();
				ImGui::PopStyleColor(1);
				ImGui::SameLine();
				float volumeLevel = video->getVolumeLevel();
				sprintf_s(buf, "##vvol%d", i);
				ImGui::SetNextItemWidth(60.0f * uiScale);
				if (ImGui::SliderFloat(buf, &volumeLevel, 0.0f, 1.0f, "vol %.2f")) video->setVolumeLevel(volumeLevel);
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Volume (x the highest weight of the fbos showing it)");
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
		xPos += mVDParams->getUILargePreviewW() + mVDParams->getUIMargin();

		// fbo-pane-sized windows: a new row every 6 (initial placement only)
		if (i % 6 == 5)
		{
			xPos = mVDParams->getUIMargin() + mVDParams->getUIXPosCol1();
			yPos -= mVDParams->getUILargePreviewH() * 1.4f + mVDParams->getUIMargin();
			if (yPos < mVDParams->getUIYPosRow2() + 200) yPos = mVDParams->getUIYPosRow3();
		}
	}

}
