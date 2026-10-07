#pragma once
#include "cinder/Cinder.h"
#if defined( CINDER_MSW )
#if ! defined( WIN32_LEAN_AND_MEAN )
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
// WIN32_LEAN_AND_MEAN leaves these out of windows.h
#include <ole2.h>
#include <shellapi.h>
#include "cinder/Filesystem.h"
#include "cinder/Vector.h"
#include <atomic>
#include <functional>
#include <vector>
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")

namespace videodromm
{
	// Windows OLE drop target for files dragged from Explorer. Cinder's own fileDrop
	// (WM_DROPFILES) only reports the final drop; this also reports the position while the files
	// hover the window, so the UI can highlight where they'd land (an fbo pane, or the texture pool).
	// Once registered, Explorer drops come through here instead of Cinder's fileDrop.
	// Positions are client-area pixels, the space ImGui windows use in this app.
	class VDFileDropTarget : public IDropTarget {
	public:
		// called on drop, on the UI thread (OLE dispatches through the window's message loop)
		std::function<void(const std::vector<ci::fs::path>&, const ci::ivec2&)>	onDrop;

		~VDFileDropTarget() { revoke(); }

		bool registerWindow(HWND aHwnd) {
			if (mHwnd) return true;
			HRESULT hr = ::OleInitialize(nullptr);
			if (FAILED(hr)) return false;
			mOleInitialized = true;
			if (FAILED(::RegisterDragDrop(aHwnd, this))) return false;
			mHwnd = aHwnd;
			return true;
		}
		void revoke() {
			if (mHwnd) {
				::RevokeDragDrop(mHwnd);
				mHwnd = nullptr;
			}
			if (mOleInitialized) {
				::OleUninitialize();
				mOleInitialized = false;
			}
		}
		bool		isDragging() const { return mDragging; }
		ci::ivec2	getPos() const { return mPos; }

		// IUnknown: lifetime is owned by VDUI (unique_ptr), the count only keeps OLE happy
		HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
			if (riid == IID_IUnknown || riid == IID_IDropTarget) {
				*ppv = static_cast<IDropTarget*>(this);
				AddRef();
				return S_OK;
			}
			*ppv = nullptr;
			return E_NOINTERFACE;
		}
		ULONG STDMETHODCALLTYPE AddRef() override { return ++mRefCount; }
		ULONG STDMETHODCALLTYPE Release() override { return --mRefCount; }

		// IDropTarget
		HRESULT STDMETHODCALLTYPE DragEnter(IDataObject* aData, DWORD, POINTL aPt, DWORD* aEffect) override {
			FORMATETC format = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
			mAccept = aData && aData->QueryGetData(&format) == S_OK;
			mDragging = mAccept;
			updatePos(aPt);
			*aEffect = mAccept ? DROPEFFECT_COPY : DROPEFFECT_NONE;
			return S_OK;
		}
		HRESULT STDMETHODCALLTYPE DragOver(DWORD, POINTL aPt, DWORD* aEffect) override {
			updatePos(aPt);
			*aEffect = mAccept ? DROPEFFECT_COPY : DROPEFFECT_NONE;
			return S_OK;
		}
		HRESULT STDMETHODCALLTYPE DragLeave() override {
			mDragging = false;
			return S_OK;
		}
		HRESULT STDMETHODCALLTYPE Drop(IDataObject* aData, DWORD, POINTL aPt, DWORD* aEffect) override {
			mDragging = false;
			updatePos(aPt);
			*aEffect = DROPEFFECT_NONE;
			if (!mAccept || !aData) return S_OK;
			FORMATETC format = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
			STGMEDIUM medium = {};
			if (aData->GetData(&format, &medium) != S_OK) return S_OK;
			std::vector<ci::fs::path> files;
			HDROP drop = static_cast<HDROP>(::GlobalLock(medium.hGlobal));
			if (drop) {
				UINT count = ::DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0);
				for (UINT i = 0; i < count; i++) {
					UINT length = ::DragQueryFileW(drop, i, nullptr, 0);
					std::wstring name(length + 1, L'\0');
					::DragQueryFileW(drop, i, &name[0], length + 1);
					name.resize(length);
					files.push_back(ci::fs::path(name));
				}
				::GlobalUnlock(medium.hGlobal);
			}
			::ReleaseStgMedium(&medium);
			if (!files.empty() && onDrop) {
				onDrop(files, mPos);
				*aEffect = DROPEFFECT_COPY;
			}
			return S_OK;
		}
	private:
		void updatePos(POINTL aPt) {
			POINT p = { aPt.x, aPt.y };
			if (mHwnd) ::ScreenToClient(mHwnd, &p);
			mPos = ci::ivec2(p.x, p.y);
		}
		HWND					mHwnd = nullptr;
		bool					mOleInitialized = false;
		std::atomic<ULONG>		mRefCount{ 1 };
		bool					mAccept = false;
		bool					mDragging = false;
		ci::ivec2				mPos;
	};
}
#endif
