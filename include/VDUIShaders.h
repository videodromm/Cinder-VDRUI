#pragma once

#include "cinder/app/App.h"

// UserInterface
#include "CinderImGui.h"
// Settings
#include "VDSettings.h"
// Session
#include "VDSessionFacade.h"
// Params
#include "VDParams.h"


using namespace ci;
using namespace ci::app;
using namespace std;
#define IM_ARRAYSIZE(_ARR)			((int)(sizeof(_ARR)/sizeof(*_ARR)))

namespace videodromm
{
	// stores the pointer to the VDUIShaders instance
	typedef std::shared_ptr<class VDUIShaders> VDUIShadersRef;

	class VDUIShaders
	{
	public:
		VDUIShaders(VDSettingsRef aVDSettings, VDSessionFacadeRef aVDSession);
		static VDUIShadersRef	create(VDSettingsRef aVDSettings, VDSessionFacadeRef aVDSession)
		{
			return shared_ptr<VDUIShaders>(new VDUIShaders(aVDSettings, aVDSession));
		}
		~VDUIShaders();
		void    Run(const char* title);
	private:
		// Settings
		VDSettingsRef				mVDSettings;
		// Session
		VDSessionFacadeRef			mVDSession;
		// Params
		VDParamsRef					mVDParams;

		// imgui
		char						buf[64];
		//! default fragment shader
		std::string					mFboTextureFragmentShaderString;
		string						mError;

		int										xPos, yPos;
		// uniforms
		unsigned int							ctrl;
		//map<int, float>							localValues;
		float							getValue(unsigned int aCtrl, unsigned int aFboIndex) {
			float rtn;
			if (aCtrl == 0) {
				rtn = mVDSession->getUniformValue(aCtrl);
			}
			else {
				rtn = mVDSession->getUniformValueByLocation(aCtrl);
			}
			return rtn;
		};
		void							setValue(unsigned int aCtrl, unsigned int aFboIndex, float aValue) {
			mVDSession->setUniformValueByLocation(aCtrl, aValue);
		};
	};
}