cmake_minimum_required(VERSION 3.16)
project(Kotocord VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(MSVC)
    add_compile_options(/utf-8)
endif()

file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

set(CMAKE_AUTOMOC ON) 
set(CMAKE_AUTOUIC ON) 
set(CMAKE_AUTORCC ON) 

# 注意：这里新增了 Multimedia 模块！
# 如果 CMake 报错找不到 Multimedia，请运行 Qt Maintenance Tool 勾选安装 "Qt Multimedia" 组件。
find_package(Qt6 COMPONENTS Widgets Multimedia REQUIRED)

# 更新：移除了 TransparentWidget
set(UI_SOURCES
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.ui
)

# 2. 核心架构源码
set(CORE_SOURCES
    src/main.cpp
    src/core/AppController.cpp
    src/core/AppController.h
    src/core/DataTypes.h
)

# 3. 模块源码 (注意：我为你预留了 AudioCapture.cpp 的位置，你需要创建它)
set(MODULE_SOURCES
    src/modules/input/IAudioTranscriber.h

    src/modules/input/VoskTranscriber.h
    src/modules/input/VoskTranscriber.cpp

    src/modules/input/WhisperTranscriber.h

    src/modules/input/AudioCapture.h
    src/modules/input/AudioCapture.cpp 

    src/modules/llm/KaomojiManager.h
    src/modules/llm/KaomojiManager.cpp

    src/modules/llm/ILanguageModel.h
    src/modules/llm/DeepSeekAPIWorker.h
    src/modules/llm/DeepSeekAPIWorker.cpp

    #src/modules/llm/OpenAILLMWorker.h

    src/modules/llm/MockLLMWorker.h
    src/modules/llm/MockLLMWorker.cpp

    src/modules/input/AudioFileSimulator.h
    src/modules/input/AudioFileSimulator.cpp

    src/modules/render/SubtitleRenderer.h
    src/modules/render/SubtitleRenderer.cpp
)

# 4. 资源文件
#set(RESOURCE_FILES
#    resources/resources.qrc
#)

add_executable(${PROJECT_NAME} 
    ${CORE_SOURCES} 
    ${UI_SOURCES} 
    ${MODULE_SOURCES}
    ${RESOURCE_FILES}
)

# 设置 vosk 根目录变量，方便后续引用
set(VOSK_DIR "${CMAKE_SOURCE_DIR}/third_party/vosk")

# 告诉项目去哪里找 vosk_api.h
target_include_directories(${PROJECT_NAME} PRIVATE ${VOSK_DIR}/include)

# Vosk 库的查找
find_library(VOSK_LIB
    NAMES vosk libvosk
    PATHS ${VOSK_DIR}/lib
    NO_DEFAULT_PATH  # 只在指定路径查找
)
if(NOT VOSK_LIB)
    message(FATAL_ERROR "Vosk library not found in ${VOSK_DIR}/lib")
endif()

# 链接 Widgets, Multimedia 以及 Vosk 的静态导入库
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets
    Qt6::Multimedia
    ${VOSK_LIB}   # 链接 Vosk 库
)

# 1. 查找 third_party/vosk/lib 下所有的 .dll .so .dylib 运行时库到输出目录（根据平台选择）
if(WIN32)
    file(GLOB VOSK_RUNTIME_LIBS "${VOSK_DIR}/lib/*.dll")
elseif(APPLE)
    file(GLOB VOSK_RUNTIME_LIBS "${VOSK_DIR}/lib/*.dylib")
else()
    file(GLOB VOSK_RUNTIME_LIBS "${VOSK_DIR}/lib/*.so")
endif()

if(VOSK_RUNTIME_LIBS)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${VOSK_RUNTIME_LIBS}
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()
