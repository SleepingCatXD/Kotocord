cmake_minimum_required(VERSION 3.16)
project(Kotocord VERSION 0.1.0 LANGUAGES CXX)

# ==========================================
# 1. 编译选项与全局配置
# ==========================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(MSVC)
    add_compile_options(/utf-8)
endif()

file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin/Debug")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/Release")

set(CMAKE_AUTOMOC ON) 
set(CMAKE_AUTOUIC ON) 
set(CMAKE_AUTORCC ON) 

find_package(Qt6 COMPONENTS Widgets Multimedia REQUIRED)

# ==========================================
# 2. 源码文件分类整理
# ==========================================
set(UI_SOURCES
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.ui
)

set(CORE_SOURCES
    src/main.cpp
    src/core/AppController.cpp
    src/core/AppController.h
    src/core/DataTypes.h
)

set(MODULE_SOURCES
    # Input / ASR
    src/modules/input/IAudioTranscriber.h
    src/modules/input/VoskTranscriber.h
    src/modules/input/VoskTranscriber.cpp
    src/modules/input/WhisperTranscriber.h
    src/modules/input/WhisperTranscriber.cpp

    src/modules/input/AudioCapture.h
    src/modules/input/AudioCapture.cpp 
    src/modules/input/AudioFileSimulator.h
    src/modules/input/AudioFileSimulator.cpp

    # LLM & Emotion
    src/modules/llm/ILanguageModel.h
    src/modules/llm/DeepSeekAPIWorker.h
    src/modules/llm/DeepSeekAPIWorker.cpp
    src/modules/llm/MockLLMWorker.h
    src/modules/llm/MockLLMWorker.cpp

    src/modules/llm/KaomojiManager.h
    src/modules/llm/KaomojiManager.cpp

    #src/modules/llm/OpenAILLMWorker.h

    # Render
    src/modules/render/SubtitleRenderer.h
    src/modules/render/SubtitleRenderer.cpp

    # System
    src/modules/system/SystemResourceMonitor.h
    src/modules/system/SystemResourceMonitor.cpp

)

#set(RESOURCE_FILES resources/resources.qrc)

# ==========================================
# 3. 定义生成目标 (Executable)
# ==========================================
add_executable(${PROJECT_NAME} 
    ${CORE_SOURCES} 
    ${UI_SOURCES} 
    ${MODULE_SOURCES}
    #${RESOURCE_FILES}
)

# ==========================================
# 4. 第三方库配置 (Vosk & Whisper)
# ==========================================
set(VOSK_DIR "${CMAKE_SOURCE_DIR}/third_party/vosk")
set(WHISPER_DIR "${CMAKE_SOURCE_DIR}/third_party/whisper")

# 4.1 引入头文件目录
target_include_directories(${PROJECT_NAME} PRIVATE ${VOSK_DIR}/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${WHISPER_DIR}/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${WHISPER_DIR}/ggml/include)

# 4.2 查找静态导入库 (.lib / .a)
find_library(VOSK_LIB
    NAMES vosk libvosk
    PATHS ${VOSK_DIR}/lib
    NO_DEFAULT_PATH
)
if(NOT VOSK_LIB)
    message(FATAL_ERROR "Vosk library not found in ${VOSK_DIR}/lib")
endif()

find_library(WHISPER_LIB
    NAMES whisper
    PATHS ${WHISPER_DIR}/lib
    NO_DEFAULT_PATH
)
if(NOT WHISPER_LIB)
    message(FATAL_ERROR "Whisper library not found in ${WHISPER_DIR}/lib")
endif()

# 4.3 链接所有依赖库
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets
    Qt6::Multimedia
    ${VOSK_LIB}
    ${WHISPER_LIB}
)

# 4.4 编译完成后，自动拷贝所有相关的 DLL 到执行目录
if(WIN32)
    # 为了防止你把 dll 放错地方，这里同时扫描 lib 和 bin 目录
    file(GLOB RUNTIME_LIBS 
        "${VOSK_DIR}/lib/*.dll" 
        "${WHISPER_DIR}/lib/*.dll"
        "${WHISPER_DIR}/ggml/lib/*.dll"
    )
elseif(APPLE)
    file(GLOB RUNTIME_LIBS 
        "${VOSK_DIR}/lib/*.dylib" 
        "${WHISPER_DIR}/lib/*.dylib"
        "${WHISPER_DIR}/ggml/lib/*.dylib"
    )
else()
    file(GLOB RUNTIME_LIBS 
        "${VOSK_DIR}/lib/*.so" 
        "${WHISPER_DIR}/lib/*.so"
        "${WHISPER_DIR}/ggml/lib/*.so"
    )
endif()

if(RUNTIME_LIBS)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${RUNTIME_LIBS}
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()

# ==========================================
# 5. 开启预编译头 (PCH) 优化编译
# ==========================================
target_precompile_headers(${PROJECT_NAME} PRIVATE
    <QString>
    <QObject>
    <QWidget>
    <QDebug>
    <QMutex>
    <QThread>
    <QSettings>
    <QTimer>
    <QJsonDocument>
    <QNetworkAccessManager>
)
