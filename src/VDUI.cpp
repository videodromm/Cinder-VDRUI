#include "VDUI.h"

#if ! defined( CINDER_MSW )
#define sprintf_s(buffer, format, ...) snprintf((buffer), sizeof(buffer), (format), ##__VA_ARGS__)
#endif

using namespace videodromm;

VDUI::VDUI(VDSettingsRef aVDSettings, VDSessionFacadeRef aVDSession, VDUniformsRef aVDUniforms) {
	mVDSettings = aVDSettings;
	mVDSession = aVDSession;
	mVDUniforms = aVDUniforms;
	// Params
	mVDParams = VDParams::create();
	// UITextures
	mUITextures = VDUITextures::create(mVDUniforms, mVDSession);
	// UIFbos
	mUIFbos = VDUIFbos::create(mVDSettings, mVDSession, mVDUniforms);
	// UIAnimation
	mUIAnimation = VDUIAnimation::create(mVDSettings, mVDSession, mVDUniforms);

	// UIWarps
	mUIWarps = VDUIWarps::create(mVDSettings, mVDSession, mVDUniforms);
	// UIFolders
	mUIFolders = VDUIFolders::create(mVDUniforms, mVDSession);
	// projector output window (opened on demand from the "Output" panel)
	mOutputWindow = VDOutputWindow::create(mVDSession);
#if defined( CINDER_MSW )
	// UIHtmlPage (WebView2, Windows-only)
	mUIHtmlPage = VDUIHtmlPage::create(mVDSession);
	mShowHtmlPage = false;
#endif
	// imgui
	mouseGlobal = false;
	//mouseZ = false;
	mIsResizing = true;
	mShowWarps = false;
	mShowFbos = true;
	mShowTextures = true;
	mShowBlend = false;
	mShowFolders = true;
}

void VDUI::registerPoolSources() {
	// the 2 Spout outputs: main output (what the main window shows) and the live code view
	if (auto mainTex = mOutputWindow->getMainViewTexture()) mVDSession->registerPoolTexture("Spout: main output", mainTex);
	if (auto codeView = mVDSession->getCodeView()) {
		if (auto codeTex = codeView->getTexture()) mVDSession->registerPoolTexture("Spout: VDCode", codeTex);
	}
	// fixed name: the audio texture's own name follows its source (line in, a file...); assigning
	// "audio" puts the fbo in audio mode (VDFboShader::assignInputTexture)
	if (auto audioTex = mVDSession->getAudioTexture()) mVDSession->registerPoolTexture("audio", audioTex);
#if defined( CINDER_MSW )
	// Spout senders from other apps: "Spout in: <sender>"
	mSpoutSources.update(mVDSession);
#endif
}

void VDUI::Run(const char* title, unsigned int fps) {
	static int currentWindowRow1 = 1;
	static int currentWindowRow2 = 0;

#if defined( CINDER_MSW )
	if (!mFileDropTargetTried) {
		mFileDropTargetTried = true;
		auto target = std::make_unique<VDFileDropTarget>();
		ci::app::WindowRef mainWindow = ci::app::getWindowIndex(0);
		target->onDrop = [this, mainWindow](const std::vector<ci::fs::path>& aFiles, const ci::ivec2& aPixels) {
			// same path as Cinder's fileDrop: VDSession::fileDrop() expects points
			float scale = mainWindow->getContentScale();
			mVDSession->fileDrop(ci::app::FileDropEvent(mainWindow, (int)(aPixels.x / scale), (int)(aPixels.y / scale), aFiles));
		};
		if (target->registerWindow((HWND)mainWindow->getNative())) mFileDropTarget = std::move(target);
		else CI_LOG_W("OLE drop target not registered: files still drop through Cinder, without hover highlight");
	}
	mVDSession->setExternalDrag(mFileDropTarget && mFileDropTarget->isDragging(), mFileDropTarget ? ci::vec2(mFileDropTarget->getPos()) : ci::vec2(0.0f));
#endif
	registerPoolSources();

	
	//ImGuiStyle& style = ImGui::GetStyle();

	if (mIsResizing) {
		mIsResizing = false;
		// set ui window and io events callbacks 
		// version 903 doesn't need init here
#if CINDER_VERSION == 902
		ImGui::connectWindow(getWindow());
		ImGui::initialize();
#endif

#pragma region style
		ImGuiStyle& style = ImGui::GetStyle();
		// seed the user-adjustable UI scale from the real display content scale; from here on it's
		// a manual slider (see "UI Scale" next to the Mic button) so it can be fine-tuned independently
		mVDUniforms->setUniformValue(mVDUniforms->IUISCALE, ci::app::getWindow()->getContentScale());
		float styleScale = mVDUniforms->getUniformValue(mVDUniforms->IUISCALE);
		// our theme variables
		style.WindowRounding = 8 * styleScale;
		style.WindowPadding = ImVec2(3 * styleScale, 3 * styleScale);
		style.FramePadding = ImVec2(2 * styleScale, 2 * styleScale);
		style.FrameRounding = 6 * styleScale;
		style.ItemSpacing = ImVec2(3 * styleScale, 3 * styleScale);
		style.ItemInnerSpacing = ImVec2(3 * styleScale, 3 * styleScale);
		//style.WindowMinSize = ImVec2(mVDParams->getPreviewFboWidth(), mVDParams->getPreviewFboHeight());
		style.WindowMinSize = ImVec2(mVDParams->getUISmallPreviewW() * styleScale, mVDParams->getUISmallPreviewH() * styleScale);
		style.Alpha = 0.65f;

		style.Colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
		style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
		style.Colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.70f);
		style.Colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.10f, 0.90f);
		style.Colors[ImGuiCol_Border] = ImVec4(0.70f, 0.70f, 0.70f, 0.65f);
		style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		style.Colors[ImGuiCol_FrameBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.30f);
		style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.90f, 0.80f, 0.80f, 0.40f);
		style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.90f, 0.65f, 0.65f, 0.45f);
		style.Colors[ImGuiCol_TitleBg] = ImVec4(0.27f, 0.27f, 0.54f, 0.83f);
		style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.40f, 0.40f, 0.80f, 0.20f);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.32f, 0.32f, 0.63f, 0.87f);
		style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.40f, 0.40f, 0.55f, 0.80f);
		style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.20f, 0.25f, 0.30f, 0.60f);
		style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.40f, 0.40f, 0.80f, 0.30f);
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.80f, 0.40f);
		style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.80f, 0.50f, 0.50f, 0.40f);
		style.Colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.90f, 0.90f, 0.50f);
		style.Colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
		style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.80f, 0.50f, 0.50f, 1.00f);
		style.Colors[ImGuiCol_Button] = ImVec4(0.67f, 0.40f, 0.40f, 0.60f);
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.67f, 0.40f, 0.40f, 1.00f);
		style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.80f, 0.50f, 0.50f, 1.00f);
		style.Colors[ImGuiCol_Header] = ImVec4(0.40f, 0.40f, 0.90f, 0.45f);
		style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.45f, 0.45f, 0.90f, 0.80f);
		style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.53f, 0.53f, 0.87f, 0.80f);
		style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
		style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.60f);
		style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.90f);
		style.Colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
		// style.Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

#pragma endregion style
	}
#pragma region menu
	/*if (ImGui::BeginMainMenuBar()) {

		if (ImGui::BeginMenu("Options"))
		{
			ImGui::DragFloat("Global Alpha", &style.Alpha, 0.005f, 0.20f, 1.0f, "%.2f");
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
	*/
#pragma endregion menu
	// right panel
	float uiScale = mVDUniforms->getUniformValue(mVDUniforms->IUISCALE);
	ImGui::SetNextWindowSize(ImVec2(300.0f * uiScale, mVDParams->getUILargeH() * 2.2f * uiScale), ImGuiCond_Once);
	ImGui::SetNextWindowPos(ImVec2(mVDParams->getUIXPosCol3() * uiScale, mVDParams->getUIYPosRow1() * uiScale), ImGuiCond_Once);

	ImGui::Begin(" Messages", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse);
	{
		if (ImGui::Button("Clear")) {
			mVDSettings->setMsg("");
			mVDSettings->setErrorMsg("");
			// TODO mVDSettings->mMidiMsg = ""; mVDSession->setmi("");

			mVDSettings->mSocketIOMsg = "";
			mVDSession->setOSCMsg("");
			//mVDSettings->mShaderMsg = "";
		}

		ImGui::TextColored(ImColor(200, 200, 0), "Msg %s", mVDSettings->getMsg().c_str());
		int ec = mVDSession->getErrorCode();
		if (ec > 0) {
			switch (ec)
			{
			case 1:
				ImGui::TextColored(ImColor(255, 0, 0), "Set Uniform max=0");
				break;
			case 2:
				ImGui::TextColored(ImColor(255, 0, 0), "Set Uniform value out of bounds");
				break;
			case 3:
				ImGui::TextColored(ImColor(255, 0, 0), "Uniform value not set");
				break;
			default:
				ImGui::TextColored(ImColor(0, 255, 0), "Code %d", ec);
				break;
			}
		}
		//ImGui::TextWrapped("Shader: %s", mVDSettings->mShaderMsg.c_str());
		ImGui::TextWrapped("Midi %s", mVDSession->getMidiMsg().c_str());

		// websockets
		// ImGui::TextWrapped("WS");
		if (ImGui::Button("WS"))
		{
			mVDSession->wsConnect();
		}
		ImGui::SameLine();
		if (mVDSession->isWSClientConnected()) 
		{
			if (ImGui::Button("Ping")) { mVDSession->wsPing(); }
		}
		
		ImGui::SameLine();		
		ImGui::Text("%s", mVDSession->getWSMsg().c_str());

		// OSC
		ImGui::TextWrapped("OSC %s", mVDSession->getOSCMsg().c_str());
		ImGui::TextColored(ImColor(255, 0, 0), "Last error %s", mVDSettings->getErrorMsg().c_str());
	}
	ImGui::End();
	// Center panel
	ImGui::SetNextWindowSize(ImVec2(748.0f * uiScale, mVDParams->getUILargePreviewH() * uiScale), ImGuiCond_Once);
	ImGui::SetNextWindowPos(ImVec2(mVDParams->getUIXPosCol1() * uiScale, mVDParams->getUIYPosRow1() * uiScale), ImGuiCond_Once);

	sprintf_s(buf, " Fps %c %d ###fps", "|/-\\"[(int)(ImGui::GetTime() / 0.25f) & 3], fps);
	ImGui::Begin(buf, NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse);
	{
		// line 1
		ImGui::PushItemWidth(mVDParams->getPreviewFboWidth() * uiScale);
		ImGui::Image(mVDSession->buildPostFboTexture(), ivec2(mVDParams->getPreviewFboWidth() * uiScale, mVDParams->getPreviewFboHeight() * uiScale));
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Post");
		ImGui::SameLine();
		ImGui::Image(mVDSession->buildRenderedWarpFboTexture(), ivec2(mVDParams->getPreviewFboWidth() * uiScale, mVDParams->getPreviewFboHeight() * uiScale));
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Warp");
		ImGui::SameLine();
		ImGui::Image(mVDSession->buildFxFboTexture(), ivec2(mVDParams->getPreviewFboWidth() * uiScale, mVDParams->getPreviewFboHeight() * uiScale));
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Fx");

		ImGui::SameLine();

		// tempo
		static ImVector<float> tempoValues; if (tempoValues.empty()) { tempoValues.resize(40); memset(&tempoValues.front(), 0, tempoValues.size() * sizeof(float)); }
		static int tempoValues_offset = 0;
		static float tRefresh_time = -1.0f + 1.0f / 20.0f;
		if (ImGui::GetTime() > tRefresh_time)
		{
			tRefresh_time = ImGui::GetTime();
			tempoValues[tempoValues_offset] = mVDSession->getUniformValue(mVDUniforms->ITEMPOTIME);
			tempoValues_offset = (tempoValues_offset + 1) % tempoValues.size();
		}

		if (mVDSession->getUniformValue(mVDUniforms->ITEMPOTIME) < 0.1) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
		ImGui::PlotLines("T", &tempoValues.front(), (int)tempoValues.size(), tempoValues_offset, toString(int(mVDSession->getUniformValue(mVDUniforms->IBPM))).c_str(), 0.0f, 0.6f, ImVec2(0, 30));
		if (mVDSession->getUniformValue(mVDUniforms->ITEMPOTIME) < 0.1) ImGui::PopStyleColor();
		ImGui::SameLine();


		// fps
		static ImVector<float> fpsValues; if (fpsValues.empty()) { fpsValues.resize(100); memset(&fpsValues.front(), 0, fpsValues.size() * sizeof(float)); }
		static int fpsValues_offset = 0;
		static float refresh_time = -1.0f + 1.0f / 6.0f;
		if (ImGui::GetTime() > refresh_time)
		{
			refresh_time = ImGui::GetTime();
			fpsValues[fpsValues_offset] = fps;
			fpsValues_offset = (fpsValues_offset + 1) % fpsValues.size();
		}
		if (fps < 24.0) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
		sprintf_s(buf, "%d", fps);
		ImGui::PlotLines("F", &fpsValues.front(), (int)fpsValues.size(), fpsValues_offset, buf, 0.0f, 100.0f, ImVec2(0, 30));
		if (fps < 24.0) ImGui::PopStyleColor();
		// audio spectrum, max volume and gain (AX): in the "audio" texture pane (VDUITextures)

		int hue = 0;
		// Done in UIAnimation
		// ImGui::SameLine();
		//(mVDSession->getUseLineIn()) ? ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 16.0f, 1.0f, 0.5f)) : ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1.0f, 0.1f, 0.1f));
		//// audio preferred
		//if (ImGui::Button("Mic")) {
		//	mVDSession->toggleUseLineIn();
		//}
		//ImGui::PopStyleColor(1);

		// manual UI scale override (see IUISCALE) - lets the panel/font/spacing scale be
		// fine-tuned independently of the display's auto-detected content scale
		// ImGui::SameLine();
		ImGui::PushItemWidth(80.0f * uiScale);
		float uiScaleCtrl = uiScale;
		if (ImGui::SliderFloat("UIX", &uiScaleCtrl, 0.5f, 4.0f)) {
			mVDUniforms->setUniformValue(mVDUniforms->IUISCALE, uiScaleCtrl);
		}
		ImGui::PopItemWidth();
		#if defined( _DEBUG )
			// file logging is opt-in at runtime (see VDLog::setFileLoggingEnabled()) rather than
			// always-on for the whole debug session - only ever meaningful in a debug build at all,
			// so the button itself doesn't even exist in release. Toggling it on starts three fresh,
			// session-timestamped files in <repos>/logs/ (everything, warnings+, errors+) instead of
			// the previous single file that rotated at midnight and could mix multiple runs together.
			ImGui::SameLine();
			bool fileLogging = VDLog::isFileLoggingEnabled();
			if (ImGui::Checkbox("FileLog", &fileLogging)) {
				VDLog::setFileLoggingEnabled(fileLogging);
			}
			// debug
			ImGui::SameLine();
			ctrl = mVDUniforms->IDEBUG;
			(getFloatValue(ctrl)) ? ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 16.0f, 1.0f, 0.5f)) : ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1.0f, 0.1f, 0.1f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue / 16.0f, 0.7f, 0.7f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue / 16.0f, 0.8f, 0.8f));
			if (ImGui::Button("Dbg")) {
				toggleValue(ctrl);
			}
			ImGui::PopStyleColor(3);
		#endif
		hue++;
		ImGui::SameLine();


		// "Warp++" (create a new warp) moved into the Warps panel itself (VDUIWarps.cpp) - this
		// button only shows/hides that panel
		ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 16.0f, 1.0f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue / 16.0f, 0.7f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue / 16.0f, 0.8f, 0.8f));
		if (ImGui::Button("Warps")) {
			mToggleShowWarps();
		}
		ImGui::PopStyleColor(3);
		hue++;
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 16.0f, 1.0f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue / 16.0f, 0.7f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue / 16.0f, 0.8f, 0.8f));
		if (ImGui::Button("Fbos")) {
			mToggleShowFbos();
		}
		ImGui::PopStyleColor(3);
		hue++;
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 16.0f, 1.0f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue / 16.0f, 0.7f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue / 16.0f, 0.8f, 0.8f));
		if (ImGui::Button("Tex")) {
			mToggleShowTextures();
		}
		ImGui::PopStyleColor(3);
		hue++;
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 16.0f, 1.0f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue / 16.0f, 0.7f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue / 16.0f, 0.8f, 0.8f));
		if (ImGui::Button("Blend")) {
			mToggleShowBlend();
		}
		ImGui::PopStyleColor(3);
		hue++;
		ImGui::SameLine();

#if defined( CINDER_MSW )
		ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 16.0f, 1.0f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue / 16.0f, 0.7f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue / 16.0f, 0.8f, 0.8f));
		if (ImGui::Button("Web")) {
			mToggleShowHtmlPage();
		}
		ImGui::PopStyleColor(3);
		hue++;
		ImGui::SameLine();
#endif

		ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 16.0f, 1.0f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue / 16.0f, 0.7f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue / 16.0f, 0.8f, 0.8f));
		if (ImGui::Button("Code")) {
			mToggleShowCodeView();
		}
		ImGui::PopStyleColor(3);
		hue++;
		ImGui::SameLine();

		// projector output window: green while it's open
		if (mOutputWindow->isOpen()) {
			ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0, 140, 0, 255));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor(0, 180, 0, 255));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor(0, 210, 0, 255));
		}
		else {
			ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 16.0f, 1.0f, 0.5f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue / 16.0f, 0.7f, 0.7f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue / 16.0f, 0.8f, 0.8f));
		}
		if (ImGui::Button("Output")) {
			mShowOutput = !mShowOutput;
		}
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Projector output window (2nd display or several projectors)");
		ImGui::PopStyleColor(3);
		hue++;
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 16.0f, 1.0f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue / 16.0f, 0.7f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue / 16.0f, 0.8f, 0.8f));
		if (ImGui::Button("Folders")) {
			mToggleShowFolders();
		}
		ImGui::PopStyleColor(3);
		hue++;
		ImGui::SameLine();

		// Audio/Midi/Tempo/OSC/Websocket used to be always-visible CollapsingHeaders inside the
		// "Animation" window, making it very tall - each is now its own toggleable window (see
		// VDUIAnimation.cpp), shown/hidden from here the same way Warps/Fbos/Tex/Blend/Folders
		// already are
		ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 16.0f, 1.0f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue / 16.0f, 0.7f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue / 16.0f, 0.8f, 0.8f));
		if (ImGui::Button("Audio")) {
			mUIAnimation->toggleShowAudio();
		}
		ImGui::PopStyleColor(3);
		hue++;
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 16.0f, 1.0f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue / 16.0f, 0.7f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue / 16.0f, 0.8f, 0.8f));
		if (ImGui::Button("Midi##toggle")) {
			mUIAnimation->toggleShowMidi();
		}
		ImGui::PopStyleColor(3);
		hue++;
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 16.0f, 1.0f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue / 16.0f, 0.7f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue / 16.0f, 0.8f, 0.8f));
		if (ImGui::Button("Tempo")) {
			mUIAnimation->toggleShowTempo();
		}
		ImGui::PopStyleColor(3);
		hue++;
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 16.0f, 1.0f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue / 16.0f, 0.7f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue / 16.0f, 0.8f, 0.8f));
		if (ImGui::Button("OSC##toggle")) {
			mUIAnimation->toggleShowOSC();
		}
		ImGui::PopStyleColor(3);
		hue++;
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 16.0f, 1.0f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue / 16.0f, 0.7f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue / 16.0f, 0.8f, 0.8f));
		if (ImGui::Button("Websocket##toggle")) {
			mUIAnimation->toggleShowWebsocket();
		}
		ImGui::PopStyleColor(3);
		hue++;
		
		/*
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.9f, 0.7f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.9f, 0.8f, 0.8f));

		if (ImGui::Button("Auto Layout")) {
			mVDSession->toggleAutoLayout();
		}
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Auto Layout for render window");

		// Auto Layout for render window
		if (mVDSession->isAutoLayout()) {
			ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.9f, 1.0f, 0.5f));
		}
		else {
			ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.1f, 0.1f));
			// render window width
			static int rw = mVDSettings->mRenderWidth;
			if (ImGui::SliderInt("rdr w", &rw, 640, 4080))
			{
				//mVDSession->setRenderWidth(rw);
				mVDSettings->mRenderWidth = rw;
			}
			ImGui::SameLine();
			// render window height
			static int rh = mVDSettings->mRenderHeight;
			if (ImGui::SliderInt("rdr h", &rh, 480, 1280))
			{
				//mVDSession->setRenderHeight(rh);
				mVDSettings->mRenderHeight = rh;
			}
		}
		ImGui::PopStyleColor(3);
		ImGui::SameLine();
*/


		/*
		ImGui::SameLine();
		ctrl = mVDUniforms->IFLIPPOSTH;
		(getFloatValue(ctrl)) ? ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 16.0f, 1.0f, 0.5f)) : ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1.0f, 0.1f, 0.1f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue / 16.0f, 0.7f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue / 16.0f, 0.8f, 0.8f));
		if (ImGui::Button("FlipPostH")) {
			toggleValue(ctrl);
		}
		ImGui::PopStyleColor(3);

		ImGui::Text(" Fp %dx%d F %dx%d", mVDParams->getPreviewFboWidth(), mVDParams->getPreviewFboHeight(), mVDParams->getFboWidth(), mVDParams->getFboHeight());
		ImGui::SameLine();*/
		
		// line 3
		for (unsigned int m = 0; m < mVDSession->getModesCount(); m++) {
			if (m > 0) ImGui::SameLine();
			sprintf_s(buf, "%s##mode", mVDSession->getModeName(m).c_str());
			if (mVDSession->getUniformValue(mVDUniforms->IDISPLAYMODE) == m) {
				ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(m / 16.0f, 1.0f, 0.5f));
			}
			else {
				ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(m / 16.0f, 0.1f, 0.1f));
			}
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(m / 16.0f, 0.7f, 0.7f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(m / 16.0f, 0.8f, 0.8f));
			if (ImGui::Button(buf)) mVDSession->setUniformValue(mVDUniforms->IDISPLAYMODE, m);
			sprintf_s(buf, "Set mode to %s", mVDSession->getModeName(m).c_str());
			if (ImGui::IsItemHovered()) ImGui::SetTooltip(buf);
			ImGui::PopStyleColor(3);
		}
		ImGui::SameLine();
		ctrl = mVDUniforms->IGLITCH;
		(getFloatValue(ctrl)) ? ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 16.0f, 1.0f, 0.5f)) : ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1.0f, 0.1f, 0.1f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue / 16.0f, 0.7f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue / 16.0f, 0.8f, 0.8f));
		if (ImGui::Button("Glitch")) {
			toggleValue(ctrl);
		}
		ImGui::PopStyleColor(3);
		hue++;
		ImGui::SameLine();

		ctrl = mVDUniforms->IGREYSCALE;
		(getFloatValue(ctrl)) ? ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 16.0f, 1.0f, 0.5f)) : ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1.0f, 0.1f, 0.1f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue / 16.0f, 0.7f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue / 16.0f, 0.8f, 0.8f));
		if (ImGui::Button("Greyscale")) {
			toggleValue(ctrl);
		}
		ImGui::PopStyleColor(3);
		hue++;
		ImGui::SameLine();

		ctrl = mVDUniforms->ITOGGLE;
		(getFloatValue(ctrl)) ? ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 16.0f, 1.0f, 0.5f)) : ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1.0f, 0.1f, 0.1f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue / 16.0f, 0.7f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue / 16.0f, 0.8f, 0.8f));
		if (ImGui::Button("Toggle")) {
			toggleValue(ctrl);
		}
		ImGui::PopStyleColor(3);
		hue++;
		ImGui::SameLine();

		ctrl = mVDUniforms->IINVERT;
		(getFloatValue(ctrl)) ? ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 16.0f, 1.0f, 0.5f)) : ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1.0f, 0.1f, 0.1f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue / 16.0f, 0.7f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue / 16.0f, 0.8f, 0.8f));
		if (ImGui::Button("Invert")) {
			toggleValue(ctrl);
		}
		ImGui::PopStyleColor(3);
		hue++;
		ImGui::SameLine();


		// iVignette
		ctrl = mVDUniforms->IVIGNETTE;
		(getFloatValue(ctrl)) ? ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 16.0f, 1.0f, 0.5f)) : ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1.0f, 0.1f, 0.1f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue / 16.0f, 0.7f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue / 16.0f, 0.8f, 0.8f));
		if (ImGui::Button("Vignette")) {
			toggleValue(ctrl);
		}
		ImGui::PopStyleColor(3);
		hue++;
		ImGui::SameLine();

		// iClear
		ctrl = mVDUniforms->ICLEAR;
		(getFloatValue(ctrl)) ? ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 16.0f, 1.0f, 0.5f)) : ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1.0f, 0.1f, 0.1f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue / 16.0f, 0.7f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue / 16.0f, 0.8f, 0.8f));
		if (ImGui::Button("Clear")) {
			toggleValue(ctrl);
		}
		ImGui::PopStyleColor(3);
		hue++;
		ImGui::SameLine();
		/*
		// iflipv
		ctrl = mVDUniforms->IFLIPV;
		(getFloatValue(ctrl)) ? ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 16.0f, 1.0f, 0.5f)) : ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1.0f, 0.1f, 0.1f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue / 16.0f, 0.7f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue / 16.0f, 0.8f, 0.8f));
		if (ImGui::Button("FlipV")) {
			toggleValue(ctrl);
		}
		ImGui::PopStyleColor(3);
		hue++;
		ImGui::SameLine();

		// ifliph
		ctrl = mVDUniforms->IFLIPH;
		(getFloatValue(ctrl)) ? ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 16.0f, 1.0f, 0.5f)) : ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1.0f, 0.1f, 0.1f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue / 16.0f, 0.7f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue / 16.0f, 0.8f, 0.8f));
		if (ImGui::Button("FlipH")) {
			toggleValue(ctrl);
		}
		ImGui::PopStyleColor(3);
		hue++;
		ImGui::SameLine();
		*/
		// post flip		
		ctrl = mVDUniforms->IFLIPPOSTV;
		(getFloatValue(ctrl)) ? ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue / 16.0f, 1.0f, 0.5f)) : ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1.0f, 0.1f, 0.1f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue / 16.0f, 0.7f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue / 16.0f, 0.8f, 0.8f));
		if (ImGui::Button("FlipPostV")) {
			toggleValue(ctrl);
		}
		ImGui::PopStyleColor(3);
		hue++;
		//ImGui::SameLine();
		// midi preferred - Midi Learn now lives in the "Midi" panel itself (VDUIAnimation.cpp),
		// not here; this button only starts up the MIDI subsystem
		/*  if( ! mVDSession->isMidiSetup() ) {
			ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1.0f, 0.1f, 0.1f));
			sprintf_s(buf, "Midi");
			if (ImGui::Button(buf)) mVDSession->setupMidi();
			ImGui::PopStyleColor(1);
		} */
		ImGui::Text(" Main %dx%d", mVDSettings->mMainWindowWidth, mVDSettings->mMainWindowHeight);
		ImGui::SameLine();
		// windows
		ImGui::Text(" Render window %dx%d", mVDSettings->mRenderWidth, mVDSettings->mRenderHeight);
		ImGui::SameLine();
		// mouse
		ImGui::Text(" Clic %d", ImGui::GetIO().MouseDown[0]);
		ImGui::SameLine();
		ImGui::Text(" Pos: %.1f,%.1f", ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y);

		// time
		/*smooth = mVDSession->getUniformValue(mVDUniforms->ISMOOTH);
		if (ImGui::SliderFloat("Smooth", &smooth, 0.001f, 0.02f)) {
			mVDSession->setUniformValue(mVDUniforms->ISMOOTH, smooth);
		}
		ImGui::SameLine();*/
		ImGui::Text(" Time %.2f", mVDSession->getUniformValue(mVDUniforms->ITIME));
		ImGui::SameLine();
		ImGui::Text(" TStart %.2f", mVDSession->getUniformValue(mVDUniforms->ISTART));
		ImGui::SameLine();
		ImGui::Text(" Factor %.2f", mVDSession->getUniformValue(mVDUniforms->ITIMEFACTOR));
		//ImGui::SameLine();
		//ImGui::Text(" Tempo Time %.2f", mVDSession->getUniformValue(mVDUniforms->ITEMPOTIME));
		ImGui::SameLine();
		ImGui::Text(" Delta %.2f", mVDSession->getUniformValue(mVDUniforms->IDELTATIME));
		// LiveOSC Obsolete ImGui::Text("Trk %s %.2f", mVDSettings->mTrackName.c_str(), mVDSettings->liveMeter);
		//ImGui::SameLine();
		//			ImGui::Checkbox("Playing", &mVDSettings->mIsPlaying);
		//ImGui::Text(" Tempo %.2f", mVDSession->getUniformValue(mVDUniforms->IBPM));
		ImGui::SameLine();
		ImGui::Text(" Beat %.0f", mVDSession->getUniformValue(mVDUniforms->IBEAT));
		ImGui::SameLine();
		ImGui::Text(" Bar %.0f", mVDSession->getUniformValue(mVDUniforms->IBAR));
		ImGui::SameLine();
		ImGui::Text(" BStart %.0f", mVDSession->getUniformValue(mVDUniforms->IBARSTART));
		ImGui::SameLine();
		ImGui::Text(" Bb %.0f", mVDSession->getUniformValue(mVDUniforms->IBARBEAT));
		ImGui::SameLine();
		ImGui::Text(" %s (%.0f)", mVDSession->getTrackName().c_str(), mVDSession->getUniformValue(mVDUniforms->ITRACK));
		ImGui::SameLine();
		ImGui::Text(" BPM %.0f", mVDSession->getUniformValue(mVDUniforms->IBPM));

		//ImGui::SameLine();
		//sprintf_s(buf, "%s", mVDSession->getTrackName().c_str());
		//ImGui::Text("Trk %s %.2f", mVDSettings->mTrackName.c_str(), mVDSettings->liveMeter);
		//ImGui::Text(" ", mVDSession->getTrackName());

/*
		const float spacing = 4;
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, spacing));

		ImGui::PushID("fbomixes");
		for (unsigned int m = 0; m < mVDSession->getFboShaderListSize(); m++)
		{
			if (m > 0) ImGui::SameLine();
			ctrl = mVDUniforms->IWEIGHT0 + m;
			float iWeight = mVDSession->getUniformValue(ctrl);
			ImGui::PushID(m);

			ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor::HSV(m / 16.0f, 0.5f, 0.5f));
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, (ImVec4)ImColor::HSV(m / 16.0f, 0.6f, 0.5f));
			ImGui::PushStyleColor(ImGuiCol_FrameBgActive, (ImVec4)ImColor::HSV(m / 16.0f, 0.7f, 0.5f));
			ImGui::PushStyleColor(ImGuiCol_SliderGrab, (ImVec4)ImColor::HSV(m / 16.0f, 0.9f, 0.9f));
			ImGui::Image(mVDSession->buildFboRenderedTexture(m), ivec2(mVDParams->getPreviewFboWidth() * uiScale, mVDParams->getPreviewFboHeight() * uiScale));

			ImGui::SameLine();
			if (ImGui::VSliderFloat("##v", ImVec2(28 * uiScale, 80 * uiScale), &iWeight, 0.0f, 1.0f, ""))
			{
				setFloatValue(ctrl, iWeight);
			};
			if (ImGui::IsItemActive() || ImGui::IsItemHovered())
				ImGui::SetTooltip("%.3f", iWeight);
			ImGui::PopStyleColor(4);
			ImGui::PopID();
		}
		ImGui::PopID();
		ImGui::PopStyleVar();*/

		ImGui::PopItemWidth();
	}
	ImGui::End();

	mUIAnimation->Run("Animation");
	/*switch (currentWindowRow1) {
	case 0:

		break;
	case 1:
		// Animation
		mUIAnimation->Run("Animation");
		break;
	case 2:
		// Mouse
		mUIMouse->Run("Mouse");
		break;
	case 3:
		// Render
		mUIRender->Run("Render");
		// Blend
		mUIBlend->Run("Blend");
		break;
	case 4:
		// Audio
		mUIAudio->Run("Audio");
		break;
	case 5:
		// Color
		mUIColor->Run("Color");
		break;
	case 6:
		// Osc
		mUIOsc->Run("Osc");
		break;
	case 7:
		// SocketIO
		mUISocketIO->Run("SocketIO");
		break;
	case 8:
		// Midi
		mUIMidi->Run("Midi");
		break;
	}
	mVDSession->blendRenderEnable(currentWindowRow1 == 3);*/
	// Warps
	if (mShowWarps) {
		mUIWarps->Run("Warps");
	}
	// Fbos
	if (mShowFbos) {
		mUIFbos->Run("Fbos");
	}
	// textures
	if (mShowTextures) {
		mUITextures->Run("Textures");
	}
	// Folders
	if (mShowFolders) {
		mUIFolders->Run("Folders");
	}
#if defined( CINDER_MSW )
	// HtmlPage (WebView2)
	if (mShowHtmlPage) {
		mUIHtmlPage->Run("WebApp");
	}
#endif
	// live code view
	if (mShowCodeView) {
		runCodeView();
	}
	// projector output window
	if (mShowOutput) {
		runOutput();
	}
	// an image/video drop no fbo pane claimed (Fbos panel hidden, or dropped elsewhere):
	// image -> shared texture pool, video -> new fbo, paused
	mVDSession->flushPendingTextureDrop();
	// blendmodes
	/*if (mShowBlend) {
		mUIBlend->Run("BlendModes");
	}*/

	// Shaders
	//mUIShaders->Run("Shaders");


}

void VDUI::runCodeView() {
	VDCodeViewRef codeView = mVDSession->getCodeView();
	if (!codeView) return;
	if (ImGui::Begin("Code view", &mShowCodeView)) {
		ImGui::TextWrapped("WebApp GLSL and Strudel editor text, sent as the transparent Spout sender \"VDCode\".");
		ImGui::Text("GLSL: %s, %d lines", codeView->isActive(VDCodeView::LANG_GLSL) ? "editor open" : "waiting", codeView->getLineCount(VDCodeView::LANG_GLSL));
		ImGui::Text("Strudel: %s, %d lines", codeView->isActive(VDCodeView::LANG_STRUDEL) ? "editor open" : "waiting", codeView->getLineCount(VDCodeView::LANG_STRUDEL));
		int layout = codeView->getLayout();
		const char* layouts[] = { "Side by side", "Stacked" };
		if (ImGui::Combo("Both shown", &layout, layouts, IM_ARRAYSIZE(layouts))) codeView->setLayout(layout);
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("When both editors are open: GLSL left / Strudel right, or GLSL top / Strudel bottom");
		float fontSize = codeView->getFontSize();
		if (ImGui::SliderFloat("Font size", &fontSize, 12.0f, 96.0f, "%.0f")) codeView->setFontSize(fontSize);
		float backgroundAlpha = codeView->getBackgroundAlpha();
		if (ImGui::SliderFloat("Background", &backgroundAlpha, 0.0f, 1.0f)) codeView->setBackgroundAlpha(backgroundAlpha);
		bool shadow = codeView->getShadow();
		if (ImGui::Checkbox("Shadow", &shadow)) codeView->setShadow(shadow);
		ImGui::SameLine();
		bool lineNumbers = codeView->getLineNumbers();
		if (ImGui::Checkbox("Line numbers", &lineNumbers)) codeView->setLineNumbers(lineNumbers);
		ImGui::SameLine();
		bool premultiplied = codeView->getPremultiplied();
		if (ImGui::Checkbox("Premultiplied", &premultiplied)) codeView->setPremultiplied(premultiplied);
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle if glyph edges show dark or bright fringes in Resolume");
		int lineStyle = codeView->getCurrentLineStyle();
		const char* lineStyles[] = { "Line fill", "Line border", "Cursor only", "Line number" };
		if (ImGui::Combo("Current line style", &lineStyle, lineStyles, IM_ARRAYSIZE(lineStyles))) codeView->setCurrentLineStyle(lineStyle);
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Border and Line number use at least 50%% opacity of the colour below.\nLine number falls back to Line fill when line numbers are hidden.");
		ci::ColorA lineColor = codeView->getCurrentLineColor();
		if (ImGui::ColorEdit4("Current line", &lineColor.r, ImGuiColorEditFlags_AlphaBar)) codeView->setCurrentLineColor(lineColor);

		// placement on the projector output window (its on/off and opacity are in the Output panel)
		ImGui::Separator();
		ImGui::TextUnformatted("Output window overlay");
		bool codeOverlay = mOutputWindow->getCodeOverlay();
		if (ImGui::Checkbox("Show on output", &codeOverlay)) mOutputWindow->setCodeOverlay(codeOverlay);
		ImGui::SameLine();
		ci::vec2 overlayPos = mOutputWindow->getCodeOverlayPosition();
		if (ImGui::Button("Left")) { overlayPos.x = 0.0f; mOutputWindow->setCodeOverlayPosition(overlayPos); mOutputWindow->saveSettings(); }
		ImGui::SameLine();
		if (ImGui::Button("Center")) { overlayPos.x = 0.5f; mOutputWindow->setCodeOverlayPosition(overlayPos); mOutputWindow->saveSettings(); }
		ImGui::SameLine();
		if (ImGui::Button("Right")) { overlayPos.x = 1.0f; mOutputWindow->setCodeOverlayPosition(overlayPos); mOutputWindow->saveSettings(); }
		if (ImGui::SliderFloat("Horizontal##codeoverlay", &overlayPos.x, 0.0f, 1.0f)) mOutputWindow->setCodeOverlayPosition(overlayPos);
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("0 = left edge, 0.5 = centred, 1 = right edge");
		if (ImGui::IsItemDeactivatedAfterEdit()) mOutputWindow->saveSettings();
		if (ImGui::SliderFloat("Vertical##codeoverlay", &overlayPos.y, 0.0f, 1.0f)) mOutputWindow->setCodeOverlayPosition(overlayPos);
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("0 = top edge, 0.5 = centred, 1 = bottom edge");
		if (ImGui::IsItemDeactivatedAfterEdit()) mOutputWindow->saveSettings();
		float overlaySize = mOutputWindow->getCodeOverlaySize();
		if (ImGui::SliderFloat("Size##codeoverlay", &overlaySize, 0.1f, 1.0f)) mOutputWindow->setCodeOverlaySize(overlaySize);
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("1 = the output window's full height (aspect ratio kept)");
		if (ImGui::IsItemDeactivatedAfterEdit()) mOutputWindow->saveSettings();
		float overlayOpacity = mOutputWindow->getCodeOverlayOpacity();
		if (ImGui::SliderFloat("Opacity##codeviewoverlay", &overlayOpacity, 0.0f, 1.0f)) mOutputWindow->setCodeOverlayOpacity(overlayOpacity);
		if (ImGui::IsItemDeactivatedAfterEdit()) mOutputWindow->saveSettings();

		if (auto tex = codeView->getTexture()) {
			float w = ImGui::GetContentRegionAvail().x;
			ImGui::Image(tex, ImVec2(w, w * tex->getHeight() / (float)tex->getWidth()));
		}
	}
	ImGui::End();
}

void VDUI::runOutput() {
	if (ImGui::Begin("Output", &mShowOutput)) {
		bool open = mOutputWindow->isOpen();
		if (ImGui::Button(open ? "Close output window" : "Open output window")) {
			if (open) mOutputWindow->requestClose();
			else mOutputWindow->requestOpen();
		}
		ImGui::SameLine();
		ImGui::TextUnformatted(mOutputWindow->getStatus().c_str());

		ImGui::Separator();
		ImGui::TextUnformatted("Displays (several projectors: select them all, the window spans them)");
		for (size_t i = 0; i < mOutputWindow->getDisplayCount(); i++) {
			bool selected = mOutputWindow->isDisplaySelected(i);
			if (ImGui::Checkbox(mOutputWindow->getDisplayLabel(i).c_str(), &selected)) mOutputWindow->setDisplaySelected(i, selected);
		}
		ci::Area span = mOutputWindow->getSpanBounds();
		ImGui::Text("Output size: %dx%d", span.getWidth(), span.getHeight());
		if (mOutputWindow->hasSpanGaps()) {
			ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "Selected displays don't form one rectangle: the window also covers the gaps");
		}

		ImGui::Separator();
		int content = mOutputWindow->getContent();
		if (ImGui::RadioButton("Main view", content == VDOutputWindow::MAIN_VIEW)) mOutputWindow->setContent(VDOutputWindow::MAIN_VIEW);
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Same image as the main window (Post/Fx/Warp/fbo display mode), stretched");
		ImGui::SameLine();
		if (ImGui::RadioButton("Warps (projection mapping)", content == VDOutputWindow::WARPS)) mOutputWindow->setContent(VDOutputWindow::WARPS);
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Cinder-Warping meshes rendered at the output's own resolution.\nOne warp per projector for a multi-projector span; 'W' toggles edit mode,\ncontrol points can be dragged directly on the output window.");
		if (content == VDOutputWindow::WARPS) {
			ImGui::TextWrapped("Each warp shows its own input (Mix, Post, Fx or an fbo): set it in the Warps panel.");
		}

		ImGui::Separator();
		bool codeOverlay = mOutputWindow->getCodeOverlay();
		if (ImGui::Checkbox("Code overlay", &codeOverlay)) mOutputWindow->setCodeOverlay(codeOverlay);
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("The WebApp editor's code (same image as the \"VDCode\" Spout sender) over the output,\nposition, size and style it in the \"Code\" panel.");
		ImGui::SameLine();
		float codeOpacity = mOutputWindow->getCodeOverlayOpacity();
		if (ImGui::SliderFloat("Opacity##codeoverlay", &codeOpacity, 0.0f, 1.0f)) mOutputWindow->setCodeOverlayOpacity(codeOpacity);
		if (ImGui::IsItemDeactivatedAfterEdit()) mOutputWindow->saveSettings();

		ImGui::Separator();
		bool pace = mOutputWindow->getPaceByVsync();
		if (ImGui::Checkbox("Pace by projector vsync", &pace)) mOutputWindow->setPaceByVsync(pace);
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Recommended: vsync on the output only, off on this window,\nframe-rate timer disabled while the output is open");
		ImGui::SameLine();
		bool onTop = mOutputWindow->getAlwaysOnTop();
		if (ImGui::Checkbox("Always on top", &onTop)) mOutputWindow->setAlwaysOnTop(onTop);
		ImGui::TextWrapped("Spout to Resolume keeps running either way. Esc on the output window closes it.");
	}
	ImGui::End();
}
