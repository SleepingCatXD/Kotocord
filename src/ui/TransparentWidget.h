#ifndef TRANSPARENTWIDGET_H
#define TRANSPARENTWIDGET_H

#include <QWidget>
#include <QString>
#include <QPoint>

class TransparentWidget : public QWidget {
    Q_OBJECT
public:
    explicit TransparentWidget(QWidget* parent = nullptr);

    // 提供一个接口，用于将来接收新台词
    void setText(const QString& text);

protected:
    // 核心绘制事件
    void paintEvent(QPaintEvent* event) override;
    // 鼠标事件，用于拖拽无边框窗口
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    QString m_currentText;
    QPoint m_dragPosition;
};

#endif // TRANSPARENTWIDGET_H