#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QObject>
#include <QQueue> 
#include <QTimer>

#include "DataTypes.h"
#include "../modules/llm/ILanguageModel.h"
//#include "../modules/render/SubtitleRenderer.h"
//#include "../modules/input/IAudioTranscriber.h"
#include "../modules/llm/KaomojiManager.h"

class KaomojiManager;

class AppController : public QObject {
    Q_OBJECT
public:
    explicit AppController(QObject* parent = nullptr);

    void setLanguageModel(ILanguageModel* llm);//依赖注入：选择LLM
    //void setRenderWidget(SubtitleRenderer* renderWidget);
    void setLLMEnabled(bool enabled);
    void setKaomojiManager(KaomojiManager* manager);// 注入颜文字管理器的接口

signals:
	void subtitleReadyForRender(const SubtitleFrame& frame);//文本渲染帧已准备好

public slots:
    void onManualTextEntered(const QString& text);//接收手动文本输入
    void onASRTextReady(const QString& text, bool isFinal);//接收语音文本解析输入

private slots:
    void onLLMTextProcessed(const SubtitleFrame& frame);//接收LLM处理结果
	void processNextInQueue(); //处理队列中下一句话的槽函数

private:
    ILanguageModel* m_llm;
    //SubtitleRenderer* m_renderWidget;
    bool m_llmEnabled; // 记录 LLM 开关状态
    KaomojiManager* m_kaomojiManager;
	qint64 m_currentFrameId;

	// --- 队列与锁控机制 ---
	bool m_isScreenLocked;                // 视觉锁：为 true 时截断 ASR 的半截话
	QQueue<SubtitleFrame> m_pendingQueue; // 句子队列：缓存来不及处理的整句话
	QTimer m_unlockTimer;                 // 解锁定时器：控制画面停留时间
};

#endif // APPCONTROLLER_H
