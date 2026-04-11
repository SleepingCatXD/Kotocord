# KotoCord (V1.0)

## 项目简介
本项目旨在开发一款面向 VRChat 无言势及 VTuber 的综合性直播辅助工具。
核心目标是通过集成语音识别（Speech-to-Text, STT）、大语言模型（Large Language Model, LLM）情绪分析与虚拟形象驱动，
将用户的语音或文本输入转化为带有情绪色彩的艺术字字幕，并实时驱动 Live2D/VRM 虚拟形象的表情，从而增强直播的互动性和表现力。

## 环境依赖与编译指南

本项目使用 CMake 构建，底层依赖 `Vosk` 和 `Whisper.cpp` 作为本地语音识别引擎。为了简化环境配置，推荐的第三方库结构如下：

### 目录结构要求
请在项目根目录下创建 `third_party` 文件夹，并严格按照以下结构放置头文件与动态/静态链接库（预编译的 Windows DLL/LIB Linux SO 可以从它们的官方 Release 页面获取）：
[vosk仓库](https://github.com/alphacep/vosk-api)
[whisper仓库](https://github.com/ggml-org/whisper.cpp)

```text
third_party
 ├── vosk
 │   ├── include/ (放置 vosk_api.h)
 │   └── lib/     (放置 libvosk.dll, libvosk.lib 等运行时依赖)
 └── whisper
     ├── include/ (放置 whisper.h)
     ├── lib/     (放置 whisper.dll, whisper.lib)
     └── ggml
         ├── include/ (放置 ggml.h 等所有 ggml 相关头文件)
         └── lib/     (放置 ggml.dll, ggml-cpu.dll 等)
```



### 模型文件准备

* **Vosk 模型**：下载轻量级中文模型，解压至 `resources/model/vosk-model-small-cn/`。
* **Whisper 模型**：下载 [ggml-small.bin](https://huggingface.co/ggerganov/whisper.cpp/blob/main/ggml-small.bin)，放置于 `resources/model/` 目录下。

### 构建步骤
确保已安装 Qt6 (含 Multimedia 组件) 与 CMake，随后执行标准 CMake 流程即可。构建完成后，脚本会自动将 `third_party` 中的 DLL 拷贝至 `bin/` 目录。

## 开源协议与致谢 (Acknowledgments)
Kotocord 的诞生离不开以下优秀的开源项目：

Qt 6 - UI 框架与多媒体处理。 (使用 LGPL v3 协议)

Vosk API - 离线流式语音识别。 (使用 Apache 2.0 协议)

Whisper.cpp - 高精度语音识别引擎。(使用 MIT 协议)

感谢这些伟大的开发者为开源社区做出的贡献！
