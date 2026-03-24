#ifndef WHISPERTRANSCRIBER_H
#define WHISPERTRANSCRIBER_H

#include "IAudioTranscriber.h"
#include "whisper.h"
#include <QThread>
#include <QMutex>
#include <atomic>
#include <QPointer>

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

	// --- VAD 断句算法所需变量 ---
	bool m_isSpeaking = false;
	int m_silenceBytes = 0;
	// 能量阈值 (16bit音频最大振幅是 32768，500 左右相当于极其微弱的底噪)
	const int VAD_THRESHOLD = 500;
	// 停顿多久算作断句？16kHz采样率，16位位深(2字节)，0.8秒 = 16000 * 2 * 0.8 = 25600 字节
	const int MAX_SILENCE_BYTES = 25600;

	// --- 后台异步推理状态控制 ---
	std::atomic<bool> m_isInferencing; // 防止多次重复触发
	QPointer<QThread> m_inferenceThread;        // 追踪后台线程，防止安全释放时崩溃

	// VAD (断句检测) 和后台推理线程的逻辑
	void processBufferInference();
};

#endif // WHISPERTRANSCRIBER_H
