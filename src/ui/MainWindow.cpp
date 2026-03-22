#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("Kotocord - controller");

    // 实例化透明窗口并独立显示 (不传入 parent，使其成为顶级窗口)
    m_overlayWidget = new TransparentWidget(nullptr);
    // 添加这一行：当所有其他主窗口关闭时，不要因为这个窗口而阻止程序退出
    m_overlayWidget->setAttribute(Qt::WA_QuitOnClose, false);
    m_overlayWidget->show();

    // 2. 在主窗口动态创建 UI (输入框和按钮)
    QVBoxLayout* layout = new QVBoxLayout(ui->centralwidget);

    QLineEdit* inputBox = new QLineEdit(this);
    inputBox->setPlaceholderText("Type your message here..."); // 提示文本

    QPushButton* sendBtn = new QPushButton("Send to Screen", this);

    layout->addWidget(inputBox);
    layout->addWidget(sendBtn);
    layout->addStretch(); // 把控件往上顶

    // 3. 核心魔法：连接按钮的点击事件，到透明窗口的渲染功能
    connect(sendBtn, &QPushButton::clicked, this, [=]() {
        QString text = inputBox->text();
        if (!text.isEmpty()) {
            m_overlayWidget->setText(text); // 把字传给透明窗口
            inputBox->clear();              // 清空输入框
        }
        });

    // 选做：按回车键也能发送
    connect(inputBox, &QLineEdit::returnPressed, sendBtn, &QPushButton::click);
}

MainWindow::~MainWindow()
{
    delete m_overlayWidget; // 记得释放内存
    delete ui;
}
