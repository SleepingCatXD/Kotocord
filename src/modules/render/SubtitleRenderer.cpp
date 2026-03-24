#include "SubtitleRenderer.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QFontMetrics>
#include <QWindow>

SubtitleRenderer::SubtitleRenderer(QWidget* parent)
    : QWidget(parent), m_currentText("Kotocord Ready!") {

    // 1. 设置窗口标志：无边框 | 保持置顶 | 作为工具窗口（不在任务栏显示独立图标，可选）
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    // 2. 核心属性：允许背景透明
    setAttribute(Qt::WA_TranslucentBackground);

    // 设置一个比较宽的初始大小，适合放字幕
    resize(1000, 200);
}

void SubtitleRenderer::updateFrame(const SubtitleFrame& frame) {
	// 简单的时序保护：如果屏幕上已经是新的一句话了，但上一句话的大模型结果才迟迟传回来，就丢弃旧结果，防止画面乱跳
	if(m_currentFrame.frameId > frame.frameId) {
		qDebug() << "[Renderer] 收到过期的附魔结果，已丢弃。";
		return;
	}

	m_currentFrame = frame;
	update(); // 触发重绘
}

void SubtitleRenderer::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
	if(m_currentFrame.displayText.isEmpty()) return;

    QPainter painter(this);
    // 每次绘制前清空背景，防止 OBS 捕获到残影
    painter.fillRect(rect(), Qt::transparent);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    // 1. 定义安全渲染区域 (留出 20 像素的内边距，防止描边被切断)
    QRect renderBox = rect().adjusted(20, 20, -20, -20);

    // 2. 初始字体设置
    int fontSize = 60; // 初始最大字体
    QFont font("Arial", fontSize, QFont::Black);
    QStringList lines;
    int lineHeight = 0;

    // 3. 核心排版循环：动态换行与缩放
    while (fontSize > 12) { // 字体最小不能小于 12
        font.setPointSize(fontSize);
        QFontMetrics fm(font);
        lineHeight = fm.height();
        lines.clear();

        QString currentLine = "";
        // 遍历当前文本，逐字测量宽度
        for (int i = 0; i < m_currentFrame.displayText.length(); ++i) {
            QChar c = m_currentFrame.displayText[i];
            QString testLine = currentLine + c;

            // 如果加上这个字超宽了，就断行
            if (fm.horizontalAdvance(testLine) > renderBox.width()) {
                lines.append(currentLine);
                currentLine = c;
            }
            else {
                currentLine = testLine;
            }
        }
        lines.append(currentLine); // 把最后一行加进去

        // 检查总高度是否能放进窗口
        if (lines.size() * lineHeight <= renderBox.height()) {
            break; // 完美容纳，跳出循环！
        }

        // 放不下，缩小字体继续算
        fontSize -= 2;
    }

    // 4. 将排版好的多行文字转换为 QPainterPath
    QPainterPath path;
    QFontMetrics finalMetrics(font);

    // 计算 Y 轴整体居中的起始位置
    int totalTextHeight = lines.size() * lineHeight;
    int startY = renderBox.top() + (renderBox.height() - totalTextHeight) / 2 + finalMetrics.ascent();

    for (int i = 0; i < lines.size(); ++i) {
        // 计算每一行 X 轴居中的位置
        int lineWidth = finalMetrics.horizontalAdvance(lines[i]);
        int startX = renderBox.left() + (renderBox.width() - lineWidth) / 2;

        // 逐行添加到路径中
        path.addText(startX, startY + i * lineHeight, font, lines[i]);
    }

    // --- 5. 开始图层渲染 (保持你原来的艺术字配方) ---

    // 底层：黑色半透明阴影
    painter.translate(4, 4);
    painter.setPen(QPen(QColor(0, 0, 0, 150), 6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);
    painter.translate(-4, -4);

    // 中层：白色粗描边
    painter.setPen(QPen(QColor(255, 255, 255), 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);

    // 顶层：文字内部渐变填充
	painter.setPen(Qt::NoPen);

	if(!m_currentFrame.isLlmProcessed && m_currentFrame.isFinal) {
		// 【第一阶段：大模型正在思考中】
		// 比如用纯白色，并且在句尾悄悄加上 "..." 提示观众 AI 正在算
		painter.setBrush(QColor(255,255,255));
		// 你甚至可以在上面 path.addText 的时候，给这段文字加上一个淡入淡出的动画
	} else {
		// 【第二阶段：大模型附魔完毕】
		// 使用你之前酷炫的粉蓝渐变色，配合颜文字华丽登场！
		QLinearGradient gradient(0,renderBox.top(),0,renderBox.bottom());
		gradient.setColorAt(0.0,QColor(255,175,204));
		gradient.setColorAt(1.0,QColor(162,210,255));
		painter.setBrush(gradient);
	}

	painter.drawPath(path);
}

// --- 鼠标拖拽逻辑实现 ---
void SubtitleRenderer::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        // Wayland 支持
        if (windowHandle()) {
            windowHandle()->startSystemMove();
            event->accept();
        } else {
            // 如果 windowHandle 不可用
            m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
            event->accept();
        }
    }
}

void SubtitleRenderer::mouseMoveEvent(QMouseEvent* event) {
    // 仅非 Wayland
    if ((event->buttons() & Qt::LeftButton) && !m_dragPosition.isNull()) {
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    }
}
