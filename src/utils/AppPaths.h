#ifndef APPPATHS_H
#define APPPATHS_H

#include <QString>
#include <QCoreApplication>
#include <QDir>

class AppPaths {
public:
	//项目根目录
	static QString getProjectRootDir() {//exe 在 bin/Debug 下，向上两级，起点"H:/.../Kotocord"
		QString exeDir = QCoreApplication::applicationDirPath();
		return QDir::cleanPath(exeDir + "/../..");
	}

	//资源根目录
	static QString getResourcesDir() {
		return getProjectRootDir() + "/resources";
	}

	//具体的资源寻址方法 (对外暴露的 API)
	static QString getKaomojiPath() {
		return getResourcesDir() + "/kaomoji.json";
	}

	static QString getVoskModelPath() {
		return getResourcesDir() + "/model/vosk-model-small-cn-0.22";
	}

	static QString getWhisperModelPath() {
		return getResourcesDir() + "/model/ggml-small.bin";
	}
};

#endif // APPPATHS_H
