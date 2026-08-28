# FFmpeg Media Toolbox — AI Development Plan

> 本文档是本项目的长期开发约束。
>
> 所有参与本项目开发的 AI 在生成、修改、重构代码之前，都必须阅读并遵守本文档。
>
> 如果用户的临时要求与本文档冲突：
>
> 1. 明确指出冲突点；
> 2. 优先执行用户最新的明确指令；
> 3. 不得在没有说明的情况下擅自改变项目核心架构；
> 4. 如果只是实现细节冲突，应尽量保持本文档定义的架构原则。

---

# 1. 项目定位

本项目是一个：

**基于 Qt 6 + C++ + FFmpeg 的跨平台桌面多媒体处理工具。**

核心目标：

- 提供简单、直观的 FFmpeg GUI；
- 降低 FFmpeg 的使用门槛；
- 支持常见视频、音频、图片处理；
- 支持批处理；
- 支持任务队列；
- 保留 FFmpeg 的高级能力；
- 对高级用户展示实际生成的 FFmpeg 命令；
- 保持内部架构足够清晰，以便未来扩展。

本项目当前不是：

- 专业 NLE 视频剪辑器；
- DaVinci Resolve 替代品；
- Premiere 替代品；
- After Effects 替代品；
- 多轨实时视频编辑器。

在基础媒体处理功能成熟之前，不实现复杂时间轴、多轨编辑、关键帧动画等功能。

---

# 2. 第一阶段产品方向

当前开发重点：

1. 格式转换；
2. 图片处理；
3. 基础视频工具；
4. 基础音频工具；
5. 批处理；
6. 任务队列；
7. FFmpeg 能力检测；
8. 媒体信息查看；
9. FFmpeg 命令生成；
10. 日志与错误处理。

优先把：

**输入文件 → 配置处理 → 加入任务 → FFmpeg 执行 → 显示进度 → 输出文件**

这一完整工作流做好。

---

# 3. 技术栈约束

默认技术栈：

- C++23
- Qt 6
- Qt Quick
- QML
- Qt Quick Controls
- Qt Multimedia
- QProcess
- CMake
- FFmpeg CLI
- FFprobe CLI
- QJsonDocument
- QSettings
- Qt Test

除非用户明确要求，否则：

- 不引入其他 GUI Framework；
- 不引入 Electron；
- 不引入 WebView 作为主要 UI；
- 不使用 Rust 重写核心；
- 不使用 Python 作为运行时依赖；
- 不直接使用 libavcodec/libavformat/libavfilter；
- 不引入大型第三方 Framework。

第三方依赖应尽可能少。

---

# 4. UI 技术约束

UI 使用：

**Qt Quick / QML**

业务逻辑使用：

**C++**

禁止把大量业务逻辑直接写进 QML。

QML 主要负责：

- Layout；
- Controls；
- Animation；
- 用户输入；
- 数据展示；
- 页面导航；
- UI State；
- 调用公开的 C++ 接口。

C++ 负责：

- 文件处理；
- FFmpeg；
- FFprobe；
- 数据模型；
- Job Queue；
- Preset；
- Settings；
- 命令生成；
- 参数验证；
- 错误处理；
- 业务规则。

禁止：

```qml
onClicked: {
    var cmd = "ffmpeg -i " + input + " -c:v libx265 ..."
}
```

正确方式：

```text
QML
 ↓
C++ API
 ↓
Domain Model
 ↓
Command Builder
 ↓
FFmpeg
```

---

# 5. FFmpeg 集成原则

第一阶段只通过：

```text
ffmpeg
ffprobe
```

两个独立 executable 工作。

调用方式：

```text
QProcess
```

禁止第一阶段直接集成：

```text
libavcodec
libavformat
libavfilter
libswscale
libswresample
libavutil
```

原因：

- CLI API 足以覆盖当前需求；
- 减少 ABI 问题；
- 降低跨平台打包复杂度；
- 降低代码复杂度；
- 更方便调试；
- 更容易查看实际命令；
- 后期可以逐步替换内部实现。

只有出现以下需求时才考虑 libav：

- frame accurate preview；
- 实时 Filter Preview；
- 自定义 Decoder Pipeline；
- Zero-copy GPU Pipeline；
- 深度 Frame Processing；
- FFmpeg CLI 无法合理满足的需求。

---

# 6. FFmpeg 路径管理

必须提供统一的：

```text
FFmpegLocator
```

负责：

- 查找 ffmpeg；
- 查找 ffprobe；
- 验证 executable；
- 获取版本；
- 用户自定义路径；
- 自动检测；
- 保存路径设置。

禁止：

- 页面自己寻找 ffmpeg；
- 不同模块自行决定 ffmpeg 路径；
- 写死系统路径。

调用 FFmpeg 的所有模块必须通过统一服务获取 executable 路径。

---

# 7. FFmpeg 能力检测

程序不能假设用户的 FFmpeg 支持某个 encoder/filter/container。

启动后应支持扫描：

```bash
ffmpeg -version
ffmpeg -formats
ffmpeg -codecs
ffmpeg -encoders
ffmpeg -decoders
ffmpeg -filters
ffmpeg -hwaccels
```

推荐内部模型：

```cpp
struct FFmpegCapabilities
{
    QString version;

    QSet<QString> formats;
    QSet<QString> codecs;
    QSet<QString> encoders;
    QSet<QString> decoders;
    QSet<QString> filters;
    QSet<QString> hardwareAccelerators;
};
```

UI 中可用功能必须尽可能根据实际 FFmpeg Build 动态决定。

禁止硬编码：

```cpp
bool supportsNvenc = true;
bool supportsAv1 = true;
```

---

# 8. 媒体信息获取

所有媒体信息原则上统一通过：

```text
ffprobe
```

获取。

推荐：

```bash
ffprobe \
    -v error \
    -of json \
    -show_format \
    -show_streams \
    -show_chapters \
    input
```

然后解析 JSON。

统一的数据模型应包含：

```cpp
struct MediaFile
{
    QString path;

    MediaType type;

    qint64 fileSize;
    double duration;

    QList<VideoStream> videoStreams;
    QList<AudioStream> audioStreams;
    QList<SubtitleStream> subtitleStreams;

    QVariantMap metadata;
};
```

禁止不同页面重复解析 ffprobe 数据。

---

# 9. Command Builder 原则

项目内部禁止到处直接拼 FFmpeg command。

错误：

```cpp
QString command =
    "ffmpeg -i " +
    input +
    " -vf crop=" +
    crop +
    " " +
    output;
```

必须存在统一：

```text
FFmpegCommandBuilder
```

推荐模型：

```cpp
struct FFmpegCommand
{
    QString program;
    QStringList arguments;
};
```

通过：

```cpp
FFmpegCommand FFmpegCommandBuilder::build(const Job &job);
```

产生最终命令。

---

# 10. 禁止 Shell Command String

尽量不要生成：

```cpp
QString fullCommand;
```

再交给 Shell。

应该生成：

```cpp
QString program;
QStringList arguments;
```

例如：

```cpp
process.start(
    ffmpegPath,
    {
        "-i",
        inputPath,
        "-c:v",
        "libx265",
        outputPath
    }
);
```

这样可以：

- 避免路径转义问题；
- 避免空格问题；
- 避免 shell injection；
- 保持跨平台兼容。

完整字符串只用于：

- UI Command Preview；
- Debug；
- Copy Command。

---

# 11. Domain Model 原则

业务数据必须结构化。

错误：

```cpp
QString codec = "libx265";
QString quality = "20";
QString resize = "1920x1080";
```

推荐：

```cpp
enum class VideoCodec
{
    Copy,
    H264,
    H265,
    AV1,
    VP9
};
```

例如：

```cpp
struct VideoSettings
{
    VideoCodec codec;

    std::optional<int> crf;
    std::optional<int> bitrate;

    std::optional<QSize> resolution;

    std::optional<double> frameRate;

    QString preset;
};
```

必须尽量避免非法状态。

---

# 12. Filter 不允许保存为裸字符串

禁止业务层直接保存：

```text
crop=1920:1080:10:10,scale=1280:720
```

应该保存：

```cpp
struct CropOperation
{
    int x;
    int y;
    int width;
    int height;
};
```

以及：

```cpp
struct ResizeOperation
{
    QSize targetSize;
    ResizeMode mode;
    ScalingAlgorithm algorithm;
};
```

最终由：

```text
FilterGraphBuilder
```

转换成 FFmpeg filtergraph。

---

# 13. Pipeline 设计

图片和视频处理应尽量统一采用：

```text
Input
 ↓
Operation
 ↓
Operation
 ↓
Operation
 ↓
Output
```

例如：

```text
Input
 ↓
Crop
 ↓
Resize
 ↓
Rotate
 ↓
Watermark
 ↓
Output
```

建议：

```cpp
class MediaPipeline
{
public:
    std::vector<MediaOperation> operations;
};
```

图片 Pipeline：

```text
Crop
Resize
Rotate
Flip
Padding
Watermark
Text
```

未来视频 Pipeline：

```text
Trim
Crop
Resize
Rotate
Subtitle
Watermark
FPS
```

FFmpeg FilterGraph 只在 Pipeline 最终编译阶段生成。

---

# 14. Job 是所有处理的统一抽象

所有媒体处理最终都必须转化为：

```text
Job
```

禁止页面直接启动 FFmpeg。

例如：

```cpp
class Job
{
public:
    QUuid id;

    JobType type;

    QStringList inputFiles;
    QString outputFile;

    JobStatus status;

    double progress;

    QString errorMessage;
};
```

推荐状态：

```cpp
enum class JobStatus
{
    Pending,
    Preparing,
    Running,
    Completed,
    Failed,
    Cancelled
};
```

---

# 15. Job Queue

统一使用：

```text
JobQueue
```

处理：

- 添加任务；
- 删除任务；
- 开始任务；
- 停止任务；
- 取消任务；
- Retry；
- Reorder；
- 并发控制；
- Job History；
- 状态管理。

页面不得自行管理 FFmpeg Process 生命周期。

正确：

```text
UI
 ↓
JobQueue.add(job)
 ↓
JobQueue
 ↓
FFmpegProcess
```

---

# 16. FFmpeg Process 封装

所有 FFmpeg 执行必须通过：

```text
FFmpegProcess
```

统一管理：

- QProcess；
- stdout；
- stderr；
- progress；
- exit code；
- terminate；
- kill；
- error；
- log。

不要在 Converter、ImagePage、VideoPage 等模块里重复写 QProcess。

---

# 17. Progress 解析

禁止依赖 FFmpeg 默认 human-readable stderr 解析进度。

使用：

```bash
-progress pipe:1
-nostats
```

根据：

```text
key=value
```

解析。

例如：

```text
frame=123
fps=60.1
out_time_us=10000000
speed=2.3x
progress=continue
```

内部：

```cpp
struct FFmpegProgress
{
    qint64 frame = 0;

    qint64 outTimeUs = 0;

    double fps = 0;

    double speed = 0;

    qint64 totalSize = 0;
};
```

进度：

```text
progress = outTime / totalDuration
```

---

# 18. 错误处理

底层必须保留完整 FFmpeg stderr。

但是 UI 不应直接把大量 stderr 当成错误提示。

应进行错误分类，例如：

```text
FFmpegNotFound
FFprobeNotFound

UnsupportedCodec
UnsupportedEncoder
UnsupportedFilter

InputNotFound
OutputNotWritable

FileAlreadyExists

InvalidResolution

HardwareEncoderUnavailable

InsufficientDiskSpace

ProcessCrashed

UnknownFFmpegError
```

用户界面显示友好错误。

同时提供：

```text
View Log
Copy Log
Copy Command
```

---

# 19. Logging

必须建立统一日志系统。

最低支持：

```text
DEBUG
INFO
WARNING
ERROR
```

每个 Job 保存：

- 实际 arguments；
- FFmpeg stdout；
- FFmpeg stderr；
- Process exit code；
- 开始时间；
- 结束时间；
- 错误信息。

禁止使用大量：

```cpp
qDebug()
```

替代正式日志结构。

开发期间可以使用 qDebug，但长期代码应逐步统一。

---

# 20. 输出文件策略

必须统一管理输出文件。

支持：

```text
Original Folder
Custom Folder
Same Folder
```

文件存在时：

```text
Ask
Replace
Rename
Skip
```

自动 Rename 示例：

```text
video.mp4
video_1.mp4
video_2.mp4
```

禁止不同工具重复实现输出命名逻辑。

---

# 21. Naming Template

批处理支持命名模板。

第一阶段：

```text
{name}
{ext}
{index}
{width}
{height}
```

例如：

```text
{name}_converted.{ext}
{name}_{width}x{height}.{ext}
{name}_{index}.{ext}
```

未来可以加入：

```text
{codec}
{date}
{time}
{preset}
```

所有文件命名必须经过统一的：

```text
OutputNameGenerator
```

---

# 22. 第一阶段必须支持的转换

## Video

最低支持：

```text
MP4
MKV
MOV
WebM
```

编码器根据 FFmpeg capabilities 动态显示。

常见：

```text
H.264
H.265
AV1
VP9
```

---

# 23. Audio

最低考虑：

```text
MP3
AAC / M4A
FLAC
WAV
Opus
OGG
```

支持：

```text
codec
bitrate
sample rate
channel count
```

第一阶段可只暴露常用参数。

---

# 24. Images

最低支持常见格式：

```text
PNG
JPEG
WebP
BMP
TIFF
GIF
```

如果 FFmpeg Build 支持，可以显示：

```text
AVIF
JPEG XL
```

不能假定某个高级编码器一定存在。

---

# 25. Smart Conversion

格式转换支持：

```text
Smart
Stream Copy
Re-encode
```

Smart 模式：

如果：

- 输入 codec 与目标 container 兼容；
- 用户没有应用需要重新编码的 Filter；
- 音视频参数不要求改变；

则优先考虑：

```text
-c copy
```

否则重新编码。

Smart 模式的决策必须在业务层完成，不要写在 QML。

---

# 26. Basic / Advanced 参数

FFmpeg 参数数量巨大。

禁止默认把所有 FFmpeg 参数全部暴露给普通用户。

UI 分：

```text
Basic
Advanced
```

Basic：

```text
Format
Codec
Quality
Resolution
FPS
Audio Codec
Audio Bitrate
```

Advanced：

```text
Preset
Profile
Level
Pixel Format
Bitrate
CRF
GOP
Tune
Hardware Encoder
Custom Extra Arguments
```

Custom Arguments 必须明确标记为高级功能。

---

# 27. Image Crop

P0 功能。

必须支持：

```text
Free
1:1
4:3
3:2
16:9
9:16
Custom
```

交互支持：

```text
拖动边
拖动角
移动 Crop Box
固定 Aspect Ratio
Reset
Center
```

数据显示：

```text
X
Y
Width
Height
```

Crop 数据必须保存成结构化对象。

---

# 28. Image Resize

P0。

模式：

```text
Exact
Fit
Fill
Percentage
```

选项：

```text
Keep Aspect Ratio
Prevent Upscaling
```

算法：

```text
Automatic
Nearest
Bilinear
Bicubic
Lanczos
```

普通用户默认：

```text
Automatic
```

---

# 29. Rotate / Flip

P0。

支持：

```text
Rotate 90 CW
Rotate 90 CCW
Rotate 180

Flip Horizontal
Flip Vertical
```

Custom Angle 后续再实现。

---

# 30. Image Join

P0。

必须支持：

```text
Horizontal
Vertical
Grid
```

并支持：

```text
Order
Spacing
Margin
Alignment
Background
Size Mode
```

Alignment：

```text
Top
Center
Bottom
Left
Right
```

Size Mode：

```text
Original
Same Width
Same Height
Fit
Fill
```

---

# 31. Grid / Contact Sheet

应支持：

```text
Columns
Rows
Thumbnail Size
Spacing
Margin
Background
```

Rows 可以允许：

```text
Auto
```

以后可以复用于视频截图 Contact Sheet。

---

# 32. 图片批处理

P0。

支持：

```text
多个文件
整个目录
```

批量：

```text
Convert
Resize
Rotate
Flip
```

所有文件生成独立 Job 或受控 Batch Job。

不得一次性阻塞主线程。

---

# 33. Preview

图片 Preview：

```text
QML Image
```

视频 Preview：

```text
Qt Multimedia
MediaPlayer
VideoOutput
```

第一阶段预览源文件即可。

暂时不实现：

```text
Real-time FFmpeg Filter Preview
```

不要为了预览提前引入 libav。

---

# 34. Command Preview

所有 Job 都应该可以查看：

```text
Generated FFmpeg Command
```

支持：

```text
Copy
```

Command Preview 必须来源于真正执行的：

```text
program + arguments
```

不能另外构造一套展示字符串逻辑。

---

# 35. Preset 系统

P1。

Preset 应是结构化配置，不是单纯 FFmpeg 字符串。

例如：

```text
MP4 High Quality
MP4 Small Size
H264 Compatibility
H265 High Quality
AV1 Archive

1080p
720p
Vertical 1080x1920

MP3 320k
AAC 256k
FLAC

WebP High Quality
JPEG High Quality
```

用户支持：

```text
Save Preset
Rename Preset
Duplicate Preset
Delete Preset
```

内置 Preset 与用户 Preset 分开管理。

---

# 36. 视频工具开发顺序

图片和转换稳定之后，实现：

## P1

```text
Trim
Join
Extract Audio
Mute Audio
Screenshot
GIF Maker
Resize
Change FPS
```

---

# 37. Video Trim

提供：

```text
Start
End
Duration
```

模式：

```text
Fast
Accurate
```

Fast：

尽量使用 Stream Copy。

Accurate：

允许重新编码以获得更准确剪切点。

UI 必须明确两者差异。

---

# 38. Video Join

支持多个视频拖入。

支持拖动排序。

如果流参数兼容：

```text
Fast Join
```

否则：

```text
Re-encode Join
```

不要在参数不兼容时默默失败。

---

# 39. Extract Audio

最低支持：

```text
Original Audio
MP3
AAC
FLAC
WAV
Opus
```

如果允许 Stream Copy，应根据原 Codec 与目标容器决定。

---

# 40. Video Screenshot

支持：

```text
Current Frame
Specific Timestamp
Multiple Timestamps
```

输出：

```text
PNG
JPEG
WebP
```

第一版可以只实现：

```text
Specific Timestamp
```

---

# 41. GIF

P1。

支持：

```text
Start
Duration
Width
FPS
Quality
Loop
```

后续可优化 palette。

不要第一版花大量时间做极致 GIF 优化。

---

# 42. Hardware Encoding

P1。

根据 FFmpeg capabilities 动态检测。

典型：

```text
NVENC
QSV
AMF
VideoToolbox
VAAPI
```

不要根据 OS 简单判断支持情况。

例如：

```text
Windows != 一定支持 NVENC
```

必须看当前 FFmpeg Build 与实际环境。

---

# 43. 元数据

P1/P2。

支持：

```text
View Metadata
Preserve Metadata
Remove Metadata
```

未来支持编辑：

```text
Title
Artist
Comment
Date
```

---

# 44. Settings

设置页面至少包含：

## General

```text
Theme
Language
Default Output Folder
Overwrite Behavior
```

## FFmpeg

```text
FFmpeg Path
FFprobe Path
Version
Auto Detect
Scan Capabilities
```

## Processing

```text
Concurrent Jobs
Temporary Directory
Keep Temporary Files
```

## Video

```text
Preferred Codec
Preferred Hardware Encoder
```

## Advanced

```text
Show Generated Commands
Log Level
```

---

# 45. Theme

支持：

```text
System
Light
Dark
```

颜色、字体、Spacing 尽量统一定义 Theme Tokens。

不要在几十个 QML 文件中写死：

```qml
color: "#222222"
```

应该通过统一 Theme 系统引用。

---

# 46. QML Component 化

重复 UI 必须提取 Component。

例如：

```text
FileDropArea.qml
MediaCard.qml
JobCard.qml
OutputSelector.qml
CodecSelector.qml
QualitySelector.qml
ProgressCard.qml
ErrorBanner.qml
SectionHeader.qml
```

禁止复制大量相同 QML。

---

# 47. Model/View

任务列表、文件列表等动态数据：

优先：

```text
QAbstractListModel
QAbstractTableModel
```

不要把大型数组以 QVariantList 频繁整体传递给 QML。

典型：

```text
JobListModel
MediaFileListModel
PresetListModel
```

---

# 48. 主线程约束

禁止在 GUI 主线程执行：

```text
长时间文件扫描
FFmpeg
FFprobe 等待
大量图片处理
复杂同步 I/O
```

QProcess 本身异步使用。

任何可能阻塞 UI 的操作都必须评估是否需要：

```text
async API
worker
QtConcurrent
thread
```

但不要为了简单操作过度多线程化。

---

# 49. 并发控制

Job Queue 支持：

```text
1
2
4
Custom
```

默认视频编码：

```text
1
```

避免自动同时运行多个 CPU-heavy 视频 Encoder。

以后可以实现智能并发。

---

# 50. Cancel

任务必须支持取消。

正常取消优先尝试：

```text
terminate / graceful quit
```

必要时：

```text
kill
```

取消后必须正确：

- 更新状态；
- 停止进度；
- 清理资源；
- 根据策略处理未完成文件。

---

# 51. 临时文件

如果功能需要临时文件：

必须统一由：

```text
TemporaryFileManager
```

管理。

要求：

- 使用系统 temp directory；
- Job 完成后清理；
- 程序异常退出时尽可能恢复；
- Debug 设置可选择保留临时文件。

禁止任意模块自行在当前工作目录创建临时垃圾文件。

---

# 52. 跨平台

目标：

```text
Windows
macOS
Linux
```

代码中禁止无必要使用平台专属路径。

路径操作优先使用：

```text
QFileInfo
QDir
QStandardPaths
```

平台特殊代码必须明确隔离。

例如：

```text
platform/
    WindowsIntegration.cpp
    MacIntegration.mm
    LinuxIntegration.cpp
```

---

# 53. 文件路径安全

必须考虑：

```text
空格
中文
日文
Emoji
特殊字符
长路径
Unicode
```

不能因为：

```text
C:\Users\Test User\视频 文件.mp4
```

而失败。

这也是禁止使用裸 Shell String 的重要原因。

---

# 54. 自动保存状态

建议使用：

```text
QSettings
```

保存：

```text
窗口尺寸
窗口位置
Theme
FFmpeg Path
默认输出路径
最近目录
最近设置
并发任务数
```

不应该保存：

- 临时 UI 状态；
- 巨量日志；
- 大型媒体数据。

---

# 55. 最近文件

P1。

支持：

```text
Recent Files
Recent Folders
Recent Jobs
```

设置中允许：

```text
Clear History
```

---

# 56. Drag & Drop

拖放是核心交互。

应该允许：

```text
文件
多个文件
目录
```

程序自动识别：

```text
Image
Video
Audio
Subtitle
Unknown
```

页面根据类型决定是否接受。

不能让用户拖进去之后静默失败。

---

# 57. Clipboard

P1。

图片页面以后支持：

```text
Ctrl + V
```

读取剪贴板图片。

也可以：

```text
Copy Result
```

写回剪贴板。

---

# 58. Undo / Redo

针对图片编辑 Pipeline 支持：

```text
Undo
Redo
```

Undo/Redo 修改：

```text
Pipeline State
```

而不是反复重新修改原图文件。

源文件永远保持不变。

---

# 59. 非破坏编辑原则

默认任何编辑：

```text
不得直接覆盖输入文件
```

必须显式输出到：

```text
新文件
```

只有用户明确选择：

```text
Replace Original
```

时才允许覆盖。

即使覆盖，也建议：

1. 先输出到临时文件；
2. 验证成功；
3. 再替换源文件。

避免编码失败造成源文件损坏。

---

# 60. UI 默认保持简单

核心原则：

**简单模式服务普通用户，高级模式服务 FFmpeg 用户。**

第一页不要展示：

```text
pix_fmt
gop_size
b_frames
level
profile
rc-lookahead
aq-mode
```

除非进入：

```text
Advanced
```

---

# 61. 不要照搬 FFmpeg CLI UI

本软件不是：

> FFmpeg 参数表单生成器。

应该转换成用户理解的概念。

例如普通用户看到：

```text
Quality
High / Medium / Small File
```

高级用户才看到：

```text
CRF
Bitrate
Preset
```

---

# 62. 但必须保留高级能力

虽然默认简单，但不能限制高级用户。

推荐最终提供：

```text
Advanced
    ├ Codec Settings
    ├ Filter Settings
    ├ Metadata
    └ Extra FFmpeg Arguments
```

用户填入 Extra Arguments 时要明确说明：

```text
Advanced FFmpeg option.
May override generated settings.
```

---

# 63. MVP v0.1

v0.1 目标：

```text
文件拖放

FFmpeg 自动检测
FFprobe 自动检测

FFmpeg capability scan

媒体信息

图片格式转换
音频格式转换
视频格式转换

Smart Remux

Image Crop
Image Resize
Image Rotate
Image Flip

Horizontal Join
Vertical Join
Grid Join

Batch Image Processing

Output Naming

Job Queue

Progress

Cancel

Retry

Logs

Command Preview

Settings
```

在这些完成之前：

不要主动扩展 Timeline 等大型功能。

---

# 64. v0.2

计划：

```text
Preset

Video Trim

Video Join

Extract Audio

Mute Video

Video Screenshot

GIF Maker

Image Sequence -> Video

Video -> Image Sequence

Watermark

Metadata

Hardware Encoder Detection

NVENC
QSV
AMF
VideoToolbox
VAAPI

Output Size Estimate

Recent Jobs
```

---

# 65. v0.3

计划：

```text
Subtitle Mux

Subtitle Burn-in

Video Crop

Video Resize

Video Rotate

Video Watermark

Picture-in-Picture

Advanced Audio Settings

Advanced Codec Settings

Pixel Format

Color Space

HDR

Custom Filter
```

---

# 66. 暂缓功能

除非用户明确要求，否则以下功能在 v0.3 前不要实现：

```text
Timeline
Multi-track editing
Waveform editor
Transitions
Keyframe animation
Real-time filter preview
Node editor
Filter graph visual editor
Professional color grading
Audio mixer
Proxy media system
Project file system
```

这些属于后续大型版本。

---

# 67. 推荐项目结构

```text
src/
├── app/
│   ├── Application.h
│   ├── Application.cpp
│   ├── AppSettings.h
│   ├── AppSettings.cpp
│   ├── RecentFiles.h
│   └── RecentFiles.cpp
│
├── media/
│   ├── MediaFile.h
│   ├── MediaStream.h
│   ├── MediaMetadata.h
│   ├── MediaProbeService.h
│   └── MediaProbeService.cpp
│
├── ffmpeg/
│   ├── FFmpegLocator.h
│   ├── FFmpegLocator.cpp
│   ├── FFmpegCapabilities.h
│   ├── FFmpegCapabilities.cpp
│   ├── FFmpegProcess.h
│   ├── FFmpegProcess.cpp
│   ├── FFmpegProgress.h
│   ├── FFmpegCommand.h
│   ├── FFmpegCommandBuilder.h
│   ├── FFmpegCommandBuilder.cpp
│   ├── FilterGraphBuilder.h
│   └── FilterGraphBuilder.cpp
│
├── jobs/
│   ├── Job.h
│   ├── Job.cpp
│   ├── JobQueue.h
│   ├── JobQueue.cpp
│   ├── JobListModel.h
│   ├── JobListModel.cpp
│   ├── JobHistory.h
│   └── JobHistory.cpp
│
├── conversion/
│   ├── ConversionJob.h
│   ├── VideoSettings.h
│   ├── AudioSettings.h
│   ├── ImageSettings.h
│   └── ConversionService.cpp
│
├── image/
│   ├── ImagePipeline.h
│   ├── CropOperation.h
│   ├── ResizeOperation.h
│   ├── RotateOperation.h
│   ├── FlipOperation.h
│   ├── JoinOperation.h
│   └── ImagePipeline.cpp
│
├── presets/
│   ├── Preset.h
│   ├── PresetManager.h
│   └── PresetManager.cpp
│
├── output/
│   ├── OutputPathManager.h
│   ├── OutputNameGenerator.h
│   └── OutputNameGenerator.cpp
│
└── utility/
    ├── Logger.h
    ├── FileUtils.h
    └── TimeUtils.h
```

QML：

```text
qml/
├── Main.qml
│
├── pages/
│   ├── HomePage.qml
│   ├── ConvertPage.qml
│   ├── ImagePage.qml
│   ├── VideoPage.qml
│   ├── AudioPage.qml
│   ├── QueuePage.qml
│   ├── PresetPage.qml
│   └── SettingsPage.qml
│
├── components/
│   ├── FileDropArea.qml
│   ├── MediaCard.qml
│   ├── JobCard.qml
│   ├── ProgressCard.qml
│   ├── OutputSelector.qml
│   ├── CodecSelector.qml
│   ├── QualitySelector.qml
│   ├── ErrorBanner.qml
│   └── SectionHeader.qml
│
├── image/
│   ├── CropEditor.qml
│   ├── ResizePanel.qml
│   ├── JoinEditor.qml
│   └── ImagePreview.qml
│
└── theme/
    ├── Theme.qml
    ├── Metrics.qml
    └── Typography.qml
```

---

# 68. 第一批开发顺序

必须尽可能按照以下顺序开发。

## Step 1

搭建：

```text
CMake
Qt 6
QML
MainWindow
```

目标：

程序正常启动。

---

## Step 2

实现：

```text
FFmpegLocator
```

目标：

```text
检测 ffmpeg
检测 ffprobe
获取版本
```

---

## Step 3

实现：

```text
FFmpegCapabilities
```

目标：

获取：

```text
formats
encoders
decoders
filters
hwaccels
```

---

## Step 4

实现：

```text
MediaProbeService
```

目标：

拖入：

```text
test.mp4
```

得到：

```text
duration
resolution
fps
codec
audio
metadata
```

---

## Step 5

实现：

```text
FFmpegCommand
FFmpegCommandBuilder
```

先完成最简单：

```text
input.mp4
→
output.mp4
```

---

## Step 6

实现：

```text
FFmpegProcess
```

目标：

```text
启动
实时 progress
stderr
成功
失败
取消
```

完整可用。

---

## Step 7

实现：

```text
Job
JobQueue
JobListModel
```

目标：

多个任务可以：

```text
Pending
Running
Completed
Failed
```

---

## Step 8

实现最简单 Convert Page。

支持：

```text
输入
输出格式
编码器
质量
输出目录
Add to Queue
```

---

## Step 9

实现图片 Pipeline。

首先：

```text
Crop
Resize
Rotate
Flip
```

---

## Step 10

实现图片 Join。

```text
Horizontal
Vertical
Grid
```

---

## Step 11

实现 Batch。

---

## Step 12

实现：

```text
Settings
Logs
Command Preview
```

完成 v0.1。

---

# 69. 测试约束

核心业务必须逐步加入测试。

优先测试：

```text
FFmpeg command generation

Output filename generation

Capability parser

ffprobe JSON parser

Progress parser

Crop filter generation

Resize filter generation

Pipeline -> filtergraph

Smart remux decision

Job state transitions
```

避免测试只覆盖 UI。

---

# 70. AI 修改代码规则

AI 每次修改代码前，应先确认：

1. 当前修改属于哪个模块；
2. 是否已有负责该逻辑的类；
3. 是否会产生重复逻辑；
4. 是否破坏分层；
5. 是否影响其他 Job；
6. 是否需要测试；
7. 是否引入新的依赖；
8. 是否真的需要新增类。

---

# 71. AI 禁止行为

AI 不允许无理由：

- 大规模重写已有项目；
- 改变整个技术栈；
- Qt Widgets/QML 混乱使用；
- 把 C++ 业务逻辑移动到 QML；
- 在多个页面复制 FFmpeg 调用；
- 每个功能单独创建 QProcess；
- 每个页面单独解析 ffprobe；
- 到处拼接 FFmpeg 字符串；
- 添加不必要的第三方库；
- 引入 Python；
- 引入 Node.js；
- 引入 Rust；
- 引入 Web 前端 Framework；
- 第一阶段直接绑定 libav；
- 创建超大型 God Class；
- 用 Singleton 解决所有依赖；
- 为简单功能建立过度复杂架构。

---

# 72. 避免 God Object

禁止产生类似：

```text
MediaManager
```

然后负责：

```text
FFmpeg
FFprobe
Jobs
Settings
Files
UI
Preset
Preview
Logs
```

应该拆分职责。

一个类尽量只有一个核心职责。

---

# 73. 避免过度设计

虽然强调架构，但不要提前创建几十个没有实际使用的 abstraction。

原则：

```text
当前需求需要
+
未来明显需要
=
现在设计
```

而不是：

```text
可能十年以后用得上
=
现在造 Framework
```

---

# 74. 新功能决策流程

新增功能时按以下顺序分析：

```text
1. 用户目标是什么？

2. FFmpeg 是否原生支持？

3. 是否可以使用已有 Pipeline？

4. 是否可以成为现有 Job？

5. 是否需要新的 Operation？

6. 是否需要新的 FFmpeg capability？

7. 是否影响 Output？

8. 是否需要新的 UI？

9. 是否需要新的 Model？

10. 是否需要新类？
```

尽量复用现有体系。

---

# 75. FFmpeg Feature Mapping

新功能实现前，优先确认它属于：

```text
Container
Codec
Filter
Mux
Demux
Metadata
Input
Output
```

不要看到一个新功能就立即写一个完全独立的 FFmpeg Pipeline。

---

# 76. 用户体验原则

目标不是最大化功能数量。

目标是：

```text
80% 用户
用 20% 参数
完成 95% 常见工作
```

高级参数放 Advanced。

---

# 77. 默认值

AI 添加参数时必须提供合理默认值。

禁止：

```text
打开页面
所有字段为空
必须全部设置才能运行
```

例如：

```text
Quality = Recommended
Resolution = Original
FPS = Original
Audio = Preserve
Output Directory = Source Directory
```

尽可能：

```text
拖入
→
点击 Convert
```

就可以成功。

---

# 78. Source 不可修改

默认必须视输入媒体为：

```text
immutable
```

所有操作生成新文件。

编辑 Pipeline 只保存参数。

---

# 79. 性能原则

不要为了性能提前复杂优化。

先保证：

```text
正确
稳定
不卡 UI
```

再优化：

```text
大量文件
高分辨率预览
大型 Queue
```

FFmpeg 才是主要 CPU/GPU Heavy 部分。

---

# 80. 内存原则

不要默认把大型视频整个读入内存。

文件处理应：

```text
stream/process based
```

图片 Preview 也应考虑大图缩略显示。

---

# 81. 项目代码风格

C++：

- 优先现代 C++；
- 使用 RAII；
- 尽可能避免裸 new/delete；
- 合理使用 smart pointer；
- 使用 enum class；
- 使用 std::optional 表示真正 Optional；
- QString 用于 Qt 边界与 UI；
- 不做无意义的 STL/Qt 容器转换；
- 保持 API 简洁。

---

# 82. QObject 使用原则

只有确实需要：

```text
signals
slots
properties
QML integration
parent ownership
```

的对象使用 QObject。

纯 Domain Data 不应全部继承 QObject。

错误：

```cpp
class CropSettings : public QObject
```

如果它只是普通数据结构。

可以：

```cpp
struct CropSettings
```

---

# 83. QML API 原则

暴露给 QML 的 C++ API 要稳定且简单。

避免让 QML 了解：

```text
FFmpeg syntax
Process implementation
Filter syntax
Raw JSON
```

QML 应看到：

```text
currentMedia
currentSettings
queueModel
start()
cancel()
```

---

# 84. 注释原则

注释解释：

```text
为什么
```

不要重复代码在做什么。

坏：

```cpp
// Increase index by one.
index++;
```

好：

```cpp
// FFmpeg's image sequence numbering starts at 1 for this preset.
index++;
```

---

# 85. Documentation

新增重要模块时应该补充：

```text
职责
输入
输出
生命周期
线程模型
错误行为
```

尤其：

```text
FFmpegProcess
JobQueue
CommandBuilder
FilterGraphBuilder
```

---

# 86. Commit / Task 粒度

AI 一次修改尽量聚焦一个明确目标。

例如：

```text
Implement ffprobe JSON parser
```

不要：

```text
重写整个媒体系统 + UI + Settings + Theme
```

小步演进优先。

---

# 87. 完成定义

一个功能只有满足以下条件才能认为完成：

```text
能正常使用

错误有处理

UI 不会卡死

Job 可以正确结束

输出路径正确

日志可查看

异常情况下不会破坏输入文件

基本测试通过
```

不仅仅是：

```text
代码能编译
```

---

# 88. 项目长期方向

理想的发展路线：

```text
v0.1
Media Converter + Image Toolbox

↓

v0.2
Practical Video Utilities

↓

v0.3
Advanced FFmpeg Tools

↓

v0.4+
Professional Workflows
```

只有基础架构充分稳定后，再考虑：

```text
Timeline
Waveform
Multi-track
Real-time Processing
libav Integration
GPU Zero-copy
```

---

# 89. 核心架构总结

必须长期保持：

```text
                  QML
                   │
                   ▼
            Application API
                   │
                   ▼
          Domain / Media Model
                   │
         ┌─────────┴─────────┐
         ▼                   ▼
     Job Queue          Media Pipeline
         │                   │
         └─────────┬─────────┘
                   ▼
          FFmpegCommandBuilder
                   │
           ┌───────┴────────┐
           ▼                ▼
     FilterGraphBuilder   Output Manager
           │                │
           └───────┬────────┘
                   ▼
             FFmpegProcess
                   │
                   ▼
                 FFmpeg
```

媒体探测：

```text
Media File
    │
    ▼
MediaProbeService
    │
    ▼
FFprobe
    │
    ▼
JSON
    │
    ▼
MediaFile Model
```

---

# 90. 十条最高优先级规则

如果 AI 只能记住十条规则，必须记住：

1. QML 负责 UI，C++ 负责业务。

2. 所有媒体信息统一通过 FFprobe Service。

3. 所有 FFmpeg 执行统一通过 FFmpegProcess。

4. 所有 FFmpeg 命令统一通过 CommandBuilder。

5. 不允许业务层直接保存 Filter 字符串。

6. 图片和视频处理使用结构化 Pipeline。

7. 所有处理必须成为 Job。

8. 所有 Job 统一由 JobQueue 管理。

9. FFmpeg 能力动态检测，绝不能假定 Encoder/Filter 存在。

10. v0.1 不做专业 Timeline/NLE，先把转换、图片处理和任务系统做好。

---

# 91. AI 开始工作前检查表

每次开始开发任务之前，AI 应在内部确认：

```text
[ ] 是否符合当前版本范围？

[ ] 是否已有模块负责此功能？

[ ] 是否可以复用现有数据结构？

[ ] 是否会产生重复 FFmpeg 调用逻辑？

[ ] 是否应该使用 Job？

[ ] 是否应该使用 Pipeline？

[ ] 是否应该使用 CommandBuilder？

[ ] FFmpeg capability 是否需要检测？

[ ] 是否会阻塞 UI？

[ ] 错误如何展示？

[ ] 输出文件如何处理？

[ ] 是否需要测试？

[ ] 是否引入不必要依赖？
```

---

# 92. AI 输出代码时的要求

AI 编写代码时：

1. 给出代码所在文件路径；
2. 尽量给出完整函数，而不是无法定位的碎片；
3. 如果新增文件，明确文件名；
4. 如果修改已有接口，说明受影响模块；
5. 不省略关键错误处理；
6. 不用伪代码代替核心实现；
7. 保持代码与现有架构一致；
8. 不擅自添加用户没有要求的大功能；
9. 对需要后续实现的部分明确标记 TODO；
10. TODO 不得代替当前任务的核心实现。

---

# 93. 最终原则

项目的核心不是：

```text
把 FFmpeg 所有参数塞进 GUI。
```

而是：

```text
把 FFmpeg 强大的多媒体能力，
整理成结构清晰、简单好用、
可批处理、可追踪、可扩展的桌面工作流。
```

任何架构设计和功能设计，都应该服务于这个目标。
