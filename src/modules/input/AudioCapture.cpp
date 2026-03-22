#include "AudioCapture.h"
#include <QDebug>

AudioCapture::AudioCapture(QObject* parent)
    : QObject(parent), m_audioSource(nullptr), m_device(nullptr), m_isRecording(false) {

    // 配置 ASR 黄金标准格式: 16kHz, 16-bit, Mono
    m_format.setSampleRate(16000);
    m_format.setChannelCount(1);
    m_format.setSampleFormat(QAudioFormat::Int16);
}

AudioCapture::~AudioCapture() {
    stop();
}

bool AudioCapture::start() {
    if (m_isRecording) return true;

    // 获取默认麦克风
    QAudioDevice defaultDevice = QMediaDevices::defaultAudioInput();
    if (defaultDevice.isNull()) {
        emit errorOccurred("No microphone detected!");
        return false;
    }

    // 检查设备是否支持我们的格式，如果不支持，Qt 底层通常会自动重采样，但打个日志有助于排错
    if (!defaultDevice.isFormatSupported(m_format)) {
        qWarning() << "Default format not supported, Qt will attempt to convert audio.";
    }

    // 初始化 QAudioSource
    m_audioSource = new QAudioSource(defaultDevice, m_format, this);

    // 设置缓冲区大小，避免延迟过高（这里设置为 100ms 左右的数据量）
    m_audioSource->setBufferSize(16000 * 2 * 1 * 0.1);

    // 以推模式 (Push) 启动：它会返回一个只读的 QIODevice
    m_device = m_audioSource->start();

    if (!m_device) {
        emit errorOccurred("Failed to open microphone.");
        delete m_audioSource;
        m_audioSource = nullptr;
        return false;
    }

    // 绑定数据读取信号
    connect(m_device, &QIODevice::readyRead, this, &AudioCapture::handleReadyRead);

    m_isRecording = true;
    qDebug() << "Microphone started. Target format: 16kHz, 16bit, Mono.";
    return true;
}

void AudioCapture::stop() {
    if (!m_isRecording) return;

    if (m_audioSource) {
        m_audioSource->stop();
        delete m_audioSource;
        m_audioSource = nullptr;
    }
    m_device = nullptr; // 设备指针由 QAudioSource 管理，无需手动 delete
    m_isRecording = false;
    qDebug() << "Microphone stopped.";
}

bool AudioCapture::isRecording() const {
    return m_isRecording;
}

void AudioCapture::handleReadyRead() {
    if (!m_device) return;

    // 读取所有可用音频数据
    QByteArray data = m_device->readAll();

    if (data.size() > 0) {
        // 发送给上层模块 (比如 VoskTranscriber)
        emit audioDataReady(data);
    }
}