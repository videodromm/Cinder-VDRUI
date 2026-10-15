#pragma once

#if defined( _WIN32 ) && ! defined( WIN32_LEAN_AND_MEAN )
// must be defined before the first Windows header (pulled in by cinder/app/App.h) is seen,
// otherwise asio/websocketpp later in the translation unit hits "WinSock.h has already been included"
#define WIN32_LEAN_AND_MEAN
#endif
#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/Surface.h"
#include "cinder/CinderImGui.h"
#include "VDSessionFacade.h"
#include "VDUniforms.h"

#if defined( CINDER_MSW )
#include "cinder/msw/CinderMsw.h"
#endif

#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <ctime>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

namespace videodromm
{
	typedef std::shared_ptr<class VDVideoExport> VDVideoExportRef;

	// Exports what the output window shows (its warps when it's open on "Warps", else the main
	// view: the texture the main window shows and Spout sends) as an mp4 in <repos>/export,
	// with the folder's song as the sound.
	// "Arm" waits for the song's next Play (fbo pane, WebApp, Setlist...); "Record from start"
	// arms and plays the song from 0. Recording stops when the song stops: its end, or a Pause.
	// Frames are read back from the GPU and piped as raw video to ffmpeg (libx264), which also
	// muxes the song from the position Play started at. Video frames follow the song's clock,
	// not the app's frame rate: a frame is written each time the song passes 1/fps s, the last
	// one repeated when the app runs slower, so picture and sound stay in sync.
	// Needs ffmpeg (Windows: winget install Gyan.FFmpeg; Mac: brew install ffmpeg). Header-only,
	// so the apps' Visual Studio and Xcode projects need no new file. The code overlay drawn on
	// the output window isn't in the export.
	class VDVideoExport
	{
	public:
		enum State { IDLE, ARMED, RECORDING, FINISHING, DONE, FAILED };

		VDVideoExport(VDUniformsRef aVDUniforms, VDSessionFacadeRef aVDSession)
			: mVDUniforms(aVDUniforms), mVDSession(aVDSession) {
			mFfmpeg = findFfmpeg();
		}
		~VDVideoExport() { abortNow(); }
		static VDVideoExportRef create(VDUniformsRef aVDUniforms, VDSessionFacadeRef aVDSession) {
			return std::shared_ptr<VDVideoExport>(new VDVideoExport(aVDUniforms, aVDSession));
		}

		State getState() const { return mState.load(); }
		bool isRecording() const { return mState.load() == RECORDING; }

		// every frame, from the main window's draw (main GL context current), with what the
		// output window shows
		void update(const ci::gl::TextureRef& aTexture) {
			const State state = mState.load();
			if (state == FINISHING) return;
			const bool playing = mVDSession->isTransportPlaying();
			if (state == ARMED) {
				if (playing && !mWasPlaying) start(aTexture);
			}
			else if (state == RECORDING) {
				if (!playing) finish("song stopped");
				else captureFrames(aTexture);
			}
			mWasPlaying = playing;
		}

		// aOpen: the panel's close box clears it (the caller may set the next window's pos/focus first)
		void Run(const char* title, bool* aOpen, const ci::gl::TextureRef& aTexture, const std::string& aSourceLabel) {
			const float uiScale = mVDUniforms->getUniformValue(mVDUniforms->IUISCALE);
			ImGui::SetNextWindowSize(ImVec2(420.0f * uiScale, 230.0f * uiScale), ImGuiCond_Once);
			ImGui::Begin(title, aOpen, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse);
			{
				const State state = mState.load();
				if (mFfmpeg.empty()) {
					ImGui::TextColored(ImVec4(1, 0.4f, 0.3f, 1), "ffmpeg not found");
					ImGui::TextWrapped("Windows: winget install Gyan.FFmpeg, then restart the app. Mac: brew install ffmpeg.");
					if (ImGui::Button("Look again")) mFfmpeg = findFfmpeg();
				}
				else {
					ImGui::TextDisabled("ffmpeg: %s", mFfmpeg.c_str());
				}
				const std::string song = mVDSession->getTransportSongName();
				ImGui::Text("Source: %s%s", aSourceLabel.c_str(), aTexture ? "" : " (nothing yet)");
				if (aTexture) { ImGui::SameLine(); ImGui::TextDisabled("%dx%d", aTexture->getWidth(), aTexture->getHeight()); }
				ImGui::Text("Song: %s", song.empty() ? "none (the folder has no wav or video)" : song.c_str());
				ImGui::TextDisabled("Output folder: %s (ffmpeg logs: logs/export)", exportFolder().string().c_str());

				const bool idle = state == IDLE || state == DONE || state == FAILED;
				if (!idle) ImGui::BeginDisabled();
				ImGui::SetNextItemWidth(80.0f * uiScale);
				ImGui::Combo("fps", &mFpsIndex, "30\0" "60\0");
				ImGui::SameLine();
				ImGui::Checkbox("Flip vertically", &mFlip);
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Turn on if the exported video comes out upside down");
				if (!idle) ImGui::EndDisabled();

				const bool canArm = !mFfmpeg.empty() && mVDSession->hasTransportSong();
				if (idle) {
					if (!canArm) ImGui::BeginDisabled();
					if (ImGui::Button("Arm")) arm();
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("Record from the song's next Play until it stops (end or Pause)");
					ImGui::SameLine();
					if (ImGui::Button("Record from start")) {
						arm();
						if (mVDSession->isTransportPlaying()) mVDSession->transport("pause", 0.0);
						mWasPlaying = false;
						mVDSession->transport("seek", 0.0);
						mVDSession->transport("play", 0.0);
					}
					if (!canArm) ImGui::EndDisabled();
				}
				else if (state == ARMED) {
					ImGui::TextColored(ImVec4(1, 0.8f, 0.2f, 1), mVDSession->isTransportPlaying()
						? "Armed: the song is playing, pause it and press Play to start" : "Armed: press Play to start recording");
					if (ImGui::Button("Cancel")) mState = IDLE;
				}
				else if (state == RECORDING) {
					const double t = mVDSession->getTransportPosition() - mStartPosition;
					ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "REC %d:%04.1f  %d frames (%d repeated, %d dropped)",
						(int)(t / 60.0), fmod(t, 60.0), (int)mFramesWritten, (int)mFramesRepeated, (int)mFramesDropped.load());
					if (ImGui::Button("Stop")) finish("stopped");
				}
				else if (state == FINISHING) {
					ImGui::Text("Finishing the file...");
				}
				const std::string statusText = status();
				if (state == DONE) ImGui::TextColored(ImVec4(0.4f, 1, 0.4f, 1), "%s", statusText.c_str());
				else if (state == FAILED) ImGui::TextColored(ImVec4(1, 0.4f, 0.3f, 1), "%s", statusText.c_str());
				else if (!statusText.empty() && state != RECORDING) ImGui::TextDisabled("%s", statusText.c_str());
				if (!mOutFile.empty() && (state == DONE || state == FAILED))
					ImGui::TextDisabled("ffmpeg log: %s", mLogFile.c_str());
			}
			ImGui::End();
		}

	private:
		VDUniformsRef			mVDUniforms;
		VDSessionFacadeRef		mVDSession;
		std::string				mFfmpeg;
		std::atomic<State>		mState{ IDLE };
		// written by the finisher thread too: read and write through status()/setStatus()
		std::string				mStatus;
		mutable std::mutex		mStatusMutex;
		std::string status() const { std::lock_guard<std::mutex> lock(mStatusMutex); return mStatus; }
		void setStatus(const std::string& aStatus) { std::lock_guard<std::mutex> lock(mStatusMutex); mStatus = aStatus; }
		std::string				mOutFile;
		int						mFpsIndex = 0;
		bool					mFlip = false;
		bool					mWasPlaying = false;

		// recording
		double					mStartPosition = 0.0;
		double					mFps = 30.0;
		ci::ivec2				mSize;
		long long				mFramesWritten = 0;
		long long				mFramesRepeated = 0;
		std::atomic<int>		mFramesDropped{ 0 };
		std::shared_ptr<std::vector<uint8_t>> mLastFrame;

		// writer thread: frames queued here, written to ffmpeg's stdin
		std::thread				mWriter;
		std::thread				mFinisher;
		std::mutex				mMutex;
		std::condition_variable	mCond;
		std::deque<std::shared_ptr<std::vector<uint8_t>>> mQueue;
		bool					mStop = false;
		std::atomic<bool>		mWriteFailed{ false };
		static const size_t		MAX_QUEUE = 8;
#if defined( CINDER_MSW )
		HANDLE					mPipe = NULL;
		HANDLE					mProcess = NULL;
#else
		FILE*					mPipe = nullptr;
#endif

		static ci::fs::path exportFolder() {
			return ci::app::getAssetPath("") / ".." / "export";
		}
		// ffmpeg's logs: <repos>/logs/export, the one part of logs Syncthing shares
		// (.stignore-shared), so an export can be analysed from another machine
		static ci::fs::path logFolder() {
			return ci::app::getAssetPath("") / ".." / "logs" / "export";
		}
		std::string				mLogFile;

		void arm() {
			if (mFinisher.joinable()) mFinisher.join();
			setStatus("");
			mWasPlaying = mVDSession->isTransportPlaying();
			mState = ARMED;
		}

		// ---- ffmpeg
#if defined( CINDER_MSW )
		static std::wstring getEnv(const wchar_t* aName, DWORD aMax = MAX_PATH) {
			std::wstring buf(aMax, wchar_t(0));
			const DWORD n = GetEnvironmentVariableW(aName, &buf[0], aMax);
			return (n > 0 && n < aMax) ? buf.substr(0, n) : std::wstring();
		}
#endif
		static std::string findFfmpeg() {
			std::vector<ci::fs::path> candidates;
#if defined( CINDER_MSW )
			const std::wstring env = getEnv(L"FFMPEG");
			if (!env.empty()) candidates.push_back(ci::fs::path(env));
			// PATH only, absolute entries only (SearchPathW would also try the current folder, where
			// any ffmpeg.exe would run)
			const std::wstring path = getEnv(L"PATH", 32767);
			size_t from = 0;
			while (from <= path.size()) {
				const size_t to = std::min(path.find(L';', from), path.size());
				const ci::fs::path dir(path.substr(from, to - from));
				if (dir.is_absolute()) candidates.push_back(dir / "ffmpeg.exe");
				from = to + 1;
			}
			const std::wstring local = getEnv(L"LOCALAPPDATA");
			if (!local.empty()) {
				const ci::fs::path winget = ci::fs::path(local) / "Microsoft" / "WinGet";
				candidates.push_back(winget / "Links" / "ffmpeg.exe");
				// winget's package folder (the app may have been started before the install updated PATH)
				std::error_code ec;
				if (ci::fs::is_directory(winget / "Packages", ec)) {
					for (auto& pkg : ci::fs::directory_iterator(winget / "Packages", ec)) {
						if (pkg.path().filename().string().find("FFmpeg") == std::string::npos) continue;
						for (auto& build : ci::fs::directory_iterator(pkg.path(), ec))
							candidates.push_back(build.path() / "bin" / "ffmpeg.exe");
					}
				}
			}
			candidates.push_back("C:/ffmpeg/bin/ffmpeg.exe");
			candidates.push_back("C:/Program Files/ffmpeg/bin/ffmpeg.exe");
#else
			if (const char* env = std::getenv("FFMPEG")) candidates.push_back(ci::fs::path(env));
			candidates.push_back("/opt/homebrew/bin/ffmpeg");
			candidates.push_back("/usr/local/bin/ffmpeg");
			candidates.push_back("/usr/bin/ffmpeg");
#endif
			for (auto& c : candidates) {
				std::error_code ec;
				if (ci::fs::is_regular_file(c, ec)) return c.string();
			}
			return "";
		}

#if defined( CINDER_MSW )
		// CreateProcessW (no shell): Windows file names can't contain '"', so plain quotes are enough
		static std::string quote(const std::string& s) { return "\"" + s + "\""; }
#else
		// popen goes through /bin/sh: single quotes, so a file name with $( ), ` or " can't run anything
		static std::string quote(const std::string& s) {
			std::string r = "'";
			for (char c : s) r += (c == '\'') ? std::string("'\\''") : std::string(1, c);
			return r + "'";
		}
#endif

		static std::string timestamp() {
			std::time_t now = std::time(nullptr);
			std::tm tm = {};
#if defined( CINDER_MSW )
			localtime_s(&tm, &now);
#else
			localtime_r(&now, &tm);
#endif
			char buf[32];
			std::strftime(buf, sizeof(buf), "%Y%m%d-%H%M%S", &tm);
			return buf;
		}

		static std::string safeName(const std::string& s) {
			std::string r;
			for (char c : s) r += (std::isalnum((unsigned char)c) || c == '-' || c == '_') ? c : '_';
			return r.empty() ? "export" : r.substr(0, 40);
		}

		void start(const ci::gl::TextureRef& aTexture) {
			if (!aTexture) { fail("nothing to record yet (no output texture)"); return; }
			mSize = aTexture->getSize();
			mFps = mFpsIndex == 1 ? 60.0 : 30.0;
			mStartPosition = mVDSession->getTransportPosition();
			mFramesWritten = mFramesRepeated = 0;
			mFramesDropped = 0;
			mWriteFailed = false;
			mLastFrame.reset();

			std::error_code ec;
			ci::fs::create_directories(exportFolder(), ec);
			const std::string folder = mVDSession->getLoadedFolder();
			ci::fs::create_directories(logFolder(), ec);
			const std::string base = safeName(folder) + "-" + timestamp();
			mOutFile = (exportFolder() / (base + ".mp4")).string();
			mLogFile = (logFolder() / (base + ".ffmpeg.log")).string();
			const std::string song = mVDSession->getTransportSongPath();

			// raw RGBA frames on stdin, the song from where Play started as the sound
			char size[64];
			snprintf(size, sizeof(size), "%dx%d", mSize.x, mSize.y);
			char ss[32];
			snprintf(ss, sizeof(ss), "%.3f", mStartPosition);
			std::string args = " -hide_banner -y -f rawvideo -pix_fmt rgba -s " + std::string(size)
				+ " -framerate " + std::to_string((int)mFps) + " -i -";
			if (!song.empty()) args += " -ss " + std::string(ss) + " -i " + quote(song);
			args += " -map 0:v";
			if (!song.empty()) args += " -map \"1:a?\"";   // optional: a video song may have no sound
			args += " -vf " + quote(std::string(mFlip ? "vflip," : "") + "scale=trunc(iw/2)*2:trunc(ih/2)*2,format=yuv420p");
			args += " -c:v libx264 -preset veryfast -crf 18";
			if (!song.empty()) args += " -c:a aac -b:a 320k -shortest";
			args += " -movflags +faststart " + quote(mOutFile);

			if (!launch(quote(mFfmpeg) + args, mLogFile)) { fail("could not start ffmpeg"); return; }
			CI_LOG_I("export: recording " << mOutFile << " from " << mStartPosition << " s, " << size << " @" << mFps);
			{
				std::lock_guard<std::mutex> lock(mMutex);
				mQueue.clear();
				mStop = false;
			}
			mWriter = std::thread([this] { writerLoop(); });
			setStatus("recording " + ci::fs::path(mOutFile).filename().string());
			mState = RECORDING;
			captureFrames(aTexture);
		}

		void captureFrames(const ci::gl::TextureRef& aTexture) {
			if (mWriteFailed) { finish("ffmpeg stopped reading (see its log)"); return; }
			const double t = mVDSession->getTransportPosition() - mStartPosition;
			const long long target = t < 0.0 ? 0 : (long long)(t * mFps) + 1;   // frames due by now
			if (mFramesWritten >= target) return;
			// a new frame (if the size changed, e.g. a warp fbo resize, the last one is repeated)
			std::shared_ptr<std::vector<uint8_t>> frame = mLastFrame;
			if (aTexture && aTexture->getSize() == mSize) frame = readBack(aTexture);
			if (!frame) return;
			for (long long n = mFramesWritten; n < target; n++) {
				{
					std::lock_guard<std::mutex> lock(mMutex);
					if (mQueue.size() >= MAX_QUEUE) { mFramesDropped++; break; }   // ffmpeg behind: retried next frame
					mQueue.push_back(frame);
				}
				if (n > mFramesWritten) mFramesRepeated++;
				mFramesWritten = n + 1;
			}
			mCond.notify_one();
			mLastFrame = frame;
		}

		std::shared_ptr<std::vector<uint8_t>> readBack(const ci::gl::TextureRef& aTexture) {
			ci::Surface8u surface(aTexture->createSource());
			const int w = surface.getWidth(), h = surface.getHeight();
			auto frame = std::make_shared<std::vector<uint8_t>>((size_t)w * h * 4);
			const ci::SurfaceChannelOrder& order = surface.getChannelOrder();
			const int r = order.getRedOffset(), g = order.getGreenOffset(), b = order.getBlueOffset();
			const int inc = surface.getPixelInc();
			uint8_t* out = frame->data();
			for (int y = 0; y < h; y++) {
				const uint8_t* row = surface.getData(ci::ivec2(0, y));
				for (int x = 0; x < w; x++, row += inc, out += 4) {
					out[0] = row[r]; out[1] = row[g]; out[2] = row[b]; out[3] = 255;
				}
			}
			return frame;
		}

		void writerLoop() {
			for (;;) {
				std::shared_ptr<std::vector<uint8_t>> frame;
				{
					std::unique_lock<std::mutex> lock(mMutex);
					mCond.wait(lock, [this] { return mStop || !mQueue.empty(); });
					if (mQueue.empty()) return;   // stopped and drained
					frame = mQueue.front();
					mQueue.pop_front();
				}
				if (!mWriteFailed && !writeAll(frame->data(), frame->size())) mWriteFailed = true;
			}
		}

		void finish(const std::string& aWhy) {
			if (mState.load() != RECORDING) return;
			mState = FINISHING;
			CI_LOG_I("export: " << aWhy << ", " << mFramesWritten << " frames");
			{
				std::lock_guard<std::mutex> lock(mMutex);
				mStop = true;
			}
			mCond.notify_one();
			const std::string file = mOutFile;
			// joining the writer and waiting for ffmpeg can take a moment: off the main thread
			if (mFinisher.joinable()) mFinisher.join();
			mFinisher = std::thread([this, file] {
				if (mWriter.joinable()) mWriter.join();
				const int code = closeAndWait();
				if (code == 0) { setStatus("saved " + file); mState = DONE; }
				else { setStatus("ffmpeg failed (exit " + std::to_string(code) + "), see the log"); mState = FAILED; }
			});
		}

		void fail(const std::string& aWhy) {
			setStatus(aWhy);
			CI_LOG_W("export: " << aWhy);
			mState = FAILED;
		}

		void abortNow() {
			if (mState.load() == RECORDING) finish("app closing");
			if (mFinisher.joinable()) mFinisher.join();
		}

		// ---- the ffmpeg process
#if defined( CINDER_MSW )
		bool launch(const std::string& aCommand, const std::string& aLogFile) {
			SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
			HANDLE readEnd = NULL, writeEnd = NULL;
			if (!CreatePipe(&readEnd, &writeEnd, &sa, 4 * 1024 * 1024)) return false;
			SetHandleInformation(writeEnd, HANDLE_FLAG_INHERIT, 0);
			HANDLE log = CreateFileW(ci::msw::toWideString(aLogFile).c_str(), GENERIC_WRITE, FILE_SHARE_READ, &sa,
				CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			STARTUPINFOW si = {};
			si.cb = sizeof(si);
			si.dwFlags = STARTF_USESTDHANDLES;
			si.hStdInput = readEnd;
			si.hStdOutput = log != INVALID_HANDLE_VALUE ? log : NULL;
			si.hStdError = si.hStdOutput;
			PROCESS_INFORMATION pi = {};
			std::wstring cmd = ci::msw::toWideString(aCommand);
			const BOOL ok = CreateProcessW(NULL, &cmd[0], NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
			CloseHandle(readEnd);
			if (log != INVALID_HANDLE_VALUE) CloseHandle(log);
			if (!ok) { CloseHandle(writeEnd); return false; }
			CloseHandle(pi.hThread);
			mProcess = pi.hProcess;
			mPipe = writeEnd;
			return true;
		}
		bool writeAll(const uint8_t* aData, size_t aSize) {
			while (aSize > 0) {
				DWORD written = 0;
				if (!WriteFile(mPipe, aData, (DWORD)std::min<size_t>(aSize, 1 << 24), &written, NULL) || written == 0) return false;
				aData += written;
				aSize -= written;
			}
			return true;
		}
		int closeAndWait() {
			if (mPipe) { CloseHandle(mPipe); mPipe = NULL; }
			DWORD code = 1;
			if (mProcess) {
				WaitForSingleObject(mProcess, INFINITE);
				GetExitCodeProcess(mProcess, &code);
				CloseHandle(mProcess);
				mProcess = NULL;
			}
			return (int)code;
		}
#else
		bool launch(const std::string& aCommand, const std::string& aLogFile) {
			mPipe = popen((aCommand + " > " + quote(aLogFile) + " 2>&1").c_str(), "w");
			return mPipe != nullptr;
		}
		bool writeAll(const uint8_t* aData, size_t aSize) {
			return fwrite(aData, 1, aSize, mPipe) == aSize;
		}
		int closeAndWait() {
			if (!mPipe) return 1;
			const int status = pclose(mPipe);
			mPipe = nullptr;
			return status == 0 ? 0 : (status < 0 ? 1 : status);
		}
#endif
	};
}
