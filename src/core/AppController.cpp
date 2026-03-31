// 核心枢纽：串联输入、LLM、渲染和 OSC
#include "AppController.h"
#include "../modules/llm/KaomojiManager.h"
#include <QDebug>

AppController::AppController(QObject* parent)
    : QObject(parent), 
	m_llm(nullptr), 
	m_renderWidget(nullptr), 
	m_llmEnabled(true),
	m_kaomojiManager (nullptr),
	m_currentFrameId(1),
	m_isScreenLocked(false) //初始化未锁屏
{
	// 配置定时器：单次触发
	m_unlockTimer.setSingleShot(true);
	connect(&m_unlockTimer,&QTimer::timeout,this,&AppController::processNextInQueue);
}

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

void AppController::setKaomojiManager(KaomojiManager* manager) {
	m_kaomojiManager = manager;
}

void AppController::onManualTextEntered(const QString& text) {
	if(text.isEmpty()) return;

	qDebug() << "[AppController] 收到手动输入:" << text;

	//可添加锁机制

	SubtitleFrame frame;
	frame.frameId = m_currentFrameId; // 拿一张身份证
	frame.rawText = text;
	frame.displayText = text;
	frame.isFinal = true;             // 定义初始状态为已完结
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
    // 如果还没说完 (isFinal == false)，就不送给 LLM，直接半透明渲染在屏幕上。

	// --- 1. 锁屏拦截机制 ---
	if(m_isScreenLocked) {
		if(!isFinal) {
			// 【静默截流】屏幕锁着，且话没说完，直接丢弃，保护当前屏幕上的附魔特效
			return;
		} else {
			// 【排队】屏幕锁着，但又说完了一整句话，放进队列缓存
			SubtitleFrame queuedFrame;
			queuedFrame.frameId = m_currentFrameId++;
			queuedFrame.rawText = text;
			queuedFrame.displayText = text;
			queuedFrame.isFinal = true;
			queuedFrame.isLlmProcessed = false;

			m_pendingQueue.enqueue(queuedFrame);
			qDebug() << "[AppController] 屏幕锁屏中，新句子进入排队。当前队列长度:" << m_pendingQueue.size();
			return;
		}
	}

	// --- 2. 正常无锁状态的处理 ---
	qDebug() << "[AppController - ASR入口] 收到文本:" << text << "| 是否完结(isFinal):" << isFinal;

	SubtitleFrame frame;
	frame.frameId = m_currentFrameId;
	frame.rawText = text;
	frame.displayText = text;
	frame.isFinal = isFinal;
	frame.isLlmProcessed = false;

	// 阶段一：白字立刻上屏
	if(m_renderWidget) m_renderWidget->updateFrame(frame);

	// 阶段二：送大模型附魔，并立刻上锁！
	if(isFinal) {
		m_isScreenLocked = true; // 关键：上锁！防止下一句半截话冲掉当前内容

		if(m_llm && m_llmEnabled) {
			m_llm->processText(frame);
		} else {
			onLLMTextProcessed(frame);
		}
		m_currentFrameId++;
	}
}
void AppController::onLLMTextProcessed(const SubtitleFrame& frame) {
	SubtitleFrame finalFrame = frame;
	finalFrame.isLlmProcessed = true;

	if(finalFrame.emotion != EmotionType::Neutral && m_kaomojiManager) {
		QString kaomoji = m_kaomojiManager->getKaomoji(finalFrame.emotion);
		finalFrame.displayText = !kaomoji.isEmpty() ? finalFrame.rawText + " " + kaomoji : finalFrame.rawText;
	} else {
		finalFrame.displayText = finalFrame.rawText;
	}

	qDebug() << "[AppController] 附魔完成，准备刷新屏幕:" << finalFrame.displayText;

	if(m_renderWidget) {
		m_renderWidget->updateFrame(finalFrame);
	}

	// 核心：启动倒计时！让这句话在屏幕上至少停留 1.5 秒，再处理下一句
	m_unlockTimer.start(1500);
}

// 定时器触发的提货函数
void AppController::processNextInQueue() {
	m_isScreenLocked = false; // 解锁屏幕

	if(!m_pendingQueue.isEmpty()) {
		SubtitleFrame nextFrame = m_pendingQueue.dequeue();
		qDebug() << "[AppController] 从队列提取句子上屏:" << nextFrame.rawText;

		// 重新走一遍阶段一和阶段二
		if(m_renderWidget) m_renderWidget->updateFrame(nextFrame);

		m_isScreenLocked = true; // 再次上锁
		if(m_llm && m_llmEnabled) {
			m_llm->processText(nextFrame);
		} else {
			onLLMTextProcessed(nextFrame);
		}
	}
}
