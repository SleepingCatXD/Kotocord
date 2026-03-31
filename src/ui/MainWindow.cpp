#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QByteArray>

#include "../core/AppController.h"
#include "../modules/llm/MockLLMWorker.h"
#include "../modules/llm/DeepSeekAPIWorker.h"

#include "../modules/llm/KaomojiManager.h"
#include "../modules/input/AudioCapture.h"
#include "../modules/system/SystemResourceMonitor.h"

#include <QScreen>
#include <QGuiApplication>
#include <QRect>

#include <QCoreApplication>
#include <QDir>

//简单的异或混淆(XOR) + Base64 加密算法
static const char CRYPT_KEY = 0x5A; // 随便定一个魔法数字作为密钥

QString obfuscateKey(const QString& plainText) {
    if (plainText.isEmpty()) return "";
    QByteArray data = plainText.toUtf8();
    for (int i = 0; i < data.size(); ++i) {
        data[i] = data[i] ^ CRYPT_KEY; // 异或操作
    }
    return QString(data.toBase64()); // 转成 Base64 字母串
}

QString deobfuscateKey(const QString& cipherText) {
    if (cipherText.isEmpty()) return "";
    QByteArray data = QByteArray::fromBase64(cipherText.toUtf8());
    for (int i = 0; i < data.size(); ++i) {
        data[i] = data[i] ^ CRYPT_KEY; // 再次异或即可还原
    }
    return QString(data);
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    // 这里会自动加载你在 XML 里画的所有 UI 控件！
    ui->setupUi(this);
    // 告诉 Qt 的事件循环，认识我们自定义的结构体
    qRegisterMetaType<SubtitleFrame>("SubtitleFrame");

    m_overlayWidget = new SubtitleRenderer(nullptr);
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
    m_deepSeekLLM = new DeepSeekAPIWorker(this);

    m_appController->setLanguageModel(m_mockLLM);
    m_appController->setRenderWidget(m_overlayWidget);

	// --- 启动系统资源监控 ---
	SystemResourceMonitor* sysMonitor = new SystemResourceMonitor(this);
	connect(sysMonitor,&SystemResourceMonitor::resourceUpdated,this,[=](double cpu,double mem) {
		ui->lblCpu->setText(QString::number(cpu,'f',2) + " %");
		ui->lblMem->setText(QString::number(mem,'f',2) + " MB");
	});
	sysMonitor->start(1000); // 每秒刷新一次

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

    // --- 补充 1：API Key 实时同步 ---
    connect(ui->inputApiKey, &QLineEdit::textChanged, this, [=](const QString& key) {
        m_deepSeekLLM->setApiConfig(key); // 默认已指向 DeepSeek V3，只需传 Key
        // 加密并存入本地 (如果输入为空则清空本地记录)
        QSettings settings("MyStudio", "Kotocord");
        if (key.isEmpty()) {
            settings.remove("API/DeepSeekKey");
        }
        else {
            settings.setValue("API/DeepSeekKey", obfuscateKey(key));
        }
    });
    // --- 补充 2：下拉框切换模型 ---
    connect(ui->comboModel, &QComboBox::currentIndexChanged, this, [=](int index) {
        if (index == 0) {
            m_appController->setLanguageModel(m_mockLLM);
        }
        else {
            m_appController->setLanguageModel(m_deepSeekLLM);
        }
        });
    // --- 补充 3：连接监控看板 (性能与消耗) ---
    connect(m_deepSeekLLM, &ILanguageModel::performanceMetricsReported, this, [=](int prompt, int completion, qint64 latency) {
        ui->lblLatency->setText(QString::number(latency) + " ms");
        ui->lblTokens->setText(QString::number(prompt + completion) +
            " (提示: " + QString::number(prompt) + ", 生成: " + QString::number(completion) + ")");
        });
    // --- 补充 4：连接监控看板 (当前情绪展示) ---
    // 写一个统一的 Lambda，方便 Mock 和 DeepSeek 共用
    auto updateEmotionLabel = [=](const SubtitleFrame& frame) {
        QString emotionName;
        switch (frame.emotion) {
        case EmotionType::Joy: emotionName = "高兴 (Joy)"; break;
        case EmotionType::Sadness: emotionName = "悲伤 (Sadness)"; break;
        case EmotionType::Anger: emotionName = "愤怒 (Anger)"; break;
        case EmotionType::Surprise: emotionName = "惊讶 (Surprise)"; break;
        default: emotionName = "平静 (Neutral)"; break;
        }
        ui->lblCurrentEmotion->setText(emotionName);
        };
    connect(m_mockLLM, &ILanguageModel::textProcessed, this, updateEmotionLabel);
    connect(m_deepSeekLLM, &ILanguageModel::textProcessed, this, updateEmotionLabel);

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

	// --- 1. 实例化双引擎和真实麦克风 ---
	m_voskEngine = new VoskTranscriber(this);
	m_whisperEngine = new WhisperTranscriber(this);
	AudioCapture* micCapture = new AudioCapture(this);

	// 默认使用 Vosk (与 UI 的 RadioButton 初始状态对应)
	m_currentASR = m_voskEngine;

	// --- 2. 动态切换逻辑 (点选单选框时，先熄火旧的，再换核心) ---
	connect(ui->radioVosk,&QRadioButton::toggled,this,[=](bool checked){
		if(checked) {
			m_whisperEngine->stop(); // 强行拔掉 Whisper 的车钥匙
			m_currentASR = m_voskEngine;
			// 如果此时麦克风总开关是开着的，立刻启动新引擎
			if(ui->chkEnableASR->isChecked()) m_currentASR->start();
		}
	});

	connect(ui->radioWhisper,&QRadioButton::toggled,this,[=](bool checked){
		if(checked) {
			m_voskEngine->stop(); // 强行拔掉 Vosk 的车钥匙
			m_currentASR = m_whisperEngine;
			if(ui->chkEnableASR->isChecked()) m_currentASR->start();
		}
	});

	// --- 3. 连线：物理麦克风的数据源源不断喂给双引擎 ---
	connect(micCapture,&AudioCapture::audioDataReady,m_voskEngine,&VoskTranscriber::onAudioDataReady);
	connect(micCapture,&AudioCapture::audioDataReady,m_whisperEngine,&WhisperTranscriber::onAudioDataReady);

	// 把双引擎的文字出口，统一汇聚到大脑
	connect(m_voskEngine,&IAudioTranscriber::textReady,m_appController,&AppController::onASRTextReady);
	connect(m_whisperEngine,&IAudioTranscriber::textReady,m_appController,&AppController::onASRTextReady);

	// --- 4. 启停控制 (彻底接管系统麦克风) ---
	connect(ui->chkEnableASR,&QCheckBox::toggled,this,[=](bool checked) {
		if(checked) {
			// 先启动底层引擎加载模型
			if(m_currentASR->start()) {
				// 模型就绪后，正式打开系统麦克风！
				micCapture->start();
			}
		} else {
			micCapture->stop(); // 立即切断麦克风
			m_currentASR->stop();
		}
	});

    // 实例化kaomojiManager
    KaomojiManager* kaomojiManager = new KaomojiManager(this);
    kaomojiManager->loadFromFile(QDir(QCoreApplication::applicationDirPath()).filePath("../resources/kaomoji.json"));

    // 补上这极其关键的一行！把兵工厂的钥匙交给指挥官
    m_appController->setKaomojiManager(kaomojiManager);
    // 把加载配置的代码移到这里：所有信号槽连接完毕后，再触发 setText！
    QSettings settings("MyStudio", "Kotocord");
    QString savedEncryptedKey = settings.value("API/DeepSeekKey", "").toString();
    QString plainKey = deobfuscateKey(savedEncryptedKey);

    if (!plainKey.isEmpty()) {
        ui->inputApiKey->setText(plainKey); // 这一步现在会完美触发 textChanged 信号，传给 DeepSeek
        qDebug() << "[System] 成功从本地加载 API Key 并同步至大脑！";
        // 增加 UI 提示：证明我们连上了
        ui->lblCurrentEmotion->setText("已就绪，等待输入...");
        ui->lblCurrentEmotion->setStyleSheet("color: #28A745;"); // 变成健康的绿色
    }
    else {
        ui->lblCurrentEmotion->setText("未配置 API Key");
        ui->lblCurrentEmotion->setStyleSheet("color: #DC3545;"); // 红色警告
    }
}

MainWindow::~MainWindow()
{
    delete m_overlayWidget; // 记得释放内存
    delete ui;
    // 注: m_appController 和 m_mockLLM 因为挂载在 this 上，会自动释放，无需手动 delete
}
