// 核心枢纽：串联输入、LLM、渲染和 OSC
#include "AppController.h"
#include "../modules/llm/KaomojiManager.h"
#include <QDebug>

//构造函数
AppController::AppController(QObject* parent)
    : QObject(parent), 
	m_llm(nullptr), 
	m_renderWidget(nullptr), 
	m_llmEnabled(true),
	m_kaomojiManager (nullptr),
	m_currentFrameId(1){}

void AppController::setLanguageModel(ILanguageModel* llm) {
	// 当 LLM 处理完毕时，连接到控制器的槽函数
    m_llm = llm;
    if (m_llm) {
        connect(m_llm, &ILanguageModel::textProcessed, this, &AppController::onLLMTextProcessed);
    }
}

void AppController::setRenderWidget(SubtitleRenderer* renderWidget) {
    m_renderWidget = renderWidget;
}

void AppController::setLLMEnabled(bool enabled) {
    m_llmEnabled = enabled;
    qDebug() << "[AppController] LLM 润色状态变更为:" << (enabled ? "开启" : "关闭");
}

void AppController::onManualTextEntered(const QString& text) {
	if(text.isEmpty()) return;

	qDebug() << "[AppController] 收到手动输入:" << text;

	SubtitleFrame frame;
	frame.frameId = m_currentFrameId; // 拿一张身份证
	frame.rawText = text;
	frame.displayText = text;
	frame.isFinal = true;             // 手动按回车，一上来就是完结状态
	frame.isLlmProcessed = false;

	// 阶段一：立刻上屏 (白字)
	if(m_renderWidget) {
		m_renderWidget->updateFrame(frame);
	}

	// 阶段二：异步送入大模型
	if(m_llm && m_llmEnabled) {
		m_llm->processText(frame);
	} else {
		onLLMTextProcessed(frame);
	}

	m_currentFrameId++; // 为下一次输入做准备
}

void AppController::onASRTextReady(const QString& text, bool isFinal) {
    if (text.isEmpty()) return;

    qDebug() << "[AppController - ASR入口] 收到文本:" << text << "| 是否完结(isFinal):" << isFinal;
    // 这里的逻辑和手动输入类似，只不过我们需要判断 isFinal。
    // 如果还没说完 (isFinal == false)，就不送给 LLM，直接半透明渲染在屏幕上。
	// 
	// 
	// 1. 组装数据帧
	SubtitleFrame frame;
	frame.frameId = m_currentFrameId; // 保持当前的句子 ID
	frame.rawText = text;
	frame.displayText = text;         // 初始状态直接显示纯文本
	frame.isFinal = isFinal;
	frame.isLlmProcessed = false;     // 大模型尚未处理

	// 2. 阶段一：【绝对优先】只要识别出字，立刻发给屏幕！实现 0 延迟渲染！
	// 此时 isLlmProcessed 为 false，Renderer 会把它画成白字。
	if(m_renderWidget) {
		m_renderWidget->updateFrame(frame);
	}

	// 3. 阶段二：这句话真的说完了，才开始大模型附魔流程
	if(isFinal) {
		if(m_llm && m_llmEnabled) {
			// 扔给后台去异步处理，不阻塞主线程
			m_llm->processText(frame);
		} else {
			// 如果没开大模型，直接走本地结算流程
			onLLMTextProcessed(frame);
		}

		// 极其关键：这句话已经完结并送去处理了，我们将 ID + 1，
		// 这样玩家接着说下一句话时，就是全新的 ID 了！
		m_currentFrameId++;
	}
}

void AppController::onLLMTextProcessed(const SubtitleFrame& frame) {
    SubtitleFrame finalFrame = frame;
	finalFrame.isLlmProcessed = true; // 标记：大模型已附魔完毕！

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
	qDebug() << "[AppController] 附魔完成，准备刷新屏幕:" << finalFrame.displayText;

	if(m_renderWidget) {
		m_renderWidget->updateFrame(finalFrame); // 发送附魔后的帧去覆盖旧画面
	}

    // 未来这里还要加上：if (m_oscClient) { m_oscClient->sendEmotion(frame.emotion); }
}

// 新增 Setter 的实现
void AppController::setKaomojiManager(KaomojiManager* manager) {
    m_kaomojiManager = manager;
}
