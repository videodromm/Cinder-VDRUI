#pragma once

#include "cinder/app/App.h"

// UserInterface
#include "CinderImGui.h"
// Settings
#include "VDSettings.h"
// Uniforms
#include "VDUniforms.h"
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
		VDUIFbos(VDSettingsRef aVDSettings, VDSessionFacadeRef aVDSession, VDUniformsRef aVDUniforms);
		static VDUIFbosRef	create(VDSettingsRef aVDSettings, VDSessionFacadeRef aVDSession, VDUniformsRef aVDUniforms)
		{
			return shared_ptr<VDUIFbos>(new VDUIFbos(aVDSettings, aVDSession, aVDUniforms));
		}
		void    Run(const char* title);
	private:
		// Params
		VDParamsRef						mVDParams;
		// Settings
		VDSettingsRef					mVDSettings;
		// Session
		VDSessionFacadeRef				mVDSession;
		// Uniforms
		VDUniformsRef					mVDUniforms;
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