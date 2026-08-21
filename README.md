# QtMonitor

> A real-time object detection and tracking desktop application built with Qt C++ and ONNX Runtime, supporting YOLOv26n inference and IOU greedy tracking on video streams.

<p align="center">
  <a href="https://opensource.org/licenses/MIT"><img src="https://img.shields.io/badge/License-MIT-yellow.svg" alt="License: MIT"></a>
  <a href="https://www.qt.io/"><img src="https://img.shields.io/badge/Qt-6.12-green.svg" alt="Qt 6.12"></a>
  <a href="https://cmake.org/"><img src="https://img.shields.io/badge/CMake-3.19+-blue.svg" alt="CMake"></a>
  <a href="docs/README.zh-CN.md"><img src="https://img.shields.io/badge/中文文档-Chinese-blue.svg" alt="中文文档"></a>
</p>

---

## 📑 Table of Contents

- [Features](#-features)
- [Demo](#-demo)
- [Getting Started](#-getting-started)
- [Technical Architecture](#-technical-architecture)
- [License](#-license)
- [Acknowledgments](#-acknowledgments)
- [Contact](#-contact)

---

## ✨ Features

- ✅ **Real-time Object Detection** - Efficient detection based on YOLOv26n model
- ✅ **IOU Greedy Tracking** - Multi-object tracking with ID management
- ✅ **Multiple Media Sources** - Camera, video files, network streams
- ✅ **GPU Accelerated Rendering** - OpenGL 4.5 hardware acceleration
- ✅ **Multi-threaded Architecture** - Non-blocking UI inference
- ✅ **Screenshot Function** - One-click save detection frames
- ✅ **Playback Control** - Pause and stop
- ❌ **Recording Function** - Planned

---

## 🎬 Demo

<p align="center">
  <img src="docs/screenshots/detection.gif" alt="Detection Demo" width="80%">
</p>

---

## 🚀 Getting Started

### Requirements

| Dependency | Version | Description |
|------------|---------|-------------|
| **Qt** | 6.12+ | Including Qt Creator, MSVC 2022 64-bit |
| **CMake** | 3.19+ | Build system |
| **Ninja** | Latest | Optional, recommended |
| **Visual Studio** | 2022 | MSVC compiler |

### 1. Install Qt Development Environment

1. Download and install **Qt Online Installer** from [Qt Official Website](https://www.qt.io/download)
2. Select the following components during installation:
   - **Developer Tools**
     - Qt Creator
   - **Qt 6.12.0** (or higher)
     - MSVC 2022 64-bit
   - **Build Tools**
     - CMake 3.19+
     - Ninja

### 2. Clone the Project

```bash
git clone https://github.com/chtholly115/QtMonitor.git
cd QtMonitor
```

### 3. Build the Project

**Option 1: Using Qt Creator**

1. Open `CMakeLists.txt`
2. Select Release configuration
3. Click the build button

**Option 2: Using Command Line**

```bash
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### 4. Deploy Runtime Files

Copy the following files to the build output directory (same level as `QtMonitor.exe`):

```
QtMonitor.exe
├── onnxruntime.dll                  # Copy from onnxruntime/lib/
├── onnxruntime_providers_shared.dll # Copy from onnxruntime/lib/
└── yolo26n.onnx                     # Model file
```

### 5. Run the Program

Double-click `QtMonitor.exe` to start the program.

**Quick Start:**

1. **Open Capture Device** - Click menu `File` → `Open Camera`, select camera device
2. **Start Detection** - Click the `Inference` button on the toolbar

You can see real-time detection results immediately!

---

## 🏗️ Technical Architecture

### System Architecture Diagram

```
┌─────────────────────────────────────────────────────────┐
│                  MainWindow (Main Thread)               │
│  ┌──────────────┬──────────────┬────────────────────┐   │
│  │  OpenGL      │   Media      │      Menu/         │   │
│  │  Widget      │  Source      │      Toolbar       │   │
│  │  (GPU Render)│  Manager     │  (User Interaction)│   │
│  └──────┬───────┴──────┬───────┴────────────────────┘   │
└─────────┼──────────────┼────────────────────────────────┘
          │              │
          │ Video Frame  │
          ▼              │
    ┌─────────────────────┐
    │ Detector (Thread)   │
    │                     │
    │  ┌───────────────┐  │
    │  │ ONNX Runtime  │  │
    │  │ + YOLOv26n    │  │
    │  └───────┬───────┘  │
    └──────────┼──────────┘
               │
          Detection Results
               │
          ┌────▼────────┐
          │   Tracker   │  ← IOU Greedy Tracking
          │ (Component) │
          └────┬────────┘
               │
          Detection Results with ID
               │
          ┌────▼───────┐
          │OpenGLWidget│
          │ (Draw Box) │
          └────────────┘
```

### Core Components

| Component | Responsibility | Key Technology |
|-----------|---------------|----------------|
| **MainWindow** | Main window, coordinates component interactions | Qt Widgets, Signals/Slots |
| **OpenGLWidget** | Video frame rendering and detection box drawing | OpenGL 4.5, GLSL Shaders |
| **MediaSourceManager** | Manages multiple media sources | Qt Multimedia (QMediaPlayer, QCamera) |
| **Detector** | ONNX inference engine | ONNX Runtime C++ API, Multi-threading |
| **Tracker** | IOU greedy tracker | Custom algorithm, ID management |
| **OpenMediaDialog** | Media source selection dialog | Qt Dialogs |

### Data Flow

**1. Video Input Flow**

```
Media Source → MediaSourceManager → QVideoFrame → OpenGLWidget (Display)
```

**2. Detection Inference Flow**

```
QVideoFrame → Detector (Thread) → Preprocessing → ONNX Runtime Inference → Postprocessing → Detection Results
```

**3. Tracking Processing Flow**

```
Detection Results → Tracker → IOU Greedy Matching → Assign/Update Track ID → Detection Results with ID
```

**4. Result Rendering Flow**

```
Detection Results with ID → OpenGLWidget → GPU Render Detection Boxes, Labels and IDs
```

### Multi-threading Design

- **Main Thread** - UI event handling, video display, user interaction
- **Inference Thread** - ONNX Runtime inference computation, non-blocking UI
- **Communication Mechanism** - Qt signals/slots mechanism, thread-safe data transfer

---

## 📄 License

This project is licensed under the [MIT License](LICENSE).

[Copyright (c) 2026 chtholly115](https://img.shields.io/badge/Copyright-2026_Chtholly-blue)

### Third-party Dependencies

| Dependency | License | Link |
|------------|---------|------|
| **ONNX Runtime** | MIT License | [License](onnxruntime/LICENSE) |
| **Qt Framework** | LGPLv3/GPLv3 | [License](https://www.qt.io/licensing) |

---

## 🙏 Acknowledgments

The implementation of this project relies on the support of the following excellent open-source projects:

- [Qt Framework](https://www.qt.io/) - Powerful cross-platform application development framework
- [ONNX Runtime](https://github.com/microsoft/onnxruntime) - High-performance machine learning inference engine developed by Microsoft
- [YOLO](https://github.com/ultralytics/ultralytics) - Industry-leading object detection model
- [OpenGL](https://www.opengl.org/) - Cross-platform graphics rendering API

---

## 📧 Contact

| Method | Link |
|--------|------|
| **Project Maintainer** | [chtholly115](https://github.com/chtholly115) |
| **Issue Feedback** | [GitHub Issues](https://github.com/chtholly115/QtMonitor/issues) |
| **Project Homepage** | [GitHub Repository](https://github.com/chtholly115/QtMonitor) |

If you encounter any problems or have suggestions for improvement during use, please submit feedback through GitHub Issues.

---

<p align="center">
  <sub>Made with ❤️ by <a href="https://github.com/chtholly115">chtholly115</a></sub>
</p>
