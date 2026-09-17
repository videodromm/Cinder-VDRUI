#include "VDUIFbos.h"

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

		sprintf_s(buf, " %s##fbolbl%d", mVDSession->getFboName(f).c_str(), f);
		ImGui::Begin(buf, NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse);
		{
			// drag-and-drop: this fbo's real, current window rect - the only place that knows it,
			// since these windows can be dragged/resized independently of the initial grid layout
			ImVec2 winMin = ImGui::GetWindowPos();
			ImVec2 winMax = ImVec2(winMin.x + ImGui::GetWindowSize().x, winMin.y + ImGui::GetWindowSize().y);
			mVDSession->consumePendingTextureDropIfInRect(f, vec2(winMin.x, winMin.y), vec2(winMax.x, winMax.y));
			// whichever fbo window last had focus is the target for a drop that lands outside
			// every fbo window (see flushPendingTextureDrop() after this loop)
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
			ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 16.0f, 1.0f, 0.5f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue / 16.0f, 0.7f, 0.7f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue / 16.0f, 0.8f, 0.8f));
			sprintf_s(buf, "tex##rdrtexuniform%d", f);
			mShowInputTexture ^= ImGui::Button(buf);
			ImGui::PopStyleColor(3);

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

			if (mShowRenderedTexture) ImGui::Image(mVDSession->buildFboRenderedTexture(f), ivec2(mVDParams->getPreviewFboWidth() * uiScale, mVDParams->getPreviewFboHeight() * uiScale));
			if (mShowInputTexture) ImGui::Image(mVDSession->getFboInputTextureListItem(f, mVDSession->getFboInputTextureIndex(f)), ivec2(mVDParams->getPreviewFboWidth() * uiScale, mVDParams->getPreviewFboHeight() * uiScale));
			ImGui::SameLine();
			if (ImGui::VSliderFloat("##v", ImVec2(14 * uiScale, 80 * uiScale), &iWeight, 0.0f, 1.0f, ""))
			{
				setValue(ctrl, f, iWeight);
			};
			// fading a movie's visual weight also fades its audio - lets weight double as a volume
			// fader instead of the video staying at full volume regardless. Done unconditionally
			// every frame (not just inside the slider's own onChange above) because the weight
			// uniform can also be driven by MIDI (VDMidi::midiListener() sets it directly via
			// VDMediator, bypassing this widget entirely) - keyed only off the slider's own drag
			// before, a MIDI-driven weight change updated the visual mix but never touched volume
			if (mVDSession->isMovie(f)) {
				mVDSession->setVideoVolume(f, iWeight);
			}

			
			ImGui::TextColored(ImColor(155, 50, 255), "%s", mVDSession->getFboStatus(f).c_str());

#pragma region tex
			// pick this fbo's active input texture from the shared pool (see VDUITextures.cpp) -
			// this used to loop over getInputTexturesCount(f) (this fbo's own internal texture-slot
			// count, a leftover from the pre-shared-pool design - almost always 1, hence "only shows
			// 0"), which has nothing to do with what's actually available to pick from; mirrors
			// VDUITextures.cpp's click-to-assign inline, highlighting whichever pool entry is active
			{
				unsigned int poolCount = mVDSession->getLoadedTextureCount();
				std::string activeName = mVDSession->getInputTextureName(f, 0);
				for (unsigned int t = 0; t < poolCount; t++) {
					if (t > 0 && (t % 6 != 0)) ImGui::SameLine();
					std::string poolName = mVDSession->getLoadedTextureName(t);
					if (poolName == activeName) {
						ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(t / 7.0f, 1.0f, 1.0f));
					}
					else {
						ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(t / 7.0f, 0.1f, 0.1f));
					}
					sprintf_s(buf, "%d##fboit%d%d", t, f, t);
					if (ImGui::Button(buf)) mVDSession->setFboInputTexture(f, mVDSession->getLoadedTexture(t), poolName);

					sprintf_s(buf, "Set input texture to %s", poolName.c_str());
					if (ImGui::IsItemHovered()) ImGui::SetTooltip(buf);
					ImGui::PopStyleColor(1);
				}
			}

			// playback controls - one panel per fbo (not per texture slot), hence "f" here
			if (mVDSession->isSequence(f) || mVDSession->isMovie(f)) {
				sprintf_s(buf, "p##s%d", f);
				if (ImGui::Button(buf))
				{
					mVDSession->togglePlayPause(f);
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Play/Pause");
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
				if (ImGui::IsItemActivated()) {
					// drag just started - mute for its duration (setVideoVolume/getVideoVolume are
					// no-ops outside MOVIE mode, safe to call unconditionally for sequences too)
					mPreScrubVolume[f] = mVDSession->getVideoVolume(f);
					mVDSession->setVideoVolume(f, 0.0f);
					mIsScrubbing[f] = true;
				}
				if (mIsScrubbing[f] && ImGui::IsItemDeactivated()) {
					// drag just ended (regardless of whether the value actually changed) - restore
					mVDSession->setVideoVolume(f, mPreScrubVolume[f]);
					mIsScrubbing[f] = false;
				}
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

						if (ImGui::DragFloat(buf, &localValues[ctrl], spd, getMinUniformValue(ctrl), getMaxUniformValue(ctrl)))
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
		ImGui::End();
		ImGui::PopStyleColor(5);
	} // for getFboList

	// nobody claimed it above (dropped outside every fbo window) - add it to the selected fbo's
	// texture list instead
	mVDSession->flushPendingTextureDrop();

#pragma endregion fbos

}
