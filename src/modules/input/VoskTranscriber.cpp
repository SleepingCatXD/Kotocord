#include "VoskTranscriber.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QCoreApplication>
#include <QDir>

VoskTranscriber::VoskTranscriber(QObject* parent)
    : IAudioTranscriber(parent), m_model(nullptr), m_recognizer(nullptr), m_isRunning(false) {

    // 把 -1 改成 1，开启底层日志
    vosk_set_log_level(1);
}

VoskTranscriber::~VoskTranscriber() {
    stop();
    // 释放 C 语言指针内存
    if (m_recognizer) vosk_recognizer_free(m_recognizer);
    if (m_model) vosk_model_free(m_model);
}

bool VoskTranscriber::start() {
    if (m_isRunning) return true;

    // 1. 加载模型 (注意：这里使用的是相对路径，确保你的 Visual Studio 工作目录在项目根目录)
    // 如果你有把模型放在其他地方，请修改这里的路径！
    if (!m_model) {
        qDebug() << "[Vosk] 正在加载语言模型，可能需要几秒钟...";

        // 核心修复：获取 Kotocord.exe 所在的 bin 目录
        QString exeDir = QCoreApplication::applicationDirPath();
        // 向上退一级到项目根目录，然后进入 resources
        QString modelPath = QDir(exeDir).filePath("../resources/model/vosk-model-small-cn-0.22");

        qDebug() << "[Vosk] 尝试加载模型的绝对路径:" << modelPath;

        // Vosk 接收的是 C 语言风格的字符串 (const char*)，转换一下
        m_model = vosk_model_new(modelPath.toLocal8Bit().constData());

        if (!m_model) {
            emit errorOccurred("无法加载 Vosk 模型，请检查路径是否正确: " + modelPath);
            return false;
        }
        qDebug() << "[Vosk] 模型加载成功！";
    }

    // 2. 初始化识别器，采样率设定为黄金标准 16000.0f
    if (!m_recognizer) {
        m_recognizer = vosk_recognizer_new(m_model, 16000.0f);
    }

    m_isRunning = true;
    return true;
}

void VoskTranscriber::stop() {
    m_isRunning = false;
}

void VoskTranscriber::onAudioDataReady(const QByteArray& data) {
    //// 新增的终极监控点：看看水到底流没流到这里，以及门有没有开！
    //qDebug() << "[Vosk - 入口] 收到信号! 数据大小:" << data.size()
    //    << " | m_isRunning:" << m_isRunning
    //    << " | 识别器非空:" << (m_recognizer != nullptr);

    if(!m_isRunning || !m_recognizer) {
        return;
    }

    // 把数据喂给 Vosk
    int isFinal = vosk_recognizer_accept_waveform(m_recognizer,data.constData(),data.size());

    if(isFinal) {
        const char* result = vosk_recognizer_result(m_recognizer);
        // 新增：打印最终识别的完整 JSON
        //qDebug() << "[Vosk - 最终结果] 原始 JSON:" << result;
        parseAndEmitResult(result,true);
    } else {
        const char* partial = vosk_recognizer_partial_result(m_recognizer);
        // 新增：打印实时识别的中间 JSON
        //qDebug() << "[Vosk - 实时中间] 原始 JSON:" << partial;
        parseAndEmitResult(partial,false);
    }
}

void VoskTranscriber::onAudioStreamFinished() {
    if(!m_isRunning || !m_recognizer) return;

    qDebug() << "[Vosk] 收到音频流结束信号，强制结算剩余缓冲...";

    // 核心机制：调用 final_result 强制清空 Vosk 内部的残余音频
    const char* final_result = vosk_recognizer_final_result(m_recognizer);

    qDebug() << "[Vosk - 强制最终结果] 原始 JSON:" << final_result;

    // 把最后这句话当作最终结果 (isFinal = true) 发送出去
    parseAndEmitResult(final_result,true);

    // 结算完成后，如果你希望引擎重置状态迎接下一段语音，可以调用：
    // vosk_recognizer_reset(m_recognizer); 
}

void VoskTranscriber::parseAndEmitResult(const char* jsonStr, bool isFinal) {
    // 将 C 字符串转换为 Qt 的 JSON 对象
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray(jsonStr));
    if(doc.isNull() || !doc.isObject()) {
        qDebug() << "[Vosk] 警告：JSON 解析失败！"; //新增
        return;
    }

    QJsonObject obj = doc.object();
    QString parsedText;

    if (isFinal) {
        // 如果是最终结果，Vosk 的 JSON 键名是 "text"
        parsedText = obj.value("text").toString();
    }
    else {
        // 如果是中间结果，Vosk 的 JSON 键名是 "partial"
        parsedText = obj.value("partial").toString();
    }

    // 去除两端空格，如果有文字，就发送给 AppController
    parsedText = parsedText.trimmed();
    if (!parsedText.isEmpty()) {
        emit textReady(parsedText, isFinal);
    }
}
