#include <QApplication>
#include "ui/MainWindow.h"

int main(int argc, char* argv[]) {
    // 初始化 Qt 应用程序实例
    QApplication app(argc, argv);

    // 实例化主控制面板窗口并显示
    MainWindow window;
    window.show();

    // 进入 Qt 的事件循环（开始监听鼠标、键盘、渲染等事件）
    return app.exec();
}