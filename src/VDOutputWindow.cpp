#include "VDOutputWindow.h"
#include "cinder/app/RendererGl.h"
#include "cinder/JsonTree.h"
#include "cinder/Log.h"

using namespace ci;
using namespace ci::app;
using namespace videodromm;

VDOutputWindow::VDOutputWindow(VDSessionFacadeRef aVDSession) {
	mVDSession = aVDSession;
	mVDParams = VDParams::create();
	load();
	// first run: default to every display except the main one (the usual projector setup)
	if (mSelectedDisplays.empty()) {
		for (auto& display : Display::getDisplays()) {
			if (display != Display::getMainDisplay()) mSelectedDisplays.push_back(display->getBounds());
		}
	}
}

std::string VDOutputWindow::getDisplayLabel(size_t aIndex) const {
	const auto& displays = Display::getDisplays();
	if (aIndex >= displays.size()) return "";
	DisplayRef display = displays[aIndex];
	Area b = display->getBounds();
	return "Display " + toString(aIndex + 1) + ": " + toString(b.getWidth()) + "x" + toString(b.getHeight())
		+ " at " + toString(b.x1) + "," + toString(b.y1) + (display == Display::getMainDisplay() ? " (main)" : "");
}

bool VDOutputWindow::isDisplaySelected(size_t aIndex) const {
	const auto& displays = Display::getDisplays();
	if (aIndex >= displays.size()) return false;
	Area b = displays[aIndex]->getBounds();
	return std::find(mSelectedDisplays.begin(), mSelectedDisplays.end(), b) != mSelectedDisplays.end();
}

void VDOutputWindow::setDisplaySelected(size_t aIndex, bool aSelected) {
	const auto& displays = Display::getDisplays();
	if (aIndex >= displays.size()) return;
	Area b = displays[aIndex]->getBounds();
	auto it = std::find(mSelectedDisplays.begin(), mSelectedDisplays.end(), b);
	if (aSelected && it == mSelectedDisplays.end()) mSelectedDisplays.push_back(b);
	if (!aSelected && it != mSelectedDisplays.end()) mSelectedDisplays.erase(it);
	save();
	if (mWindow) mRequest = REQUEST_REOPEN;
}

void VDOutputWindow::setAlwaysOnTop(bool aOnTop) {
	mAlwaysOnTop = aOnTop;
	save();
	if (mWindow) mRequest = REQUEST_REOPEN;
}

// only displays that are connected right now count - a saved projector that isn't plugged in
// is ignored rather than opening a window on an empty desktop area
Area VDOutputWindow::getSpanBounds() const {
	Area span(0, 0, 0, 0);
	bool first = true;
	for (auto& display : Display::getDisplays()) {
		Area b = display->getBounds();
		if (std::find(mSelectedDisplays.begin(), mSelectedDisplays.end(), b) == mSelectedDisplays.end()) continue;
		if (first) { span = b; first = false; }
		else span.include(b);
	}
	return span;
}

bool VDOutputWindow::hasSpanGaps() const {
	Area span = getSpanBounds();
	int64_t covered = 0;
	for (auto& display : Display::getDisplays()) {
		Area b = display->getBounds();
		if (std::find(mSelectedDisplays.begin(), mSelectedDisplays.end(), b) != mSelectedDisplays.end()) covered += (int64_t)b.getWidth() * b.getHeight();
	}
	return covered < (int64_t)span.getWidth() * span.getHeight();
}

void VDOutputWindow::update() {
	Request request = mRequest;
	mRequest = REQUEST_NONE;
	if (request == REQUEST_CLOSE || request == REQUEST_REOPEN) close();
	if (request == REQUEST_OPEN || request == REQUEST_REOPEN) open();

	if (mWindow) {
		auto app = App::get();
		if (app->getNumWindows() == 1) {
			// the main window was closed while the output stayed open: don't leave a headless
			// app running with only a borderless window on the projector
			app->quit();
			return;
		}
		// projector unplugged mid-show: close instead of leaving a window over a stale area
		if (getElapsedFrames() % 60 == 0 && getSpanBounds().calcArea() == 0) {
			close();
			mStatus = "Closed: the selected displays are no longer connected";
		}
	}
	updateFrameRate();
	if (mWindow && mContent == WARPS) renderWarps();
}

void VDOutputWindow::open() {
	if (mWindow) return;
	Area span = getSpanBounds();
	if (span.calcArea() == 0) {
		mStatus = "No connected display selected";
		return;
	}
	// any selected display can anchor the window, Format::pos() is relative to it
	DisplayRef anchor;
	for (auto& display : Display::getDisplays()) {
		if (std::find(mSelectedDisplays.begin(), mSelectedDisplays.end(), display->getBounds()) != mSelectedDisplays.end()) {
			anchor = display;
			break;
		}
	}
	auto app = App::get();
	ivec2 size = span.getSize();
#if defined( CINDER_MSW )
	// Display bounds are physical pixels on Windows, Format::size() is in points when high
	// density is enabled; the exact pixel rect is applied with SetWindowPos() right after anyway
	if (app->isHighDensityDisplayEnabled()) size = ivec2(vec2(size) / anchor->getContentScale());
#endif
	Window::Format format;
	format.display(anchor)
		.pos(span.getUL() - anchor->getBounds().getUL())
		.size(size)
		.borderless(true)
		.resizable(false)
		.alwaysOnTop(mAlwaysOnTop)
		.title("Videodromm output")
		// the output only blits one texture: no MSAA on a window that may be 2+ projectors wide
		.renderer(RendererGl::create(RendererGl::Options().msaa(0)));
	try {
		mWindow = app->createWindow(format);
	}
	catch (const std::exception& e) {
		mWindow = nullptr;
		mStatus = std::string("Could not create the output window: ") + e.what();
		CI_LOG_E(mStatus);
		restoreMainContext();
		return;
	}
#if defined( CINDER_MSW )
	HWND hwnd = (HWND)mWindow->getNative();
	::SetWindowPos(hwnd, mAlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, span.x1, span.y1, span.getWidth(), span.getHeight(), SWP_NOACTIVATE | SWP_SHOWWINDOW);
	// keep keyboard focus on the UI window
	::SetForegroundWindow((HWND)app->getWindowIndex(0)->getNative());
#endif
	mWindow->getSignalClose().connect([this]() { onWindowClosed(); });
	mOutputVsyncApplied = -1;
	mStatus = "Open: " + toString(span.getWidth()) + "x" + toString(span.getHeight()) + " at " + toString(span.x1) + "," + toString(span.y1);
	CI_LOG_I("VDOutputWindow " << mStatus);
	restoreMainContext();
}

void VDOutputWindow::close() {
	if (mWindow) {
		// emits the close signal synchronously, which clears mWindow (onWindowClosed)
		WindowRef window = mWindow;
		window->close();
	}
	mWindow = nullptr;
	mWarpFbo.reset();
	restoreMainContext();
}

void VDOutputWindow::onWindowClosed() {
	mWindow = nullptr;
	mWarpFbo.reset();
	mOutputVsyncApplied = -1;
	mStatus = "Closed";
}

// creating/closing a window makes its context current; everything the session renders in
// update() (Fbos, Batch VAOs) belongs to the main window's context
void VDOutputWindow::restoreMainContext() {
	auto app = App::get();
	if (app->getNumWindows() > 0) {
		WindowRef mainWindow = app->getWindowIndex(0);
		if (mainWindow && mainWindow != mWindow) mainWindow->getRenderer()->makeCurrentContext();
	}
}

// pacing: with "pace by vsync" the projector's swap blocks once per frame and drives the whole
// loop - the main window's vsync is off (two blocking swaps per frame can halve the frame rate)
// and Cinder's own frame-rate timer is disabled so it doesn't fight the vsync
void VDOutputWindow::updateFrameRate() {
	auto app = App::get();
	bool want = mWindow && mPaceByVsync;
	if (want && !mFrameRateOverridden) {
		mSavedFrameRateEnabled = app->isFrameRateEnabled();
		mSavedFrameRate = app->getFrameRate();
		app->disableFrameRate();
		mFrameRateOverridden = true;
	}
	else if (!want && mFrameRateOverridden) {
		if (mSavedFrameRateEnabled) app->setFrameRate(mSavedFrameRate);
		mFrameRateOverridden = false;
	}
}

void VDOutputWindow::applyMainWindowPacing() {
	if (mSavedMainVsync < 0) mSavedMainVsync = gl::isVerticalSyncEnabled() ? 1 : 0;
	int want = (mWindow && mPaceByVsync) ? 0 : mSavedMainVsync;
	if (want != mMainVsyncApplied) {
		gl::enableVerticalSync(want == 1);
		mMainVsyncApplied = want;
	}
}

bool VDOutputWindow::isCurrent() const {
	return mWindow && getWindow() == mWindow;
}

void VDOutputWindow::renderWarps() {
	ivec2 size = ivec2(mWindow->toPixels(vec2(mWindow->getSize())));
	if (size.x <= 0 || size.y <= 0) return;
	if (!mWarpFbo || mWarpFbo->getSize() != size) {
		mWarpFbo = gl::Fbo::create(size.x, size.y, gl::Fbo::Format().disableDepth());
	}
	ci::gl::TextureRef composite;
	if (mComposite == COMPOSITE_POST) composite = mVDSession->buildPostFboTexture();
	else if (mComposite == COMPOSITE_FX) composite = mVDSession->buildFxFboTexture();
	gl::ScopedFramebuffer fbScp(mWarpFbo);
	gl::ScopedViewport scpVp(ivec2(0), size);
	gl::clear(Color::black());
	mVDSession->drawWarpsToCurrentTarget(composite);
}

void VDOutputWindow::draw() {
	int wantVsync = mPaceByVsync ? 1 : 0;
	if (wantVsync != mOutputVsyncApplied) {
		gl::enableVerticalSync(wantVsync == 1);
		mOutputVsyncApplied = wantVsync;
	}
	gl::clear(Color::black());
	gl::setMatricesWindow(getWindowSize());
	ci::gl::TextureRef tex = (mContent == WARPS && mWarpFbo) ? mWarpFbo->getColorTexture() : mMainViewTexture;
	if (tex) gl::draw(tex, getWindowBounds());
}

MouseEvent VDOutputWindow::toWarpSpace(const MouseEvent& aEvent) const {
	vec2 windowSize = vec2(aEvent.getWindow()->getSize());
	if (windowSize.x <= 0 || windowSize.y <= 0) return aEvent;
	int x = (int)(aEvent.getX() * mVDParams->getFboWidth() / windowSize.x);
	int y = (int)(aEvent.getY() * mVDParams->getFboHeight() / windowSize.y);
	return MouseEvent(aEvent.getWindow(), aEvent.getInitiator(), x, y, aEvent.getModifiers(), aEvent.getWheelIncrement(), aEvent.getNativeModifiers());
}

void VDOutputWindow::load() {
	fs::path jsonFile = getAssetPath("") / mFileName;
	if (!fs::exists(jsonFile)) return;
	try {
		JsonTree json(loadFile(jsonFile));
		if (json.hasChild("content")) mContent = json.getValueForKey<int>("content");
		if (json.hasChild("composite")) mComposite = json.getValueForKey<int>("composite");
		if (json.hasChild("paceByVsync")) mPaceByVsync = json.getValueForKey<bool>("paceByVsync");
		if (json.hasChild("alwaysOnTop")) mAlwaysOnTop = json.getValueForKey<bool>("alwaysOnTop");
		if (json.hasChild("displays")) {
			for (const auto& d : json.getChild("displays")) {
				mSelectedDisplays.push_back(Area(d.getValueForKey<int>("x1"), d.getValueForKey<int>("y1"), d.getValueForKey<int>("x2"), d.getValueForKey<int>("y2")));
			}
		}
	}
	catch (const std::exception& e) {
		CI_LOG_E("VDOutputWindow load " << jsonFile << ": " << e.what());
	}
}

void VDOutputWindow::save() {
	try {
		JsonTree json;
		json.addChild(JsonTree("content", mContent));
		json.addChild(JsonTree("composite", mComposite));
		json.addChild(JsonTree("paceByVsync", mPaceByVsync));
		json.addChild(JsonTree("alwaysOnTop", mAlwaysOnTop));
		JsonTree displays = JsonTree::makeArray("displays");
		for (const auto& b : mSelectedDisplays) {
			JsonTree d;
			d.addChild(JsonTree("x1", b.x1));
			d.addChild(JsonTree("y1", b.y1));
			d.addChild(JsonTree("x2", b.x2));
			d.addChild(JsonTree("y2", b.y2));
			displays.pushBack(d);
		}
		json.addChild(displays);
		json.write(writeFile(getAssetPath("") / mFileName), JsonTree::WriteOptions());
	}
	catch (const std::exception& e) {
		CI_LOG_E("VDOutputWindow save: " << e.what());
	}
}
