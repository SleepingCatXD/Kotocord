#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>

#include "../core/AppController.h"
#include "../modules/llm/MockLLMWorker.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("Kotocord - Controller");

    // 1. 实例化透明窗口
    m_overlayWidget = new TransparentWidget(nullptr);
    m_overlayWidget->setAttribute(Qt::WA_QuitOnClose, false);
    m_overlayWidget->show();

    // 2. 实例化控制器和虚拟大模型 (传入 this 作为 parent，由 Qt 自动管理内存)
    m_appController = new AppController(this);
    m_mockLLM = new MockLLMWorker(this);

    // 3. 依赖注入：告诉控制器使用哪个大模型和哪个渲染窗口
    m_appController->setLanguageModel(m_mockLLM);
    m_appController->setRenderWidget(m_overlayWidget);

    // 4. 搭建测试 UI
    QVBoxLayout* layout = new QVBoxLayout(ui->centralwidget);
    QLineEdit* inputBox = new QLineEdit(this);
    inputBox->setPlaceholderText("input text, test LLM...");
    QPushButton* sendBtn = new QPushButton("send to AppController", this);

    layout->addWidget(inputBox);
    layout->addWidget(sendBtn);
    layout->addStretch();

    // 5. 核心改变：以前是直接发给透明窗口，现在发给中枢神经 AppController
    connect(sendBtn, &QPushButton::clicked, this, [=]() {
        QString text = inputBox->text();
        if (!text.isEmpty()) {
            m_appController->onManualTextEntered(text); // 送入控制器！
            inputBox->clear();
        }
        });

    // 按回车键也能发送
    connect(inputBox, &QLineEdit::returnPressed, sendBtn, &QPushButton::click);
}

MainWindow::~MainWindow()
{
    delete m_overlayWidget; // 记得释放内存
    delete ui;
    // 注: m_appController 和 m_mockLLM 因为挂载在 this 上，会自动释放，无需手动 delete
}
