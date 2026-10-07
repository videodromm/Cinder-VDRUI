#include "VDUIFolders.h"

#if ! defined( CINDER_MSW )
#define sprintf_s(buffer, format, ...) snprintf((buffer), sizeof(buffer), (format), ##__VA_ARGS__)
#endif

using namespace videodromm;

VDUIFolders::VDUIFolders(VDUniformsRef aVDUniforms, VDSessionFacadeRef aVDSession) {
	mVDUniforms = aVDUniforms;
	mVDSession = aVDSession;
	// Params
	mVDParams = VDParams::create();
}

void VDUIFolders::Run(const char* title) {

	float uiScale = mVDUniforms->getUniformValue(mVDUniforms->IUISCALE);
	// folders on the left, the selected folder's files on the right
	ImGui::SetNextWindowSize(ImVec2(600.0f * uiScale, mVDParams->getUILargeH() * 2.0f * uiScale), ImGuiCond_Once);
	ImGui::SetNextWindowPos(ImVec2(mVDParams->getUIXPosCol3() * uiScale, mVDParams->getUIYPosRow3() * uiScale), ImGuiCond_Once);

	ImGui::Begin(title, NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse);
	{
		if (!mFoldersRequested) {
			mFoldersRequested = true;
			mVDSession->listFolders();
		}
		if (ImGui::Button("Refresh")) {
			mVDSession->listFolders();
		}

		ImGui::SameLine();
		ImGui::PushItemWidth(80.0f * uiScale);
		sprintf_s(buf, "%s", mExtension.c_str());
		if (ImGui::InputText("ext", buf, IM_ARRAYSIZE(buf))) {
			mExtension = buf;
		}
		ImGui::PopItemWidth();

		ImGui::Separator();
		// two columns, each scrolling on its own
		ImGui::BeginChild("##folderlist", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0.0f), ImGuiChildFlags_Borders);
		ImGui::TextColored(ImColor(155, 255, 0), "Folders");
		auto folders = mVDSession->getFolderList();
		for (auto& folder : folders) {
			// loads this folder the same way dropping it onto the app window does (VDSession::
			// fileDrop()'s "folder was dropped" branch) - both just call loadFolder() with the
			// folder's plain name, resolved against the assets path
			
			sprintf_s( buf, "L##folderload%s", folder.c_str() );
			if( ImGui::Button( buf ) ) {
				mVDSession->loadFolder( folder );
			}
			if( ImGui::IsItemHovered() )
				ImGui::SetTooltip( "Load this folder (same as dropping it onto the app)" );
			ImGui::SameLine();

			bool isSelected = (folder == mSelectedFolder);
			if (isSelected) ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(200, 150, 0, 220));
			sprintf_s(buf, "%s##folder", folder.c_str());
			if (ImGui::Button(buf)) {
				mSelectedFolder = folder;
				mVDSession->listShaders(mSelectedFolder, mExtension);
			}
			if (isSelected) ImGui::PopStyleColor(1);
		}
		ImGui::EndChild();
		ImGui::SameLine();
		ImGui::BeginChild("##folderfiles", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
		if (mSelectedFolder.empty()) {
			ImGui::TextColored(ImColor(150, 150, 150), "Select a folder to list its files");
		}
		else {
			ImGui::TextColored(ImColor(155, 255, 0), "%s/*.%s", mSelectedFolder.c_str(), mExtension.c_str());
			auto shaders = mVDSession->getShaderList();
			for (auto& shaderName : shaders) {
				sprintf_s(buf, "%s##shader", shaderName.c_str());
				if (ImGui::Button(buf)) {
					// into the selected fbo (the last fbo pane clicked, highlighted in purple)
					mVDSession->loadShaderFromFolder(mSelectedFolder, mExtension, shaderName);
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Load into the selected fbo (%d, purple)", mVDSession->getSelectedFbo());
			}
		}
		ImGui::EndChild();
	}
	ImGui::End();
}
