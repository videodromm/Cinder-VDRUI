#pragma once

#include "cinder/app/App.h"

// UserInterface
#include "cinder/CinderImGui.h"
// Settings
#include "VDSettings.h"
// Session
#include "VDSessionFacade.h"
// Params
#include "VDParams.h"

using namespace ci;
using namespace ci::app;
using namespace std;

namespace videodromm
{
	// stores the pointer to the VDUIWarps instance
	typedef std::shared_ptr<class VDUIWarps> VDUIWarpsRef;

	class VDUIWarps
	{
	public:
		VDUIWarps(VDSettingsRef aVDSettings, VDSessionFacadeRef aVDSession);
		static VDUIWarpsRef	create(VDSettingsRef aVDSettings, VDSessionFacadeRef aVDSession)
		{
			return shared_ptr<VDUIWarps>(new VDUIWarps(aVDSettings, aVDSession));
		}
		void    Run(const char* title);
	private:
		// Params
		VDParamsRef					mVDParams;
		// Settings
		VDSettingsRef				mVDSettings;
		// Session
		VDSessionFacadeRef			mVDSession;
		// imgui
		char						buf[64];
		int							xPos, yPos;
		bool						mUseMixette = false;
		void setValue(unsigned int aCtrl, float aValue) {
			mVDSession->setUniformValue(aCtrl, aValue);
		}
		int							ctrl;
	};
}