#include "TransparentWidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QFontMetrics>

TransparentWidget::TransparentWidget(QWidget* parent)
    : QWidget(parent), m_currentText("Kotocord Ready!") {

    // 1. 设置窗口标志：无边框 | 保持置顶 | 作为工具窗口（不在任务栏显示独立图标，可选）
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    // 2. 核心属性：允许背景透明
    setAttribute(Qt::WA_TranslucentBackground);

    // 设置一个比较宽的初始大小，适合放字幕
    resize(800, 200);
}

void TransparentWidget::setText(const QString& text) {
    m_currentText = text;
    update(); // 触发 paintEvent 重新绘制
}

void TransparentWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    if (m_currentText.isEmpty()) return;

    QPainter painter(this);
    // 开启抗锯齿，保证边缘平滑
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    // 设置字体 (你可以替换成你电脑里的任何好看的字体)
    QFont font("Arial", 36, QFont::Black);
    painter.setFont(font);

    // 计算文字路径和居中位置
    QPainterPath path;
    QFontMetrics metrics(font);
    int x = (width() - metrics.horizontalAdvance(m_currentText)) / 2;
    int y = (height() + metrics.ascent() - metrics.descent()) / 2;
    path.addText(x, y, font, m_currentText);

    // --- 开始图层渲染 (从下到上) ---

    // 底层：黑色半透明阴影 (向右下角偏移 4 像素)
    painter.translate(4, 4);
    painter.setPen(QPen(QColor(0, 0, 0, 150), 6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);
    painter.translate(-4, -4); // 恢复坐标系

    // 中层：白色粗描边
    painter.setPen(QPen(QColor(255, 255, 255), 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);

    // 顶层：文字内部渐变填充
    painter.setPen(Qt::NoPen); // 填充不需要画笔
    QLinearGradient gradient(0, y - metrics.ascent(), 0, y); // 垂直渐变
    gradient.setColorAt(0.0, QColor(255, 175, 204)); // 顶部粉色
    gradient.setColorAt(1.0, QColor(162, 210, 255)); // 底部蓝色
    painter.setBrush(gradient);
    painter.drawPath(path);
}

// --- 鼠标拖拽逻辑实现 ---
void TransparentWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        // 记录鼠标按下时，相对于窗口左上角的偏移量
        m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void TransparentWidget::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) {
        // 移动窗口
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    }
}