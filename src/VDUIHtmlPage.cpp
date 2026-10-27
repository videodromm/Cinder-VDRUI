// Windows only (WebView2): TinderBox adds every src/*.cpp of the block to Mac projects too,
// so the file compiles to nothing there. VDUI only uses it under CINDER_MSW.
#include "cinder/Cinder.h"
#include "VDAccessToken.h"
#if defined( CINDER_MSW )
#include "VDUIHtmlPage.h"

using namespace videodromm;

namespace {
	std::wstring widen(const std::string& s) {
		if (s.empty()) return std::wstring();
		int len = ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
		std::wstring result(len, L'\0');
		::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &result[0], len);
		return result;
	}
}

VDUIHtmlPage::VDUIHtmlPage(VDSessionFacadeRef aVDSession) {
	mVDSession = aVDSession;
	mWebViewPanel = std::make_shared<CinderWebView2Panel>();
	mInitialized = false;
}

VDUIHtmlPage::~VDUIHtmlPage() {
}

void VDUIHtmlPage::setVisible(bool visible) {
	mWebViewPanel->setVisible(visible);
}

void VDUIHtmlPage::Run(const char* title) {
	if (!mInitialized) {
		// lazy: only actually spins up the WebView2 environment/controller once the
		// panel is first shown, not at app startup
		HWND parentHwnd = (HWND)ci::app::getWindow()->getNative();
		// "host=cinder": the WebApp then draws its shader previews once a second, the GPU is Cinder's
		// (WebApp/src/lib/host.ts); before any #fragment
		std::string url = mVDSession->getWebAppUrl();
		if (url.find("host=cinder") == std::string::npos) {
			const size_t hash = url.find('#');
			std::string fragment = hash == std::string::npos ? "" : url.substr(hash);
			if (hash != std::string::npos) url = url.substr(0, hash);
			url += (url.find('?') == std::string::npos ? "?" : "&") + std::string("host=cinder") + fragment;
		}
		// the server's access token, when set: the WebApp stores it and removes it from its address
		mWebViewPanel->initialize(parentHwnd, widen(access::withToken(url)));
		mInitialized = true;
	}

	ImGui::SetNextWindowSize(ImVec2(900, 650), ImGuiCond_Once);
	bool isOpen = ImGui::Begin(title, NULL, ImGuiWindowFlags_NoSavedSettings);
	// keep the title bar on screen: the native WebView2 below it takes every click, so once the
	// title bar was dragged above the top (or past a side) the pane couldn't be moved anymore
	{
		const ImVec2 pos = ImGui::GetWindowPos();
		const ImVec2 display = ImGui::GetIO().DisplaySize;
		const float grab = 80.0f; // title bar width kept inside, enough to grab
		ImVec2 clamped(std::clamp(pos.x, grab - ImGui::GetWindowWidth(), std::max(0.0f, display.x - grab)),
			std::clamp(pos.y, 0.0f, std::max(0.0f, display.y - ImGui::GetFrameHeight())));
		if (clamped.x != pos.x || clamped.y != pos.y) ImGui::SetWindowPos(clamped);
	}
	if (isOpen) {
		if (mWebViewPanel->hasError()) {
			// ASCII-only diagnostic text (see CinderWebView2Panel) - a naive wide-to-narrow
			// truncation is safe here, unlike arbitrary OS-provided text elsewhere in this codebase
			std::wstring werr = mWebViewPanel->getErrorMessage();
			std::string err;
			err.reserve(werr.size());
			for (wchar_t wc : werr) err.push_back((char)wc);
			ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "WebView2 error:");
			ImGui::TextWrapped("%s", err.c_str());
		}
		else if (!mWebViewPanel->isReady()) {
			ImGui::TextDisabled("Loading WebView2...");
		}
		// reserve the title bar for ImGui's own drag/resize/collapse chrome - only the
		// content area below it is covered by the native WebView2 child window. These
		// coordinates are already in this app's device-pixel client-area space (same
		// convention VDUIFbos.cpp/VDSession.cpp use for drag-and-drop hit-testing), so
		// no further ci::app::toPixels() conversion is needed here.
		ImVec2 contentMin = ImGui::GetCursorScreenPos();
		ImVec2 contentSize = ImGui::GetContentRegionAvail();
		RECT bounds;
		bounds.left = (LONG)contentMin.x;
		bounds.top = (LONG)contentMin.y;
		bounds.right = (LONG)(contentMin.x + contentSize.x);
		bounds.bottom = (LONG)(contentMin.y + contentSize.y);
		mWebViewPanel->setBounds(bounds);
		mWebViewPanel->setVisible(true);
	}
	else {
		// collapsed - Begin() didn't draw content, so nothing above refreshed the
		// bounds/visibility; explicitly hide instead of leaving it floating in place
		mWebViewPanel->setVisible(false);
	}
	ImGui::End();
}

#endif // CINDER_MSW
