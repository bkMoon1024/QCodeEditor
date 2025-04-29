#pragma once

// Qt
#include <QCompleter> // Required for inheritance
#include <QMap>

// Forward declaration
class QSymbolExtracter;
class QPythonExtracter;
class QStringListModel;

/**
 * @brief 类，用于描述带有 Python 特定类型和函数的补全器
 */
class QPythonCompleter : public QCompleter
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象指针
     */
    explicit QPythonCompleter(QObject* parent=nullptr);

    /**
     * @brief 析构函数
     */
    virtual ~QPythonCompleter();

    /**
     * @brief 设置符号提取器
     * @param extracter 符号提取器
     */
    void setExtracter(QSymbolExtracter* extracter);
    
    /**
     * @brief 获取符号提取器
     * @return 符号提取器指针
     */
    QSymbolExtracter* extracter() const;
    
    /**
     * @brief 更新补全列表
     * @param userSymbols 用户定义的符号列表
     */
    void updateUserSymbols(const QStringList& userSymbols);
    
    /**
     * @brief 重写pathFromIndex，处理对象成员访问
     */
    QString pathFromIndex(const QModelIndex& index) const override;
    
    /**
     * @brief 重写splitPath，处理对象成员访问
     */
    QStringList splitPath(const QString& path) const override;
    
    /**
     * @brief 设置特殊补全项
     * @param keyword 关键字
     * @param expansion 展开内容
     */
    void addSpecialCompletion(const QString& keyword, const QString& expansion);
    
    /**
     * @brief 获取特殊补全项
     * @param keyword 关键字
     * @return 对应的展开内容，如果不存在则返回空字符串
     */
    QString getSpecialCompletion(const QString& keyword) const;
    
    /**
     * @brief 检查是否是特殊补全项
     * @param text 补全文本
     * @return 是否是特殊补全项
     */
    bool isSpecialCompletion(const QString& text) const;

private slots:
    /**
     * @brief 处理符号更新信号
     * @param symbols 更新后的符号列表
     */
    void onSymbolsUpdated(const QStringList& symbols);

private:
    /**
     * @brief 更新补全模型
     */
    void updateCompleterModel();
    
    /**
     * @brief 初始化默认的特殊补全项
     */
    void initSpecialCompletions();
    
private:
    QSymbolExtracter* m_extracter = nullptr;  // 符号提取器
    QStringListModel* m_model = nullptr;      // 补全数据模型
    QStringList m_builtinSymbols;             // 内置符号列表
    QStringList m_userSymbols;                // 用户定义的符号列表
    
    // 对象成员补全相关
    mutable QString m_currentCompletionObject;  // 当前正在补全的对象
    mutable QStringList m_currentCompletionList; // 当前对象的成员列表
    
    // 特殊补全相关
    QMap<QString, QString> m_specialCompletions; // 特殊补全项（关键字 -> 展开内容）
};


