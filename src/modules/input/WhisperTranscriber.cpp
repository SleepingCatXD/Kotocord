#include "WhisperTranscriber.h"
#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include "whisper.h" // 等你放好头文件后取消注释

WhisperTranscriber::WhisperTranscriber(QObject* parent)
	: IAudioTranscriber(parent),m_ctx(nullptr),m_isRunning(false) {}

WhisperTranscriber::~WhisperTranscriber() {
	stop();
}

bool WhisperTranscriber::start() {
	qDebug() << "[Whisper] 正在初始化 GGML 引擎...";
	// 未来在这里调用 whisper_init_from_file(...) 加载 .bin 模型

	m_isRunning = true;
	m_audioBuffer.clear();
	return true;
}

void WhisperTranscriber::stop() {
	m_isRunning = false;
	// 未来在这里调用 whisper_free(m_ctx)
	qDebug() << "[Whisper] 引擎已关闭。";
}

void WhisperTranscriber::onAudioDataReady(const QByteArray& data) {
	if(!m_isRunning) return;

	// Whisper 不能像 Vosk 那样来一点吃一点，它需要攒够一句话！
	QMutexLocker locker(&m_bufferMutex);
	m_audioBuffer.append(data);

	// TODO: 这里需要引入 VAD 算法，判断是否停顿了。
	// 如果停顿了，就触发 processBufferInference() 把这段 buffer 喂给大模型推理。
}

void WhisperTranscriber::onAudioStreamFinished() {
	// 强制把最后没说完的缓冲推给模型识别
	if(!m_audioBuffer.isEmpty()) {
		processBufferInference();
	}
}

void WhisperTranscriber::processBufferInference() {
	// 核心推理逻辑预留地
	// whisper_full(...)
	// emit textReady("识别出的文本", true);
}
