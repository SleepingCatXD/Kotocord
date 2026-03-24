// ASR 纯虚类接口(定义 start(), stop(), 信号 textReady)
#ifndef IAUDIOTRANSCRIBER_H
#define IAUDIOTRANSCRIBER_H

#include <QObject>
#include <QString>

// 这是所有语音识别引擎的抽象基类
class IAudioTranscriber : public QObject {
    Q_OBJECT
public:
    explicit IAudioTranscriber(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~IAudioTranscriber() = default;

    // 启动/停止识别 (需由子类实现具体逻辑)
    virtual bool start() = 0;
    virtual void stop() = 0;

//新增这部分：纯虚槽函数，让子类必须实现它们
public slots:
	virtual void onAudioDataReady(const QByteArray& data) = 0;
	virtual void onAudioStreamFinished() = 0;

signals:
    // 核心信号：当引擎识别出文字时触发。
    // isFinal = false: 比如你说 "你好"，还在识别中，用来做界面实时滚动显示。
    // isFinal = true: 比如你说完了，停顿了，这句 "你好世界" 是最终结果，可以发给大模型了。
    void textReady(const QString& text, bool isFinal);

    // 报错信号
    void errorOccurred(const QString& errorMsg);
};

#endif // IAUDIOTRANSCRIBER_H
