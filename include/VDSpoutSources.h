#pragma once
#include "cinder/Cinder.h"
#if defined( CINDER_MSW )
#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/Filesystem.h"
#include "SpoutReceiver.h"
#include "VDSessionFacade.h"
#include <map>
#include <memory>
#include <set>
#include <string>

namespace videodromm
{
	// Spout senders from other apps, received and offered in the shared texture pool as
	// "Spout in: <sender>" (any fbo can then pick one as its input texture). The sender list is
	// refreshed about once a second; each sender has its own receiver, received every frame.
	// This app's own senders (main output = the exe name, and "VDCode") are skipped: they're
	// already in the pool directly (VDUI::registerPoolSources).
	class VDSpoutSources {
	public:
		static const size_t		MAX_SOURCES = 8;

		void update(VDSessionFacadeRef aVDSession) {
			if (ci::app::getElapsedFrames() % 60 == 1) refreshSenders(aVDSession);
			for (auto& entry : mSources) {
				Source& source = entry.second;
				if (!source.texture) source.texture = ci::gl::Texture2d::create(16, 16);
				// same calls as CiSpoutIn: a size change is reported by IsUpdated(), the texture is
				// then reallocated and filled from the next frame
				if (source.receiver->ReceiveTexture(source.texture->getId(), source.texture->getTarget(), true)) {
					if (source.receiver->IsUpdated()) {
						source.texture = ci::gl::Texture2d::create(source.receiver->GetSenderWidth(), source.receiver->GetSenderHeight());
					}
					else {
						aVDSession->registerPoolTexture(poolName(entry.first), source.texture);
					}
				}
			}
		}
		static std::string poolName(const std::string& aSender) { return "Spout in: " + aSender; }

	private:
		struct Source {
			std::unique_ptr<SpoutReceiver>	receiver;
			ci::gl::Texture2dRef			texture;
		};
		std::map<std::string, Source>	mSources;
		SpoutReceiver					mLister;
		std::string						mOwnSenderName;

		void refreshSenders(VDSessionFacadeRef aVDSession) {
			if (mOwnSenderName.empty()) {
				// Spout names a sender after the executable when no name is set (the main output)
				char exePath[MAX_PATH] = { 0 };
				::GetModuleFileNameA(nullptr, exePath, MAX_PATH);
				mOwnSenderName = ci::fs::path(exePath).stem().string();
			}
			std::set<std::string> current;
			for (const auto& name : mLister.GetSenderList()) {
				if (name.empty() || name == mOwnSenderName || name == "VDCode") continue;
				current.insert(name);
			}
			// senders gone: drop their receiver and their pool entry
			for (auto it = mSources.begin(); it != mSources.end();) {
				if (current.count(it->first) == 0) {
					it->second.receiver->ReleaseReceiver();
					aVDSession->unregisterPoolTexture(poolName(it->first));
					it = mSources.erase(it);
				}
				else ++it;
			}
			for (const auto& name : current) {
				if (mSources.count(name) || mSources.size() >= MAX_SOURCES) continue;
				Source source;
				source.receiver = std::make_unique<SpoutReceiver>();
				source.receiver->SetReceiverName(name.c_str());
				mSources[name] = std::move(source);
			}
		}
	};
}
#endif
