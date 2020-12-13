#pragma once

#include "cinder/app/App.h"

// UserInterface
#include "Cinder/CinderImGui.h"
// Settings
#include "VDSettings.h"
// Params
#include "VDParams.h"
// Session
#include "VDSessionFacade.h"

using namespace ci;
using namespace ci::app;
using namespace std;

namespace videodromm
{
	// stores the pointer to the VDUIFbos instance
	typedef std::shared_ptr<class VDUIFbos> VDUIFbosRef;

	class VDUIFbos
	{
	public:
		VDUIFbos(VDSettingsRef aVDSettings, VDSessionFacadeRef aVDSession);
		static VDUIFbosRef	create(VDSettingsRef aVDSettings, VDSessionFacadeRef aVDSession)
		{
			return shared_ptr<VDUIFbos>(new VDUIFbos(aVDSettings, aVDSession));
		}
		void    Run(const char* title);
	private:
		// Params
		VDParamsRef						mVDParams;
		// Settings
		VDSettingsRef					mVDSettings;
		// Session
		VDSessionFacadeRef				mVDSession;
		// imgui
		char							buf[64];
		int								xPos, yPos;
		int								xStep = 1;
		int								yStep = 1;
		// sequence
		int								playheadPositions[12];
		float							speeds[12];
		// uniforms
		unsigned int					ctrl;
		map<int, float>					localValues;
		float							getValue(unsigned int aCtrl, unsigned int aFboIndex) {
			float rtn;
			//if (mVDSession->getGlobal(aFboIndex) || aCtrl == 0) {
				rtn = mVDSession->getUniformValue(aCtrl);
			/*}
			else {
				rtn = mVDSession->getFboFloatUniformValueByIndex(aCtrl, aFboIndex);
			}*/
			return rtn;
		};
		void							setValue(unsigned int aCtrl, unsigned int aFboIndex, float aValue) {
			//if (mVDSession->getGlobal(aFboIndex)) {
				mVDSession->setUniformValue(aCtrl, aValue);
			/*}
			else {
				mVDSession->setFboFloatUniformValueByIndex(aCtrl, aFboIndex, aValue);
			}*/
			
		};
		float							getMinUniformValue(unsigned int aIndex) {
			return mVDSession->getMinUniformValue(aIndex);
		};
		float							getMaxUniformValue(unsigned int aIndex) {
			return mVDSession->getMaxUniformValue(aIndex);
		};
		void							toggleValue(unsigned int aCtrl, unsigned int aFboIndex) {
			mVDSession->toggleValue(aCtrl); //toggleFboValue(aCtrl, aFboIndex);
		};
		// mouse
		float						mouseX, mouseY;
		bool						mouseZ;
		bool						mShowRenderedTexture = true;
		bool						mShowInputTexture = true;
	};
}