#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>

#include "../core/AppController.h"
#include "../modules/llm/MockLLMWorker.h"

#include <QScreen>
#include <QGuiApplication>
#include <QRect>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    // 这里会自动加载你在 XML 里画的所有 UI 控件！
    ui->setupUi(this);

    m_overlayWidget = new TransparentWidget(nullptr);
    m_overlayWidget->setAttribute(Qt::WA_QuitOnClose, false);

    //新增：计算屏幕中心靠下的位置
    QScreen* screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    // X坐标居中
    int x = (screenGeometry.width() - m_overlayWidget->width()) / 2;
    // Y坐标设定在屏幕底部往上抬 150 像素的位置
    int y = screenGeometry.height() - m_overlayWidget->height() - 150;
    m_overlayWidget->move(x, y);

    m_overlayWidget->show();

    m_appController = new AppController(this);
    m_mockLLM = new MockLLMWorker(this);

    m_appController->setLanguageModel(m_mockLLM);
    m_appController->setRenderWidget(m_overlayWidget);

    // ==========================================
    // 信号槽连接：打通 UI 和中枢神经
    // ==========================================

    // 1. 手动文本发送 (按钮和回车键)
    connect(ui->btnSend, &QPushButton::clicked, this, [=]() {
        QString text = ui->inputBox->text();
        m_appController->onManualTextEntered(text);
        ui->inputBox->clear();
        });
    connect(ui->inputBox, &QLineEdit::returnPressed, ui->btnSend, &QPushButton::click);

    // 2. LLM 开关同步到 AppController
    connect(ui->chkEnableLLM, &QCheckBox::toggled, m_appController, &AppController::setLLMEnabled);

    // 3. UI 交互小细节：勾选了“启用 ASR”，才能选择 Vosk 或 Whisper
    connect(ui->chkEnableASR, &QCheckBox::toggled, this, [=](bool checked) {
        ui->radioVosk->setEnabled(checked);
        ui->radioWhisper->setEnabled(checked);

        // 此处预留：未来控制 ASR 引擎 start() / stop() 的代码
        if (checked) {
            // m_currentASR->start();
        }
        else {
            // m_currentASR->stop();
        }
        });
}

MainWindow::~MainWindow()
{
    delete m_overlayWidget; // 记得释放内存
    delete ui;
    // 注: m_appController 和 m_mockLLM 因为挂载在 this 上，会自动释放，无需手动 delete
}
