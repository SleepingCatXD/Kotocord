// 核心枢纽：串联输入、LLM、渲染和 OSC
#include "AppController.h"
#include "../modules/llm/KaomojiManager.h"
#include <QDebug>

AppController::AppController(QObject* parent)
    : QObject(parent), m_llm(nullptr), m_renderWidget(nullptr), m_llmEnabled(true),m_kaomojiManager (nullptr){
}

void AppController::setLanguageModel(ILanguageModel* llm) {
    m_llm = llm;
    // 当 LLM 处理完毕时，连接到控制器的槽函数
    if (m_llm) {
        connect(m_llm, &ILanguageModel::textProcessed, this, &AppController::onLLMTextProcessed);
    }
}

void AppController::setRenderWidget(SubtileRenderer* renderWidget) {
    m_renderWidget = renderWidget;
}

void AppController::setLLMEnabled(bool enabled) {
    m_llmEnabled = enabled;
    qDebug() << "[AppController] LLM 润色状态变更为:" << (enabled ? "开启" : "关闭");
}

void AppController::onManualTextEntered(const QString& text) {
    if (text.isEmpty()) return;

    qDebug() << "[AppController] 收到手动输入:" << text;

    // 封装成基础的数据帧
    SubtitleFrame frame;
    frame.rawText = text;
    frame.isFinal = true;

    // 路由判断：如果有 LLM，就交给 LLM 处理；如果没有，直接渲染
    if (m_llm && m_llmEnabled) {
        m_llm->processText(frame);
    }
    else {
        // 大模型关闭或不存在时，原样输出
        frame.displayText = text;
        onLLMTextProcessed(frame);
    }
}

void AppController::onASRTextReady(const QString& text, bool isFinal) {
    if (text.isEmpty()) return;

    // 这里的逻辑和手动输入类似，只不过我们需要判断 isFinal。
    // 如果还没说完 (isFinal == false)，就不送给 LLM，直接半透明渲染在屏幕上。
    SubtitleFrame frame;
    frame.rawText = text;
    frame.isFinal = isFinal;

    if (isFinal && m_llm && m_llmEnabled) {
        // 说完了，且开了润色，交给大模型去加颜文字
        m_llm->processText(frame);
    }
    else {
        // 没说完，或者没开润色，直接渲染
        frame.displayText = text;
        onLLMTextProcessed(frame);
    }
}

void AppController::onLLMTextProcessed(const SubtitleFrame& frame) {
    SubtitleFrame finalFrame = frame;

    // 如果大模型成功分析出了情绪，且非平静，就去仓库提货！
    if (finalFrame.emotion != EmotionType::Neutral && m_kaomojiManager) {
        QString kaomoji = m_kaomojiManager->getKaomoji(finalFrame.emotion);
        if (!kaomoji.isEmpty()) {
            finalFrame.displayText = finalFrame.rawText + " " + kaomoji;
        }
        else {
            finalFrame.displayText = finalFrame.rawText;
        }
    }
    else {
        finalFrame.displayText = finalFrame.rawText;
    }

    //注意这里：把 frame 改成 finalFrame！
    qDebug() << "[AppController] 准备渲染最终文本:" << finalFrame.displayText;

    if (m_renderWidget) {
        m_renderWidget->setText(finalFrame.displayText); // 这里也要改！
    }

    // 未来这里还要加上：if (m_oscClient) { m_oscClient->sendEmotion(frame.emotion); }
}

// 新增 Setter 的实现
void AppController::setKaomojiManager(KaomojiManager* manager) {
    m_kaomojiManager = manager;
}
