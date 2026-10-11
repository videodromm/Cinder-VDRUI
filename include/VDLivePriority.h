#pragma once
// Live performance: this app comes first (60 fps target); TSWebsocketServer, the WebApp,
// Resolume... are helpers. Include this from the app's own .cpp ONLY (once per exe: it defines
// the two exported variables below), and call videodromm::applyLivePriority() in prepareSettings.
#include "cinder/Cinder.h"
#if defined( CINDER_MSW )
#include <windows.h>
#include <timeapi.h>
#pragma comment(lib, "winmm.lib")

// Laptops with two GPUs (NVIDIA Optimus, AMD PowerXpress): run this exe on the dedicated one.
// The drivers read these exports from the exe itself; Cinder only has them in its GLFW backend,
// which the Windows apps don't use, so without them Windows may pick the integrated GPU.
extern "C" {
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

namespace videodromm {
	inline void applyLivePriority() {
#if defined( CINDER_MSW )
		// above the helpers, without starving audio or input (HIGH/REALTIME could)
		::SetPriorityClass(::GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);
		// 1 ms timer: Cinder's frame-rate sleep otherwise rounds to ~15.6 ms steps, which can't
		// pace 60 fps evenly. Released by Windows when the process ends.
		::timeBeginPeriod(1);
#endif
	}
}
