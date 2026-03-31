#include "WhisperTranscriber.h"
#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include <vector>

WhisperTranscriber::WhisperTranscriber(QObject* parent)
	: IAudioTranscriber(parent),
	m_ctx(nullptr),
	m_isRunning(false),
	m_isInferencing(false),
	m_inferenceThread(nullptr) {}

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
	struct whisper_context_params cparams = whisper_context_default_params();
	m_ctx = whisper_init_from_file_with_params(modelPath.toUtf8().constData(),cparams);
	if(!m_ctx) {
		emit errorOccurred("无法加载 Whisper 模型，请检查路径和文件是否正确！");
		return false;
	}

	m_isRunning = true;
	m_audioBuffer.clear();
	qDebug() << "[Whisper] GGML 引擎启动成功！";
	return true;
}

//安全停止
void WhisperTranscriber::stop() {
	m_isRunning = false;

	// 如果后台正在狂算，必须等它算完再释放模型，否则必定闪退！
	if(m_inferenceThread && m_inferenceThread->isRunning()) {
		qDebug() << "[Whisper] 等待后台推理线程安全退出...";
		m_inferenceThread->wait();
	}

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
	if(!m_ctx) return;

	// 如果后台还在算上一句，就忽略这次触发（或者你也可以做成队列排队）
	if(m_isInferencing.load()) {
		qDebug() << "[Whisper] 后台正在推理，本次 VAD 触发已跳过";
		return;
	}

	QByteArray bufferCopy;
	{
		// 1. 获取锁，火速将当前积攒的声音切片拷贝出来，然后立刻清空原始盒子！
		// 这样滴管（麦克风）就能继续无阻碍地往原始盒子里滴水，完全不被阻塞。
		QMutexLocker locker(&m_bufferMutex);
		if(m_audioBuffer.isEmpty()) return;

		bufferCopy = m_audioBuffer;
		m_audioBuffer.clear();
	}

	// 上锁，标记正在推理
	m_isInferencing.store(true);

	// 2. 创建并剥离独立线程！
	m_inferenceThread = QThread::create([this,bufferCopy]() {
		// --- 以下代码全在后台线程运行，彻底释放主线程！ ---

		int offset = 0;
		if(bufferCopy.size() > 44 && bufferCopy.startsWith("RIFF")) offset = 44;

		int samples = (bufferCopy.size() - offset) / 2;
		const int16_t* pcm16 = reinterpret_cast<const int16_t*>(bufferCopy.constData() + offset);

		std::vector<float> pcmf32(samples);
		for(int i = 0; i < samples; ++i) {
			pcmf32[i] = static_cast<float>(pcm16[i]) / 32768.0f;
		}

		qDebug() << "[Whisper-Thread] 后台开始推理，采样点数:" << samples;

		whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
		wparams.language = "zh";
		wparams.print_progress = false;

		// 执行硬核计算...
		if(whisper_full(m_ctx,wparams,pcmf32.data(),pcmf32.size()) != 0) {
			emit errorOccurred("Whisper 后台推理失败！");
		} else {
			QString resultText;
			const int n_segments = whisper_full_n_segments(m_ctx);
			for(int i = 0; i < n_segments; ++i) {
				const char* text = whisper_full_get_segment_text(m_ctx,i);
				resultText += QString::fromUtf8(text);
			}

			resultText = resultText.trimmed();
			if(!resultText.isEmpty()) {
				qDebug() << "[Whisper-Thread] 推理完毕:" << resultText;
				// 跨线程发射信号给主线程的 AppController，完美合规
				emit textReady(resultText,true);
			}
		}

		// 解除推理锁
		m_isInferencing.store(false);
	});

	// 3. 线程运行结束后，自动清理自身内存，防止内存泄漏
	connect(m_inferenceThread,&QThread::finished,m_inferenceThread,&QObject::deleteLater);

	// 扣动扳机，后台起飞！
	m_inferenceThread->start();
}
