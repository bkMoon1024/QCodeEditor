#pragma once

// Qt
#include <QObject>
#include <QStringList>
#include <QRegularExpression>
#include <QMap>
#include <QMultiMap>
#include <QList>
#include <QSet>

// 项目
#include <QSymbolExtracter>

/**
 * @brief 类，用于提取Python代码中的符号（类、变量、函数等）
 */
class QPythonExtracter : public QSymbolExtracter
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象指针
     */
    explicit QPythonExtracter(QObject* parent = nullptr);

    /**
     * @brief 解析Python代码并提取符号
     * @param code Python代码文本
     * @return 提取的所有符号列表
     */
    QStringList extractSymbols(const QString& code) override;

    /**
     * @brief 获取已提取的所有符号
     * @return 符号列表
     */
    QStringList symbols() const override;
    
    /**
     * @brief 获取对象的成员（方法和属性）
     * @param objectName 对象名称
     * @return 对象成员列表
     */
    QStringList getObjectMembers(const QString& objectName) const override;
    
    /**
     * @brief 获取对象的类型
     * @param objectName 对象名称
     * @return 对象的类型名称，如果不存在则返回空字符串
     */
    QString getObjectType(const QString& objectName) const override;
    
    /**
     * @brief 获取所有对象类型映射
     * @return 对象到类型的映射
     */
    QMap<QString, QString> getObjectTypesMap() const override;
    
    /**
     * @brief 获取所有符号的位置信息
     * @return 符号位置信息列表
     */
    QList<SymbolInfo> getSymbolsInfo() const override;
    
    /**
     * @brief 根据行号和列号获取符号信息
     * @param line 行号
     * @param column 列号
     * @return 符号信息，如果没有找到则返回空的SymbolInfo
     */
    SymbolInfo getSymbolInfoAtPosition(int line, int column) const override;

private:
    /**
     * @brief 初始化Python关键字列表
     */
    void initPythonKeywords();

    /**
     * @brief 提取类定义
     * @param code Python代码
     * @param symbols 符号列表，用于存储提取结果
     */
    void extractClasses(const QString &code, QStringList &symbols);

    /**
     * @brief 查找类定义的结束位置
     * @param code Python代码
     * @param startPos 类定义开始位置
     * @return 类定义结束位置
     */
    int findClassEnd(const QString &code, int startPos);

    /**
     * @brief 根据位置查找所在的类
     * @param code Python代码
     * @param position 当前位置
     * @return 类名，如果不在类内则返回空字符串
     */
    QString findClassForPosition(const QString &code, int position);

    /**
     * @brief 提取类的成员（方法和属性）
     * @param className 类名
     * @param classCode 类的代码
     * @param members 成员列表，用于存储提取结果
     */
    void extractClassMembers(const QString &className, const QString &classCode, QStringList &members);

    /**
     * @brief 提取函数定义
     * @param code Python代码
     * @param symbols 符号列表，用于存储提取结果
     */
    void extractFunctions(const QString &code, QStringList &symbols);

    /**
     * @brief 提取变量定义
     * @param code Python代码
     * @param symbols 符号列表，用于存储提取结果
     */
    void extractVariables(const QString &code, QStringList &symbols);

    /**
     * @brief 提取导入的模块和对象
     * @param code Python代码
     * @param symbols 符号列表，用于存储提取结果
     */
    void extractImports(const QString &code, QStringList &symbols);

    /**
     * @brief 提取对象类型映射
     * @param code Python代码
     */
    void extractObjectTypes(const QString &code);

    /**
     * @brief 计算符号的位置信息
     * @param code Python代码
     */
    void extractSymbolPositions(const QString &code);

private:
    QStringList m_symbols;                       // 存储所有已提取的符号
    QStringList m_pythonKeywords;                // Python关键字列表
    QMap<QString, QString> m_objectTypes;        // 对象到类型的映射
    QMultiMap<QString, QString> m_classMembers;  // 类到成员的映射
    QList<SymbolInfo> m_symbolsInfo;             // 符号位置信息列表
    QMap<QString, QString> m_functionParameters; // 函数名到参数字符串的映射
}; 
