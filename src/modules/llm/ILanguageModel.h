//LLM 纯虚类接口 (定义 processText(QString))
#ifndef ILANGUAGEMODEL_H
#define ILANGUAGEMODEL_H

#include <QObject>
#include "../../core/DataTypes.h"

class ILanguageModel : public QObject {
    Q_OBJECT
public:
    explicit ILanguageModel(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~ILanguageModel() = default;

    // 核心处理接口：接收一个原始的 Frame，交给大模型处理
    virtual void processText(const SubtitleFrame& frame) = 0;

signals:
    // 处理完成后，发出包含情绪和新文本的完整 Frame
    void textProcessed(const SubtitleFrame& processedFrame);

    // 网络错误或 API 错误时发出
    void errorOccurred(const QString& errorMsg);
};

#endif // ILANGUAGEMODEL_H