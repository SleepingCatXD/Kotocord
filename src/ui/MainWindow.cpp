#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QByteArray>

#include "../core/AppController.h"

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

MainWindow::MainWindow(AppController* controller,QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
	,m_appController(controller)
{
    ui->setupUi(this);
    qRegisterMetaType<SubtitleFrame>("SubtitleFrame");//传达事件循环，说明自定义结构体

	//UI内部窗口管理
    m_overlayWidget = new SubtitleRenderer(nullptr);
    m_overlayWidget->setAttribute(Qt::WA_QuitOnClose, false);

    //设置字幕初始窗口位置：屏幕中心靠下
    QScreen* screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int x = (screenGeometry.width() - m_overlayWidget->width()) / 2;// X坐标居中
    int y = screenGeometry.height() - m_overlayWidget->height() - 150;// Y坐标屏幕底部往上抬 150 像素
    m_overlayWidget->move(x, y);
    m_overlayWidget->show();

 //   m_appController = new AppController(this);
 //   m_mockLLM = new MockLLMWorker(this);
 //   m_deepSeekLLM = new DeepSeekAPIWorker(this);

 //   m_appController->setLanguageModel(m_mockLLM);
 //   m_appController->setRenderWidget(m_overlayWidget);

	//// --- 启动系统资源监控 ---
	//SystemResourceMonitor* sysMonitor = new SystemResourceMonitor(this);
	//connect(sysMonitor,&SystemResourceMonitor::resourceUpdated,this,[=](double cpu,double mem) {
	//	ui->lblCpu->setText(QString::number(cpu,'f',2) + " %");
	//	ui->lblMem->setText(QString::number(mem,'f',2) + " MB");
	//});
	//sysMonitor->start(1000); // 每秒刷新一次

	//系统资源监控和定时如何解决？

    // ==========================================
    // 信号槽连接：打通 UI 和中枢神经
    // ==========================================

    // 向Controller发送文本
    connect(ui->btnSend, &QPushButton::clicked, this, [=]() {
        QString text = ui->inputBox->text();
        m_appController->onManualTextEntered(text);
        ui->inputBox->clear();
        });
    connect(ui->inputBox, &QLineEdit::returnPressed, ui->btnSend, &QPushButton::click);//同步手动输入
    connect(ui->chkEnableLLM, &QCheckBox::toggled, m_appController, &AppController::setLLMEnabled);//同步LLM启动

	connect(ui->comboModel,&QComboBox::currentIndexChanged,this,[=](int index) {//处理引擎切换
		emit llmEngineSwitched(index == 1);
	});

	connect(ui->chkEnableASR,&QCheckBox::toggled,this,&MainWindow::asrToggleRequested);//处理麦克风启停
	connect(ui->chkEnableASR,&QCheckBox::toggled,this,[=](bool checked) {
		ui->radioVosk->setEnabled(checked);
		ui->radioWhisper->setEnabled(checked);
	});

	connect(ui->radioVosk,&QRadioButton::toggled,this,[=](bool checked){ if(checked) emit asrEngineSwitched(false); });
	connect(ui->radioWhisper,&QRadioButton::toggled,this,[=](bool checked){ if(checked) emit asrEngineSwitched(true); });

	connect(ui->radioVosk,&QRadioButton::toggled,this,[=](bool checked){ if(checked) emit asrEngineSwitched(false); });
	connect(ui->radioWhisper,&QRadioButton::toggled,this,[=](bool checked){ if(checked) emit asrEngineSwitched(true); });

	//API Key 实时同步
	connect(ui->inputApiKey,&QLineEdit::textChanged,this,[=](const QString& key) {
		emit apiKeyChanged(key); // 通知外部
		QSettings settings("MyStudio","Kotocord");// 加密并存入本地 (如果输入为空则清空本地记录)
		if(key.isEmpty()) {
			settings.remove("API/DeepSeekKey");
		} else {
			settings.setValue("API/DeepSeekKey",obfuscateKey(key));
		}
	});

	// 读取本地配置并触发信号
	QSettings settings("MyStudio","Kotocord");
	QString savedEncryptedKey = settings.value("API/DeepSeekKey","").toString();
	QString plainKey = deobfuscateKey(savedEncryptedKey);
	if(!plainKey.isEmpty()) {
		ui->inputApiKey->setText(plainKey);
		onSystemReady(true);
	} else {
		onSystemReady(false);
	}

 //   // --- 补充 3：连接监控看板 (性能与消耗) ---
 //   connect(m_deepSeekLLM, &ILanguageModel::performanceMetricsReported, this, [=](int prompt, int completion, qint64 latency) {
 //       ui->lblLatency->setText(QString::number(latency) + " ms");
 //       ui->lblTokens->setText(QString::number(prompt + completion) +
 //           " (提示: " + QString::number(prompt) + ", 生成: " + QString::number(completion) + ")");
 //       });


 //   connect(m_mockLLM, &ILanguageModel::textProcessed, this, updateEmotionLabel);
 //   connect(m_deepSeekLLM, &ILanguageModel::textProcessed, this, updateEmotionLabel);

 //   

	//// --- 1. 实例化双引擎和真实麦克风 ---
	//m_voskEngine = new VoskTranscriber(this);
	//m_whisperEngine = new WhisperTranscriber(this);
	//AudioCapture* micCapture = new AudioCapture(this);

	//// 默认使用 Vosk (与 UI 的 RadioButton 初始状态对应)
	//m_currentASR = m_voskEngine;

	//// --- 2. 动态切换逻辑 (点选单选框时，先熄火旧的，再换核心) ---
	//connect(ui->radioVosk,&QRadioButton::toggled,this,[=](bool checked){
	//	if(checked) {
	//		m_whisperEngine->stop(); // 强行拔掉 Whisper 的车钥匙
	//		m_currentASR = m_voskEngine;
	//		// 如果此时麦克风总开关是开着的，立刻启动新引擎
	//		if(ui->chkEnableASR->isChecked()) m_currentASR->start();
	//	}
	//});

	//connect(ui->radioWhisper,&QRadioButton::toggled,this,[=](bool checked){
	//	if(checked) {
	//		m_voskEngine->stop(); // 强行拔掉 Vosk 的车钥匙
	//		m_currentASR = m_whisperEngine;
	//		if(ui->chkEnableASR->isChecked()) m_currentASR->start();
	//	}
	//});

	//// --- 3. 连线：物理麦克风的数据源源不断喂给双引擎 ---
	//connect(micCapture,&AudioCapture::audioDataReady,m_voskEngine,&VoskTranscriber::onAudioDataReady);
	//connect(micCapture,&AudioCapture::audioDataReady,m_whisperEngine,&WhisperTranscriber::onAudioDataReady);

	//// 把双引擎的文字出口，统一汇聚到大脑
	//connect(m_voskEngine,&IAudioTranscriber::textReady,m_appController,&AppController::onASRTextReady);
	//connect(m_whisperEngine,&IAudioTranscriber::textReady,m_appController,&AppController::onASRTextReady);

	//

 //   // 实例化kaomojiManager
 //   KaomojiManager* kaomojiManager = new KaomojiManager(this);
 //   kaomojiManager->loadFromFile(QDir(QCoreApplication::applicationDirPath()).filePath("../../resources/kaomoji.json"));

 //   // 补上这极其关键的一行！把兵工厂的钥匙交给指挥官
 //   m_appController->setKaomojiManager(kaomojiManager);
 //   // 把加载配置的代码移到这里：所有信号槽连接完毕后，再触发 setText！
 //   QSettings settings("MyStudio", "Kotocord");
 //   QString savedEncryptedKey = settings.value("API/DeepSeekKey", "").toString();
 //   QString plainKey = deobfuscateKey(savedEncryptedKey);

 //   if (!plainKey.isEmpty()) {
 //       ui->inputApiKey->setText(plainKey); // 这一步现在会完美触发 textChanged 信号，传给 DeepSeek
 //       qDebug() << "[System] 成功从本地加载 API Key 并同步至大脑！";
 //       // 增加 UI 提示：证明我们连上了
 //       ui->lblCurrentEmotion->setText("已就绪，等待输入...");
 //       ui->lblCurrentEmotion->setStyleSheet("color: #28A745;"); // 变成健康的绿色
 //   }
 //   else {
 //       ui->lblCurrentEmotion->setText("未配置 API Key");
 //       ui->lblCurrentEmotion->setStyleSheet("color: #DC3545;"); // 红色警告
 //   }
}

MainWindow::~MainWindow()
{
    delete m_overlayWidget; // 释放内存
    delete ui;
    // 注: m_appController 和 m_mockLLM 因为挂载在 this 上，会自动释放，无需手动 delete
}

// --- 供外部调用的槽函数 ---
void MainWindow::onSubtitleReady(const SubtitleFrame& frame) {
	if(m_overlayWidget) m_overlayWidget->updateFrame(frame);
}

void MainWindow::updateCpuMem(double cpu,double mem) {
	ui->lblCpu->setText(QString::number(cpu,'f',2) + " %");
	ui->lblMem->setText(QString::number(mem,'f',2) + " MB");
}

void MainWindow::updateLatencyAndTokens(int prompt,int completion,qint64 latency) {
	ui->lblLatency->setText(QString::number(latency) + " ms");
	ui->lblTokens->setText(QString::number(prompt + completion) +
		" (提示: " + QString::number(prompt) + ", 生成: " + QString::number(completion) + ")");
}

void MainWindow::updateEmotionLabel(const SubtitleFrame& frame) {
	QString emotionName;
	switch(frame.emotion) {
	case EmotionType::Joy: emotionName = "高兴 (Joy)"; break;
	case EmotionType::Sadness: emotionName = "悲伤 (Sadness)"; break;
	case EmotionType::Anger: emotionName = "愤怒 (Anger)"; break;
	case EmotionType::Surprise: emotionName = "惊讶 (Surprise)"; break;
	default: emotionName = "平静 (Neutral)"; break;
	}
	ui->lblCurrentEmotion->setText(emotionName);
}

void MainWindow::onSystemReady(bool ready) {
	if(ready) {
		ui->lblCurrentEmotion->setText("已就绪，等待输入...");
		ui->lblCurrentEmotion->setStyleSheet("color: #28A745;");
	} else {
		ui->lblCurrentEmotion->setText("未配置 API Key");
		ui->lblCurrentEmotion->setStyleSheet("color: #DC3545;");
	}
}
