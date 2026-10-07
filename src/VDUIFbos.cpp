// FindHoveredWindowEx(): which window is on top at the drop position. Included first: after
// VDUIFbos.h's using-directives, its use of log() is ambiguous with ci::log
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "VDUIFbos.h"
// MIDI learn on uniform widgets
#include "VDUILearn.h"

#if ! defined( CINDER_MSW )
#define sprintf_s(buffer, format, ...) snprintf((buffer), sizeof(buffer), (format), ##__VA_ARGS__)
#endif

using namespace videodromm;

VDUIFbos::VDUIFbos(VDSettingsRef aVDSettings, VDSessionFacadeRef aVDSession, VDUniformsRef aVDUniforms) {
	mVDSettings = aVDSettings;
	mVDSession = aVDSession;
	mVDUniforms = aVDUniforms;
	// Params
	mVDParams = VDParams::create();
	//globalUniforms = true;
	for (int c = 0; c < 128; c++)
	{
		localValues[c] = mVDSession->getUniformValue(c);
	}
}

void VDUIFbos::Run(const char* title) {

	/*
	** textures
	*/
	/*static int XLeft[64];
	static int YTop[64];
	static int XRight[64];
	static int YBottom[64];
	static bool rnd[64];
	static bool anim[64];*/

#pragma region fbos

	/*
	** fbos
	*/

	// drag-and-drop: the topmost window under the drop (last frame's z-order), so a drop on a
	// panel lying over an fbo pane doesn't land in the hidden fbo. Only an fbo pane claims it below;
	// anything else (another panel, or no window: the render itself) goes to VDUI's flush
	// files dragged from Explorer and hovering: the topmost window under them, highlighted below
	// when it's an fbo pane (VDUITextures highlights the pool otherwise)
	ImGuiWindow* dragWindow = nullptr;
	if (mVDSession->isExternalDragActive()) {
		vec2 dragPos = mVDSession->getExternalDragPos();
		ImGuiWindow* hoveredUnderMoving = nullptr;
		ImGui::FindHoveredWindowEx(ImVec2(dragPos.x, dragPos.y), false, &dragWindow, &hoveredUnderMoving);
	}
	ImGuiWindow* dropWindow = nullptr;
	if (mVDSession->hasPendingTextureDrop()) {
		vec2 dropPos = mVDSession->getPendingTextureDropPos();
		ImGuiWindow* hoveredUnderMoving = nullptr;
		ImGui::FindHoveredWindowEx(ImVec2(dropPos.x, dropPos.y), false, &dropWindow, &hoveredUnderMoving);
	}

	for (unsigned int f = 0; f < mVDSession->getFboShaderListSize(); f++) {
		xPos = mVDParams->getUIMargin() + mVDParams->getUIXPosCol1() + ((mVDParams->getUILargePreviewW() + mVDParams->getUIMargin()) * (f));
		yPos = mVDParams->getUIYPosRow2();
		float uiScale = mVDUniforms->getUniformValue(mVDUniforms->IUISCALE);
		ImGui::SetNextWindowSize(ImVec2(mVDParams->getUILargePreviewW() * uiScale, mVDParams->getUILargePreviewH() * 1.4f * uiScale), ImGuiCond_Once);
		ImGui::SetNextWindowPos(ImVec2(xPos * uiScale, yPos * uiScale), ImGuiCond_Once);
		ImGui::PushStyleColor(ImGuiCol_TitleBg, (ImVec4)ImColor::HSV(f / 16.0f, 0.9f, 0.9f));
		ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor::HSV(f / 16.0f, 0.5f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, (ImVec4)ImColor::HSV(f / 16.0f, 0.6f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, (ImVec4)ImColor::HSV(f / 16.0f, 0.7f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_SliderGrab, (ImVec4)ImColor::HSV(f / 16.0f, 0.9f, 0.9f));
		// the selected fbo (a click anywhere in its pane): purple title bar and border. A click
		// on a texture pane's image assigns that texture to it
		const bool isSelectedFbo = (f == mVDSession->getSelectedFbo());
		if (isSelectedFbo) {
			const ImVec4 purple = (ImVec4)ImColor(150, 60, 220, 255);
			ImGui::PushStyleColor(ImGuiCol_TitleBg, purple);
			ImGui::PushStyleColor(ImGuiCol_TitleBgActive, purple);
			ImGui::PushStyleColor(ImGuiCol_Border, purple);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 3.0f);
		}

		sprintf_s(buf, " %s##fbolbl%d", mVDSession->getFboName(f).c_str(), f);
		ImGui::Begin(buf, NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse);
		{
			// drag-and-drop: claimed when this pane (or a child window inside it) is the topmost
			// window under the drop position
			if (dropWindow) {
				ImGuiWindow* thisWindow = ImGui::GetCurrentWindow();
				for (ImGuiWindow* w = dropWindow; w; w = w->ParentWindow) {
					if (w == thisWindow) {
						mVDSession->consumePendingTextureDrop(f);
						mVDSession->setSelectedFbo(f);
						dropWindow = nullptr;
						break;
					}
				}
			}
			if (ImGui::IsWindowFocused()) {
				mVDSession->setSelectedFbo(f);
			}
			// keep the shared texture pool (see VDUITextures.cpp) in sync with whatever this fbo is
			// actually showing - IMAGE/MOVIE only, since those are the modes with a stable, meaningful
			// filename (SEQUENCE's filename changes every advanced frame and would flood the pool)
			int textureMode = mVDSession->getInputTextureMode(f);
			if (textureMode == VDTextureMode::IMAGE || textureMode == VDTextureMode::MOVIE) {
				mVDSession->registerFboActiveTextureInGlobalPool(f);
			}

			ImGui::PushID(f);
			ctrl = mVDUniforms->IWEIGHT0 + f;
			float iWeight = mVDSession->getUniformValue(ctrl);

			ImGui::PushItemWidth(mVDParams->getPreviewFboWidth() * uiScale);
			ImGui::TextColored(ImColor(155, 255, 0), "%d %dms", f+31, mVDSession->getFboMsTotal(f));
			if (!mVDSession->isFboValid(f)) {
				ImGui::TextColored(ImColor(255, 0, 0), "err: %s", mVDSession->getFboError(f).c_str());
			}
			else {
				ImGui::TextColored(ImColor(0, 255, 0), "%s", mVDSession->getFboMsg(f).c_str());
			}
			ImGui::SameLine();
			//ImGui::TextColored(ImColor(155, 255, 0), "%d/%dms ", mVDSession->getFboMs(f), mVDSession->getFboMsTotal(f));

			sprintf_s(buf, "fbo##rdrfbouniform%d", f);
			mShowRenderedTexture ^= ImGui::Button(buf);
			ImGui::SameLine();

			int hue = 0;
			const bool showInputTexture = f < MAX_FBO_PANES && mShowInputTexture[f];
			ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 16.0f, 1.0f, showInputTexture ? 0.9f : 0.5f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue / 16.0f, 0.7f, 0.7f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue / 16.0f, 0.8f, 0.8f));
			sprintf_s(buf, "tex##rdrtexuniform%d", f);
			if (ImGui::Button(buf) && f < MAX_FBO_PANES) mShowInputTexture[f] = !mShowInputTexture[f];
			ImGui::PopStyleColor(3);
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show this fbo's input texture and its type instead of its render");

			hue++;
			ImGui::SameLine();
			sprintf_s(buf, "au##audio%d", f);
			if (ImGui::Button(buf)) {
				mVDSession->setFboTextureAudioMode(f);
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Set input texture to audio");

			hue++;
			ImGui::SameLine();
			sprintf_s(buf, "tn##tn%d", f);
			if (ImGui::Button(buf)) {
				mVDSession->saveThumbnail(f);
			}
			//mVDSession->buildFboRenderedTexture(f) && 
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Save thumbnail");

			const ivec2 previewSize(mVDParams->getPreviewFboWidth() * uiScale, mVDParams->getPreviewFboHeight() * uiScale);
			if (showInputTexture) {
				// in place of the render: stacked below it, it fell outside the pane's initial height
				static const char* textureModeNames[] = { "unknown", "image", "sequence", "video", "camera", "shared", "audio", "stream", "parts", "text", "ndi" };
				int textureMode = mVDSession->getInputTextureMode(f);
				const char* modeName = (textureMode >= 0 && textureMode < IM_ARRAYSIZE(textureModeNames)) ? textureModeNames[textureMode] : "unknown";
				unsigned int texIndex = mVDSession->getFboInputTextureIndex(f);
				ImGui::TextColored(ImColor(255, 200, 0), "%s: %s", modeName, mVDSession->getInputTextureName(f, texIndex).c_str());
				ci::gl::Texture2dRef inputTexture = mVDSession->getFboInputTextureListItem(f, texIndex);
				if (inputTexture) ImGui::Image(inputTexture, previewSize);
				else ImGui::Dummy(ImVec2((float)previewSize.x, (float)previewSize.y));
			}
			else if (mShowRenderedTexture) {
				ImGui::Image(mVDSession->buildFboRenderedTexture(f), previewSize);
			}
			ImGui::SameLine();
			if (learnable(mVDSession, ImGui::VSliderFloat("##v", ImVec2(14 * uiScale, 80 * uiScale), &iWeight, 0.0f, 1.0f, ""), ctrl))
			{
				setValue(ctrl, f, iWeight);
			};
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Weight %.2f", iWeight);
			// video/audio-file fbos: volume level slider right of the weight slider. The output
			// volume is level x weight, so fading the weight (also from MIDI, which sets the uniform
			// directly) still fades the sound. Applied every frame for that reason; 0 while scrubbing
			const bool hasSound = mVDSession->isMovie(f) || mVDSession->isAudioFile(f);
			if (hasSound) {
				ImGui::SameLine();
				float volumeLevel = mVDSession->getVolumeLevel(f);
				sprintf_s(buf, "##vol%d", f);
				ImGui::PushStyleColor(ImGuiCol_SliderGrab, (ImVec4)ImColor(0, 200, 120, 255));
				if (ImGui::VSliderFloat(buf, ImVec2(14 * uiScale, 80 * uiScale), &volumeLevel, 0.0f, 1.0f, "")) {
					mVDSession->setVolumeLevel(f, volumeLevel);
				}
				ImGui::PopStyleColor(1);
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Volume %.2f (output = volume x weight)", volumeLevel);
				const bool scrubbing = f < 12 && mIsScrubbing[f];
				// a video's volume is applied by VDMix::updateVideoSources() (level x the highest
				// weight of the fbos showing it); an audio file's here
				if (mVDSession->isMovie(f)) mVDSession->setScrubbing(f, scrubbing);
				else mVDSession->setVideoVolume(f, scrubbing ? 0.0f : volumeLevel * iWeight);
			}

			
			ImGui::TextColored(ImColor(155, 50, 255), "%s", mVDSession->getFboStatus(f).c_str());

#pragma region tex
			// this fbo's input texture (iChannel0), picked from the shared pool (VDUITextures.cpp)
			{
				unsigned int poolCount = mVDSession->getLoadedTextureCount();
				std::string activeName = mVDSession->getInputTextureName(f, 0);
				sprintf_s(buf, "##fboinput%d", f);
				ImGui::SetNextItemWidth(mVDParams->getPreviewFboWidth() * uiScale);
				if (ImGui::BeginCombo(buf, activeName.c_str())) {
					for (unsigned int t = 0; t < poolCount; t++) {
						std::string poolName = mVDSession->getLoadedTextureName(t);
						bool isActive = (poolName == activeName);
						sprintf_s(buf, "%s##fboit%d_%d", poolName.c_str(), f, t);
						if (ImGui::Selectable(buf, isActive)) mVDSession->setFboInputTexture(f, mVDSession->getLoadedTexture(t), poolName);
						if (isActive) ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Input texture (iChannel0)");
			}
			// movies and audio files play once unless looping is enabled
			if( mVDSession->isMovie( f ) || mVDSession->isAudioFile( f ) ) {
				ImGui::SameLine();
				bool looping = mVDSession->isLooping( f );
				// green when looping, black when not
				ImGui::PushStyleColor( ImGuiCol_Button, looping ? (ImVec4)ImColor( 0, 170, 60, 255 ) : (ImVec4)ImColor( 0, 0, 0, 255 ) );
				sprintf_s( buf, "Loop##lp%d", f );
				if( ImGui::Button( buf ) ) {
					mVDSession->toggleLoop( f );
				}
				ImGui::PopStyleColor( 1 );
			}
			// playback controls - one panel per fbo (not per texture slot), hence "f" here
			if (mVDSession->isSequence(f) || mVDSession->isMovie(f) || mVDSession->isAudioFile(f)) {
				// the label shows the action the button performs
				// green while playing, black while paused
				const bool playing = mVDSession->isPlaying(f);
				ImGui::PushStyleColor(ImGuiCol_Button, playing ? (ImVec4)ImColor(0, 170, 60, 255) : (ImVec4)ImColor(0, 0, 0, 255));
				sprintf_s(buf, "%s##s%d", playing ? "Pause" : "Play", f);
				if (ImGui::Button(buf))
				{
					mVDSession->togglePlayPause(f);
				}
				ImGui::PopStyleColor(1);
			}
			
			if (mVDSession->isSequence(f)) {
				ImGui::SameLine();
				sprintf_s(buf, "b##sqs%d", f);
				if (ImGui::Button(buf))
				{
					mVDSession->syncToBeat(f);
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Sync to beat");

				ImGui::SameLine();
				sprintf_s(buf, "r##rs%d", f);
				if (ImGui::Button(buf))
				{
					mVDSession->reverse(f);
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Reverse");

				if (mVDSession->isLoadingFromDisk(f)) {
					ImGui::SameLine();
					sprintf_s(buf, "l##ts%d", f);
					if (ImGui::Button(buf))
					{
						mVDSession->toggleLoadingFromDisk(f);
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Pause loading from disk");
				}
			}
			// speed/scrub - unlike sync-to-beat/reverse/loading-toggle above, these apply to
			// movies too (getSpeed/setSpeed/getPosition/setPlayheadPosition/getMaxFrame already
			// dispatch correctly for MOVIE, see VDFboShader.h)
			if (mVDSession->isSequence(f) || mVDSession->isMovie(f)) {
				speeds[f] = mVDSession->getSpeed(f);
				sprintf_s(buf, "speed##spd%d", f);
				if (ImGui::SliderFloat(buf, &speeds[f], 0.0f, 1.0f))
				{
					mVDSession->setSpeed(f, speeds[f]);
				}

				// don't clobber the slider's value from the live playback position while it's
				// actively being dragged - that fight (live position resetting the handle every
				// frame, mid-drag) is what caused the reported jitter
				if (!mIsScrubbing[f]) {
					playheadPositions[f] = mVDSession->getPosition(f);
				}
				sprintf_s(buf, "scrub##srb%d", f);
				if (ImGui::SliderInt(buf, &playheadPositions[f], 0, mVDSession->getMaxFrame(f)))
				{
					mVDSession->setPlayheadPosition(f, playheadPositions[f]);
				}
				// muted while dragging: the per-frame volume above applies 0 while mIsScrubbing
				if (ImGui::IsItemActivated()) mIsScrubbing[f] = true;
				if (mIsScrubbing[f] && ImGui::IsItemDeactivated()) mIsScrubbing[f] = false;
			}

#pragma endregion tex


			// uniforms
			int channelIndex = 0;
			int texNameEndIndex = 0;
			for (auto u : mVDSession->getUniforms(f)) {
				string uName = u.getName(); // TODO use getIndex?
				ctrl = mVDSession->getUniformIndexForName(uName);

				switch (u.getType()) {
				case GL_BOOL:
					// boolean 35670 GL_BOOL 0x8B56
					(getValue(ctrl, f) > 0.0f) ? ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 16.0f, 1.0f, 0.5f)) : ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1.0f, 0.1f, 0.1f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue / 16.0f, 0.7f, 0.7f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue / 16.0f, 0.8f, 0.8f));
					sprintf_s(buf, "%s##booluniform%d", uName.c_str(), f);
					if (ImGui::Button(buf)) {
						toggleValue(ctrl, f);
					}
					ImGui::PopStyleColor(3);
					hue++;
					break;
				case GL_INT:
					// int 5124 GL_INT 0x1404
					(getValue(ctrl, f)) ? ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 16.0f, 1.0f, 0.5f)) : ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1.0f, 0.1f, 0.1f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue / 16.0f, 0.7f, 0.7f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue / 16.0f, 0.8f, 0.8f));
					sprintf_s(buf, "%s##intuniform%d", uName.c_str(), f);
					if (ImGui::Button(buf)) {
						//toggleValue(ctrl, f);
					}
					ImGui::PopStyleColor(3);
					hue++;
					break;
				case GL_SAMPLER_2D:
					// sampler2d 35678 GL_SAMPLER_2D 0x8B5E

					texNameEndIndex = uName.find("tex");
					if (texNameEndIndex != std::string::npos && texNameEndIndex != -1) {
						// hydra fbo
						ImGui::Image(mVDSession->getFboInputTextureListItem(f, channelIndex), ivec2(mVDParams->getPreviewFboWidth() * uiScale, mVDParams->getPreviewFboHeight() * uiScale));
						channelIndex++;
						ImGui::SameLine();
					}
					//sprintf_s(buf, "%s", uName.c_str());
					//ImGui::TextColored(ImColor(220, 150, 0), buf);
					break;
				case GL_FLOAT:
					// float 5126 GL_FLOAT 0x1406
					localValues[ctrl] = mVDSession->getUniformValue(ctrl);
					if (ctrl > 0) {
						sprintf_s(buf, "%s##floatuniform%d", uName.c_str(), f);

						float spd = (ctrl == mVDUniforms->IPIXELX || ctrl == mVDUniforms->IPIXELY) ? 1.0f : 0.001f;

						if (learnable(mVDSession, ImGui::DragFloat(buf, &localValues[ctrl], spd, getMinUniformValue(ctrl), getMaxUniformValue(ctrl)), ctrl))
						{
							setValue(ctrl, f, localValues[ctrl]);
						}
					}
					else {
						// 0 = time or unknown uniform
						if (uName == "iTime" || uName == "uTime" || uName == "time" || uName == "TIME") {
							// don't display
						}
						else {
							location = u.getLocation();
							mUniformValueByLocation[location] = mVDSession->getUniformValueByLocation(f, location);
							sprintf_s(buf, "%s##floatuniform%d", uName.c_str(), f);
							if (ImGui::DragFloat(buf, &mUniformValueByLocation[location], 0.001f, 0.0001f, 50.0f))
							{
								mVDSession->setUniformValueByLocation(f, location, mUniformValueByLocation[location]);

							}

						}
					}
					break;
				case GL_FLOAT_VEC2:
					// vec2 35664		
					/*if (uName == "RENDERSIZE" || uName == "resolution") {
						float fw = mVDSession->buildFboRenderedTexture(f)->getWidth();
						sprintf_s(buf, "rw %.0f", fw);						
						ImGui::TextColored(ImColor(100, 100, 100), buf);
						ImGui::SameLine();
						float fh = mVDSession->buildFboRenderedTexture(f)->getHeight();				
						sprintf_s(buf, "rh %.0f", fh);
						ImGui::TextColored(ImColor(100, 100, 100), buf);					
					}
					else {
						sprintf_s(buf, "vec2 %s", uName.c_str(), f);
						ImGui::TextColored(ImColor(150, 220, 0), buf);
					}*/
					break;
				case GL_FLOAT_VEC3:
					// vec3 35665
					//sprintf_s(buf, "vec3 %s", uName.c_str(), f);
					//ImGui::TextColored(ImColor(100, 100, 0), buf);

					break;
				case GL_FLOAT_VEC4:
					// vec4 35666 GL_FLOAT_VEC4
					/*sprintf_s(buf, "vec4 %s %d", uName.c_str(), u.getType());
					ImGui::TextColored(ImColor(100, 100, 100), buf);
					if (ctrl == mVDUniforms->IMOUSE) {
						mouseX = getValue(mVDUniforms->IMOUSEX, f);
						if (ImGui::SliderFloat("MouseX", &mouseX, 0.0f, mVDParams->getFboWidth(), "%.4f", 3.0f))
						{
							setValue(mVDUniforms->IMOUSEX, mouseX, f);
						}
						mouseY = getValue(mVDUniforms->IMOUSEY, f);
						if (ImGui::SliderFloat("MouseY", &mouseY, 0.0f, mVDParams->getFboHeight(), "%.4f", 0.3f))
						{
							setValue(mVDUniforms->IMOUSEY, mouseY, f);
						}
					}*/
					break;
				default:
					//ciModelViewProjection 35676 GL_FLOAT_MAT4 0x8B5C
					/* gl2.h
						GL_FLOAT_VEC2                     0x8B50
						GL_FLOAT_VEC3                     0x8B51
						GL_FLOAT_VEC4                     0x8B52
					*/
					if (uName != "ciModelViewProjection") {
						sprintf_s(buf, "! %s %d", uName.c_str(), u.getType());
						ImGui::TextColored(ImColor(255, 0, 0), buf);
					}
					break;
				}

			} //for uniforms

			/*float fw = mVDSession->getFboTextureWidth(f);
			sprintf_s(buf, "tw %.0f", fw);
			ImGui::TextColored(ImColor(120, 120, 120), buf);
			ImGui::SameLine();
			float fh = mVDSession->getFboTextureHeight(f);
			sprintf_s(buf, "th %.0f", fh);
			ImGui::TextColored(ImColor(120, 120, 120), buf);*/

			ImGui::PopItemWidth();
			ImGui::PopID();
		}
		{
			ImGuiWindow* thisWindow = ImGui::GetCurrentWindow();
			// a texture pane dragged here (VDUITextures.cpp): becomes this fbo's input texture.
			// ImGui highlights the pane while it's hovered
			if (ImGui::BeginDragDropTargetCustom(thisWindow->Rect(), thisWindow->ID)) {
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("VD_POOL_TEXTURE")) {
					unsigned int poolIndex = *(const unsigned int*)payload->Data;
					if (poolIndex < mVDSession->getLoadedTextureCount()) {
						mVDSession->setFboInputTexture(f, mVDSession->getLoadedTexture(poolIndex), mVDSession->getLoadedTextureName(poolIndex));
						mVDSession->setSelectedFbo(f);
					}
				}
				ImGui::EndDragDropTarget();
			}
			// files from Explorer hovering this pane: they'd become its input texture
			bool dragTarget = false;
			for (ImGuiWindow* w = dragWindow; w; w = w->ParentWindow) {
				if (w == thisWindow) { dragTarget = true; break; }
			}
			if (dragTarget) {
				ImVec2 winMin = ImGui::GetWindowPos();
				ImVec2 winMax(winMin.x + ImGui::GetWindowSize().x, winMin.y + ImGui::GetWindowSize().y);
				ImDrawList* drawList = ImGui::GetForegroundDrawList();
				drawList->AddRect(winMin, winMax, IM_COL32(0, 255, 120, 255), 0.0f, 0, 4.0f);
				drawList->AddText(ImVec2(winMin.x + 6, winMin.y + 24), IM_COL32(0, 255, 120, 255), "Drop: input texture of this fbo");
			}
		}
		ImGui::End();
		if (isSelectedFbo) {
			ImGui::PopStyleVar(1);
			ImGui::PopStyleColor(3);
		}
		ImGui::PopStyleColor(5);
	} // for getFboList

	// a drop no fbo pane claimed is handled by VDUI::Run() (flushPendingTextureDrop), which runs
	// even while this panel is hidden

#pragma endregion fbos

}
