#include "SystemResourceMonitor.h"
#include <QDebug>

#ifdef Q_OS_WIN
#include <psapi.h>
#pragma comment(lib, "psapi.lib") // 让 MSVC 自动链接 psapi 库
#endif

SystemResourceMonitor::SystemResourceMonitor(QObject* parent): QObject(parent) {
	#ifdef Q_OS_WIN
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	m_numProcessors = sysInfo.dwNumberOfProcessors;

	// 初始化时间记录
	m_lastCpu.QuadPart = 0;
	m_lastSysCpu.QuadPart = 0;
	m_lastUserCpu.QuadPart = 0;
	#endif

	connect(&m_timer,&QTimer::timeout,this,&SystemResourceMonitor::updateStats);
}

void SystemResourceMonitor::start(int intervalMs) {
	if(!m_timer.isActive()) {
		m_timer.start(intervalMs);
		qDebug() << "[ResourceMonitor] 系统资源监控已启动。";
	}
}

void SystemResourceMonitor::stop() {
	m_timer.stop();
}

void SystemResourceMonitor::updateStats() {
	#ifdef Q_OS_WIN
	// 1. 获取内存信息 (Working Set Size)
	PROCESS_MEMORY_COUNTERS pmc;
	double memoryMB = 0.0;
	if(GetProcessMemoryInfo(GetCurrentProcess(),&pmc,sizeof(pmc))) {
		memoryMB = pmc.WorkingSetSize / (1024.0 * 1024.0);
	}

	// 2. 获取 CPU 信息
	FILETIME ftime,fsys,fuser;
	ULARGE_INTEGER now,sys,user;
	double cpuPercent = 0.0;

	GetSystemTimeAsFileTime(&ftime);
	memcpy(&now,&ftime,sizeof(FILETIME));

	GetProcessTimes(GetCurrentProcess(),&ftime,&ftime,&fsys,&fuser);
	memcpy(&sys,&fsys,sizeof(FILETIME));
	memcpy(&user,&fuser,sizeof(FILETIME));

	if(m_lastCpu.QuadPart != 0) {
		double cpuDiff = (now.QuadPart - m_lastCpu.QuadPart);
		double sysDiff = (sys.QuadPart - m_lastSysCpu.QuadPart);
		double userDiff = (user.QuadPart - m_lastUserCpu.QuadPart);

		if(cpuDiff > 0) {
			cpuPercent = ((sysDiff + userDiff) / cpuDiff) * 100.0 / m_numProcessors;
		}
	}

	m_lastCpu = now;
	m_lastSysCpu = sys;
	m_lastUserCpu = user;

	// 发射带有两位小数精度的数据
	emit resourceUpdated(cpuPercent,memoryMB);
	#endif
}
