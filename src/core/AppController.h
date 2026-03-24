// 核心枢纽：串联输入、LLM、渲染和 OSC

#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QObject>
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

private:
    ILanguageModel* m_llm;
    SubtitleRenderer* m_renderWidget;
    bool m_llmEnabled; // 记录 LLM 开关状态
    KaomojiManager* m_kaomojiManager;
	qint64 m_currentFrameId;
};

#endif // APPCONTROLLER_H
