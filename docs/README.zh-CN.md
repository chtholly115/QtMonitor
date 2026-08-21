# QtMonitor

> 基于 Qt C++ 和 ONNX Runtime 构建的实时目标检测与跟踪桌面应用程序，支持在视频流上运行 YOLOv26n 推理和 IOU 贪心跟踪。

<p align="center">
  <a href="https://opensource.org/licenses/MIT"><img src="https://img.shields.io/badge/License-MIT-yellow.svg" alt="License: MIT"></a>
  <a href="https://www.qt.io/"><img src="https://img.shields.io/badge/Qt-6.12-green.svg" alt="Qt 6.12"></a>
  <a href="https://cmake.org/"><img src="https://img.shields.io/badge/CMake-3.19+-blue.svg" alt="CMake"></a>
</p>

---

## 📑 目录

- [功能特性](#-功能特性)
- [演示](#-演示)
- [快速开始](#-快速开始)
- [技术架构](#-技术架构)
- [许可证](#-许可证)
- [致谢](#-致谢)
- [联系方式](#-联系方式)

---

## ✨ 功能特性

- ✅ **实时目标检测** - 基于 YOLOv26n 模型的高效检测
- ✅ **IOU 贪心跟踪** - 多目标跟踪与 ID 管理
- ✅ **多种媒体源支持** - 摄像头、视频文件、网络流
- ✅ **GPU 加速渲染** - OpenGL 4.5 硬件加速
- ✅ **多线程架构** - 非阻塞 UI 推理
- ✅ **截图功能** - 一键保存检测画面
- ✅ **播放控制** - 暂停和停止
- ❌ **录制功能** - 计划中

---

## 🎬 演示

<p align="center">
  <img src="screenshots/detection.gif" alt="检测演示" width="80%">
</p>

---

## 🚀 快速开始

### 环境要求

| 依赖 | 版本 | 说明 |
|------|------|------|
| **Qt** | 6.12+ | 包含 Qt Creator、MSVC 2022 64-bit |
| **CMake** | 3.19+ | 构建系统 |
| **Ninja** | 最新版 | 可选，推荐使用 |
| **Visual Studio** | 2022 | MSVC 编译器 |

### 1. 安装 Qt 开发环境

1. 从 [Qt 官网](https://www.qt.io/download) 下载并安装 **Qt Online Installer**
2. 在安装器中选择安装以下组件：
   - **Developer Tools**
     - Qt Creator
   - **Qt 6.12.0** (或更高版本)
     - MSVC 2022 64-bit
   - **Build Tools**
     - CMake 3.19+
     - Ninja

### 2. 克隆项目

```bash
git clone https://github.com/chtholly115/QtMonitor.git
cd QtMonitor
```

### 3. 构建项目

**方式一：使用 Qt Creator**

1. 打开 `CMakeLists.txt`
2. 选择 Release 配置
3. 点击构建按钮

**方式二：使用命令行**

```bash
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### 4. 部署运行时文件

将以下文件复制到构建输出目录（与 `QtMonitor.exe` 同级）：

```
QtMonitor.exe
├── onnxruntime.dll                  # 从 onnxruntime/lib/ 复制
├── onnxruntime_providers_shared.dll # 从 onnxruntime/lib/ 复制
└── yolo26n.onnx                     # 模型文件
```

### 5. 运行程序

双击 `QtMonitor.exe` 启动程序。

**快速上手：**

1. **打开捕获设备** - 点击菜单 `文件` → `打开摄像头`，选择摄像头设备
2. **启动检测** - 点击工具栏的 `推理` 按钮

即可实时看到检测结果画面！

---

## 🏗️ 技术架构

### 系统架构图

```
┌─────────────────────────────────────────────────────────┐
│                  MainWindow (主线程)                     │
│  ┌──────────────┬──────────────┬────────────────────┐   │
│  │  OpenGL      │   Media      │      Menu/         │   │
│  │  Widget      │  Source      │      Toolbar       │   │
│  │  (GPU渲染)   │  Manager     │   (用户交互)        │   │
│  └──────┬───────┴──────┬───────┴────────────────────┘   │
└─────────┼──────────────┼────────────────────────────────┘
          │              │
          │ 视频帧       │
          ▼              │
    ┌─────────────────────┐
    │  Detector (独立线程) │
    │                      │
    │  ┌───────────────┐   │
    │  │ ONNX Runtime  │   │
    │  │ + YOLOv26n    │   │
    │  └───────┬───────┘   │
    └──────────┼───────────┘
               │
          Detection 结果
               │
          ┌────▼────────┐
          │   Tracker   │  ← IOU 贪心跟踪
          │ (独立组件)   │
          └────┬────────┘
               │
          带 ID 的检测结果
               │
          ┌────▼─────┐
          │OpenGLWidget│
          │ (绘制框)  │
          └───────────┘
```

### 核心组件

| 组件 | 职责 | 关键技术 |
|------|------|----------|
| **MainWindow** | 主窗口，协调各组件交互 | Qt Widgets, 信号/槽 |
| **OpenGLWidget** | 视频帧渲染和检测框绘制 | OpenGL 4.5, GLSL Shaders |
| **MediaSourceManager** | 管理多种媒体源 | Qt Multimedia (QMediaPlayer, QCamera) |
| **Detector** | ONNX 推理引擎 | ONNX Runtime C++ API, 多线程 |
| **Tracker** | IOU 贪心跟踪器 | 自定义算法, ID 管理 |
| **OpenMediaDialog** | 媒体源选择对话框 | Qt Dialogs |

### 数据流程

**1. 视频输入流程**

```
媒体源 → MediaSourceManager → QVideoFrame → OpenGLWidget (显示)
```

**2. 检测推理流程**

```
QVideoFrame → Detector (独立线程) → 预处理 → ONNX Runtime 推理 → 后处理 → Detection 结果
```

**3. 跟踪处理流程**

```
Detection 结果 → Tracker → IOU 贪心匹配 → 分配/更新 Track ID → 带 ID 的 Detection 结果
```

**4. 结果渲染流程**

```
带 ID 的 Detection 结果 → OpenGLWidget → GPU 渲染检测框、标签和 ID
```

### 多线程设计

- **主线程** - UI 事件处理、视频显示、用户交互
- **推理线程** - ONNX Runtime 推理计算，不阻塞 UI
- **通信机制** - Qt 信号/槽机制，线程安全的数据传递

---

## 📄 许可证

本项目基于 [MIT License](LICENSE) 开源。

[Copyright (c) 2026 chtholly115](https://img.shields.io/badge/Copyright-2026_Chtholly-blue)

### 第三方依赖

| 依赖 | 许可证 | 链接 |
|------|--------|------|
| **ONNX Runtime** | MIT License | [许可证](onnxruntime/LICENSE) |
| **Qt Framework** | LGPLv3/GPLv3 | [许可证](https://www.qt.io/licensing) |

---

## 🙏 致谢

本项目的实现离不开以下优秀开源项目的支持：

- [Qt Framework](https://www.qt.io/) - 强大的跨平台应用开发框架
- [ONNX Runtime](https://github.com/microsoft/onnxruntime) - 微软开发的高性能机器学习推理引擎
- [YOLO](https://github.com/ultralytics/ultralytics) - 业界领先的目标检测模型
- [OpenGL](https://www.opengl.org/) - 跨平台的图形渲染 API

---

## 📧 联系方式

| 方式 | 链接 |
|------|------|
| **项目维护者** | [chtholly115](https://github.com/chtholly115) |
| **问题反馈** | [GitHub Issues](https://github.com/chtholly115/QtMonitor/issues) |
| **项目主页** | [GitHub Repository](https://github.com/chtholly115/QtMonitor) |

如果您在使用过程中遇到任何问题或有改进建议，欢迎通过 GitHub Issues 提交反馈。

---

<p align="center">
  <sub>Made with ❤️ by <a href="https://github.com/chtholly115">chtholly115</a></sub>
</p>
