//负责 QAudioSource 录音和重采样
#ifndef AUDIOCAPTURE_H
#define AUDIOCAPTURE_H

#include <QObject>
#include <QAudioSource>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QByteArray>
#include <QIODevice>

class AudioCapture : public QObject {
    Q_OBJECT
public:
    explicit AudioCapture(QObject* parent = nullptr);
    ~AudioCapture();

    // 控制接口
    bool start();
    void stop();
    bool isRecording() const;

signals:
    // 当麦克风捕获到新的音频块时，触发此信号
    void audioDataReady(const QByteArray& data);
    // 错误信息抛出，方便在 UI 上显示
    void errorOccurred(const QString& errorMsg);

private slots:
    // 处理底层设备发来的 readyRead 信号
    void handleReadyRead();

private:
    QAudioSource* m_audioSource;
    QIODevice* m_device; // QAudioSource 返回的内部设备指针
    QAudioFormat m_format;
    bool m_isRecording;
};

#endif // AUDIOCAPTURE_H