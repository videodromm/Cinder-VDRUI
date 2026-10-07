#pragma once

#if defined( _WIN32 ) && ! defined( WIN32_LEAN_AND_MEAN )
// must be defined before the first Windows header (pulled in by cinder/app/App.h) is seen,
// otherwise asio/websocketpp later in the translation unit hits "WinSock.h has already been included"
#define WIN32_LEAN_AND_MEAN
#endif
#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/Display.h"

// Session
#include "VDSessionFacade.h"
// Params
#include "VDParams.h"

namespace videodromm
{
	typedef std::shared_ptr<class VDOutputWindow> VDOutputWindowRef;

	// Optional projector output: a borderless window spanning one or several displays (two
	// projectors side by side = a window twice as wide), in the same process and sharing
	// textures with the main window, so nothing is copied between apps. The main window
	// (UI + Spout to Resolume) is unchanged whether this is open or not.
	//
	// App wiring (see VDRUI's _TBOX_PREFIX_App.cpp):
	//  - update(): call update() last, after the session update (main GL context is current)
	//  - draw(): if isCurrent() { draw(); return; } first; in the main window call
	//    applyMainWindowPacing() and setMainViewTexture(tex) with what it shows
	//  - mouse events from the output window: isOutputWindow(), then toWarpSpace()
	//
	// The warp content is rendered into an output-sized Fbo from the main context, never from
	// the output window's context: Cinder's Batch VAOs (used by the warp meshes) belong to the
	// context that created them, while textures are shared (wglShareLists).
	class VDOutputWindow
	{
	public:
		enum Content { MAIN_VIEW = 0, WARPS = 1 };
		enum Composite { COMPOSITE_MIX = 0, COMPOSITE_POST = 1, COMPOSITE_FX = 2 };

		VDOutputWindow(VDSessionFacadeRef aVDSession);
		static VDOutputWindowRef create(VDSessionFacadeRef aVDSession) {
			return std::shared_ptr<VDOutputWindow>(new VDOutputWindow(aVDSession));
		}

		// deferred to update(): creating/closing a window switches the current GL context,
		// which must not happen mid-draw (e.g. from an ImGui button)
		void					requestOpen() { mRequest = REQUEST_OPEN; }
		void					requestClose() { mRequest = REQUEST_CLOSE; }
		bool					isOpen() const { return mWindow != nullptr; }

		void					update();
		bool					isCurrent() const;
		bool					isOutputWindow(const ci::app::WindowRef& aWindow) const { return mWindow && aWindow == mWindow; }
		void					draw();
		void					applyMainWindowPacing();
		void					setMainViewTexture(const ci::gl::TextureRef& aTexture) { mMainViewTexture = aTexture; }
		// what the main window shows and Spout sends (also offered in the texture pool)
		ci::gl::TextureRef		getMainViewTexture() const { return mMainViewTexture; }
		// output window mouse position -> the fbo-sized space the warps' editor works in
		ci::app::MouseEvent		toWarpSpace(const ci::app::MouseEvent& aEvent) const;

		// settings (persisted in output.json next to the other assets)
		size_t					getDisplayCount() const { return ci::Display::getDisplays().size(); }
		std::string				getDisplayLabel(size_t aIndex) const;
		bool					isDisplaySelected(size_t aIndex) const;
		void					setDisplaySelected(size_t aIndex, bool aSelected);
		int						getContent() const { return mContent; }
		void					setContent(int aContent) { mContent = aContent; save(); }
		int						getComposite() const { return mComposite; }
		void					setComposite(int aComposite) { mComposite = aComposite; save(); }
		bool					getPaceByVsync() const { return mPaceByVsync; }
		void					setPaceByVsync(bool aPace) { mPaceByVsync = aPace; save(); }
		// the live-coding view (same texture as the "VDCode" Spout sender) drawn over the output
		bool					getCodeOverlay() const { return mCodeOverlay; }
		void					setCodeOverlay(bool aOverlay) { mCodeOverlay = aOverlay; save(); }
		float					getCodeOverlayOpacity() const { return mCodeOverlayOpacity; }
		// not saved on every slider step: call saveSettings() when the drag ends
		void					setCodeOverlayOpacity(float aOpacity) { mCodeOverlayOpacity = ci::math<float>::clamp(aOpacity, 0.0f, 1.0f); }
		// placement: size 1 = the window's height (aspect kept, so on a two-projector span it fills
		// one projector); position 0..1 within the free space (0 = left/top, 0.5 = centre, 1 = right/bottom)
		ci::vec2				getCodeOverlayPosition() const { return mCodeOverlayPosition; }
		void					setCodeOverlayPosition(const ci::vec2& aPosition) { mCodeOverlayPosition = glm::clamp(aPosition, ci::vec2(0.0f), ci::vec2(1.0f)); }
		float					getCodeOverlaySize() const { return mCodeOverlaySize; }
		void					setCodeOverlaySize(float aSize) { mCodeOverlaySize = ci::math<float>::clamp(aSize, 0.1f, 1.0f); }
		void					saveSettings() { save(); }
		bool					getAlwaysOnTop() const { return mAlwaysOnTop; }
		void					setAlwaysOnTop(bool aOnTop);
		// union of the selected displays, in desktop pixels (empty if none selected)
		ci::Area				getSpanBounds() const;
		// true when the selected displays don't form one rectangle (the window then also
		// covers whatever lies in the gaps)
		bool					hasSpanGaps() const;
		std::string				getStatus() const { return mStatus; }
	private:
		enum Request { REQUEST_NONE, REQUEST_OPEN, REQUEST_CLOSE, REQUEST_REOPEN };
		VDSessionFacadeRef		mVDSession;
		VDParamsRef				mVDParams;
		ci::app::WindowRef		mWindow;
		ci::gl::FboRef			mWarpFbo;
		ci::gl::TextureRef		mMainViewTexture;
		Request					mRequest = REQUEST_NONE;
		std::string				mStatus;

		// selected displays, stored by their desktop bounds (stable across runs, unlike indices)
		std::vector<ci::Area>	mSelectedDisplays;
		int						mContent = MAIN_VIEW;
		int						mComposite = COMPOSITE_POST;
		bool					mPaceByVsync = true;
		bool					mAlwaysOnTop = true;
		bool					mCodeOverlay = false;
		float					mCodeOverlayOpacity = 1.0f;
		ci::vec2				mCodeOverlayPosition = ci::vec2(0.0f);	// left, top
		float					mCodeOverlaySize = 1.0f;

		// pacing state: what was applied, and what to restore on close
		int						mSavedMainVsync = -1;
		int						mMainVsyncApplied = -1;
		int						mOutputVsyncApplied = -1;
		bool					mFrameRateOverridden = false;
		bool					mSavedFrameRateEnabled = true;
		float					mSavedFrameRate = 60.0f;

		void					open();
		void					close();
		void					onWindowClosed();
		void					updateFrameRate();
		void					renderWarps();
		void					drawCodeOverlay();
		void					restoreMainContext();
		void					load();
		void					save();
		std::string				mFileName = "output.json";
	};
}
