# KotoCord (V1.0)

## 项目简介
本项目旨在开发一款面向 VRChat 无言势及 VTuber 的综合性直播辅助工具。
核心目标是通过集成语音识别（Speech-to-Text, STT）、大语言模型（Large Language Model, LLM）情绪分析与虚拟形象驱动，
将用户的语音或文本输入转化为带有情绪色彩的艺术字字幕，并实时驱动 Live2D/VRM 虚拟形象的表情，从而增强直播的互动性和表现力。

## 编译与环境搭建
本项目依赖 Qt6 和 Vosk-API。在编译前，请手动配置以下依赖：

### 准备 Vosk-API

前往 Vosk Releases 下载 vosk-win64.zip。

在项目根目录的 third_party/ 下创建 vosk 文件夹。

将解压出的 vosk_api.h 放入 third_party/vosk/include/。

将解压出的 libvosk.dll 和 libvosk.lib 放入 third_party/vosk/lib/。

### 准备语音模型

前往 Vosk Models 下载轻量级中文模型。

将模型解压到 resources/model/ 目录下。

## 开源协议与致谢 (Acknowledgments)
Kotocord 的诞生离不开以下优秀的开源项目：

Qt 6 - UI 框架与多媒体处理。 (使用 LGPL v3 协议)

Vosk API - 离线流式语音识别。 (使用 Apache 2.0 协议)

Whisper.cpp - 高精度语音识别引擎。(使用 MIT 协议)

感谢这些伟大的开发者为开源社区做出的贡献！