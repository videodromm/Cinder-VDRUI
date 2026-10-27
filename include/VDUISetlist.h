#pragma once

#if defined( _WIN32 ) && ! defined( WIN32_LEAN_AND_MEAN )
// must be defined before the first Windows header (pulled in by cinder/app/App.h) is seen,
// otherwise asio/websocketpp later in the translation unit hits "WinSock.h has already been included"
#define WIN32_LEAN_AND_MEAN
#endif
#include "cinder/app/App.h"
#include "cinder/Json.h"

// UserInterface
#include "cinder/CinderImGui.h"
// Session
#include "VDSessionFacade.h"
// Uniforms
#include "VDUniforms.h"
// Params
#include "VDParams.h"

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <cstdio>
#include <fstream>

namespace videodromm
{
	typedef std::shared_ptr<class VDUISetlist> VDUISetlistRef;

	// imgui panel for the show's setlist, the native twin of the WebApp's Setlist pane
	// (WebApp/src/panes/SetlistPane.tsx). A setlist is assets/<name>.json
	// {"name","title","project","tracks":[{"title","folder"}]}, each folder being assets/glsl/<folder>;
	// the current one is "settings.setlist" in assets/mix.json (TSWebsocketServer routes/setlists
	// reads the same files). Read from disk, so it works without the server.
	// Clicking a track, or Previous/Next, loads its folder (VDSession::loadFolder), which the
	// WebApp follows through the folder sync. Picking another setlist here isn't saved.
	// Header-only, so the apps' Visual Studio and Xcode projects need no new file.
	class VDUISetlist
	{
	public:
		VDUISetlist(VDUniformsRef aVDUniforms, VDSessionFacadeRef aVDSession)
			: mVDUniforms(aVDUniforms), mVDSession(aVDSession), mVDParams(VDParams::create()) {}
		static VDUISetlistRef	create(VDUniformsRef aVDUniforms, VDSessionFacadeRef aVDSession)
		{
			return std::shared_ptr<VDUISetlist>(new VDUISetlist(aVDUniforms, aVDSession));
		}
		// a section of VDUI's tabbed "Panel" (2026-10-27), drawn inside its window
		void RunContent() {
			const float uiScale = mVDUniforms->getUniformValue(mVDUniforms->IUISCALE);
			{
				if (!mScanned) scan();
				const std::string loaded = mVDSession->getLoadedFolder();

				// setlist picker
				ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 70.0f * uiScale);
				const char* preview = mCurrent < mSetlists.size() ? mSetlists[mCurrent].title.c_str() : "no setlist in assets";
				if (ImGui::BeginCombo("##setlist", preview)) {
					for (size_t i = 0; i < mSetlists.size(); i++) {
						snprintf(buf, sizeof(buf), "%s (%d)##setlist%d", mSetlists[i].title.c_str(), (int)mSetlists[i].tracks.size(), (int)i);
						if (ImGui::Selectable(buf, i == mCurrent)) {
							mCurrent = i;
							mPicked = -1;
						}
					}
					ImGui::EndCombo();
				}
				ImGui::PopItemWidth();
				ImGui::SameLine();
				if (ImGui::Button("Refresh")) scan();
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Re-read assets/*.json and mix.json's settings.setlist");

				if (mCurrent < mSetlists.size()) {
					const auto& tracks = mSetlists[mCurrent].tracks;
					const int active = activeIndex(tracks, loaded);
					if (ImGui::Button("< Previous") && !tracks.empty()) pick(tracks, active <= 0 ? 0 : active - 1, -1);
					ImGui::SameLine();
					if (ImGui::Button("Next >") && !tracks.empty()) pick(tracks, active < 0 ? 0 : std::min(active + 1, (int)tracks.size() - 1), 1);
					ImGui::SameLine();
					ImGui::TextColored(ImColor(150, 150, 150), "%d/%d", active + 1, (int)tracks.size());

					ImGui::Separator();
					ImGui::BeginChild("##setlisttracks", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
					for (int i = 0; i < (int)tracks.size(); i++) {
						const auto& t = tracks[i];
						const bool isActive = (i == active);
						if (isActive) ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0, 160, 0, 255));
						if (!t.exists) ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor(150, 150, 150, 255));
						snprintf(buf, sizeof(buf), "%2d  %s##track%d", i + 1, t.title.c_str(), i);
						if (ImGui::Button(buf, ImVec2(-FLT_MIN, 0.0f)) && t.exists) pick(tracks, i, 1);
						if (!t.exists) ImGui::PopStyleColor(1);
						if (isActive) ImGui::PopStyleColor(1);
						if (ImGui::IsItemHovered()) {
							if (!t.exists) ImGui::SetTooltip("assets/glsl/%s is missing", t.folder.c_str());
							else ImGui::SetTooltip(isActive ? "Loaded: %s" : "Load %s (the WebApp follows)", t.folder.c_str());
						}
					}
					ImGui::EndChild();
				}
				else {
					ImGui::TextColored(ImColor(150, 150, 150), "No setlist: add assets/<name>.json with a \"tracks\" array");
				}
			}
		}
	private:
		struct Track {
			std::string	title;
			std::string	folder;
			bool		exists = false;
		};
		struct Setlist {
			std::string			name;
			std::string			title;
			std::vector<Track>	tracks;
		};
		VDUniformsRef				mVDUniforms;
		VDSessionFacadeRef			mVDSession;
		VDParamsRef					mVDParams;
		std::vector<Setlist>		mSetlists;
		size_t						mCurrent = 0;
		// the entry picked last: two entries can share a folder (zombification / dead or alive)
		int							mPicked = -1;
		bool						mScanned = false;
		char						buf[256];

		// same rules as TSWebsocketServer routes/setlists: a name [A-Za-z0-9_-]{1,64}
		static bool isName(const std::string& s) {
			if (s.empty() || s.size() > 64) return false;
			for (char c : s) if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-')) return false;
			return true;
		}
		// a bare folder name, nothing outside assets
		static bool isFolderName(const std::string& s) {
			if (s.empty() || s.size() > 64 || s.find("..") != std::string::npos || s.front() == '.' || s.front() == ' ') return false;
			for (char c : s) if (!(std::isalnum(static_cast<unsigned char>(c)) || c == ' ' || c == '_' || c == '-' || c == '.')) return false;
			return true;
		}
		static ci::Json readJson(const ci::fs::path& file) {
			std::ifstream in(file);
			if (!in) return ci::Json();
			// no exceptions: a half-synced or invalid file is skipped
			ci::Json j = ci::Json::parse(in, nullptr, false);
			return j.is_discarded() ? ci::Json() : j;
		}
		void scan() {
			mScanned = true;
			const std::string previous = mCurrent < mSetlists.size() ? mSetlists[mCurrent].name : "";
			mSetlists.clear();
			mCurrent = 0;
			mPicked = -1;
			try {
				const ci::fs::path assets = ci::app::getAssetPath("");
				if (assets.empty()) return;
				for (const auto& entry : ci::fs::directory_iterator(assets)) {
					const ci::fs::path p = entry.path();
					if (!entry.is_regular_file() || p.extension() != ".json") continue;
					const std::string name = p.stem().string();
					if (!isName(name)) continue;
					const ci::Json j = readJson(p);
					if (!j.is_object() || !j.contains("tracks") || !j["tracks"].is_array()) continue;
					Setlist s;
					s.name = name;
					s.title = j.contains("title") && j["title"].is_string() ? j["title"].get<std::string>() : name;
					for (const auto& t : j["tracks"]) {
						if (!t.is_object() || !t.contains("folder") || !t["folder"].is_string()) continue;
						Track track;
						track.folder = t["folder"].get<std::string>();
						track.title = t.contains("title") && t["title"].is_string() ? t["title"].get<std::string>() : track.folder;
						track.exists = isFolderName(track.folder) && ci::fs::is_directory(assets / "glsl" / track.folder);
						s.tracks.push_back(track);
					}
					mSetlists.push_back(s);
				}
				std::sort(mSetlists.begin(), mSetlists.end(), [](const Setlist& a, const Setlist& b) { return a.name < b.name; });
				// the one picked before a Refresh, else mix.json's settings.setlist
				std::string wanted = previous;
				if (wanted.empty()) {
					const ci::Json mix = readJson(assets / "mix.json");
					if (mix.is_object() && mix.contains("settings") && mix["settings"].is_object()
						&& mix["settings"].contains("setlist") && mix["settings"]["setlist"].is_string()) wanted = mix["settings"]["setlist"].get<std::string>();
				}
				for (size_t i = 0; i < mSetlists.size(); i++) if (mSetlists[i].name == wanted) mCurrent = i;
			}
			catch (const std::exception& e) {
				CI_LOG_W("setlist scan: " << e.what());
			}
		}
		// the highlighted entry: the one picked here if it's still the loaded folder, else the
		// first entry with the loaded folder (picked in the WebApp or the Folders panel), else -1
		int activeIndex(const std::vector<Track>& tracks, const std::string& loaded) const {
			if (mPicked >= 0 && mPicked < (int)tracks.size() && tracks[mPicked].folder == loaded) return mPicked;
			for (int i = 0; i < (int)tracks.size(); i++) if (tracks[i].folder == loaded) return i;
			return -1;
		}
		// loads entry i, skipping missing folders in direction step
		void pick(const std::vector<Track>& tracks, int i, int step) {
			while (i >= 0 && i < (int)tracks.size() && !tracks[i].exists) i += step;
			if (i < 0 || i >= (int)tracks.size()) return;
			mPicked = i;
			if (tracks[i].folder != mVDSession->getLoadedFolder()) mVDSession->loadFolder(tracks[i].folder);
		}
	};
}
