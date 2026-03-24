#ifndef SUBTITLERENDERER_H
#define SUBTITLERENDERER_H

#include <QWidget>
#include <QString>
#include <QPoint>
#include "../../core/DataTypes.h"

class SubtitleRenderer : public QWidget {
    Q_OBJECT
public:
    explicit SubtitleRenderer(QWidget* parent = nullptr);

public slots:
	void updateFrame(const SubtitleFrame& frame);

protected:
    // 核心绘制事件
    void paintEvent(QPaintEvent* event) override;
    // 鼠标事件，用于拖拽无边框窗口
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    QPoint m_dragPosition;
	SubtitleFrame m_currentFrame; // 存下当前的完整状态帧
};

#endif // SUBTITLERENDERER_H
