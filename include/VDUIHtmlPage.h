#pragma once

#if defined( _WIN32 ) && ! defined( WIN32_LEAN_AND_MEAN )
// must be defined before the first Windows header (pulled in by cinder/app/App.h) is seen,
// otherwise asio/websocketpp later in the translation unit hits "WinSock.h has already been included"
#define WIN32_LEAN_AND_MEAN
#endif
#include "cinder/app/App.h"

// UserInterface
#include "cinder/CinderImGui.h"

// Session
#include "VDSessionFacade.h"

// WebView2 wrapper
#include "CinderWebView2Panel.h"

using namespace ci;
using namespace ci::app;
using namespace std;

namespace videodromm
{
	// Docks the Videodromm WebApp (a live, JS-driven React/Ionic control UI,
	// normally opened separately at http://localhost:5173) as its own ImGui
	// panel inside VDUI - lets a live-performance layout reclaim the screen
	// space a separate browser window would otherwise need. Not a GL-texture
	// composite: the WebView2 controller is a real native child window kept
	// positioned over wherever this ImGui window currently is (see Run()), so
	// it always renders on top of the GL/ImGui surface and can't be occluded
	// by another floating ImGui panel dragged over it.
	typedef std::shared_ptr<class VDUIHtmlPage> VDUIHtmlPageRef;

	class VDUIHtmlPage
	{
	public:
		VDUIHtmlPage(VDSessionFacadeRef aVDSession);
		static VDUIHtmlPageRef	create(VDSessionFacadeRef aVDSession)
		{
			return shared_ptr<VDUIHtmlPage>(new VDUIHtmlPage(aVDSession));
		}
		~VDUIHtmlPage();
		void	Run(const char* title);
		// hides the native child window immediately - needed because, unlike a
		// pure-ImGui panel, it won't disappear on its own just because Run()
		// stops being called once the panel is toggled off.
		void	setVisible(bool visible);
	private:
		// Session
		VDSessionFacadeRef			mVDSession;
		// WebView2
		CinderWebView2PanelRef		mWebViewPanel;
		bool						mInitialized;
	};
}
