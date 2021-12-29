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

	static int XLeft[64];
	static int YTop[64];
	static int XRight[64];
	static int YBottom[64];
	static bool rnd[64];
	static bool anim[64];

	xPos = mVDParams->getUIMargin() + mVDParams->getUIXPosCol1();
	yPos = mVDParams->getUIYPosRow3();
	for (int t = 0; t < mVDSession->getInputTexturesCount(); t++) {
		ImGui::SetNextWindowSize(ImVec2(mVDParams->getUILargePreviewW(), mVDParams->getPreviewHeight()), ImGuiSetCond_Once);
		ImGui::SetNextWindowPos(ImVec2(xPos, yPos), ImGuiSetCond_Once);
		int hue = 0;
		sprintf(buf, "%s##s%d", mVDSession->getInputTextureName(t).c_str(), t);
		ImGui::Begin(buf, NULL, ImVec2(0, 0), ImGui::GetStyle().Alpha, ImGuiWindowFlags_NoSavedSettings);
		{
			ImGui::PushItemWidth(mVDParams->getPreviewFboWidth());
			ImGui::PushID(t);
			ImGui::Image(mVDSession->getFboInputTexture(t), ivec2(mVDParams->getPreviewFboWidth(), mVDParams->getPreviewFboHeight()));
			ImGui::PushItemWidth(mVDParams->getPreviewFboWidth());
			/* 20210108 TODO
			if (mVDSession->isSequence(t) ) {
				sprintf(buf, "p##s%d", t);
				if (ImGui::Button(buf))
				{
					mVDSession->togglePlayPause(t);
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Play/Pause");

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
				if (ImGui::DragFloat(buf, &speeds[t], 0.01f, 0.0f, 1.0f))
				{
					mVDSession->setSpeed(t, speeds[t]);
				}

				playheadPositions[t] = mVDSession->getPosition(t);
				sprintf(buf, "scrub##srb%d", t);
				if (ImGui::SliderInt(buf, &playheadPositions[t], 0, mVDSession->getMaxFrame(t) - 1))
				{
					mVDSession->setPlayheadPosition(t, playheadPositions[t]);
				}


			} // end seq

			ImGui::SameLine();
			(rnd[t]) ? ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 7.0f, 1.0f, 0.5f)) : ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1.0f, 0.1f, 0.1f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue / 7.0f, 0.7f, 0.7f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue / 7.0f, 0.8f, 0.8f));
			sprintf(buf, "rnd##r%d", t);
			rnd[t] ^= ImGui::Button(buf);
			ImGui::PopStyleColor(3);
			hue++;*/


			//}
		//}
			ImGui::PopItemWidth();
			ImGui::PopID();
			ImGui::PopItemWidth();
		}
		ImGui::End();
		xPos += mVDParams->getUILargePreviewW() + mVDParams->getUIMargin();
		//if (xPos > (mVDSettings->mRenderWidth - mVDSettings->uiLargePreviewW))
		//ImGui::SetNextWindowPos(ImVec2(mVDParams->getUIMargin() + mVDParams->getUIXPosCol1()
		//	+ ((mVDParams->getUILargePreviewW() + mVDParams->getUIMargin()) * t), mVDParams->getUIYPosRow3()), ImGuiSetCond_Once);
		if (t % 8 == 7)
		{
			xPos = mVDParams->getUIMargin() + mVDParams->getUIXPosCol1();
			yPos += mVDParams->getPreviewHeight() + 10.0f + mVDParams->getUIMargin();
		}
	}

}
