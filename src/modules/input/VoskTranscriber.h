#ifndef VOSKTRANSCRIBER_H
#define VOSKTRANSCRIBER_H

#include "IAudioTranscriber.h"
#include <QByteArray>
#include "vosk_api.h" 

class VoskTranscriber : public IAudioTranscriber {
    Q_OBJECT
public:
    explicit VoskTranscriber(QObject* parent = nullptr);
    ~VoskTranscriber() override;

    bool start() override;
    void stop() override;

public slots:
    void onAudioDataReady(const QByteArray& data); //接收声音PCM数据
    void onAudioStreamFinished();//声音数据输入完成

private:
    VoskModel* m_model;
    VoskRecognizer* m_recognizer;
    bool m_isRunning;

    // 解析 Vosk 返回的 JSON 字符串
    void parseAndEmitResult(const char* jsonStr, bool isFinal);
};

#endif // VOSKTRANSCRIBER_H
