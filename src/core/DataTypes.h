//定义subtitleFrame，进行文本流通
#ifndef DATATYPES_H
#define DATATYPES_H

#include <QString>

// 定义支持的情绪类型 (未来可以通过 OSC 映射到虚拟形象的 Blendshapes/Morphs)
enum class EmotionType {
    Neutral = 0, // 平静
    Joy,         // 开心/喜悦
    Sadness,     // 悲伤
    Anger,       // 生气
    Surprise     // 惊讶
};

// 贯穿整个程序的数据包
struct SubtitleFrame {
    QString rawText;        // 原始输入文本 (手打的或 ASR 识别的)
    QString displayText;    // 最终显示的文本 (LLM 润色后、加了颜文字的版本)
    EmotionType emotion;    // 当前语句的情绪
    bool isFinal;           // 是否为完整的一句话 (目前手动输入始终为 true)

    // 默认构造函数
    SubtitleFrame() : emotion(EmotionType::Neutral), isFinal(true) {}
};

#endif // DATATYPES_H