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
// Session
#include "VDSessionFacade.h"
// Uniforms
#include "VDUniforms.h"
// Params
#include "VDParams.h"

using namespace ci;
using namespace ci::app;
using namespace std;

namespace videodromm
{
	// stores the pointer to the VDUIAnimation instance
	typedef std::shared_ptr<class VDUIAnimation> VDUIAnimationRef;
	typedef std::shared_ptr<class VDUIOsc> VDUIOscRef;
	// imgui.h (pulled in via cinder/CinderImGui.h above) already defines this - redefining it
	// unconditionally triggered "warning C4005: redéfinition de macro" on every build; guard it
	// the same way VDUIFolders.h already does
#ifndef IM_ARRAYSIZE
#define IM_ARRAYSIZE(_ARR)			((int)(sizeof(_ARR)/sizeof(*_ARR)))
#endif
	class VDUIAnimation
	{
	public:
		VDUIAnimation(VDSettingsRef aVDSettings, VDSessionFacadeRef aVDSession, VDUniformsRef aVDUniforms);
		static VDUIAnimationRef	create(VDSettingsRef aVDSettings, VDSessionFacadeRef aVDSession, VDUniformsRef aVDUniforms)
		{
			return shared_ptr<VDUIAnimation>(new VDUIAnimation(aVDSettings, aVDSession, aVDUniforms));
		}
		~VDUIAnimation();
		void    Run(const char* title);
		// Audio/Midi/Tempo/OSC/Websocket used to always be visible as CollapsingHeaders inside
		// this one "Animation" window, making it very tall - each is now its own separate window,
		// hidden by default and toggled independently via a button in VDUI.cpp, so only the
		// panels actually in use take up screen space.
		void	toggleShowAudio() { mShowAudio = !mShowAudio; }
		void	toggleShowMidi() { mShowMidi = !mShowMidi; }
		void	toggleShowTempo() { mShowTempo = !mShowTempo; }
		void	toggleShowOSC() { mShowOSC = !mShowOSC; }
		void	toggleShowWebsocket() { mShowWebsocket = !mShowWebsocket; }
		bool	isShowingAudio() { return mShowAudio; }
		bool	isShowingMidi() { return mShowMidi; }
		bool	isShowingTempo() { return mShowTempo; }
		bool	isShowingOSC() { return mShowOSC; }
		bool	isShowingWebsocket() { return mShowWebsocket; }
	private:
		bool							mShowAudio = false;
		bool							mShowMidi = false;
		bool							mShowTempo = false;
		bool							mShowOSC = false;
		bool							mShowWebsocket = false;
		// midi learn - the uniform index currently typed into the Midi panel's target box
		// (VDMidi itself only tracks the actually-armed target, set via armMidiLearn(); this is
		// just this input widget's own displayed value, distinct so the box can be edited
		// without re-arming on every keystroke)
		int								mMidiLearnTargetUniform = 1;
		// Params
		VDParamsRef						mVDParams;
		// Settings
		VDSettingsRef					mVDSettings;
		// Session
		VDSessionFacadeRef				mVDSession;
		// Uniforms
		VDUniformsRef					mVDUniforms;

		// imgui
		float							getFloatValue(unsigned int aCtrl) {
			return mVDSession->getUniformValue(aCtrl);
		};
		void							setFloatValue(unsigned int aCtrl, float aValue) {
			mVDSession->setUniformValue(aCtrl, aValue);
		};
		void							toggleValue(unsigned int aCtrl) {
			mVDSession->toggleValue(aCtrl);
		};

		float							getMinUniformValue(unsigned int aIndex) {
			return mVDSession->getMinUniformValue(aIndex); // TODO ERROR UI 0 for value
		};
		float							getMaxUniformValue(unsigned int aIndex) {
			return mVDSession->getMaxUniformValue(aIndex); // TODO ERROR UI 0 for value
		};

		unsigned int					ctrl;
		map<int, float>					localValues;
		float							f = 0.0f;
		char							buf[64];
		// audio device list - a separate, larger buffer since device names (unlike the other
		// labels using buf) can easily exceed 64 chars, and sprintf_s aborts on overflow
		bool							mAudioDevicesRequested = false;
		char							audioBuf[256];
		// zoom
		float							minZoom;
		float							maxZoom;
		// render
		float							contour, iVAmount, iVFallOff;
		int								iBlendmode;
		int								iResolutionX, iResolutionY;
		int								iOutW, iOutH, iBarBeat;
		// contour
		float							minContour;
		float							maxContour;
		// color
		float							color[4];
		float							backcolor[4];
	};
}