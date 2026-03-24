#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "../modules/render/SubtitleRenderer.h"
#include "../modules/input/VoskTranscriber.h"
#include "../modules/input/WhisperTranscriber.h"

// 引入前向声明或直接 include
class AppController;
class MockLLMWorker;
class DeepSeekAPIWorker;

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow* ui;
    SubtitleRenderer* m_overlayWidget; // 添加透明窗口指针

	VoskTranscriber* m_voskEngine;
	WhisperTranscriber* m_whisperEngine;
	IAudioTranscriber* m_currentASR; // 当前激活的引擎指针

    //新增：核心控制器和虚拟大模型
    AppController* m_appController;
    MockLLMWorker* m_mockLLM;
    DeepSeekAPIWorker* m_deepSeekLLM;
};
#endif // MAINWINDOW_H
