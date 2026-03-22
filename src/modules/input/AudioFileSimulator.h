#ifndef AUDIOFILESIMULATOR_H
#define AUDIOFILESIMULATOR_H

#include <QObject>
#include <QFile>
#include <QTimer>
#include <QByteArray>

class AudioFileSimulator : public QObject {
    Q_OBJECT
public:
    explicit AudioFileSimulator(QObject* parent = nullptr);
    ~AudioFileSimulator();

    // 传入你的 WAV 文件路径开始模拟
    bool start(const QString& filePath);
    void stop();

signals:
    // 模拟麦克风，源源不断地吐出 PCM 数据
    void audioDataReady(const QByteArray& data);
    void finished(); // 文件读完了
    void errorOccurred(const QString& errorMsg);

private slots:
    void readNextChunk();

private:
    QFile m_file;
    QTimer m_timer;
    const int CHUNK_SIZE = 3200; // 16kHz * 16bit(2bytes) * 1ch * 0.1s = 3200 bytes
};

#endif // AUDIOFILESIMULATOR_H