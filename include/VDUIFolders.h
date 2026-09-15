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
// Uniforms
#include "VDUniforms.h"
// Params
#include "VDParams.h"

using namespace ci;
using namespace ci::app;
using namespace std;

namespace videodromm
{
	// stores the pointer to the VDUIFolders instance
	typedef std::shared_ptr<class VDUIFolders> VDUIFoldersRef;
#ifndef IM_ARRAYSIZE
#define IM_ARRAYSIZE(_ARR)			((int)(sizeof(_ARR)/sizeof(*_ARR)))
#endif

	// imgui panel to browse the folders/shaders exposed by the http://localhost:40088/api/folders
	// API (via Cinder-HTTP, wired up in VDSession/VDSessionFacade) and load one into the mix:
	// click a folder to list its shaders, click a shader to load it onto the first fbo shader
	// slot (0-7) whose iWeight uniform is 0.0f, so it doesn't cause a sudden change in the rendering.
	class VDUIFolders
	{
	public:
		VDUIFolders(VDUniformsRef aVDUniforms, VDSessionFacadeRef aVDSession);
		static VDUIFoldersRef	create(VDUniformsRef aVDUniforms, VDSessionFacadeRef aVDSession)
		{
			return shared_ptr<VDUIFolders>(new VDUIFolders(aVDUniforms, aVDSession));
		}
		void    Run(const char* title);
	private:
		// Params
		VDParamsRef					mVDParams;
		// Uniforms
		VDUniformsRef				mVDUniforms;
		// Session
		VDSessionFacadeRef			mVDSession;
		// imgui
		char						buf[256];
		bool						mFoldersRequested = false;
		std::string					mSelectedFolder = "";
		std::string					mExtension = "glsl";
	};
}
