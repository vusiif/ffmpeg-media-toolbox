# FFmpeg 媒体工具箱 (ffmedia-toolbox)

<div align="center">

![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg?style=flat-square)
![Qt](https://img.shields.io/badge/Qt-6.11%20%7C%206.12-brightgreen.svg?style=flat-square)
![Platform](https://img.shields.io/badge/Platform-Windows%20x64-0078d7.svg?style=flat-square)
![License](https://img.shields.io/badge/License-MIT-orange.svg?style=flat-square)

**基于 Qt 6 Widgets 与 ElaWidgetTools 构建的现代化、全能音视频与图像处理工作台**

[特性概览](#-核心特性) • [技术架构](#-技术架构) • [编译与运行](#-编译与构建) • [目录结构](#-目录结构) • [许可证](#-开源协议)

</div>

---

## 📖 项目简介

**FFmpeg 媒体工具箱 (`ffmedia-toolbox`)** 是一款面向创作者、剪辑师与技术极客的现代化桌面多媒体工具。软件基于最新的 **C++23** 标准与 **Qt 6 (Widgets / Multimedia / Concurrent)** 开发，采用 **ElaWidgetTools** 呈现典雅且响应迅捷的 **Windows Fluent Design** 交互界面，深度整合 **FFmpeg** 命令行生态与 **Untrunc** 视频损坏重建引擎，提供从常规音视频转换、批量转码、画面后期裁剪，到高级 HDR 色彩调优与损坏视频结构级修复的全流程解决方案。

---

## ✨ 核心特性

### 1. 仪表盘与运行环境 (Fluent Dashboard)
* **硬件加速探测**：实时识别 NVIDIA NVENC、Intel QSV、AMD AMF、Apple VideoToolbox、VAAPI 等 GPU 硬解/硬编环境。
* **引擎状态监测**：一键检测本地 FFmpeg 核心版本、可用音视频编解码器数，以及 Untrunc 视频修复组件就绪状态。
* **即时队列状态**：实时监控任务队列中的运行中、等待中与已完成任务数量，支持直达详情页。
* **核心功能直达**：主页专属 Fluent 卡片入口，一键前往格式转换、视频工具箱、媒体修复与批处理页面。

### 2. 格式转换与压制 (Format Conversion)
* **格式互转**：支持 MP4、MKV、MOV、WebM、AVI、FLV、WMV 以及 MP3、AAC、FLAC、WAV、Opus 等主流容器格式。
* **智能画质调优**：精准调节 CRF 参数，支持音视频无损流复制 (Stream Copy)。
* **高级色彩与 HDR**：支持色彩空间配置文件映射（BT.709、BT.2020 PQ、BT.2020 HLG、BT.601），集成 HDR to SDR 色调映射算法（Hable、Mobius、Reinhard）。
* **音频精细配置**：自定义音频比特率、采样率、声道布局，支持 EBU R128 音频响度标准化 (Loudnorm)。
* **硬件转码回退保护**：当 GPU 显存不足或硬件编码驱动异常失败时，系统自动无缝回退至 CPU 软编重试，保障批处理无阻断。
* **参数预设管理**：支持自定义转码配置的存储、加载与快速删除。

### 3. 视频工具箱 (Video Toolbox)
采用分段式 `ElaPivot` 标签页组织，包含 11 项实用工具：
* **剪辑与调整 (Clip & Adjust)**：
  * **精准截取 (Trim)**：支持高精度起止时间毫秒微调与流复制秒切。
  * **画面变换 (Adjust)**：按像素裁剪 (Crop)、分辨率等比/自定义缩放 (Resize) 以及 90°/180°/270° 旋转。
  * **视频拼接 (Join)**：多个同规格或异构视频快速串联。
* **音频处理 (Audio Tools)**：
  * **音轨提取**：一键导出视频音轨至 MP3、AAC、FLAC 等纯音频格式。
  * **静音消除**：无损剥离视频原生音轨。
* **效果与合成 (Overlay & Subtitles)**：
  * **图片水印**：支持 PNG/WebP 水印图叠加、预设方位（九宫格方位）与透明度调节。
  * **画中画合成 (PiP)**：支持副视频尺寸指定、外边距设置与角落自由贴合。
  * **字幕封装与压制**：支持 SRT、ASS、SSA、VTT 字幕流软封装以及滤镜硬字幕烧录。
* **动图与帧提取 (Frames & Animation)**：
  * **高清截屏**：在指定时间戳抓取 PNG / JPEG / WebP 原始画面。
  * **制作 GIF**：自定义时间区间、输出帧率与宽度，自动优化调色板。
  * **序列帧互转**：支持视频切分为连续序列帧，或多张帧图合成视频。

### 4. 视频损坏智能修复 (Video Repair)
* **损坏坏帧全量扫描**：集成 `CorruptedVideoScanner`，全速离线遍历视频流，精准捕捉所有破损包并定位时间段区间。
* **损坏跳帧播放器 (`VideoPlayerWidget`)**：
  * 专为损坏视频设计的回放控件，搭配 `DamageTimelineSlider` 呈现红绿损伤时间轴分布。
  * 自适应密集损坏区间合并与寻道跳跃算法，遇损坏区自动瞬间跳过，杜绝播放器卡死死循环。
* **Untrunc 深度结构重建**：通过正常参考视频 (Reference Video) 的 moov/atom 头信息结构，针对性修补非正常断电、录制中断或传输不全导致的 MP4 / MOV 损坏文件。

### 5. 图像与拼接处理 (Image & Join)
* **图像基础处理**：批量调整分辨率、任意角度裁剪、无损/有损格式互转。
* **长图与网格拼接**：支持多张图片水平横向、垂直纵向或矩阵网格排列拼接。

### 6. 任务队列并发管控 (Job Queue)
* **队列管理**：支持任务多线程并行执行，随时暂停、重试、取消与清空完成项。
* **日志与排错**：每个任务均提供完整的 FFmpeg 命令行文本（支持一键复制）与实时标准输出/错误日志监控。

### 7. 优雅设计与国际化 (UX & i18n)
* **Fluent Design 视觉**：全站基于 `ElaScrollPageArea` 圆角卡片打造，完美自适应 Windows 浅色/深色主题。
* **动态国际化**：原生内建简体中文 (`zh_CN`) 与英文 (`en`)，运行时实时动态切换界面语言，无需重启应用。

---

## 🛠 技术架构

```
┌────────────────────────────────────────────────────────┐
│                   ffmedia-toolbox                      │
├────────────────────────────────────────────────────────┤
│  UI 层 (Qt 6.11 / 6.12 Widgets + ElaWidgetTools)       │
│  - MainWindow (ElaWindow)                              │
│  - HomePage / ConvertPage / VideoPage / RepairPage...  │
│  - 自定义组件: VideoPlayerWidget, DamageTimelineSlider  │
├────────────────────────────────────────────────────────┤
│  服务层 (Core Logic & Concurrency)                      │
│  - JobQueue & JobListModel (多任务并发管控与软解自动降级) │
│  - MediaProbeService (媒体信息后台异步探测)             │
│  - CorruptedVideoScanner (流损坏快速定位与区间粘连合并)  │
│  - UntruncLocator & UntruncProcess (异步寻址与深层重建)  │
├────────────────────────────────────────────────────────┤
│  引擎适配层 (FFmpeg / Untrunc CLI Wrapper)              │
│  - FFmpegCommandBuilder (命令生成与参数校验)            │
│  - FFmpegLocator & FFmpegCapabilities (环境探测与硬解)  │
│  - FFmpegProcess (异步进程管控与实时进度解析)           │
└────────────────────────────────────────────────────────┘
```

---

## 💻 编译与构建

### 必备构建环境
* **操作系统**：Windows 10 / 11 (64-bit)
* **C++ 编译器**：MSVC 2022 (Visual Studio 2022 v143 或更新版本，要求支持 `/std:c++latest`)
* **Qt 框架**：Qt 6.11.x 或 Qt 6.12.x (组件要求：`Widgets`, `Multimedia`, `MultimediaWidgets`, `Concurrent`, `Test`, `LinguistTools`)
* **构建工具**：CMake 3.21+，推荐配合 Ninja 或 Visual Studio 生成器
* **外部引擎工具**（运行所需）：
  * `ffmpeg.exe` 与 `ffprobe.exe`（建议放置在系统 PATH 中，或应用启动后在“设置”页自定义指定）
  * `untrunc.exe`（用于损坏视频修复，可放置在可执行程序同级目录下）

### 本地编译步骤

#### 方式一：使用 CMake 命令行
```powershell
# 1. 打开 64位开发人员命令提示 (x64 Native Tools Command Prompt for VS 2022)
# 2. 进入项目根目录并配置 CMake
cmake -B cmake-build-release -G "Ninja" `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_PREFIX_PATH="C:/Qt/6.12.0/msvc2022_64"

# 3. 执行编译构建
cmake --build cmake-build-release --config Release

# 4. 运行全量测试套件
ctest --test-dir cmake-build-release --output-on-failure
```

#### 方式二：使用 CLion 或 Qt Creator
1. 在 IDE 中直接打开项目根目录 `CMakeLists.txt`。
2. 配置 CMake 选项：设置 `CMAKE_PREFIX_PATH` 为您的 Qt 6 MSVC 路径（例如 `C:/Qt/6.12.0/msvc2022_64`）。
3. 选用 MSVC 2022 x64 工具链，点击 **Build** 即可自动完成构建、依赖库拷贝与资源打包。

---

## 📁 目录结构

```text
.
├── src/
│   ├── app/                # 前端主窗口与各业务功能界面
│   │   ├── pages/          # 独立页面 (HomePage, ConvertPage, VideoPage, RepairPage...)
│   │   ├── widgets/        # 自定义控件 (VideoPlayerWidget, DamageTimelineSlider...)
│   │   ├── MainWindow.cpp  # 主窗口框架与路由
│   │   └── main.cpp        # 应用程序主入口
│   ├── conversion/         # 转码与混流策略 (RemuxPolicy)
│   ├── ffmpeg/             # FFmpeg 命令构造、能力探测与进程执行封装
│   ├── media/              # 媒体探测与坏帧扫描服务
│   ├── jobs/               # 任务模型、队列管控与进度回调
│   ├── repair/             # Untrunc 定位寻址与结构修补进程
│   ├── presets/            # 转码配置预设管理器
│   ├── output/             # 输出体积估算与文件命名模板解析
│   └── i18n/               # 语言管理器与 trId 宏定义
├── ElaWidgetTools/         # 现代化 Fluent UI 组件库 (Vendored, MIT License)
├── i18n/                   # 多语言翻译源文件 (.ts)
├── tests/                  # GoogleTest / QTest 自动化单元与回归测试套件
└── CMakeLists.txt          # 主 CMake 构建脚本
```

---

## 🧪 自动化测试

项目内建完备的测试驱动验证套件，覆盖核心参数构造、时间戳解析、任务队列并发、损坏扫描算法与修复流程：
```powershell
ctest --test-dir cmake-build-debug --output-on-failure
```
通过全部 12 项回归测试用例：
* `FFmpegCommandBuilder`
* `FFmpegProcess`
* `JobListModel`
* `OutputEstimate`
* `OutputNaming`
* `Parsers`
* `RecentFiles`
* `RemuxPolicy`
* `Job`
* `JobQueue`
* `UntruncProcess`
* `CorruptedVideoScanner`

---

## 📄 开源协议

本项目基于 **MIT License** 许可开源。
项目中集成的 [ElaWidgetTools](https://github.com/Liniyous/ElaWidgetTools) 遵循 MIT License。
关联使用的 [FFmpeg](https://ffmpeg.org/) 遵循 LGPL/GPL 许可，本项目仅通过子进程 CLI 方式解耦调用。
关联使用的 [Untrunc](https://github.com/ponchio/untrunc) 遵循 GPLv2 许可。
