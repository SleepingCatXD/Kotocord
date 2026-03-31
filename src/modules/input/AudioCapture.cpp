#include "AudioCapture.h"
#include <QDebug>

AudioCapture::AudioCapture(QObject* parent)
	: QObject(parent),m_audioSource(nullptr),m_audioDevice(nullptr) {}

AudioCapture::~AudioCapture() {
	stop();
}

bool AudioCapture::start() {
	if(m_audioSource) return true; // 已经在录音了

	// 1. 检查系统是否有可用的麦克风输入设备
	QAudioDevice defaultDevice = QMediaDevices::defaultAudioInput();
	if(defaultDevice.isNull()) {
		emit errorOccurred("未检测到任何麦克风设备，请检查硬件连接！");
		return false;
	}

	qDebug() << "[AudioCapture] 找到默认麦克风:" << defaultDevice.description();

	// 2. 极其严格的格式对齐 (为 AI 语音模型定制)
	QAudioFormat format;
	format.setSampleRate(16000);                // 16kHz
	format.setChannelConfig(QAudioFormat::ChannelConfigMono); // 单声道
	format.setSampleFormat(QAudioFormat::Int16); // 16位 PCM (也就是 Short)

	// 检查麦克风是否支持这个格式 (有些高端声卡强制 48kHz，需要系统重采样)
	if(!defaultDevice.isFormatSupported(format)) {
		qDebug() << "[AudioCapture] 警告：麦克风原生不支持 16kHz 16-bit 单声道，Qt 将尝试系统级重采样！";
	}

	// 3. 实例化音频源
	m_audioSource = new QAudioSource(defaultDevice,format,this);

	// 设置内部缓冲区大小 (比如 200ms 的数据)，避免频繁中断
	// 16000(率) * 2(字节) * 1(声道) = 32000 Bytes/s. 200ms = 6400 Bytes.
	m_audioSource->setBufferSize(6400);

	// 监听状态变化
	connect(m_audioSource,&QAudioSource::stateChanged,this,&AudioCapture::onStateChanged);

	// 4. 启动录音！(返回一个 QIODevice 让我们读取实时数据)
	m_audioDevice = m_audioSource->start();

	if(!m_audioDevice) {
		emit errorOccurred("无法打开麦克风！可能是权限被拒绝，或被其他程序独占。");
		delete m_audioSource;
		m_audioSource = nullptr;
		return false;
	}

	// 5. 绑定实时读取信号
	connect(m_audioDevice,&QIODevice::readyRead,this,&AudioCapture::onReadyRead);

	qDebug() << "[AudioCapture] 麦克风已开启，开始持续监听...";
	return true;
}

void AudioCapture::stop() {
	if(m_audioSource) {
		m_audioSource->stop();
		delete m_audioSource;
		m_audioSource = nullptr;
		m_audioDevice = nullptr;
		qDebug() << "[AudioCapture] 麦克风已关闭。";
	}
}

void AudioCapture::onReadyRead() {
	if(!m_audioDevice) return;

	// 一口气读出当前缓冲区里所有的声音切片
	QByteArray data = m_audioDevice->readAll();

	if(!data.isEmpty()) {
		// 直接顺着信号管子，把 PCM 数据发射给下游的双引擎
		emit audioDataReady(data);
	}
}

void AudioCapture::onStateChanged(QAudio::State state) {
	if(state == QAudio::StoppedState) {
		if(m_audioSource && m_audioSource->error() != QAudio::NoError) {
			emit errorOccurred("麦克风工作异常，错误代码: " + QString::number(m_audioSource->error()));
		}
	}
}
