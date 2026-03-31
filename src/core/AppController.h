// 核心枢纽：串联输入、LLM、渲染和 OSC

#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QObject>
#include <QQueue> 
#include <QTimer>


#include "DataTypes.h"
#include "../modules/llm/ILanguageModel.h"
#include "../modules/render/SubtitleRenderer.h"
#include "../modules/input/IAudioTranscriber.h"
#include "../modules/llm/KaomojiManager.h"

// 在顶部前向声明
class KaomojiManager;

class AppController : public QObject {
    Q_OBJECT
public:
    explicit AppController(QObject* parent = nullptr);

    // 依赖注入：告诉控制器使用哪个 LLM 和渲染窗口
    void setLanguageModel(ILanguageModel* llm);
    void setRenderWidget(SubtitleRenderer* renderWidget);

    void setLLMEnabled(bool enabled);
    // 新增：注入颜文字管理器的接口
    void setKaomojiManager(KaomojiManager* manager);

public slots:
    // 接收来自 MainWindow 的手动文本输入
    void onManualTextEntered(const QString& text);
    void onASRTextReady(const QString& text, bool isFinal);

private slots:
    // 接收来自 LLM 的处理结果
    void onLLMTextProcessed(const SubtitleFrame& frame);
	void processNextInQueue(); //处理队列中下一句话的槽函数

private:
    ILanguageModel* m_llm;
    SubtitleRenderer* m_renderWidget;
    bool m_llmEnabled; // 记录 LLM 开关状态
    KaomojiManager* m_kaomojiManager;
	qint64 m_currentFrameId;

	// --- 队列与锁控机制 ---
	bool m_isScreenLocked;                // 视觉锁：为 true 时截断 ASR 的半截话
	QQueue<SubtitleFrame> m_pendingQueue; // 句子队列：缓存来不及处理的整句话
	QTimer m_unlockTimer;                 // 解锁定时器：控制画面停留时间
};

#endif // APPCONTROLLER_H
