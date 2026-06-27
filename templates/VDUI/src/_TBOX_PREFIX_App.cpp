/*
 Copyright (c) 2013-2022, Bruce Lane - All rights reserved.
 This code is intended for use with the Cinder C++ library: http://libcinder.org

 Using Cinder-Warping from Paul Houx.

 Cinder-Warping is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Cinder-Warping is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Cinder-Warping.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

// Animation
#include "VDAnimation.h"
// Session Facade
#include "VDSessionFacade.h"
// Spout
#include "CiSpoutOut.h"
// Uniforms
#include "VDUniforms.h"
// Params
#include "VDParams.h"
// Mix
#include "VDMix.h"

// UI
#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS 1
#include "VDUI.h"
#define IM_ARRAYSIZE(_ARR)			((int)(sizeof(_ARR)/sizeof(*_ARR)))
using namespace ci;
using namespace ci::app;
using namespace videodromm;

class _TBOX_PREFIX_App : public App {
public:
	_TBOX_PREFIX_App();
	void cleanup() override;
	void update() override;
	void draw() override;
	void resize() override;
	void mouseMove(MouseEvent event) override;
	void mouseDown(MouseEvent event) override;
	void mouseDrag(MouseEvent event) override;
	void mouseUp(MouseEvent event) override;
	void keyDown(KeyEvent event) override;
	void keyUp(KeyEvent event) override;
	void fileDrop(FileDropEvent event) override;
private:
	// Settings
	VDSettingsRef					mVDSettings;
	// Animation
	VDAnimationRef					mVDAnimation;
	// Session
	VDSessionFacadeRef				mVDSessionFacade;
	// Mix
	VDMixRef						mVDMix;
	// Uniforms
	VDUniformsRef					mVDUniforms;
	// Params
	VDParamsRef						mVDParams;
	// UI
	VDUIRef							mVDUI;

	bool							mFadeInDelay = true;
	void							toggleCursorVisibility(bool visible);
	SpoutOut 						mSpoutOut;
	int								mTrack = 0;
};


_TBOX_PREFIX_App::_TBOX_PREFIX_App() : mSpoutOut("VDRUI", app::getWindowSize())
{

	// Settings
	mVDSettings = VDSettings::create("VDRUI");
	// Uniform
	mVDUniforms = VDUniforms::create();
	// Params
	mVDParams = VDParams::create();
	// Animation
	mVDAnimation = VDAnimation::create(mVDSettings, mVDUniforms);
	// Mix
	mVDMix = VDMix::create(mVDSettings, mVDAnimation, mVDUniforms);
	// Session
	mVDSessionFacade = VDSessionFacade::createVDSession(mVDSettings, mVDAnimation, mVDUniforms, mVDMix)
		->setUniformValue(mVDUniforms->IDISPLAYMODE, VDDisplayMode::POST)
		->setupSession()
		->setupWSClient()
		->wsConnect()
		//->setupOSCReceiver()
		//->addOSCObserver(mVDSettings->mOSCDestinationHost, mVDSettings->mOSCDestinationPort)
		->addUIObserver(mVDSettings, mVDUniforms)
		->toggleUI()
		->setUniformValue(mVDUniforms->IBPM, 160.0f)
		->setUniformValue(mVDUniforms->IMOUSEY, 0.5648f) // sos only
		->setUniformValue(mVDUniforms->IEXPOSURE, 0.965f)
		//->setUniformValue(mVDUniforms->ISOBEL, 0.999f) // mtrlvr  only 
		->setUniformValue(mVDUniforms->ITOGGLE, 1.0f) // sos only
		->setUniformValue(mVDUniforms->IWEIGHT0, 1.0f)
		->setUniformValue(mVDUniforms->IWEIGHT1, 1.0f) // yeah tests only0.1
		->setUniformValue(mVDUniforms->IWEIGHT2, 1.0f)
		->setUniformValue(mVDUniforms->IWEIGHT3, 0.0f) // yeah tests only0.2
		->setUniformValue(mVDUniforms->IWEIGHT4, 0.0f);

	mFadeInDelay = true;
	// UI
	mVDUI = VDUI::create(mVDSettings, mVDSessionFacade, mVDUniforms);
	// Initialize ImGui context
	ImGui::Initialize();
}

void _TBOX_PREFIX_App::toggleCursorVisibility(bool visible)
{
	if (visible)
	{
		showCursor();
	}
	else
	{
		hideCursor();
	}
}

void _TBOX_PREFIX_App::fileDrop(FileDropEvent event)
{
	mVDSessionFacade->fileDrop(event);
}

void _TBOX_PREFIX_App::mouseMove(MouseEvent event)
{
	if (!mVDSessionFacade->handleMouseMove(event)) {

	}
}

void _TBOX_PREFIX_App::mouseDown(MouseEvent event)
{

	if (!mVDSessionFacade->handleMouseDown(event)) {

	}
}

void _TBOX_PREFIX_App::mouseDrag(MouseEvent event)
{

	if (!mVDSessionFacade->handleMouseDrag(event)) {

	}
}

void _TBOX_PREFIX_App::mouseUp(MouseEvent event)
{

	if (!mVDSessionFacade->handleMouseUp(event)) {

	}
}

void _TBOX_PREFIX_App::keyDown(KeyEvent event)
{

	// warp editor did not handle the key, so handle it here
	if (!mVDSessionFacade->handleKeyDown(event)) {
		switch (event.getCode()) {
		case KeyEvent::KEY_F12:
			// quit the application
			quit();
			break;
		case KeyEvent::KEY_f:
			// toggle full screen
			setFullScreen(!isFullScreen());
			break;

		case KeyEvent::KEY_l:
			mVDSessionFacade->createWarp();
			break;
		}
	}
}

void _TBOX_PREFIX_App::keyUp(KeyEvent event)
{

	// let your application perform its keyUp handling here
	if (!mVDSessionFacade->handleKeyUp(event)) {
		
	}
}
void _TBOX_PREFIX_App::cleanup()
{
	CI_LOG_V("cleanup and save");
	ui::Shutdown();
	mVDSessionFacade->saveWarps();
	mVDSettings->save();
	CI_LOG_V("quit");
}

void _TBOX_PREFIX_App::update()
{
	if (mTrack != (int)mVDSessionFacade->getUniformValue(mVDUniforms->ITRACK)) {
		//mVDSessionFacade->setUniformValue(mVDUniforms->ITIME, (float)getElapsedSeconds());
		//mVDSessionFacade->setUniformValue(mVDUniforms->ISTART, mVDUniforms->getUniformValue(mVDUniforms->ITIME));
		mVDSessionFacade->useTimeWithTempo();
		mTrack = (int)mVDSessionFacade->getUniformValue(mVDUniforms->ITRACK);
		switch (mTrack)
		{
		case 110:
			// Zoom D7
			// mVDSessionFacade->loadFolder("zoom");
			mVDSessionFacade->loadFolder("boomer");
			break;
		case 111:
			// Binary D#7
			//mVDSessionFacade->loadFolder("onezero");
			// ntb
			mVDSessionFacade->loadFolder("dd");
			break;
		case 112:
			// SOS E7
			//mVDSessionFacade->loadFolder("sos");
			mVDSessionFacade->loadFolder("fear"); // k21
			break;
		case 113:
			// HumanET F7
			mVDSessionFacade->loadFolder("spidermoon");
			
			break;
		case 114:
			// HumanET F#7
			
			mVDSessionFacade->loadFolder("julien");
			break;
		case 115:
			// HumanET G#7
			//mVDSessionFacade->loadFolder("love");
			mVDSessionFacade->loadFolder("evolution");
			break;
		case 116:
			// HumanET G7
			//mVDSessionFacade->loadFolder("green");
			mVDSessionFacade->loadFolder("sky");
			break;
		case 117:
			// HumanET A7
			//mVDSessionFacade->loadFolder("purple");
			//mVDSessionFacade->loadFolder("gali");
			mVDSessionFacade->loadFolder("lovehate");
			break;
		case 118:
			// HumanET A#7 
			//mVDSessionFacade->loadFolder("logo");
			mVDSessionFacade->loadFolder("polo");
			break;
		case 119:
			// Yeah Beauso Disco Zombification 130 B7
			//mVDSessionFacade->loadFolder("yf");
			mVDSessionFacade->loadFolder("onezero");
			break;
		case 120:
			// welcometo C8
			mVDSessionFacade->loadFolder("welcometo");
			mVDSessionFacade->setUniformValue(mVDUniforms->IWEIGHT0, 1.0f);
			mVDSessionFacade->setUniformValue(mVDUniforms->IWEIGHT1, 0.0f);
			mVDSessionFacade->setUniformValue(mVDUniforms->IWEIGHT2, 0.0f);
			mVDSessionFacade->setUniformValue(mVDUniforms->IDISPLAYMODE, VDDisplayMode::FX);
			break;
		// champo
		case 108:
			// galeriens C7
			mVDSessionFacade->loadFolder("galeriens");
			break;
		case 109:
			// guns C#7
			mVDSessionFacade->loadFolder("guns");
			break;
		case 107:
			// mission B6
			mVDSessionFacade->loadFolder("mission");
			break;
		case 106:
			// fous A#6
			mVDSessionFacade->loadFolder("fous");
			break;
		case 105:
			// google A6
			mVDSessionFacade->loadFolder("google");
			break;
		case 121:
			// C#8 glasses
			mVDSessionFacade->loadFolder("glasses");
			break;
		case 122:
			// D8 bombe
			mVDSessionFacade->loadFolder("bombe");
			break;
		case 123:
			// D#8 egyptien
			mVDSessionFacade->loadFolder("egyptien");
			break;
		case 124:
			// E8 house
			mVDSessionFacade->loadFolder("house");
			break;
		case 125:
			// F8 hejoe
			mVDSessionFacade->loadFolder("hejoe");
			break;
		case 126:
			// F#8 dream
			mVDSessionFacade->loadFolder("dream");
			break;
		case 127:
			// G8 heaven
			mVDSessionFacade->loadFolder("heaven");
			break;

		default:
			break;
		}
	}
	mVDSessionFacade->setUniformValue(mVDUniforms->IFPS, getAverageFps());
	mVDSessionFacade->update();
}


void _TBOX_PREFIX_App::resize()
{
	mVDUI->resize();
}
void _TBOX_PREFIX_App::draw()
{
	// clear the window and set the drawing color to white
	gl::clear();
	gl::color(Color::white());
	if (mFadeInDelay) {
		mVDSettings->iAlpha = 0.0f;
		if (getElapsedFrames() > 10.0) {// mVDSessionFacade->getFadeInDelay()) {
			mFadeInDelay = false;
			timeline().apply(&mVDSettings->iAlpha, 0.0f, 1.0f, 1.5f, EaseInCubic());
		}
	}
	else {
		gl::setMatricesWindow(mVDParams->getFboWidth(), mVDParams->getFboHeight());
		
		int m = mVDSessionFacade->getUniformValue(mVDUniforms->IDISPLAYMODE);
		if (m == VDDisplayMode::MIXETTE) {
			gl::draw(mVDSessionFacade->buildRenderedMixetteTexture(0));
			mSpoutOut.sendTexture(mVDSessionFacade->buildRenderedMixetteTexture(0));
		}
		else if (m == VDDisplayMode::POST) {
			gl::draw(mVDSessionFacade->buildPostFboTexture());
			mSpoutOut.sendTexture(mVDSessionFacade->buildPostFboTexture());
		}
		else if (m == VDDisplayMode::FX) {
			gl::draw(mVDSessionFacade->buildFxFboTexture());
			mSpoutOut.sendTexture(mVDSessionFacade->buildFxFboTexture());
		}
		else {
			if (m < mVDSessionFacade->getFboShaderListSize()) {
				gl::draw(mVDSessionFacade->getFboShaderTexture(m));
				mSpoutOut.sendTexture(mVDSessionFacade->getFboShaderTexture(m));
			}
			else {
				gl::draw(mVDSessionFacade->buildRenderedMixetteTexture(0), Area(50, 50, mVDParams->getFboWidth() / 2, mVDParams->getFboHeight() / 2));
				gl::draw(mVDSessionFacade->buildPostFboTexture(), Area(mVDParams->getFboWidth() / 2, mVDParams->getFboHeight() / 2, mVDParams->getFboWidth(), mVDParams->getFboHeight()));
			}
			//gl::draw(mVDSession->getRenderedMixetteTexture(0), Area(0, 0, mVDSettings->mFboWidth, mVDSettings->mFboHeight));
			// ok gl::draw(mVDSession->getWarpFboTexture(), Area(0, 0, mVDSettings->mFboWidth, mVDSettings->mFboHeight));//getWindowBounds()
		}
	}	
	// imgui
	if (mVDSessionFacade->showUI()) {
		mVDUI->Run("UI", (int)getAverageFps());
		if (mVDUI->isReady()) {
		}
	}
	getWindow()->setTitle(toString((int)getAverageFps()) + " fps");
}
void prepareSettings(App::Settings *settings)
{
	settings->setWindowSize(1280, 720);
}
CINDER_APP(_TBOX_PREFIX_App, RendererGl(RendererGl::Options().msaa(8)),  prepareSettings)
