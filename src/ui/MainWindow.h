#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "../modules/render/SubtitleRenderer.h"

// 引入前向声明或直接 include
class AppController;
class MockLLMWorker;

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
    SubtileRenderer* m_overlayWidget; // 添加透明窗口指针

    //新增：核心控制器和虚拟大模型
    AppController* m_appController;
    MockLLMWorker* m_mockLLM;
};
#endif // MAINWINDOW_H