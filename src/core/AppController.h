// 核心枢纽：串联输入、LLM、渲染和 OSC

#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QObject>
#include "DataTypes.h"
#include "../modules/llm/ILanguageModel.h"
#include "../ui/TransparentWidget.h"

class AppController : public QObject {
    Q_OBJECT
public:
    explicit AppController(QObject* parent = nullptr);

    // 依赖注入：告诉控制器使用哪个 LLM 和渲染窗口
    void setLanguageModel(ILanguageModel* llm);
    void setRenderWidget(TransparentWidget* renderWidget);

public slots:
    // 接收来自 MainWindow 的手动文本输入
    void onManualTextEntered(const QString& text);

private slots:
    // 接收来自 LLM 的处理结果
    void onLLMTextProcessed(const SubtitleFrame& frame);

private:
    ILanguageModel* m_llm;
    TransparentWidget* m_renderWidget;
};

#endif // APPCONTROLLER_H