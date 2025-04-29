cmake_minimum_required(VERSION 3.6)
project(QCodeEditor)

set(CMAKE_CXX_STANDARD 11)

# 添加MSVC静态运行时库设置
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  foreach(flag_var CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    if(${flag_var} MATCHES "/MD")
      string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
    endif()
  endforeach()
endif()

option(BUILD_EXAMPLE "Example building required" Off)

if (${BUILD_EXAMPLE})
    message(STATUS "QCodeEditor example will be built.")
    add_subdirectory(example)
endif()

set(RESOURCES_FILE
    resources/qcodeeditor_resources.qrc
)

set(INCLUDE_FILES
    include/QHighlightRule
    include/QHighlightBlockRule
    include/QCodeEditor
    include/QCXXHighlighter
    include/QLineNumberArea
    include/QStyleSyntaxHighlighter
    include/QSyntaxStyle
    include/QSymbolExtracter
    include/QGLSLCompleter
    include/QGLSLHighlighter
    include/QLanguage
    include/QXMLHighlighter
    include/QJSONHighlighter
    include/QLuaCompleter
    include/QLuaHighlighter
    include/QPythonHighlighter
    include/QPythonExtracter
    include/QFramedTextAttribute
    include/internal/QHighlightRule.hpp
    include/internal/QHighlightBlockRule.hpp
    include/internal/QCodeEditor.hpp
    include/internal/QCXXHighlighter.hpp
    include/internal/QLineNumberArea.hpp
    include/internal/QStyleSyntaxHighlighter.hpp
    include/internal/QSyntaxStyle.hpp
    include/internal/QSymbolExtracter.hpp
    include/internal/QGLSLCompleter.hpp
    include/internal/QGLSLHighlighter.hpp
    include/internal/QLanguage.hpp
    include/internal/QXMLHighlighter.hpp
    include/internal/QJSONHighlighter.hpp
    include/internal/QLuaCompleter.hpp
    include/internal/QLuaHighlighter.hpp
    include/internal/QPythonCompleter.hpp
    include/internal/QPythonHighlighter.hpp
    include/internal/QPythonExtracter.hpp
    include/internal/QFramedTextAttribute.hpp
)

set(SOURCE_FILES
    src/internal/QCodeEditor.cpp
    src/internal/QLineNumberArea.cpp
    src/internal/QCXXHighlighter.cpp
    src/internal/QSyntaxStyle.cpp
    src/internal/QStyleSyntaxHighlighter.cpp
    src/internal/QGLSLCompleter.cpp
    src/internal/QGLSLHighlighter.cpp
    src/internal/QLanguage.cpp
    src/internal/QXMLHighlighter.cpp
    src/internal/QJSONHighlighter.cpp
    src/internal/QLuaCompleter.cpp
    src/internal/QLuaHighlighter.cpp
    src/internal/QPythonCompleter.cpp
    src/internal/QPythonHighlighter.cpp
    src/internal/QPythonExtracter.cpp
    src/internal/QFramedTextAttribute.cpp
)

# Create code for QObjects
set(CMAKE_AUTOMOC On)

# Create code from resource files
set(CMAKE_AUTORCC ON)

# Find includes in corresponding build directories
find_package(Qt5Core    CONFIG REQUIRED)
find_package(Qt5Widgets CONFIG REQUIRED)
find_package(Qt5Gui     CONFIG REQUIRED)

add_library(QCodeEditor STATIC
    ${RESOURCES_FILE}
    ${SOURCE_FILES}
    ${INCLUDE_FILES}
)

# 为QCodeEditor目标设置MSVC_RUNTIME_LIBRARY属性
if(MSVC)
    set_property(TARGET QCodeEditor PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

target_include_directories(QCodeEditor PUBLIC
    include
)

if(CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(QCodeEditor
		PRIVATE
		-ansi
		-pedantic
		-Wall
		-Wextra
		-Weffc++
		-Woverloaded-virtual
		-Winit-self
		-Wunreachable-code
	)
endif(CMAKE_COMPILER_IS_GNUCXX)

target_link_libraries(QCodeEditor
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
)
