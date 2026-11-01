# FFmpeg 媒体工具箱 (ffmedia-toolbox)

<div align="center">

![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg?style=flat-square)
![Qt](https://img.shields.io/badge/Qt-6.11%20%7C%206.12-brightgreen.svg?style=flat-square)
![Platform](https://img.shields.io/badge/Platform-Windows%20x64-0078d7.svg?style=flat-square)
![License](https://img.shields.io/badge/License-MIT-orange.svg?style=flat-square)

**基于 Qt 6 Widgets 与 ElaWidgetTools 构建的现代化、全能音视频与图像处理工作台**

[特性概览](#-核心特性全景) • [系统架构](#-系统架构) • [编译与构建](#-编译与构建) • [目录结构](#-目录结构) • [自动化测试](#-自动化测试) • [开源协议](#-开源协议)

</div>

---

## 📖 项目简介

**FFmpeg 媒体工具箱 (`ffmedia-toolbox`)** 是一款面向内容创作者、剪辑师与技术极客的现代化桌面级全能音视频与图像工具。

软件深度遵循 **C++23** 标准与 **Qt 6 (Widgets / Multimedia / Concurrent)** 架构，全面采用 **ElaWidgetTools** 呈现典雅且响应流畅的 **Windows 11 Fluent Design** 交互风格。底层深度整合 **FFmpeg** 原生命令行生态与 **Untrunc** 视频结构重建引擎，提供从常规格式转换、目标体积定向压制、交互式剪辑、独立音频精细调优、多文件批量作业，到高级 HDR 色彩调优与损坏视频结构级修复的全流程专业解决方案。

---

## ✨ 核心特性全景

### 1. 🏠 仪表盘与运行环境 (Fluent Dashboard)
* **硬件加速探测**：实时识别 NVIDIA NVENC、Intel QSV、AMD AMF、Apple VideoToolbox、VAAPI 等 GPU 硬解/硬编加速环境。
* **引擎状态自检**：一键检测本地 FFmpeg / FFprobe 核心版本、可用音视频编解码器全貌，以及 Untrunc 视频修复组件就绪状态。
* **即时队列状态**：实时监控任务队列中的运行中、排队等待与已完成任务数量，卡片支持一键直达详情。
* **核心功能直达**：主页 Fluent 磁贴卡片组织，一键直达格式转换、视频工具箱、音频工具箱、媒体修复与批处理页面。

### 2. 🔄 格式转换与体积压制 (Format Conversion)
* **全格式互转**：完整支持 MP4、MKV、MOV、WebM、AVI、FLV、WMV 以及 MP3、AAC、FLAC、WAV、Opus 等主流音视频容器。
* **双模式码率控制**：
  * **CRF 恒定画质**：精准控制画质损失，支持音视频无损流复制 (Stream Copy) 秒切。
  * **定向目标体积压制 (Target File Size)**：支持直接设定 25MB、50MB、100MB 快捷预设或自定义大小，根据输入时长智能推导视频码率与缓冲区约束，完美适配 Discord、微信等各平台上传限制。
* **高级色彩与 HDR**：支持色彩空间转换（BT.709、BT.2020 PQ、BT.2020 HLG、BT.601），集成 HDR to SDR 色调映射算法（Hable、Mobius、Reinhard）。
* **硬件转码回退保护**：当 GPU 显存不足或硬件编码驱动异常报错时，系统自动无缝回退至 CPU 软编重试，保障长时间无人值守批处理不阻断。
* **参数预设管理**：支持自定义转码配置的存储、加载、覆盖与快速删除。

### 3. 🎬 视频工具箱 (Video Toolbox)
采用分段式 `ElaPivot` 标签页组织，包含 12 项实用工具：
* **剪辑与调整 (Clip & Adjust)**：
  * **交互式所见即所得截取 (Visual Interactive Trim)**：内嵌低延迟视频播放器，提供 `[设为起点 (A)]`、`[设为终点 (B)]`、`[预览选段]` 标记点交互，时间轴 Fluent 高亮条双向联动，支持高精度毫秒微调与流复制秒切。
  * **智能黑边检测 (Auto Cropdetect)**：一键异步嗅探画面有效黑边区域，自动提取并回填裁切坐标 `crop=W:H:X:Y`。
  * **画面变换 (Adjust)**：按像素精确裁剪 (Crop)、分辨率等比/自定义缩放 (Resize) 以及 90°/180°/270° 旋转与翻转。
  * **视频拼接 (Join)**：多个同规格或异构视频快速串联合并。
* **音频提取与静音 (Audio Tools)**：
  * **音轨提取**：一键导出视频音轨至 MP3、AAC、FLAC 等纯音频格式。
  * **静音消除**：无损剥离视频原生音轨。
* **效果与合成 (Overlay & Subtitles)**：
  * **图片水印**：支持 PNG/WebP 水印图叠加、九宫格预设方位与透明度调节。
  * **画中画合成 (PiP)**：支持副视频尺寸指定、外边距设置与角落贴合。
  * **字幕封装与压制**：支持 SRT、ASS、SSA、VTT 字幕流软封装以及滤镜硬字幕烧录。
* **动图与帧提取 (Frames & Animation)**：
  * **高清截屏**：在指定时间戳抓取 PNG / JPEG / WebP 原始画面。
  * **制作 GIF**：自定义时间区间、输出帧率与宽度，自动应用调色板优化。
  * **序列帧互转**：支持视频切分为连续序列帧，或多张帧图合成视频。

### 4. 🎵 独立音频工具箱 (Audio Toolbox)
专为音频创作者、播客主播与自媒体打造的独立专业音频工作台：
* **格式与提取 (Format & Transcode)**：MP3、AAC、FLAC、WAV、Opus、OGG、M4A 格式互转；比特率（至 320k）、采样率（至 96kHz）、声道布局（单声道/立体声/5.1 环绕声）精细控制。
* **降噪与滤波 (Denoise & Filter)**：集成基于 FFT 的 `afftdn` 智能音频降噪器；低切高通滤波（有效过滤喷麦与环境低频轰鸣）与高切低通滤波（消除刺耳高频杂音）。
* **响度与静音 (Loudness & Silence)**：内置 EBU R128 响度标准化（-16 LUFS 播客工业标准）；自动识别剔除无声停顿片段（`silenceremove`）；音量任意倍数增益。
* **变速与效果 (Tempo & Effects)**：无变调 0.5x - 2.0x 变速微调（附带快捷倍速胶囊）；音频一键倒放（`areverse`）。

### 5. 📦 批量处理 (Batch Processing)
* **多文件批量作业**：支持文件多选或直接文件夹拖拽批量加入列表。
* **统一参数模板**：全量应用输出格式、编码器、CRF、色彩空间与音视频滤镜。
* **独立输出目录**：自定义批量导出路径，自动继承源文件扩展名与命名规则。
* **智能复制透传**：支持同格式无损流复制策略批量应用。

### 6. 🔧 损坏视频结构级修复 (Video Repair)
* **损坏坏帧全量扫描**：集成 `CorruptedVideoScanner`，全速离线遍历视频流，精准捕捉破损数据包并定位时间段区间。
* **损坏跳帧播放器 (`VideoPlayerWidget`)**：
  * 专为损坏视频设计的回放控件，搭配 `DamageTimelineSlider` 呈现红绿损伤时间轴分布。
  * 自适应密集损坏区间合并与寻道跳跃算法，遇损坏区自动瞬间跳过，杜绝播放器卡死死循环。
* **Untrunc 深度结构重建**：通过正常参考视频 (Reference Video) 的 moov/atom 头信息结构，针对性修补非正常断电、录制中断或传输不全导致的 MP4 / MOV 损坏文件。

### 7. 🖼️ 图像处理与拼接 (Image & Join)
* **图像基础处理**：批量调整分辨率、任意角度裁剪、无损/有损格式互转与画质压缩。
* **长图与网格拼接**：支持多张图片水平横向、垂直纵向或矩阵网格排列拼接，支持自定义间距与背景色。

### 8. 📋 任务队列并发管控 (Job Queue)
* **多并发槽位**：支持设置 1、2、4 个并行任务槽位，充分释放多核 CPU 与 GPU 性能。
* **任务全生命周期**：随时暂停、重试、取消单个或全量任务，一键清理已完成项目。
* **透明度与排错**：每个任务均提供完整的 FFmpeg 命令行文本（支持一键复制）与实时标准输出/错误日志监控。

### 9. 💻 优雅设计、系统托盘与国际化 (UX, Tray & i18n)
* **Windows 11 Fluent 视觉**：全站基于 `ElaScrollPageArea` 圆角卡片打造，自适应 Windows 浅色/深色主题。
* **系统托盘常驻 (System Tray)**：支持窗口关闭时后台最小化至 Windows 系统任务栏托盘静默压制，托盘右键菜单支持快速呼出、查看任务队列与安全退出。
* **任务完成桌面通知 (Desktop Notifications)**：队列任务全部完成或单项任务异常失败时，自动发射系统原生 Toast / 气泡提醒，点击通知直达队列详情。
* **动态国际化**：原生内建简体中文 (`zh_CN`) 与英文 (`en`)，运行时实时动态切换界面语言，无需重启应用。

---

## 🛠 系统架构

```
┌────────────────────────────────────────────────────────┐
│                   ffmedia-toolbox                      │
├────────────────────────────────────────────────────────┤
│  UI 层 (Qt 6.11 / 6.12 Widgets + ElaWidgetTools)       │
│  - MainWindow (ElaWindow)                              │
│  - HomePage / ConvertPage / VideoPage / AudioPage...   │
│  - BatchPage / JoinPage / ImagePage / RepairPage...    │
│  - 自定义组件: VideoPlayerWidget, DamageTimelineSlider  │
├────────────────────────────────────────────────────────┤
│  服务层 (Core Logic & Concurrency)                      │
│  - JobQueue & JobListModel (多任务并发管控与软解自动降级) │
│  - MediaProbeService (媒体信息后台异步探测)             │
│  - CorruptedVideoScanner (流损坏快速定位与区间粘连合并)  │
│  - UntruncLocator & UntruncProcess (异步寻址与深层重建)  │
├────────────────────────────────────────────────────────┤
│  引擎适配层 (FFmpeg / Untrunc CLI Wrapper)              │
│  - FFmpegCommandBuilder (命令生成、Muxer 别名映射规范化) │
│  - FFmpegLocator & FFmpegCapabilities (环境探测与硬解)  │
│  - FFmpegProcess (异步进程管控与实时进度解析)           │
└────────────────────────────────────────────────────────┘
```

---

## 💻 编译与构建

### 必备构建环境
* **操作系统**：Windows 10 / 11 (64-bit)
* **C++ 编译器**：MSVC 2022 (Visual Studio 2022 v143 或更新版本，要求开启 `/std:c++latest`)
* **Qt 框架**：Qt 6.11.x 或 Qt 6.12.x (组件要求：`Widgets`, `Multimedia`, `MultimediaWidgets`, `Concurrent`, `Network`, `Test`, `LinguistTools`)
* **构建工具**：CMake 3.21+，推荐搭配 Ninja 生成器
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
│   │   ├── pages/          # 独立页面 (HomePage, ConvertPage, VideoPage, AudioPage, BatchPage, RepairPage...)
│   │   ├── widgets/        # 自定义控件 (VideoPlayerWidget, DamageTimelineSlider...)
│   │   ├── MainWindow.cpp  # 主窗口框架、系统托盘与路由
│   │   └── main.cpp        # 应用程序主入口
│   ├── conversion/         # 转码与混流策略 (RemuxPolicy)
│   ├── ffmpeg/             # FFmpeg 命令构造、封装格式映射、能力探测与进程执行封装
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

项目内建完备的测试驱动验证套件，覆盖核心参数构造、封装器别名校验、时间戳解析、任务队列并发、损坏扫描算法与修复流程：

```powershell
ctest --test-dir cmake-build-release --output-on-failure
```

通过全部 12 项自动化测试套件（100% 通过率）：
* `FFmpegCommandBuilder`：参数拼装、Muxer 格式别名自动纠偏、色彩空间、软编降级
* `FFmpegProcess`：进程生命周期、进度行解析、中断与安全杀死
* `JobListModel`：任务模型数据项增删查改与通知
* `OutputEstimate`：目标体积估算与码率动态推导
* `OutputNaming`：通配模板与扩展名自适应拼装
* `Parsers`：探针 JSON 与控制台日志解析
* `RecentFiles`：最近文件缓存与持久化
* `RemuxPolicy`：音视频流拷贝策略判断
* `Job`：作业实体属性、重试标记与软编回退
* `JobQueue`：并发槽位调度、取消重试、托盘完成信号发射
* `UntruncProcess`：Untrunc 修复进程控制与文件预测
* `CorruptedVideoScanner`：离线损坏帧捕捉与相邻区间合并

---

## 📄 开源协议

本项目基于 **MIT License** 许可开源。
* 项目中集成的 [ElaWidgetTools](https://github.com/Liniyous/ElaWidgetTools) 遵循 MIT License。
* 关联使用的 [FFmpeg](https://ffmpeg.org/) 遵循 LGPL/GPL 许可，本项目仅通过子进程 CLI 方式解耦调用。
* 关联使用的 [Untrunc](https://github.com/ponchio/untrunc) 遵循 GPLv2 许可。
