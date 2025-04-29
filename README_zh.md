# Qt 代码编辑器组件
这是一个用于编辑/查看代码的组件。

本项目使用名为 `qcodeeditor_resources.qrc` 的资源文件。主应用程序不能使用相同名称的资源文件。

(这不是一个 Qt 示例项目。)

## 要求
0. 支持 C++11 的编译器
0. Qt 5

## 功能
1. 自动括号匹配
1. 不同的语法高亮规则
1. 自动缩进
1. 用空格替换制表符
1. GLSL 代码补全规则
1. GLSL 语法高亮规则
1. C++ 语法高亮规则
1. XML 语法高亮规则
1. JSON 语法高亮规则
1. 框选功能
1. Qt Creator 样式

## 构建
这是一个基于 CMake 的库，可以作为子模块使用（参见示例）。
以下是将其构建为静态库的步骤（例如用于外部使用）：

1. 克隆仓库：`git clone https://gitee.com/h1239922542/QCodeEditor`
1. 进入仓库：`cd QCodeEditor`
1. 创建构建文件夹：`mkdir build`
1. 进入构建文件夹：`cd build`
1. 为你的编译器生成构建文件：`cmake ..`
    1. 如果需要构建示例，在此步骤指定 `-DBUILD_EXAMPLE=On`
1. 构建库：`cmake --build .`

## 示例

默认情况下，`QCodeEditor` 使用标准的 QtCreator 主题。但你可以通过 `QSyntaxStyle` 解析来指定自己的主题。示例使用了 [Dracula](https://draculatheme.com) 主题。
（更多内容请参见示例。）

<img src="https://gitee.com/h1239922542/QCodeEditor/blob/master/example/image/preview.png">

## 许可证

<img align="right" src="http://opensource.org/trademarks/opensource/OSI-Approved-License-100x137.png">

本库采用 [MIT 许可证](https://opensource.org/licenses/MIT)

特此免费授予任何获得本软件及相关文档文件（"软件"）副本的人不受限制地处理本软件的权利，包括但不限于使用、复制、修改、合并、发布、分发、再许可和/或销售本软件副本的权利，并允许向其提供本软件的人这样做，但须符合以下条件：

上述版权声明和本许可声明应包含在本软件的所有副本或重要部分中。

本软件按"原样"提供，不提供任何明示或暗示的保证，包括但不限于对适销性、特定用途适用性和非侵权性的保证。在任何情况下，作者或版权持有人均不对任何索赔、损害或其他责任负责，无论是在合同诉讼、侵权诉讼或其他诉讼中，由软件或软件的使用或其他交易引起的、由软件引起的或与之相关的。
