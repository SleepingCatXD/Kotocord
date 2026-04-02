#ifndef AUDIOCAPTURE_H
#define AUDIOCAPTURE_H

#include <QObject>
#include <QByteArray>
#include <QAudioSource>
#include <QIODevice>
#include <QAudioDevice>
#include <QAudioFormat>
#include <QMediaDevices>

class AudioCapture: public QObject {
	Q_OBJECT
public:
	explicit AudioCapture(QObject* parent = nullptr);
	~AudioCapture();

	bool start();
	void stop();

signals:
	// 源源不断吐出 PCM 原始数据的信号
	void audioDataReady(const QByteArray& data);
	// 报错信号
	void errorOccurred(const QString& errorMsg);

private slots:
	// 当麦克风缓冲区有新数据时触发
	void onReadyRead();
	// 监听设备状态变化 (如麦克风被突然拔出)
	void onStateChanged(QAudio::State state);

private:
	QAudioSource* m_audioSource;
	QIODevice* m_audioDevice; // 麦克风的实时数据流句柄
};

#endif // AUDIOCAPTURE_H
