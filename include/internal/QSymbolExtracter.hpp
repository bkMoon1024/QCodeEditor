#pragma once

// Qt
#include <QObject>
#include <QStringList>
#include <QList>
#include <QMap>

/**
 * @brief 符号信息结构体，存储符号及其位置信息
 */
struct SymbolInfo {
    enum SymbolType {
        Keyword,    // 关键字
        Class,      // 类
        Function,   // 函数
        Variable,   // 变量
        Import,     // 导入
        Other       // 其他
    };
    
    QString name;         // 符号名称
    SymbolType type;      // 符号类型
    int line;             // 行号
    int column;           // 列号
    int length;           // 长度
    QString scope;        // 作用域，例如类名或命名空间
    QString parameters;   // 函数参数字符串，仅当type为Function时有效
    
    SymbolInfo() : type(Other), line(0), column(0), length(0), scope(""), parameters("") {}
    
    SymbolInfo(const QString& n, SymbolType t, int l, int c, int len, const QString& s = "", const QString& p = "") 
        : name(n), type(t), line(l), column(c), length(len), scope(s), parameters(p) {}
};

/**
 * @brief 符号提取器的基类，为不同语言的符号提取提供统一接口
 */
class QSymbolExtracter : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象指针
     */
    explicit QSymbolExtracter(QObject* parent = nullptr) : QObject(parent) {}
    
    /**
     * @brief 析构函数
     */
    virtual ~QSymbolExtracter() {}

    /**
     * @brief 解析代码并提取符号
     * @param code 代码文本
     * @return 提取的所有符号列表
     */
    virtual QStringList extractSymbols(const QString& code) = 0;

    /**
     * @brief 获取已提取的所有符号
     * @return 符号列表
     */
    virtual QStringList symbols() const = 0;
    
    /**
     * @brief 获取对象的成员（方法和属性）
     * @param objectName 对象名称
     * @return 对象成员列表
     */
    virtual QStringList getObjectMembers(const QString& objectName) const = 0;
    
    /**
     * @brief 获取对象的类型
     * @param objectName 对象名称
     * @return 对象的类型名称，如果不存在则返回空字符串
     */
    virtual QString getObjectType(const QString& objectName) const = 0;
    
    /**
     * @brief 获取所有对象类型映射
     * @return 对象到类型的映射
     */
    virtual QMap<QString, QString> getObjectTypesMap() const = 0;
    
    /**
     * @brief 获取所有符号的位置信息
     * @return 符号位置信息列表
     */
    virtual QList<SymbolInfo> getSymbolsInfo() const = 0;
    
    /**
     * @brief 根据行号和列号获取符号信息
     * @param line 行号
     * @param column 列号
     * @return 符号信息，如果没有找到则返回空的SymbolInfo
     */
    virtual SymbolInfo getSymbolInfoAtPosition(int line, int column) const = 0;

signals:
    /**
     * @brief 当符号列表更新时发出信号
     * @param symbols 更新后的符号列表
     */
    void symbolsUpdated(const QStringList& symbols);
    
    /**
     * @brief 当符号位置信息更新时发出信号
     * @param symbolsInfo 更新后的符号位置信息
     */
    void symbolsInfoUpdated(const QList<SymbolInfo>& symbolsInfo);

protected:
    /**
     * @brief 计算文本中的行号和列号
     * @param text 完整文本
     * @param position 字符位置
     * @param line 输出参数：行号
     * @param column 输出参数：列号
     */
    void calculateLineAndColumn(const QString& text, int position, int& line, int& column)
    {
        line = 1; // 行号从1开始
        column = 1; // 列号从1开始
        
        for (int i = 0; i < position && i < text.length(); ++i) {
            if (text[i] == '\n') {
                line++;
                column = 1;
            } else {
                column++;
            }
        }
    }
}; 