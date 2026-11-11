# Media Formats & Protocol Support Matrix

- **Specification ID:** SPEC-FORMATS-001
- **Clean-Room Evidence Reference:** `CR-003` (docs/clean-room-log.md)
- **Target Application:** `outplayer` (Linux Qt 6 + libmpv)
- **Reference Application:** Daum PotPlayer 64-bit (Windows)
- **Engine Baseline:** `libmpv` (v0.38+) / `FFmpeg` (v6.0+) / `libass` (v0.17+)

---

## 1. Executive Summary & Architecture

The Daum PotPlayer reference implementation relies on a hybrid playback engine combining built-in decoders (derived from FFmpeg), DirectShow filters, and Media Foundation transforms. In `outplayer`, full functional parity across containers, video codecs, audio decoders, subtitle renderers, and network streaming protocols is achieved on Linux natively through `libmpv` and modern FFmpeg libraries, backed by hardware-accelerated decoding via VA-API and NVDEC.

This document establishes the comprehensive matrix of supported media types, comparing reference behavior with `outplayer` implementations.

---

## 2. Container Formats Matrix

| Container Format | Common File Extensions | MIME Type | libmpv Demuxer | Seeking Precision | PotPlayer Parity Status | Implementation Notes |
|---|---|---|---|---|---|---|
| **Matroska (MKV)** | `.mkv`, `.mka`, `.mks`, `.mk3d` | `video/x-matroska`, `audio/x-matroska` | `matroska` | Sample-accurate (Keyframe / Exact) | Full Parity | Full chapter, multi-track audio/subtitle, and attachment support (embedded fonts). |
| **MP4 / MOV (ISO BMFF / QuickTime)** | `.mp4`, `.m4v`, `.m4a`, `.mov`, `.qt` | `video/mp4`, `video/quicktime`, `audio/mp4` | `mov,mp4,m4a,3gp,3g2,mj2` | Sample-accurate (with moov/sidx index) | Full Parity | Fast-start fragment playback, Dolby Vision box parsing, timed text metadata. |
| **AVI (Audio Video Interleaved)** | `.avi`, `.divx` | `video/x-msvideo` | `avi` | Keyframe (Index required) | Full Parity | Automatic ODML index reconstruction for truncated or damaged AVI files. |
| **WebM** | `.webm` | `video/webm`, `audio/webm` | `matroska,webm` | Sample-accurate | Full Parity | VP8, VP9, AV1, Vorbis, Opus streams; live DASH-WebM streaming support. |
| **MPEG-TS (Transport Stream)** | `.ts`, `.m2ts`, `.mts`, `.trp` | `video/mp2t` | `mpegts` | Timestamp / PCR-based | Full Parity | Broadcast stream handling, multi-program PID selection, Blu-ray BDAV/BDMV structures. |
| **Flash Video (FLV)** | `.flv`, `.f4v` | `video/x-flv` | `flv` | Keyframe / Timestamp | Full Parity | Legacy streaming container; H.264/AAC and Sorenson Spark support. |
| **Ogg Media (OGG)** | `.ogg`, `.ogv`, `.oga`, `.ogx` | `application/ogg`, `video/ogg`, `audio/ogg` | `ogg` | Granule-based | Full Parity | Theora video, Vorbis/Opus audio, Kate/OggText subtitles. |
| **Windows Media** | `.wmv`, `.wma`, `.asf` | `video/x-ms-wmv`, `audio/x-ms-wma`, `video/x-ms-asf` | `asf` | Packet / Timestamp | Full Parity | ASF packet stream demuxing; WMV1/2/3, WVC1, WMA9/10/Pro. |
| **MPEG Program Stream** | `.mpg`, `.mpeg`, `.vob`, `.evo`, `.m2v` | `video/mpeg` | `mpeg` | Timestamp / NavPack | Full Parity | DVD VOB navigation packet extraction; multi-angle and audio selection. |
| **RealMedia** | `.rm`, `.rmvb`, `.ra` | `application/vnd.rn-realmedia` | `rm` | Keyframe | Full Parity | RV30, RV40 video, Cook/RA audio demuxing. |
| **FLAC Container (FLAC)** | `.flac` | `audio/flac` | `flac` | Sample-accurate | Full Parity | Native metadata block parsing, embedded cue sheets, embedded Vorbis comments. |
| **MP3 (MPEG Audio Layer III)** | `.mp3` | `audio/mpeg` | `mp3` | Bitrate-dependent / Xing/Info TOC | Full Parity | ID3v1, ID3v2.3, ID3v2.4 tag extraction and embedded album art extraction. |
| **Advanced Audio Coding** | `.aac` | `audio/aac` | `aac` | ADTS frame based | Full Parity | Raw ADTS stream demuxing; gapless playback support. |
| **Monkey's Audio** | `.ape` | `audio/x-ape` | `ape` | Sample-accurate | Full Parity | Lossless compression with APE tags v1/v2. |
| **WavPack** | `.wv` | `audio/x-wavpack` | `wv` | Sample-accurate | Full Parity | Lossless / lossy hybrid WavPack stream handling. |
| **Waveform Audio** | `.wav` | `audio/wav`, `audio/x-wav` | `wav` | Sample-accurate | Full Parity | PCM, IEEE Float, extensible WAVEFORMATEXTENSIBLE multichannel layouts. |
| **3GP / 3G2** | `.3gp`, `.3g2` | `video/3gpp`, `video/3gpp2` | `mov,mp4` | Keyframe / Sample | Full Parity | Mobile media streams; AMR-NB/AMR-WB audio and H.263/H.264 video. |
| **Raw Stdin Stream** | `-` (Standard Input) | `application/octet-stream` | Dynamic / Pipe probe | Unseekable / Buffer-only | Full Parity | Pipe streaming from CLI (`stdin`), stdin buffer caching. |

---

## 3. Video Codecs & Hardware Acceleration Matrix

`outplayer` targets zero-copy hardware acceleration using modern Linux graphics APIs (`VA-API` for Intel/AMD and `NVDEC` for NVIDIA, with optional Vulkan Video).

| Video Codec | Format Standard | libmpv/FFmpeg Decoder | Hardware Acceleration (Linux) | Color Formats & Bit Depths | PotPlayer Parity Status | Implementation Notes |
|---|---|---|---|---|---|---|
| **H.264 / AVC** | ISO/IEC 14496-10 | `h264` | `vaapi`, `nvdec`, `vulkan` | 4:2:0, 4:2:2, 4:4:4 (8-bit, 10-bit Hi10P) | Full Parity | Baseline, Main, High, High 10 profiles. Hi10P decodes via CPU if GPU lacks profile support. |
| **HEVC / H.265** | ISO/IEC 23008-2 | `hevc` | `vaapi`, `nvdec`, `vulkan` | 8-bit, 10-bit (Main, Main 10), HDR10, HLG | Full Parity | Native HDR tone mapping using libmpv GLSL compute shaders (`target-prim=auto`). |
| **AV1** | AOMedia AV1 | `av1`, `libdav1d` | `vaapi`, `nvdec`, `vulkan` | 8-bit, 10-bit, 12-bit (Main, High), HDR10 | Full Parity | High-performance CPU fallback powered by multithreaded `libdav1d`. |
| **VP9** | Google VP9 | `vp9` | `vaapi`, `nvdec` | Profile 0 (8-bit), Profile 2 (10-bit HDR) | Full Parity | Standard YouTube high-resolution video streams. |
| **VP8** | RFC 6386 | `vp8` | `vaapi`, `nvdec` | 8-bit 4:2:0 | Full Parity | Legacy WebM video playback. |
| **MPEG-2 Video** | ISO/IEC 13818-2 | `mpeg2video` | `vaapi`, `nvdec` | Main, Simple (8-bit) | Full Parity | DVD-Video and digital broadcast television streams. Deinterlacing via `yadif`/`bwdif`/`vaapi-vpp`. |
| **MPEG-4 Part 2** | ISO/IEC 14496-2 | `mpeg4` | None (CPU optimized) | Simple, Advanced Simple (DivX 4/5/6, Xvid) | Full Parity | B-frame packed bitstream unpacking. |
| **Apple ProRes** | Apple ProRes | `prores` | None (Multi-thread CPU) | 422 Proxy/LT/Standard/HQ, 4444, 4444 XQ (10/12-bit) | Full Parity | Professional editing and mastering playback. |
| **VC-1 / WMV3** | SMPTE 421M | `vc1` | `vaapi`, `nvdec` | Simple, Main, Advanced Profile (WVC1) | Full Parity | Legacy Blu-ray and Windows Media Video streams. |
| **Theora** | Xiph.Org | `theora` | None (CPU) | 4:2:0 8-bit | Full Parity | Open-source video codec in Ogg containers. |
| **Motion JPEG** | ISO/IEC 10918-1 | `mjpeg` | `vaapi`, `nvdec` | YUV420P, YUV422P, YUV444P | Full Parity | Video capture streams from USB webcams. |
| **DNxHD / DNxHR** | SMPTE VC-3 | `dnxhd` | None (Multi-thread CPU) | 8-bit, 10-bit, 12-bit intra-frame | Full Parity | Avid professional mastering codec. |

---

## 4. Audio Codecs & Bitstreaming Matrix

`outplayer` routes audio through PulseAudio, PipeWire, or ALSA. It supports multichannel bitstreaming (S/PDIF, HDMI pass-through) for home theater configurations.

| Audio Codec | libmpv/FFmpeg Decoder | Max Channels | Passthrough / Bitstream | Sample Rates | PotPlayer Parity Status | Implementation Notes |
|---|---|---|---|---|---|---|
| **AAC** (LC, HE-AAC, LD) | `aac` | Up to 7.1 | LPCM decode | 8 kHz – 96 kHz | Full Parity | Gapless playback, spectral band replication (SBR), parametric stereo (PS). |
| **AC-3 (Dolby Digital)** | `ac3` | 5.1 | Supported (IEC 61937) | 32, 44.1, 48 kHz | Full Parity | Dynamic range compression (DRC) controls, AC-3 pass-through over optical/HDMI. |
| **E-AC-3 (Dolby Digital Plus)** | `eac3` | 7.1 / Atmos | Supported (IEC 61937) | Up to 48 kHz | Full Parity | Enhanced AC-3 bitstream passthrough to AV receivers. |
| **TrueHD** | `truehd` | 7.1 / Atmos | Supported (IEC 61937) | Up to 192 kHz | Full Parity | Lossless audio decompression; Dolby Atmos object metadata preservation. |
| **DTS (Core)** | `dts` | 5.1 | Supported (IEC 61937) | 44.1, 48 kHz | Full Parity | Standard DTS Coherent Acoustics digital audio. |
| **DTS-HD MA / HRA** | `dts` | 7.1 / DTS:X | Supported (IEC 61937) | Up to 192 kHz | Full Parity | DTS-HD Master Audio bitstream extraction and lossless decoding. |
| **FLAC** | `flac` | Up to 8 channels | LPCM decode | Up to 192 kHz / 24-bit | Full Parity | Native lossless decoding, cue sheet splitting. |
| **Opus** | `opus`, `libopus` | Up to 8 channels | LPCM decode | 48 kHz native | Full Parity | Voice and streaming codec with ultra-low latency. |
| **MP3 (Layer 3)** | `mp3float` | 2.0 | LPCM decode | 16 kHz – 48 kHz | Full Parity | Full VBR/CBR support with gapless LAME tag compensation. |
| **Vorbis** | `vorbis` | Up to 8 channels | LPCM decode | 8 kHz – 96 kHz | Full Parity | Ogg Vorbis streaming and media playback. |
| **ALAC** | `alac` | Up to 7.1 | LPCM decode | Up to 192 kHz / 24-bit | Full Parity | Apple Lossless Audio Codec. |
| **LPCM / WAV** | `pcm_*` | Up to 32 channels | LPCM decode | 8 kHz – 384 kHz / 32-bit | Full Parity | Uncompressed integer and IEEE floating-point audio. |
| **Monkey's Audio (APE)** | `ape` | 2.0 | LPCM decode | Up to 96 kHz / 24-bit | Full Parity | Fast, Normal, High, Extra High, Insane modes. |
| **WavPack** | `wavpack` | Up to 8 channels | LPCM decode | Up to 192 kHz / 32-bit | Full Parity | Lossless audio with correction file (`.wvc`) handling. |

---

## 5. Subtitle Formats & Rendering Matrix

Subtitle rendering is governed by `libass` for styled script formats, supplemented by native bitmap blitters for DVD and Blu-ray graphic overlays.

| Subtitle Format | Extensions | Engine / Parser | Rendering Capabilities | PotPlayer Parity Status | Implementation Notes |
|---|---|---|---|---|---|
| **ASS/SSA (SubStation Alpha)** | `.ass`, `.ssa` | `libass` | Full script styling, vector drawing, fonts, positioning, alpha blending, karaoke | Full Parity | Industry standard for stylized anime/film subtitles. Font cache via Fontconfig. |
| **SRT (SubRip)** | `.srt` | `libass` (converted) | Configurable font, size, outlines, colors, margin positioning | Full Parity | Plain text formatting with basic HTML tag interpretation (`<i>`, `<b>`, `<u>`, `<font>`). |
| **WebVTT** | `.vtt` | `libass` / mpv | CSS positioning, cue timing, formatting tags | Full Parity | Web standard for HTML5 media and HLS/DASH subtitle streams. |
| **SAMI** | `.smi`, `.sami` | Internal parser | Synchronized Accessible Media Interchange (HTML-like) | Full Parity | Historically popular format in Korean media ecosystems (primary PotPlayer format). |
| **PGS (Blu-ray Subtitles)** | Embedded in M2TS/MKV | Native bitmap blitter | High-resolution indexed bitmaps | Full Parity | Scaled rendering matching video canvas geometry. |
| **VobSub (DVD Subtitles)** | `.sub` + `.idx`, embedded VOB | Native bitmap blitter | DVD 4-color palette subpictures | Full Parity | Palette remapping and aspect-ratio compensation. |
| **MicroDVD** | `.sub` | Internal parser | Frame-number indexed text subtitles | Full Parity | Legacy frame-based synchronization. |
| **LRC (Timed Lyrics)** | `.lrc` | Internal parser | Line and word-by-word karaoke synchronization | Full Parity | Audio music player lyric overlay on OSD. |
| **MP4 Timed Text (tx3g)** | Embedded MP4 | Internal parser | Basic styling and positioning | Full Parity | iOS/QuickTime standard mobile subtitles. |

---

## 6. Network Protocols & Streaming Matrix

| Streaming Protocol | URL Schemes | libmpv / FFmpeg Handler | Authentication & Headers | PotPlayer Parity Status | Implementation Notes |
|---|---|---|---|---|---|
| **HTTP / HTTPS** | `http://`, `https://` | `http`, `tls` | User-Agent, Referer, Cookies, Custom Headers | Full Parity | Direct file streaming, chunked transfer, range-request seeking. |
| **HLS (HTTP Live Streaming)** | `.m3u8`, `hls+http://` | `hls` | AES-128 decryption, adaptive bitrate selection | Full Parity | Live and VOD master playlist parsing; automatic track switching. |
| **DASH (Dynamic Adaptive Streaming over HTTP)** | `.mpd`, `dash+http://` | `dash` | Segment timeline, adaptation set selection | Full Parity | Multi-stream video/audio dynamic adaptation. |
| **RTSP (Real Time Streaming Protocol)** | `rtsp://` | `rtsp` | Basic / Digest auth, TCP/UDP transport | Full Parity | Security cameras, surveillance feeds, IP broadcasts. |
| **RTMP / RTMPS** | `rtmp://`, `rtmps://` | `librtmp`, `rtmp` | Flash streaming tokens, live broadcast | Full Parity | Live media ingestion and playout feeds. |
| **FTP / SFTP** | `ftp://`, `sftp://` | `ftp`, `sftp` | Username / password authentication | Full Parity | Remote file access over file transfer protocols. |
| **SMB / CIFS** | `smb://` | `libsmbclient` | Windows domain / NTLM credentials | Full Parity | Local Area Network NAS and shared folder playback. |
| **Standard Input Pipe** | `-` | `pipe` | Stdin streaming from external CLIs (e.g. yt-dlp) | Full Parity | `yt-dlp -o - <URL> \| outplayer -` |

---

## 7. Linux-Specific Hardware Acceleration Architecture

In PotPlayer (Windows), hardware acceleration depends on Microsoft DXVA2 and Direct3D 11 Video Acceleration (`D3D11VA`), communicating with proprietary GPU vendor drivers.

In `outplayer` (Linux), the playback pipeline leverages Linux-standard hardware decoding through `libmpv`'s `hwdec` subsystem:

1. **VA-API (`hwdec=vaapi`)**:
   - Primary standard on Intel GPUs (via `intel-media-driver` / `iHD`) and AMD GPUs (via `radeonsi` Mesa driver).
   - Zero-copy surface sharing directly with OpenGL (`EGL_EXT_image_dma_buf_import`) and Vulkan surfaces.
2. **NVDEC (`hwdec=nvdec` / `hwdec=nvdec-copy`)**:
   - Proprietary NVIDIA acceleration via the CUDA/NVDEC video decoding engine.
3. **Vulkan Video (`hwdec=vulkan`)**:
   - Modern vendor-neutral standard for H.264, H.265, and AV1 decoding directly on Vulkan graphics queues.
4. **Deinterlacing Engines**:
   - Hardware deinterlacing via VA-API VPP (`vaapi-vpp`) and high-quality software fallback via `bwdif` (Bob Weaver Deinterlacing Filter) and `yadif`.
