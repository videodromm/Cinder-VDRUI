#include "VDUIFbos.h"

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
	static int XLeft[64];
	static int YTop[64];
	static int XRight[64];
	static int YBottom[64];
	static bool rnd[64];
	static bool anim[64];

	/*for (int t = 0; t < mVDSession->getInputTexturesCount(); t++) {
		xPos = mVDParams->getUIMargin() + mVDParams->getUIXPosCol1() + ((mVDParams->getUILargePreviewW() + mVDParams->getUIMargin()) * t);
		yPos = mVDSettings->uiYPosRow2;

		ImGui::SetNextWindowSize(ImVec2(mVDParams->getUILargePreviewW(), mVDSettings->uiLargePreviewH), ImGuiSetCond_Once);
		ImGui::SetNextWindowPos(ImVec2(xPos, yPos), ImGuiSetCond_Once);
		int hue = 0;
		printf(buf, "%s##s%d", mVDSession->getInputTextureName(t).c_str(), t);
		ImGui::Begin(buf, NULL, ImVec2(0, 0), ImGui::GetStyle().Alpha, ImGuiWindowFlags_NoSavedSettings);
		{
			ImGui::PushItemWidth(mVDParams->getPreviewFboWidth());
			ImGui::PushID(t);
			ImGui::Image((void*)mVDSession->getInputTexture(t)->getId(), ivec2(mVDParams->getPreviewFboWidth(), mVDParams->getPreviewFboHeight()));
			/*ImGui::PushItemWidth(mVDParams->getPreviewFboWidth() * 0.7);
			// flip vertically
			mVDSession->isFlipVInputTexture(t) ? ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 16.0f, 1.0f, 0.5f)) : ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1.0f, 0.1f, 0.1f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue / 16.0f, 0.7f, 0.7f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue / 16.0f, 0.8f, 0.8f));
			sprintf(buf, "FlipV##vd%d", t);
			if (ImGui::Button(buf)) {
				mVDSession->inputTextureFlipV(t);
			}
			ImGui::PopStyleColor(3);
			hue++;
			ImGui::SameLine();
			// flip horizontally
			mVDSession->isFlipHInputTexture(t) ? ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 16.0f, 1.0f, 0.5f)) : ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1.0f, 0.1f, 0.1f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue / 16.0f, 0.7f, 0.7f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue / 16.0f, 0.8f, 0.8f));
			sprintf(buf, "FlipH##hd%d", t);
			if (ImGui::Button(buf)) {
				mVDSession->inputTextureFlipH(t);
			}
			ImGui::PopStyleColor(3);
			hue++;

			//for (unsigned int f = 0; f < mVDMix->getWarpCount(); f++) {
			//if (f > 0) ImGui::SameLine();
			////int ti = mVDMix->getFboInputTextureIndex(f);
			////CI_LOG_V("fbo" + toString(f) + " t" + toString(t) + " ti" + toString(ti));
			//if (mVDMix->getWarpATextureIndex(f) == t) {
			//ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(t / 16.0f, 1.0f, 1.0f));
			//}
			//else {
			//ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(t / 16.0f, 0.1f, 0.1f));
			//}
			//ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(t / 16.0f, 0.7f, 0.7f));
			//ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(t / 16.0f, 0.8f, 0.8f));
			//sprintf(buf, "%d##fboinputtex%d%d", f, t, f);
			//if (ImGui::Button(buf)) mVDMix->setFboInputTexture(f, t);
			//if (ImGui::IsItemHovered()) ImGui::SetTooltip("Set input texture for warp");
			//ImGui::PopStyleColor(3);
			//}

			//sprintf(buf, "WS##s%d", i);
			//if (ImGui::Button(buf))
			//{
			//sprintf(buf, "IMG=%d.jpg", i);
			////mVDRouter->wsWrite(buf);
			//}
			//if (ImGui::IsItemHovered()) ImGui::SetTooltip("Send texture file name via SocketIO");
			//

			if (mVDSession->isSequence(t) || mVDSession->isMovie(t)) {
				sprintf(buf, "p##s%d", t);
				if (ImGui::Button(buf))
				{
					mVDSession->togglePlayPause(t);
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Play/Pause");
			}
			if (mVDSession->isSequence(t)) {
				ImGui::SameLine();
				sprintf(buf, "b##sqs%d", t);
				if (ImGui::Button(buf))
				{
					mVDSession->syncToBeat(t);
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Sync to beat");

				ImGui::SameLine();
				sprintf(buf, "r##rs%d", t);
				if (ImGui::Button(buf))
				{
					mVDSession->reverse(t);
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Reverse");

				if (mVDSession->isLoadingFromDisk(t)) {
					ImGui::SameLine();
					sprintf(buf, "l##ts%d", t);
					if (ImGui::Button(buf))
					{
						mVDSession->toggleLoadingFromDisk(t);
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Pause loading from disk");
				}
				speeds[t] = mVDSession->getSpeed(t);
				sprintf(buf, "speed##spd%d", t);
				if (ImGui::SliderFloat(buf, &speeds[t], 0.0f, 1.0f))
				{
					mVDSession->setSpeed(t, speeds[t]);
				}

				playheadPositions[t] = mVDSession->getPosition(t);
				sprintf(buf, "scrub##srb%d", t);
				if (ImGui::SliderInt(buf, &playheadPositions[t], 0, mVDSession->getMaxFrame(t)))
				{
					mVDSession->setPlayheadPosition(t, playheadPositions[t]);
				}


			}
			else {
				if (!mVDSession->isMovie(t)) {
					// not a sequence nor video, animate x y...
					XLeft[t] = mVDSession->getInputTextureXLeft(t);
					if (anim[t]) {
						if (rnd[t]) {
							XLeft[t] += xStep * Rand::randBool();
						}
						else {
							XLeft[t] += xStep;
						}
						// check bounds
						if (XLeft[t] < 1) {
							xStep = -xStep;
						}
						if (XLeft[t] > mVDSession->getInputTextureOriginalWidth(t) - mVDParams->getFboWidth() - 1) {
							xStep = -xStep;
						}
					}
					sprintf(buf, "XL##xl%d", t);
					ImGui::SliderInt(buf, &XLeft[t], 0, mVDSession->getInputTextureOriginalWidth(t));// CHECK - mVDParams->getFboWidth()
					mVDSession->setInputTextureXLeft(t, XLeft[t]);

					YTop[t] = mVDSession->getInputTextureYTop(t);
					if (anim[t]) {
						if (rnd[t]) {
							YTop[t] += yStep * Rand::randBool();
						}
						else {
							YTop[t] += yStep;
						}
						// check bounds
						if (YTop[t] < 1) {
							yStep = -yStep;
						}
						if (YTop[t] > mVDSession->getInputTextureOriginalHeight(t) - mVDParams->getFboHeight() - 1) {
							yStep = -yStep;
						}
					}
					sprintf(buf, "YT##yt%d", t);
					ImGui::SliderInt(buf, &YTop[t], 0, mVDSession->getInputTextureOriginalHeight(t));// - mVDParams->getFboHeight()
					mVDSession->setInputTextureYTop(t, YTop[t]);


					ImGui::SameLine();
					(mVDSession->getInputTextureLockBounds(t)) ? ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 16.0f, 1.0f, 0.5f)) : ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1.0f, 0.1f, 0.1f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue / 16.0f, 0.7f, 0.7f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue / 16.0f, 0.8f, 0.8f));
					sprintf(buf, "8##lk%d", t);
					if (ImGui::Button(buf)) {
						mVDSession->toggleInputTextureLockBounds(t);
					}
					ImGui::PopStyleColor(3);
					hue++;

					XRight[t] = mVDSession->getInputTextureXRight(t);
					sprintf(buf, "XR##xr%d", t);
					if (ImGui::SliderInt(buf, &XRight[t], 0, mVDSession->getInputTextureOriginalWidth(t))) {
						mVDSession->setInputTextureXRight(t, XRight[t]);
					}
					ImGui::SameLine();
					(anim[t]) ? ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 16.0f, 1.0f, 0.5f)) : ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1.0f, 0.1f, 0.1f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue / 16.0f, 0.7f, 0.7f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue / 16.0f, 0.8f, 0.8f));
					sprintf(buf, "ani##r%d", t);
					anim[t] ^= ImGui::Button(buf);
					ImGui::PopStyleColor(3);
					hue++;

					YBottom[t] = mVDSession->getInputTextureYBottom(t);
					sprintf(buf, "YB##yb%d", t);
					if (ImGui::SliderInt(buf, &YBottom[t], 0, mVDSession->getInputTextureOriginalHeight(t))) {
						mVDSession->setInputTextureYBottom(t, YBottom[t]);
					}

					ImGui::SameLine();
					(rnd[t]) ? ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 16.0f, 1.0f, 0.5f)) : ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1.0f, 0.1f, 0.1f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue / 16.0f, 0.7f, 0.7f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue / 16.0f, 0.8f, 0.8f));
					sprintf(buf, "rnd##r%d", t);
					rnd[t] ^= ImGui::Button(buf);
					ImGui::PopStyleColor(3);
					hue++;
				}
			}
			ImGui::PopItemWidth();
			ImGui::PopID();
			ImGui::PopItemWidth();
		}
		ImGui::End();
	}*/


#pragma region mix
	int w = 0;
	xPos = mVDParams->getUIMargin() + mVDParams->getUIXPosCol1();
	yPos = mVDParams->getUIYPosRow2();// - mVDSettings->uiLargePreviewH

	ImGui::SetNextWindowSize(ImVec2(mVDParams->getUILargePreviewW(), mVDParams->getUILargePreviewH()), ImGuiSetCond_Once);
	ImGui::SetNextWindowPos(ImVec2(xPos, yPos), ImGuiSetCond_Once);
	/*
	sprintf(buf, "%s##sh%d", "mix", 0);
	ImGui::Begin(buf, NULL, ImVec2(0, 0), ImGui::GetStyle().Alpha, ImGuiWindowFlags_NoSavedSettings);
	{
		ImGui::PushItemWidth(mVDParams->getPreviewFboWidth());
		ImGui::Image((void*)mVDSession->getMixTexture(w)->getId(), ivec2(mVDParams->getPreviewFboWidth(), mVDParams->getPreviewFboHeight()));
		// crossfade
		float xFade = mVDSession->getCrossfade();// getWarpCrossfade(w);
		sprintf(buf, "xfade##xf%d", w);
		if (ImGui::SliderFloat(buf, &xFade, 0.0f, 1.0f))
		{
			mVDSession->setCrossfade(xFade);
		}

		for (int s = 0; s < mVDSession->getShadersCount(); s++) {
			int f = 0;
			if (s > 0 && (s % 9 != 0)) ImGui::SameLine();

			if (mVDSession->getFboFragmentShaderIndex(f) == s) {
				ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 1.0f, 0.5f));
			}
			else {
				ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.1f, 0.1f));
			}
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.7f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
			sprintf(buf, "%d##sf%d", s, f);
			if (ImGui::Button(buf)) mVDSession->setFboFragmentShaderIndex(f, s);
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Set fbo to left");
			ImGui::PopStyleColor(3);
		}
		for (int s = 0; s < mVDSession->getShadersCount(); s++) {
			int f = 1;
			if (s > 0 && (s % 9 != 0)) ImGui::SameLine();

			if (mVDSession->getFboFragmentShaderIndex(f) == s) {
				ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 1.0f, 0.5f));
			}
			else {
				ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.1f, 0.1f));
			}
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.7f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
			sprintf(buf, "%d##sf%d", s, f);
			if (ImGui::Button(buf)) mVDSession->setFboFragmentShaderIndex(f, s);
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Set shader to right");
			ImGui::PopStyleColor(3);
		}

		ImGui::PopItemWidth();
	}
	ImGui::End();
	*/
#pragma endregion mix
#pragma region fbos

	/*
	** fbos
	*/

	for (unsigned int f = 0; f < mVDSession->getFboShaderListSize(); f++) {
		xPos = mVDParams->getUIMargin() + mVDParams->getUIXPosCol1() + ((mVDParams->getUILargePreviewW() + mVDParams->getUIMargin()) * (f));//+1

		ImGui::SetNextWindowSize(ImVec2(mVDParams->getUILargePreviewW(), mVDParams->getUILargePreviewH() * 1.4), ImGuiSetCond_Once);
		ImGui::SetNextWindowPos(ImVec2(xPos, yPos), ImGuiSetCond_Once);
		ImGui::PushStyleColor(ImGuiCol_TitleBg, (ImVec4)ImColor::HSV(f / 16.0f, 0.7f, 0.7f));
		sprintf(buf, "%s##fbolbl%d", mVDSession->getFboName(f).c_str(), f);
		ImGui::Begin(buf, NULL, ImGuiWindowFlags_NoSavedSettings);
		{
			ImGui::PushID(f);
			ImGui::PushItemWidth(mVDParams->getPreviewFboWidth());
			if (!mVDSession->isFboValid(f)) {
				ImGui::TextColored(ImColor(255, 0, 0), "Invalid: %s", mVDSession->getError(f).c_str());
			}
			else {
				ImGui::TextColored(ImColor(0, 255, 0), "%s", mVDSession->getMsg(f).c_str());
			}
			sprintf(buf, "fbo##rdrtexuniform%d", f);
			mShowRenderedTexture ^= ImGui::Button(buf);
			ImGui::SameLine();
			sprintf(buf, "tex##rdrtexuniform%d", f);
			mShowInputTexture ^= ImGui::Button(buf);
			ImGui::SameLine();
			sprintf(buf, "audio##fboinputaudiotex%d", f);
			if (ImGui::Button(buf)) mVDSession->setFboAudioInputTexture(f);
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Set input texture to Audio");
		

			/*ImGui::SameLine();
			sprintf(buf, "global %d##gu%d", mVDSession->getGlobal(f), f);
			if (ImGui::Button(buf)) {
				mVDSession->toggleGlobal(f);
			}
			ImGui::SameLine();
			if (!mVDSession->isFboValid(f)) {
				ImGui::TextColored(ImColor(255, 0, 0), "Invalid: %s - %s", mVDSettings->mFboMsg.c_str(), mVDSettings->mShaderMsg.c_str());
			}
			else {
				sprintf(buf, "T##fboupd%d", f);
				if(ImGui::Button(buf)) mVDSession->updateShaderThumbFile(f);
			}*/
			//if (mVDSession->getFboRenderedTexture(f)) ImGui::Image((void*)mVDSession->getFboRenderedTexture(f)->getId(), ivec2(mVDParams->getPreviewFboWidth(), mVDParams->getPreviewFboHeight()));
			if (mVDSession->buildFboRenderedTexture(f) && mShowRenderedTexture) ImGui::Image(mVDSession->buildFboRenderedTexture(f), ivec2(mVDParams->getPreviewFboWidth(), mVDParams->getPreviewFboHeight()));

			//if (mVDSession->getFboInputTexture(f)) ImGui::Image((void*)mVDSession->getFboInputTexture(f)->getId(), ivec2(mVDParams->getPreviewFboWidth(), mVDParams->getPreviewFboHeight()));
			if (mVDSession->buildFboInputTexture(f) && mShowInputTexture) ImGui::Image(mVDSession->buildFboInputTexture(f), ivec2(mVDParams->getPreviewFboWidth(), mVDParams->getPreviewFboHeight()));
			sprintf(buf, "%s", mVDSession->getFboInputTextureName(f).c_str());
			if (ImGui::IsItemHovered()) ImGui::SetTooltip(buf);

			// uniforms
			//std::vector<ci::gl::GlslProg::Uniform> u = mVDSession->getUniforms(f);
			int hue = 0;
			for (auto u : mVDSession->getUniforms(f)) {
				string uName = u.getName(); // TODO use getIndex?
				ctrl = mVDSession->getUniformIndexForName(uName);

				switch (u.getType()) {
				case GL_BOOL:
					// boolean 35670 GL_BOOL 0x8B56
					(getValue(ctrl, f) > 0.0f) ? ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 16.0f, 1.0f, 0.5f)) : ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1.0f, 0.1f, 0.1f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue / 16.0f, 0.7f, 0.7f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue / 16.0f, 0.8f, 0.8f));
					sprintf(buf, "%s##booluniform%d", uName.c_str(), f);
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
					sprintf(buf, "%s##intuniform%d", uName.c_str(), f);
					if (ImGui::Button(buf)) {
						//toggleValue(ctrl, f);
					}
					ImGui::PopStyleColor(3);
					hue++;
					break;
				case GL_SAMPLER_2D:
					// sampler2d 35678 GL_SAMPLER_2D 0x8B5E
					sprintf(buf, "%s##texuniform%d", uName.c_str(), f);
					mShowInputTexture ^= ImGui::Button(buf);
					break;
				case GL_FLOAT:
					// float 5126 GL_FLOAT 0x1406
					localValues[ctrl] = mVDSession->getUniformValue(ctrl);
					if (ctrl > 0) {						
						sprintf(buf, "%s##floatuniform%d", uName.c_str(), f);
						if (ImGui::DragFloat(buf, &localValues[ctrl], 0.001f, getMinUniformValue(ctrl), getMaxUniformValue(ctrl)))
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
							sprintf(buf, "%s##floatuniform%d", uName.c_str(), f);
							if (ImGui::DragFloat(buf, &mUniformValueByLocation[location], 0.001f, 0.0001f, 50.0f))
							{
								mVDSession->setUniformValueByLocation(f, location, mUniformValueByLocation[location]);
								
							}
							
						}
					}
					break;
				case GL_FLOAT_VEC2:
					// vec2 35664		
					if (uName == "RENDERSIZE" || uName == "resolution") {
						float fw = mVDSession->buildFboRenderedTexture(f)->getWidth();
						//ctrl = mVDSession->getUniformIndexForName("iResolutionX");
						//localValues[ctrl] = mVDSession->getUniformValue(ctrl);
						//sprintf(buf, "rw %.0f##v2x%d", localValues[ctrl], f);
						sprintf(buf, "rw %.0f", fw);
						//ImGui::Button(buf);
						ImGui::TextColored(ImColor(100, 100, 100), buf);
						ImGui::SameLine();
						float fh = mVDSession->buildFboRenderedTexture(f)->getHeight();
						//ctrl = mVDSession->getUniformIndexForName("iResolutionY");
						//localValues[ctrl] = mVDSession->getUniformValue(ctrl);
						//sprintf(buf, "rh %.0f##v2y%d", localValues[ctrl], f);
						sprintf(buf, "rh %.0f", fh);
						ImGui::TextColored(ImColor(100, 100, 100), buf);
						//ImGui::Button(buf);
					}
					else {
						sprintf(buf, "vec2 %s", uName.c_str(), f);
						ImGui::TextColored(ImColor(150, 220, 0), buf);
					}
					break;
				case GL_FLOAT_VEC3:
					// vec3 35665
					sprintf(buf, "vec3 %s", uName.c_str(), f);
					ImGui::TextColored(ImColor(100, 100, 0), buf);

					break;
				case GL_FLOAT_VEC4:
					// vec4 35666 GL_FLOAT_VEC4
					sprintf(buf, "vec4 %s %d", uName.c_str(), u.getType());
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
					}
					break;
				default:
					//ciModelViewProjection 35676 GL_FLOAT_MAT4 0x8B5C
					/* gl2.h
						GL_FLOAT_VEC2                     0x8B50
						GL_FLOAT_VEC3                     0x8B51
						GL_FLOAT_VEC4                     0x8B52
					*/
					if (uName != "ciModelViewProjection") {
						sprintf(buf, "! %s %d", uName.c_str(), u.getType());
						ImGui::TextColored(ImColor(255, 0, 0), buf);
					}
					break;
				}

			} //for uniforms



			float fw = mVDSession->getFboTextureWidth(f);
			sprintf(buf, "tw %.0f", fw);
			ImGui::TextColored(ImColor(120, 120, 120), buf);
			ImGui::SameLine();
			float fh = mVDSession->getFboTextureHeight(f);
			sprintf(buf, "th %.0f", fh);
			ImGui::TextColored(ImColor(120, 120, 120), buf);
			//sprintf(buf, "%s", mVDSession->getFboStatus(f).c_str());
			//ImGui::TextColored(ImColor(220, 220, 0), buf);
			/*ImGui::SameLine();
			sprintf(buf, "T##fboupd%d", f);
			i/f (ImGui::Button(buf)) mVDSession->updateShaderThumbFile(f);*
			ImGui::Text("wh %dx%d", mVDSession->getFboRenderedTexture(f)->getWidth(), mVDSession->getFboRenderedTexture(f)->getHeight());*/
			ImGui::PopItemWidth();
			ImGui::PopID();
		}
		ImGui::End();
		ImGui::PopStyleColor(1);
	} // for getFboList

#pragma endregion fbos

}
