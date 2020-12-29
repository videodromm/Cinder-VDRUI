#include "VDUIShaders.h"

using namespace videodromm;

VDUIShaders::VDUIShaders(VDSettingsRef aVDSettings, VDSessionFacadeRef aVDSession) {
	mVDSettings = aVDSettings;
	mVDSettings = aVDSettings;
	mVDSession = aVDSession;
	mVDParams = VDParams::create();
	for (int c = 0; c < 128; c++)
	{
		localValues[c] = 0.2f;// mVDSession->getUniformValueByLocation(c);
	}
}
VDUIShaders::~VDUIShaders() {

}

void VDUIShaders::Run(const char* title) {

	xPos = mVDParams->getUIMargin() + mVDParams->getUIXPosCol1();
	yPos = mVDParams->getUIYPosRow2();
	for (unsigned int s = 0; s < mVDSession->getFboShaderListSize(); s++) {
		ImGui::SetNextWindowSize(ImVec2(mVDParams->getUILargePreviewW(), mVDParams->getUILargePreviewH()), ImGuiSetCond_Once);
		ImGui::SetNextWindowPos(ImVec2(xPos, yPos), ImGuiSetCond_Once);
		//int hue = 0;
		sprintf(buf, "%s##sh%d", mVDSession->getFboShaderName(s).c_str(), s);
		ImGui::Begin(buf, NULL, ImVec2(0, 0), ImGui::GetStyle().Alpha, ImGuiWindowFlags_NoSavedSettings);
		{
			ImGui::PushItemWidth(mVDParams->getPreviewFboWidth());
			ImGui::PushID(s);
			ImGui::Image((void*)mVDSession->getFboShaderTexture(s)->getId(), ivec2(mVDParams->getPreviewFboWidth(), mVDParams->getPreviewFboHeight()));
			
			
			int hue = 0;
			
			for (auto u : mVDSession->getFboShaderUniforms(s)) {
				string uName = u.getName(); // TODO use getIndex?
				ctrl = u.getLocation();// mVDSession->getUniformIndexForName(uName);
				
				switch (u.getType()) {
				case GL_BOOL:
					// boolean 35670 GL_BOOL 0x8B56
					(getValue(ctrl, s) > 0.0f) ? ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 16.0f, 1.0f, 0.5f)) : ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1.0f, 0.1f, 0.1f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue / 16.0f, 0.7f, 0.7f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue / 16.0f, 0.8f, 0.8f));
					sprintf(buf, "%s##booluniform%d", uName.c_str(), s);
					/*if (ImGui::Button(buf)) {
						toggleValue(ctrl, s);
					}*/
					ImGui::PopStyleColor(3);
					hue++;
					break;
				case GL_INT:
					// int 5124 GL_INT 0x1404
					(getValue(ctrl, s)) ? ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 16.0f, 1.0f, 0.5f)) : ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1.0f, 0.1f, 0.1f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue / 16.0f, 0.7f, 0.7f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue / 16.0f, 0.8f, 0.8f));
					sprintf(buf, "%s##intuniform%d", uName.c_str(), s);
					if (ImGui::Button(buf)) {
						//toggleValue(ctrl, f);
					}
					ImGui::PopStyleColor(3);
					hue++;
					break;
				case GL_SAMPLER_2D:
					// sampler2d 35678 GL_SAMPLER_2D 0x8B5E
					sprintf(buf, "%s##texuniform%d", uName.c_str(), s);
					//mShowInputTexture ^= ImGui::Button(buf);
					break;
				case GL_FLOAT:
					// float 5126 GL_FLOAT 0x1406
					//localValues[ctrl] = getValue(ctrl, s);
					//val = getValue(ctrl, s);
					if (uName == "time") {
						sprintf(buf, "time %.0f", mVDSession->getUniformValue(0));// localValues[ctrl]);
						ImGui::Text(buf);
						ImGui::TextColored(ImColor(200, 200, 200), buf);
					}
					else {
						sprintf(buf, "%s##floatuniform%d", uName.c_str(), s);
						//if (ImGui::DragFloat(buf, &val, 0.001f, 0.0f, 1.0f));//localValues[ctrl] getMinUniformValue(ctrl), getMaxUniformValue(ctrl)))
						if (ImGui::SliderFloat(buf, &localValues[ctrl], 0.0f, 1.0f, "%.3f", 3.0f))
						{
							//if (localValues[ctrl] > 0.0f) {
								setValue(ctrl, s, localValues[ctrl]);//localValues[ctrl]
							//}
						}
					}
					/*mouseX = getFloatValue(mVDUniforms->IMOUSEX);
					if (ImGui::SliderFloat("MouseX", &mouseX, 0.0f, mVDParams->getFboWidth(), "%.4f", 3.0f))
					{
						setFloatValue(mVDUniforms->IMOUSEX, mouseX);
					}*/
					break;
				case GL_FLOAT_VEC2:
					// vec2 35664 GL_FLOAT_VEC2 0x8B50
					if (uName == "RENDERSIZE" || uName == "resolution") {
						float fw = mVDSession->buildFboRenderedTexture(s)->getWidth();
						//ctrl = mVDSession->getUniformIndexForName("iResolutionX");
						//localValues[ctrl] = mVDSession->getUniformValue(ctrl);
						//sprintf(buf, "rw %.0f##v2x%d", localValues[ctrl], f);
						sprintf(buf, "rw %.0f", fw);
						//ImGui::Button(buf);
						ImGui::TextColored(ImColor(100, 100, 100), buf);
						ImGui::SameLine();
						float fh = mVDSession->buildFboRenderedTexture(s)->getHeight();
						//ctrl = mVDSession->getUniformIndexForName("iResolutionY");
						//localValues[ctrl] = mVDSession->getUniformValue(ctrl);
						//sprintf(buf, "rh %.0f##v2y%d", localValues[ctrl], f);
						sprintf(buf, "rh %.0f", fh);
						ImGui::TextColored(ImColor(100, 100, 100), buf);
						//ImGui::Button(buf);
					}
					else {
						sprintf(buf, "vec2 %s %d", uName.c_str(), u.getType());
						ImGui::TextColored(ImColor(150, 220, 0), buf);
					}
					break;
				case GL_FLOAT_VEC3:
					// vec3 35665 GL_FLOAT_VEC3 0x8B51
					sprintf(buf, "vec3 %s %d##v3uniform%d", uName.c_str(), u.getType(), s);
					ImGui::TextColored(ImColor(100, 100, 0), buf);

					break;
				case GL_FLOAT_VEC4:
					// vec4 35666 GL_FLOAT_VEC4 0x8B52
					sprintf(buf, "vec4 %s %d", uName.c_str(), u.getType());
					ImGui::TextColored(ImColor(100, 100, 100), buf);
					/*if (ctrl == mVDUniforms->IMOUSE) {
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
					if (uName != "ciModelViewProjection") {
						sprintf(buf, "! %s %d", uName.c_str(), u.getType());
						ImGui::TextColored(ImColor(255, 0, 0), buf);
					}
					break;
				}

			} //for uniforms

			ImGui::SameLine();

		
			ImGui::PopID();
			ImGui::PopItemWidth();
		}
		ImGui::End();

		xPos += mVDParams->getUILargePreviewW() + mVDParams->getUIMargin();
		if (xPos > mVDSettings->mRenderWidth + mVDParams->getUILargePreviewW())
		{
			xPos = mVDParams->getUIMargin();
			yPos += mVDParams->getUILargePreviewH() + mVDParams->getUIMargin();
		}
		
	}

}
