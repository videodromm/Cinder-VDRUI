#include "VDUIFolders.h"

// snprintf on every platform: MSVC's sprintf_s ends the app (invalid-parameter handler) when
// the text doesn't fit the 64-char buffers, e.g. a long shader name; snprintf truncates
#ifdef sprintf_s
#undef sprintf_s
#endif
#define sprintf_s(buffer, format, ...) snprintf((buffer), sizeof(buffer), (format), ##__VA_ARGS__)

using namespace videodromm;

VDUIFolders::VDUIFolders(VDUniformsRef aVDUniforms, VDSessionFacadeRef aVDSession) {
	mVDUniforms = aVDUniforms;
	mVDSession = aVDSession;
	// Params
	mVDParams = VDParams::create();
}

// folders on the left, the selected folder's files on the right; a section of VDUI's tabbed
// "Panel" (2026-10-27), drawn inside its window
void VDUIFolders::RunContent() {
	float uiScale = mVDUniforms->getUniformValue(mVDUniforms->IUISCALE);
	{
		if (!mFoldersRequested) {
			mFoldersRequested = true;
			mVDSession->listFolders();
		}
		// a folder was loaded (here, by a drop, or picked in the WebApp): select it and list its files
		if (mVDSession->getFolderLoadCount() != mFolderLoadCount) {
			mFolderLoadCount = mVDSession->getFolderLoadCount();
			mSelectedFolder = mVDSession->getLoadedFolder();
			if (!mSelectedFolder.empty()) mVDSession->listShaders(mSelectedFolder, mExtension);
		}
		const std::string loadedFolder = mVDSession->getLoadedFolder();
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
		const std::string defaultFolder = mVDSession->getDefaultFolder();
		for (auto& folder : folders) {
			// D: this folder is loaded at startup (assets/mix.json), green when it is
			const bool isDefault = (folder == defaultFolder);
			if (isDefault) ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0, 160, 0, 255));
			sprintf_s(buf, "D##folderdefault%s", folder.c_str());
			if (ImGui::Button(buf) && !isDefault) {
				mVDSession->setDefaultFolder(folder);
			}
			if (isDefault) ImGui::PopStyleColor(1);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip(isDefault ? "Loaded at startup (assets/mix.json)" : "Load this folder at startup (writes assets/mix.json)");
			ImGui::SameLine();

			// loads this folder the same way dropping it onto the app window does (VDSession::
			// fileDrop()'s "folder was dropped" branch) - both just call loadFolder() with the
			// folder's plain name, resolved against the assets path
			
			// L: green on the loaded folder (the WebApp shows the same one)
			const bool isLoaded = (folder == loadedFolder);
			if (isLoaded) ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0, 160, 0, 255));
			sprintf_s( buf, "L##folderload%s", folder.c_str() );
			if( ImGui::Button( buf ) ) {
				mVDSession->loadFolder( folder );
			}
			if (isLoaded) ImGui::PopStyleColor(1);
			if( ImGui::IsItemHovered() )
				ImGui::SetTooltip( isLoaded ? "Loaded (also the WebApp's folder)" : "Load this folder (same as dropping it onto the app; the WebApp follows)" );
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
}
