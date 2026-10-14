# Cinder-VDRUI

The C++ history, decisions and open items for the Videodromm Cinder apps live in one file: `../Cinder-VDR/CLAUDE.md` (Cinder-VDRUI entries are there too, since most changes touch both blocks). Read it before changing anything here. The `repos` root CLAUDE.md has the overview, the machines and the sync rules.

### Changed (2026-10-11, Web panel adds host=cinder)

`VDUIHtmlPage` opens the WebApp URL with `host=cinder` added (before any `#fragment`); the WebApp then draws its shader previews once a second (WebApp `lib/host.ts`). Also see Cinder-VDR's CLAUDE.md (2026-10-11): fbo previews and unused Spout senders refresh once a second.

### Added (2026-10-14, video export)

`include/VDVideoExport.h` (header-only): the Export panel/button, recording the output window to `<repos>/export/*.mp4` through ffmpeg. Details in `../Cinder-VDR/CLAUDE.md` ("video export of the output window") and `security-review-2026-10-14.md`.
