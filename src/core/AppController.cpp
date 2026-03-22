// 核心枢纽：串联输入、LLM、渲染和 OSC
#include "AppController.h"
#include <QDebug>

AppController::AppController(QObject* parent)
    : QObject(parent), m_llm(nullptr), m_renderWidget(nullptr) {
}

void AppController::setLanguageModel(ILanguageModel* llm) {
    m_llm = llm;
    // 当 LLM 处理完毕时，连接到控制器的槽函数
    if (m_llm) {
        connect(m_llm, &ILanguageModel::textProcessed, this, &AppController::onLLMTextProcessed);
    }
}

void AppController::setRenderWidget(TransparentWidget* renderWidget) {
    m_renderWidget = renderWidget;
}

void AppController::onManualTextEntered(const QString& text) {
    if (text.isEmpty()) return;

    qDebug() << "[AppController] 收到手动输入:" << text;

    // 封装成基础的数据帧
    SubtitleFrame frame;
    frame.rawText = text;
    frame.isFinal = true;

    // 路由判断：如果有 LLM，就交给 LLM 处理；如果没有，直接渲染
    if (m_llm) {
        m_llm->processText(frame);
    }
    else {
        frame.displayText = text;
        onLLMTextProcessed(frame); // 直接跳到渲染步骤
    }
}

void AppController::onLLMTextProcessed(const SubtitleFrame& frame) {
    qDebug() << "[AppController] 准备渲染最终文本:" << frame.displayText;

    if (m_renderWidget) {
        m_renderWidget->setText(frame.displayText);
    }

    // 未来这里还要加上：if (m_oscClient) { m_oscClient->sendEmotion(frame.emotion); }
}