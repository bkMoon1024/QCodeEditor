// QCodeEditor
#include <QPythonCompleter>
#include <QLanguage>
#include <QSymbolExtracter>

// Qt
#include <QStringListModel>
#include <QFile>
#include <QTextBlock>
#include <QTextCursor>
#include <QAbstractItemView>

QPythonCompleter::QPythonCompleter(QObject *parent) :
    QCompleter(parent),
    m_extracter(nullptr)
{
    // 创建模型
    m_model = new QStringListModel(this);
    setModel(m_model);
    
    // 设置基本属性
    setCompletionColumn(0);
    setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    setCaseSensitivity(Qt::CaseSensitive);
    setWrapAround(true);
    
    // 加载内置符号
    m_builtinSymbols.clear();
    
    Q_INIT_RESOURCE(qcodeeditor_resources);
    QFile fl(":/languages/python.xml");

    if (fl.open(QIODevice::ReadOnly))
    {
        QLanguage language(&fl);

        if (language.isLoaded())
        {
            auto keys = language.keys();
            for (auto&& key : keys)
            {
                auto names = language.names(key);
                m_builtinSymbols.append(names);
            }
        }
        
        fl.close();
    }
    
    // 初始化特殊补全项
    initSpecialCompletions();
    
    // 更新模型
    updateCompleterModel();
}

QPythonCompleter::~QPythonCompleter()
{
    // m_model和m_extracter由QObject系统自动释放
}

void QPythonCompleter::initSpecialCompletions()
{
    // 添加标准Python代码模板
    addSpecialCompletion("main", "if __name__ == \"__main__\":\n    ");
    addSpecialCompletion("try", "try:\n    \nexcept Exception as e:\n    ");
    addSpecialCompletion("for", "for i in range(10):\n    ");
    addSpecialCompletion("while", "while True:\n    ");
    addSpecialCompletion("if", "if condition:\n    ");
    addSpecialCompletion("elif", "elif condition:\n    ");
    addSpecialCompletion("else", "else:\n    ");
    addSpecialCompletion("class", "class ClassName:\n    def __init__(self):\n        ");
    addSpecialCompletion("def", "def function_name(parameters):\n    ");
    
    // 确保Python关键字被添加为补全项
    addSpecialCompletion("return", "return ");
    addSpecialCompletion("import", "import ");
    addSpecialCompletion("from", "from module import ");
}

void QPythonCompleter::addSpecialCompletion(const QString& keyword, const QString& expansion)
{
    m_specialCompletions[keyword] = expansion;
    
    // 确保特殊关键字也出现在补全列表中
    QStringList list;
    
    // 获取当前模型中的所有项
    QStringListModel* model = qobject_cast<QStringListModel*>(this->model());
    if (model) {
        list = model->stringList();
    }
    
    // 如果列表中没有这个关键字，添加它
    if (!list.contains(keyword)) {
        list.append(keyword);
        list.sort(Qt::CaseInsensitive);
        
        if (model) {
            model->setStringList(list);
        }
    }
}

QString QPythonCompleter::getSpecialCompletion(const QString& keyword) const
{
    return m_specialCompletions.value(keyword, QString());
}

bool QPythonCompleter::isSpecialCompletion(const QString& text) const
{
    return m_specialCompletions.contains(text);
}

QString QPythonCompleter::pathFromIndex(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return QString();
    }
    
    return index.data().toString();
}

QStringList QPythonCompleter::splitPath(const QString& path) const
{
  // 如果不包含点号，使用默认的完成模式
  if (!path.contains(".")) {
      // 记录用户当前输入
      QString currentInput = path.trimmed();
      
      // 临时创建一个不包含当前输入的列表
      if (!currentInput.isEmpty()) {
          QStringListModel* tempModel = qobject_cast<QStringListModel*>(model());
          if (tempModel) {
              QStringList completeList = tempModel->stringList();
              
              // 移除用户当前输入的完全匹配项
              completeList.removeAll(currentInput);
              
              // 设置新的列表
              tempModel->setStringList(completeList);
          }
      }
      
      return QCompleter::splitPath(path);
  }
  
  // 解析对象成员访问（object.member）
  int lastDotPos = path.lastIndexOf(".");
  if (lastDotPos > 0) {
    // 获取对象名
    QString objectName = path.left(lastDotPos);
    // 获取当前正在输入的成员前缀
    QString memberPrefix = path.mid(lastDotPos + 1);
    
    // 清空当前补全列表
    m_currentCompletionList.clear();
    m_currentCompletionObject = objectName;
    
    // 如果有符号提取器，获取对象成员
    if (m_extracter) {
        // 确保符号提取器已更新最新代码
        QStringList symbols = m_extracter->symbols();
        
        if (m_currentCompletionList.isEmpty()) {
            m_currentCompletionList = m_extracter->getObjectMembers(objectName);
        }
        
        // 如果没有找到成员，尝试从类型信息中查找
        if (m_currentCompletionList.isEmpty()) {
            QString objectType = m_extracter->getObjectType(objectName);
            
            if (!objectType.isEmpty()) {
                m_currentCompletionList = m_extracter->getObjectMembers(objectType);
            }
        }
        
        // 移除用户当前输入的完全匹配项
        if (!memberPrefix.isEmpty()) {
            m_currentCompletionList.removeAll(memberPrefix);
        }
    } 

    // 设置临时模型
    QStringListModel* tempModel = qobject_cast<QStringListModel*>(model());
    if (tempModel) {
      // 根据前缀排序列表，让匹配度更高的排在前面
      if (!memberPrefix.isEmpty()) {
          // 先按完全匹配排序（完全匹配项在上面已移除）
          QStringList startsWithMatches, containsMatches, otherMatches;
          for (const QString& item : m_currentCompletionList) {
              if (item.startsWith(memberPrefix, Qt::CaseInsensitive)) {
                  startsWithMatches << item;
              } else if (item.contains(memberPrefix, Qt::CaseInsensitive)) {
                  containsMatches << item;
              } else {
                  otherMatches << item;
              }
          }
        
        // 按照优先级合并列表
        startsWithMatches.sort(Qt::CaseInsensitive);
        containsMatches.sort(Qt::CaseInsensitive);
        otherMatches.sort(Qt::CaseInsensitive);
        
        QStringList sortedList = startsWithMatches + containsMatches + otherMatches;
        m_currentCompletionList = sortedList;
      } else {
        // 无前缀时简单排序
        m_currentCompletionList.sort(Qt::CaseInsensitive);
      }
      
      // 直接设置模型的字符串列表
      tempModel->setStringList(m_currentCompletionList);
      
      // 强制显示补全弹出窗口
      if (popup()) {
          popup()->setVisible(true);
          popup()->update();
      }
    }
    return QStringList(memberPrefix);
  }
  return QStringList(path);
}

void QPythonCompleter::setExtracter(QSymbolExtracter* extracter)
{
    // 断开旧的连接
    if (m_extracter) {
        disconnect(m_extracter, &QSymbolExtracter::symbolsUpdated,
                  this, &QPythonCompleter::onSymbolsUpdated);
    }
    
    m_extracter = extracter;
    
    // 建立新的连接
    if (m_extracter) {
        connect(m_extracter, &QSymbolExtracter::symbolsUpdated,
                this, &QPythonCompleter::onSymbolsUpdated);
        
        // 立即更新符号列表
        onSymbolsUpdated(m_extracter->symbols());
    }
}

QSymbolExtracter* QPythonCompleter::extracter() const
{
    return m_extracter;
}

void QPythonCompleter::updateUserSymbols(const QStringList& userSymbols)
{
    m_userSymbols = userSymbols;
    updateCompleterModel();
}

void QPythonCompleter::onSymbolsUpdated(const QStringList& symbols)
{
    m_userSymbols = symbols;
    updateCompleterModel();
}

void QPythonCompleter::updateCompleterModel()
{
    // 合并内置符号和用户符号
    QStringList completeList = m_builtinSymbols + m_userSymbols;
    
    // 添加特殊补全项的关键字
    QStringList specialKeys = m_specialCompletions.keys();
    for (const QString& key : specialKeys) {
        if (!completeList.contains(key)) {
            completeList.append(key);
        }
    }
    
    // 移除重复项
    completeList.removeDuplicates();
    
    // 排序
    completeList.sort(Qt::CaseInsensitive);
    
    // 获取当前补全前缀
    QString currentCompletion = completionPrefix().trimmed();
    
    // 如果有当前输入内容，从列表中移除完全匹配项
    if (!currentCompletion.isEmpty()) {
        completeList.removeAll(currentCompletion);
    }
    
    // 更新模型
    m_model->setStringList(completeList);
}
