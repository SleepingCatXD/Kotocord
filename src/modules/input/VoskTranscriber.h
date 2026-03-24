//继承自 IAudioTranscriber
#ifndef VOSKTRANSCRIBER_H
#define VOSKTRANSCRIBER_H

#include "IAudioTranscriber.h"
#include <QByteArray>
// 引入 vosk 核心头文件
#include "vosk_api.h" 

class VoskTranscriber : public IAudioTranscriber {
    Q_OBJECT
public:
    explicit VoskTranscriber(QObject* parent = nullptr);
    ~VoskTranscriber() override;

    bool start() override;
    void stop() override;

public slots:
    // 接收来自 AudioCapture 或 AudioFileSimulator 的 PCM 数据
    void onAudioDataReady(const QByteArray& data);
    //新增：当音频文件读取完毕，或者麦克风被手动关闭时调用
    void onAudioStreamFinished();

private:
    VoskModel* m_model;
    VoskRecognizer* m_recognizer;
    bool m_isRunning;

    // 解析 Vosk 返回的 JSON 字符串
    void parseAndEmitResult(const char* jsonStr, bool isFinal);
};

#endif // VOSKTRANSCRIBER_H
