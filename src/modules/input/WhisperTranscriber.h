#ifndef WHISPERTRANSCRIBER_H
#define WHISPERTRANSCRIBER_H

#include "IAudioTranscriber.h"
#include "whisper.h"
#include <QThread>
#include <QMutex>

// 前向声明 whisper_context (隐藏底层 C API 细节)
struct whisper_context;

class WhisperTranscriber: public IAudioTranscriber {
	Q_OBJECT
public:
	explicit WhisperTranscriber(QObject* parent = nullptr);
	~WhisperTranscriber() override;

	bool start() override;
	void stop() override;

public slots:
	// 统一接口：接收音频滴管传来的数据
	void onAudioDataReady(const QByteArray& data) override;
	void onAudioStreamFinished() override;

private:
	whisper_context* m_ctx;
	bool m_isRunning;

	// Whisper 需要累积一段完整的音频 (通常至少 1-3 秒) 才能进行一次有效推理
	// 所以我们需要一个内部缓冲区来暂存音频数据
	QByteArray m_audioBuffer;
	QMutex m_bufferMutex;

	// 预留给未来 VAD (断句检测) 和后台推理线程的逻辑
	void processBufferInference();
};

#endif // WHISPERTRANSCRIBER_H
