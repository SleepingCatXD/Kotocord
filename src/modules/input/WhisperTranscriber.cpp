#include "WhisperTranscriber.h"
#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include <vector>

WhisperTranscriber::WhisperTranscriber(QObject* parent)
	: IAudioTranscriber(parent),m_ctx(nullptr),m_isRunning(false) {}

WhisperTranscriber::~WhisperTranscriber() {
	stop();
}

bool WhisperTranscriber::start() {
	if(m_ctx) return true; // 已经启动了

	// 动态拼接模型路径
	QString exeDir = QCoreApplication::applicationDirPath();
	QString modelPath = QDir(exeDir).filePath("../resources/model/ggml-small.bin");
	modelPath = QDir::cleanPath(modelPath);

	qDebug() << "[Whisper] 正在加载模型:" << modelPath;

	//核心：初始化 Whisper 上下文
	m_ctx = whisper_init_from_file(modelPath.toUtf8().constData());
	if(!m_ctx) {
		emit errorOccurred("无法加载 Whisper 模型，请检查路径和文件是否正确！");
		return false;
	}

	m_isRunning = true;
	m_audioBuffer.clear();
	qDebug() << "[Whisper] GGML 引擎启动成功！";
	return true;
}

void WhisperTranscriber::stop() {
	m_isRunning = false;
	if(m_ctx) {
		whisper_free(m_ctx);
		m_ctx = nullptr;
	}
	qDebug() << "[Whisper] 引擎已安全释放。";
}

void WhisperTranscriber::onAudioDataReady(const QByteArray& data) {
	if(!m_isRunning) return;

	bool shouldTriggerInference = false;

	{   // --- 开始临界区 ---
		QMutexLocker locker(&m_bufferMutex);
		m_audioBuffer.append(data);

		// 1. 解析这段音频的音量 (基于 16-bit PCM)
		const int16_t* pcm = reinterpret_cast<const int16_t*>(data.constData());
		int samples = data.size() / 2;
		bool hasVoice = false;

		// 寻找这段数据里的最大音量
		for(int i = 0; i < samples; ++i) {
			if(std::abs(pcm[i]) > VAD_THRESHOLD) {
				hasVoice = true;
				break;
			}
		}

		// 2. 状态机流转：判断是否达到了断句条件
		if(hasVoice) {
			m_isSpeaking = true;
			m_silenceBytes = 0; // 打断施法，重新计算静音时间
		} else if(m_isSpeaking) {
			m_silenceBytes += data.size();

			// 如果连续静音时间超过了 0.8 秒！
			if(m_silenceBytes > MAX_SILENCE_BYTES) {
				m_isSpeaking = false;
				m_silenceBytes = 0;
				shouldTriggerInference = true; // 标记：需要触发大模型了！
			}
		}
	}   // --- 临界区结束 (locker自动解锁) ---

	// 3. 必须在锁释放后调用推理，防止死锁
	if(shouldTriggerInference) {
		qDebug() << "[VAD] 检测到停顿，触发 Whisper 推理...";
		processBufferInference();
	}
}

void WhisperTranscriber::onAudioStreamFinished() {
	// 强制把最后没说完的缓冲推给模型识别
	if(!m_audioBuffer.isEmpty()) {
		processBufferInference();
	}
}

void WhisperTranscriber::processBufferInference() {
	if(!m_ctx || m_audioBuffer.isEmpty()) return;

	QMutexLocker locker(&m_bufferMutex);

	// 1. 简单的 WAV 头部跳过 (如果是纯 PCM 则不需要，这里做个兼容)
	int offset = 0;
	if(m_audioBuffer.size() > 44 && m_audioBuffer.startsWith("RIFF")) {
		offset = 44;
	}

	// 2. 将 16-bit PCM 转换为 Whisper 需要的 32-bit Float (-1.0 ~ 1.0)
	const int16_t* pcm16 = reinterpret_cast<const int16_t*>(m_audioBuffer.constData() + offset);
	int samples = (m_audioBuffer.size() - offset) / 2;

	std::vector<float> pcmf32(samples);
	for(int i = 0; i < samples; ++i) {
		pcmf32[i] = static_cast<float>(pcm16[i]) / 32768.0f;
	}

	qDebug() << "[Whisper] 开始推理，音频采样点数:" << samples;

	// 3. 配置推理参数
	whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
	wparams.language = "zh"; // 强制中文，或者填 "auto" 自动检测
	wparams.print_progress = false;

	// 4. 执行核心推理运算！(这里会吃满 CPU，所以后续我们要把它移到独立线程)
	if(whisper_full(m_ctx,wparams,pcmf32.data(),pcmf32.size()) != 0) {
		emit errorOccurred("Whisper 推理失败！");
		m_audioBuffer.clear();
		return;
	}

	// 5. 提取识别结果
	QString resultText;
	const int n_segments = whisper_full_n_segments(m_ctx);
	for(int i = 0; i < n_segments; ++i) {
		const char* text = whisper_full_get_segment_text(m_ctx,i);
		resultText += QString::fromUtf8(text);
	}

	resultText = resultText.trimmed(); // 去除头尾空格

	if(!resultText.isEmpty()) {
		qDebug() << "[Whisper] 识别结果:" << resultText;
		emit textReady(resultText,true); // 发送给指挥官 AppController！
	}

	// 6. 清空缓冲区，准备听下一句话
	m_audioBuffer.clear();
}
