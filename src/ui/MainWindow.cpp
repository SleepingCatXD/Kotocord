#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>

#include "../core/AppController.h"
#include "../modules/llm/MockLLMWorker.h"

#include "../modules/input/VoskTranscriber.h"
#include "../modules/input/AudioFileSimulator.h"

#include <QScreen>
#include <QGuiApplication>
#include <QRect>

#include <QCoreApplication>
#include <QDir>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    // 这里会自动加载你在 XML 里画的所有 UI 控件！
    ui->setupUi(this);

    m_overlayWidget = new SubtileRenderer(nullptr);
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

    // 1. 实例化 ASR 引擎和音频模拟器
    VoskTranscriber* voskEngine = new VoskTranscriber(this);
    AudioFileSimulator* audioSim = new AudioFileSimulator(this);

    // 2. 连接数据流：音频滴管 -> Vosk -> AppController
    connect(audioSim, &AudioFileSimulator::errorOccurred, this, [](const QString& msg) {
        qDebug() << "[错误]" << msg;
        });
    connect(voskEngine, &IAudioTranscriber::errorOccurred, this, [](const QString& msg) {
        qDebug() << "[错误]" << msg;
        });

    // 强行检查连接是否成功，并打印它们在内存里的身份证号（地址）
    bool isConnected = connect(audioSim,&AudioFileSimulator::audioDataReady,voskEngine,&VoskTranscriber::onAudioDataReady);

    qDebug() << "================ 测谎仪 ================";
    qDebug() << "AudioSim 地址:" << audioSim;
    qDebug() << "VoskEngine 地址:" << voskEngine;
    qDebug() << "信号槽是否接通? :" << (isConnected ? "✅ 是的!" : "❌ 失败!");
    qDebug() << "========================================";

    //新增这根极其关键的线：点滴打完了，通知 Vosk 强制结算！
    connect(audioSim,&AudioFileSimulator::finished,voskEngine,&VoskTranscriber::onAudioStreamFinished);

    // 补上这根丢失的神经！把 Vosk 的文字发送给中枢神经控制器
    connect(voskEngine,&IAudioTranscriber::textReady,m_appController,&AppController::onASRTextReady);

    // 3. 在 UI 的“启用麦克风”选框上绑定启动逻辑 (偷天换日，其实启动的是文件模拟器)
    connect(ui->chkEnableASR, &QCheckBox::toggled, this, [=](bool checked) {
        if (checked) {
            if (voskEngine->start()) {
                // 动态计算 test_audio.wav 的绝对路径
                QString exeDir = QCoreApplication::applicationDirPath();
                QString rawPath = QDir(exeDir).filePath("../resources/test_audio.wav");
                //消除路径里的 "../"，让它变成纯粹的绝对路径
                QString audioPath = QDir::cleanPath(rawPath);

                qDebug() << "[UI] 尝试读取测试音频:" << audioPath;
                audioSim->start(audioPath);
            }
        }
        else {
            // 修改这里：在停止引擎之前，先手动触发一次强制结算
            voskEngine->onAudioStreamFinished();

            audioSim->stop();
            voskEngine->stop();
            qDebug() << "[UI] ASR 引擎与音频模拟已手动关闭。";
        }
    });
}

MainWindow::~MainWindow()
{
    delete m_overlayWidget; // 记得释放内存
    delete ui;
    // 注: m_appController 和 m_mockLLM 因为挂载在 this 上，会自动释放，无需手动 delete
}
