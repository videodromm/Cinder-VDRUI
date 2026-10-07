#pragma once

#if defined( _WIN32 ) && ! defined( WIN32_LEAN_AND_MEAN )
// must be defined before the first Windows header (pulled in by cinder/app/App.h) is seen,
// otherwise asio/websocketpp later in the translation unit hits "WinSock.h has already been included"
#define WIN32_LEAN_AND_MEAN
#endif
#include "cinder/app/App.h"

// UserInterface
#include "cinder/CinderImGui.h"
// Settings
#include "VDSettings.h"
// Uniforms
#include "VDUniforms.h"
// Session
#include "VDSessionFacade.h"
// UIFbos
#include "VDUIFbos.h"
// UITextures
#include "VDUITextures.h"
// UIBlend
//#include "VDUIBlend.h"
// Animation
#include "VDUIAnimation.h"
// Warps
#include "VDUIWarps.h"
// Folders
#include "VDUIFolders.h"
#if defined( CINDER_MSW )
// HtmlPage (WebView2, Windows-only)
#include "VDUIHtmlPage.h"
// files dragged from Explorer: hover position for highlighting, then the drop
#include "VDFileDropTarget.h"
// Spout senders from other apps, offered in the texture pool
#include "VDSpoutSources.h"
#endif
// projector output window
#include "VDOutputWindow.h"
// Params
#include "VDParams.h"
// Log
#include "VDLog.h"

using namespace ci;
using namespace ci::app;
using namespace std;

namespace videodromm
{
	// stores the pointer to the VDConsole instance
	typedef std::shared_ptr<class VDUI> VDUIRef;

	class VDUI
	{
	public:
		VDUI(VDSettingsRef aVDSettings, VDSessionFacadeRef aVDSessionFacade, VDUniformsRef aVDUniforms);
		static VDUIRef	create(VDSettingsRef aVDSettings, VDSessionFacadeRef aVDSessionFacade, VDUniformsRef aVDUniforms)
		{
			return shared_ptr<VDUI>(new VDUI(aVDSettings, aVDSessionFacade, aVDUniforms));
		}

		void Run(const char* title, unsigned int fps);
		void resize() {
			mIsResizing = true;
			// disconnect ui window and io events callbacks
			//ImGui::disconnectWindow(getWindow());
		}
		bool	isReady() { return !mIsResizing; };
		// optional projector output window, driven by the app (see VDOutputWindow.h)
		VDOutputWindowRef	getOutputWindow() { return mOutputWindow; }
	private:
		// Params
		VDParamsRef					mVDParams;
		// Settings
		VDSettingsRef				mVDSettings;
		// Session
		VDSessionFacadeRef			mVDSession;
		// Uniforms
		VDUniformsRef				mVDUniforms;
		// UIFbos
		VDUIFbosRef					mUIFbos;
		bool						showUIFbos;
		bool						mShowFbos;
		bool						mShowTextures;

		bool						mShowBlend;		
		// 		VDUIBlendRef				mUIBlend;

		// UITextures
		VDUITexturesRef				mUITextures;
		// UIAnimation
		VDUIAnimationRef			mUIAnimation;
		bool						showUIAnimation;
		
		// UIWarps
		VDUIWarpsRef				mUIWarps;
		bool						showUIWarps;
		bool						mShowWarps;

		// UIFolders
		VDUIFoldersRef				mUIFolders;
		bool						mShowFolders;

#if defined( CINDER_MSW )
		// UIHtmlPage (WebView2, Windows-only)
		VDUIHtmlPageRef				mUIHtmlPage;
		bool						mShowHtmlPage;
#endif

		// live code view (Spout "VDCode") settings + preview
		bool						mShowCodeView = false;
		void						runCodeView();

		// files dragged from Explorer (Windows): registered on the first Run(), see VDFileDropTarget.h
#if defined( CINDER_MSW )
		std::unique_ptr<VDFileDropTarget>	mFileDropTarget;
		bool						mFileDropTargetTried = false;
		VDSpoutSources				mSpoutSources;
#endif
		// Spout outputs + audio texture offered in the shared texture pool, every frame
		void						registerPoolSources();

		// projector output window settings
		VDOutputWindowRef			mOutputWindow;
		bool						mShowOutput = false;
		void						runOutput();

		// imgui
		char						buf[64];
		bool						mIsResizing;
		float						color[4];
		float						backcolor[4];
		float						multx;
		float						smooth;
		bool						mouseGlobal;
		int							ctrl;
		float						contour, iVAmount, iVFallOff, iWeight0, iWeight1, iWeight2, iWeight3, iWeight4, iWeight5, iWeight6, iWeight7;

		void toggleValue(unsigned int aCtrl) {
			mVDSession->toggleValue(aCtrl);
		}
		void mToggleShowWarps() {
			mShowWarps = !mShowWarps;
		}
		void mToggleShowFbos() {
			mShowFbos = !mShowFbos;
		}
		void mToggleShowTextures() {
			mShowTextures = !mShowTextures;
		}
		void mToggleShowBlend() {
			mShowBlend = !mShowBlend;
		}
		void mToggleShowFolders() {
			mShowFolders = !mShowFolders;
		}
		void mToggleShowCodeView() {
			mShowCodeView = !mShowCodeView;
		}
#if defined( CINDER_MSW )
		void mToggleShowHtmlPage() {
			mShowHtmlPage = !mShowHtmlPage;
			// a hidden native child window won't disappear on its own just because
			// Run() stops being called - unlike every other, pure-ImGui panel here
			mUIHtmlPage->setVisible(mShowHtmlPage);
		}
#endif
		/*void setFloatValue(unsigned int aCtrl, float aValue) {
			mVDSession->setUniformValue(aCtrl, aValue);
		}*/
		float getMinUniformValue(unsigned int aIndex) {
			return mVDSession->getMinUniformValue(aIndex);
		}
		float getMaxUniformValue(unsigned int aIndex) {
			return mVDSession->getMaxUniformValue(aIndex);
		}
		float							getFloatValue(unsigned int aCtrl) {
			return mVDSession->getUniformValue(aCtrl);
		};
	};
}