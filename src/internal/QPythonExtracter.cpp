// QCodeEditor
#include <QPythonExtracter>

// Qt
#include <QRegularExpression>

QPythonExtracter::QPythonExtracter(QObject *parent) :
    QSymbolExtracter(parent)
{
    initPythonKeywords();
}

QStringList QPythonExtracter::extractSymbols(const QString& code)
{
    if (code.isEmpty()) {
        return QStringList();
    }

    QStringList symbolList;
    
    // 清空对象-类型映射
    m_objectTypes.clear();
    m_classMembers.clear();
    m_symbolsInfo.clear();
    
    // 添加Python关键字到符号列表
    symbolList.append(m_pythonKeywords);
    
    // 提取不同类型的符号
    extractClasses(code, symbolList);
    extractFunctions(code, symbolList);
    extractVariables(code, symbolList);
    extractImports(code, symbolList);
    
    // 提取对象类型关系
    extractObjectTypes(code);
    
    // 计算符号位置信息
    extractSymbolPositions(code);
    
    // 移除重复项并排序
    symbolList.removeDuplicates();
    symbolList.sort();
    
    // 更新成员变量
    m_symbols = symbolList;
    
    // 发出信号通知符号列表已更新
    emit symbolsUpdated(m_symbols);
    emit symbolsInfoUpdated(m_symbolsInfo);
    
    return m_symbols;
}

QStringList QPythonExtracter::symbols() const
{
    return m_symbols;
}

QStringList QPythonExtracter::getObjectMembers(const QString& objectName) const
{
    QStringList members;
    
    // 尝试直接获取类成员（如果objectName本身是类）
    for (const auto& pair : m_classMembers.toStdMap()) {
        if (pair.first == objectName) {
            members.append(pair.second);
        }
    }
    
    // 如果没有找到成员（即objectName不是类），尝试获取对象的类型
    if (members.isEmpty()) {
        // 获取对象的类型
        QString className = m_objectTypes.value(objectName);
        
        if (!className.isEmpty()) {
            // 获取该类的所有成员
            for (const auto& pair : m_classMembers.toStdMap()) {
                if (pair.first == className) {
                    members.append(pair.second);
                }
            }
        }
    }
    
    // 如果仍然没有找到成员，检查是否是Python内置类型
    if (members.isEmpty()) {
        // 为常见的Python内置类型添加默认成员
        if (objectName == "str" || objectName.startsWith("\"") || objectName.startsWith("'")) {
            members << "upper" << "lower" << "strip" << "split" << "join" << "replace" << "find";
        } else if (objectName == "list" || objectName.endsWith("]")) {
            members << "append" << "extend" << "insert" << "remove" << "pop" << "clear" << "sort" << "count";
        } else if (objectName == "dict" || objectName.endsWith("}")) {
            members << "keys" << "values" << "items" << "get" << "update" << "pop" << "clear";
        }
    }
    
    // 移除重复项
    members.removeDuplicates();
    
    return members;
}

QString QPythonExtracter::getObjectType(const QString& objectName) const
{
    // 直接从对象-类型映射中获取
    QString type = m_objectTypes.value(objectName);
    
    return type;
}

QMap<QString, QString> QPythonExtracter::getObjectTypesMap() const
{
    return m_objectTypes;
}

void QPythonExtracter::initPythonKeywords()
{
    // Python关键字列表
    m_pythonKeywords << "and" << "as" << "assert" << "async" << "await" << "break"
                    << "class" << "continue" << "def" << "del" << "elif" << "else"
                    << "except" << "False" << "finally" << "for" << "from" << "global"
                    << "if" << "import" << "in" << "is" << "lambda" << "None"
                    << "nonlocal" << "not" << "or" << "pass" << "raise" << "return"
                    << "True" << "try" << "while" << "with" << "yield";
}

void QPythonExtracter::extractClasses(const QString& code, QStringList& symbols)
{
    // 匹配类定义: class ClassName(BaseClass):
    QRegularExpression classRegex(R"(class\s+([A-Za-z_][A-Za-z0-9_]*)\s*(?:\(.*\))?:)");
    QRegularExpressionMatchIterator matches = classRegex.globalMatch(code);
    
    while (matches.hasNext()) {
        QRegularExpressionMatch match = matches.next();
        QString className = match.captured(1);
        
        // 过滤掉私有类（以单下划线或双下划线开头）
        if (!className.startsWith("_")) {
            symbols << className;
            
            // 提取类的位置
            int classPos = match.capturedStart();
            
            // 找到类定义的结束位置
            int endPos = findClassEnd(code, classPos);
            
            if (endPos > classPos) {
                // 提取类的代码块
                QString classCode = code.mid(classPos, endPos - classPos);
                
                // 提取类的方法和属性
                QStringList classMembers;
                extractClassMembers(className, classCode, classMembers);
            }
        }
    }
}

int QPythonExtracter::findClassEnd(const QString& code, int startPos)
{
    // 简单处理：找到类定义后的下一个非缩进行
    int pos = code.indexOf('\n', startPos);
    if (pos == -1) return code.length();
    
    // 获取类定义的缩进级别
    int indentLevel = 0;
    while (pos < code.length() && code.at(pos) == ' ') {
        indentLevel++;
        pos++;
    }
    
    // 从类定义行的下一行开始
    pos = code.indexOf('\n', startPos) + 1;
    
    // 查找下一个同级或更低级别的缩进行
    while (pos < code.length()) {
        int currentIndent = 0;
        int currentPos = pos;
        
        // 计算当前行的缩进
        while (currentPos < code.length() && code.at(currentPos) == ' ') {
            currentIndent++;
            currentPos++;
        }
        
        // 如果是空行或注释行，继续
        if (currentPos >= code.length() || code.at(currentPos) == '\n' || code.at(currentPos) == '#') {
            pos = code.indexOf('\n', pos);
            if (pos == -1) return code.length();
            pos++;
            continue;
        }
        
        // 如果缩进小于等于类定义的缩进，表示类结束
        if (currentIndent <= indentLevel) {
            return pos;
        }
        
        // 移动到下一行
        pos = code.indexOf('\n', pos);
        if (pos == -1) return code.length();
        pos++;
    }
    
    return code.length();
}

void QPythonExtracter::extractClassMembers(const QString& className, const QString& classCode, QStringList& members)
{
    
    // 提取方法
    QRegularExpression methodRegex(R"(def\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(([^)]*)\))");
    QRegularExpressionMatchIterator methodMatches = methodRegex.globalMatch(classCode);
    
    while (methodMatches.hasNext()) {
        QRegularExpressionMatch match = methodMatches.next();
        QString methodName = match.captured(1);
        QString parameters = match.captured(2).trimmed();
        
        // 过滤掉私有方法和构造方法
        if (!methodName.startsWith("_") || methodName == "__init__") {
            members << methodName;
            m_classMembers.insert(className, methodName);
            
            // 保存参数信息到m_functionParameters映射
            m_functionParameters[className + "." + methodName] = parameters;
        }
    }
    
    // 提取初始化方法中的属性 (self.attribute = ...)
    QRegularExpression attrRegex(R"(self\.([A-Za-z_][A-Za-z0-9_]*)\s*=)");
    QRegularExpressionMatchIterator attrMatches = attrRegex.globalMatch(classCode);
    
    while (attrMatches.hasNext()) {
        QRegularExpressionMatch match = attrMatches.next();
        QString attrName = match.captured(1);
        
        // 过滤掉私有属性
        if (!attrName.startsWith("_")) {
            members << attrName;
            m_classMembers.insert(className, attrName);
        }
    }
    
    // 确保__init__方法被添加
    bool hasInit = false;
    for (int i = 0; i < members.size(); ++i) {
        if (members[i] == "__init__") {
            hasInit = true;
            break;
        }
    }
    
    if (!hasInit) {
        members << "__init__";
        m_classMembers.insert(className, "__init__");
    }
    
}

void QPythonExtracter::extractFunctions(const QString& code, QStringList& symbols)
{
    // 匹配函数定义: def function_name(param1, param2):
    QRegularExpression funcRegex(R"(def\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(([^)]*)\))");
    QRegularExpressionMatchIterator matches = funcRegex.globalMatch(code);
    
    while (matches.hasNext()) {
        QRegularExpressionMatch match = matches.next();
        QString funcName = match.captured(1);
        QString parameters = match.captured(2).trimmed();
        
        // 过滤掉私有方法（以单下划线或双下划线开头）
        if (!funcName.startsWith("_")) {
            symbols << funcName;
            
            // 保存全局函数参数
            m_functionParameters[funcName] = parameters;
        }
    }
}

void QPythonExtracter::extractVariables(const QString& code, QStringList& symbols)
{
    // 匹配变量赋值: variable_name = value
    // 这里我们只检测全局变量和类属性，不包括局部变量
    QRegularExpression varRegex(R"(^(?:\s*)([A-Za-z_][A-Za-z0-9_]*)\s*=(?!=))", 
                               QRegularExpression::MultilineOption);
    QRegularExpressionMatchIterator matches = varRegex.globalMatch(code);
    
    while (matches.hasNext()) {
        QRegularExpressionMatch match = matches.next();
        QString varName = match.captured(1);
        
        // 过滤掉Python关键字
        if (!m_pythonKeywords.contains(varName) && !varName.startsWith("_")) {
            symbols << varName;
        }
    }
    
    // 匹配类属性: self.attribute = value
    QRegularExpression attrRegex(R"(self\.([A-Za-z_][A-Za-z0-9_]*)\s*=)");
    matches = attrRegex.globalMatch(code);
    
    while (matches.hasNext()) {
        QRegularExpressionMatch match = matches.next();
        QString attrName = match.captured(1);
        
        if (!attrName.startsWith("_")) {
            symbols << attrName;
        }
    }
}

void QPythonExtracter::extractImports(const QString& code, QStringList& symbols)
{
    // 匹配直接导入: import module
    QRegularExpression importRegex(R"(import\s+([A-Za-z_][A-Za-z0-9_]*))");
    QRegularExpressionMatchIterator matches = importRegex.globalMatch(code);
    
    while (matches.hasNext()) {
        QRegularExpressionMatch match = matches.next();
        symbols << match.captured(1);
    }
    
    // 匹配别名导入: from module import name [as alias]
    QRegularExpression fromImportRegex(R"(from\s+[A-Za-z0-9_.]+\s+import\s+([A-Za-z_][A-Za-z0-9_]*)(?:\s+as\s+([A-Za-z_][A-Za-z0-9_]*))?)");
    matches = fromImportRegex.globalMatch(code);
    
    while (matches.hasNext()) {
        QRegularExpressionMatch match = matches.next();
        // 如果有别名，使用别名
        if (match.lastCapturedIndex() >= 2 && !match.captured(2).isEmpty()) {
            symbols << match.captured(2);
        } else {
            symbols << match.captured(1);
        }
    }
}

void QPythonExtracter::extractObjectTypes(const QString& code)
{
    // 首先清空现有映射
    // m_objectTypes.clear(); // 这个在extractSymbols中已经执行了
    
    // 匹配对象创建: objectName = ClassName(...)
    QRegularExpression objRegex(R"(([A-Za-z_][A-Za-z0-9_]*)\s*=\s*([A-Za-z_][A-Za-z0-9_]*)\s*\()");
    QRegularExpressionMatchIterator matches = objRegex.globalMatch(code);
    
    while (matches.hasNext()) {
        QRegularExpressionMatch match = matches.next();
        QString objName = match.captured(1);
        QString className = match.captured(2);
        
        // 将对象与类型关联起来
        if (!objName.isEmpty() && !className.isEmpty()) {
            m_objectTypes[objName] = className;
        }
    }
    
    // 匹配类内部的self属性: self.attr = value
    QRegularExpression selfAttrRegex(R"(self\.([A-Za-z_][A-Za-z0-9_]*)\s*=)");
    QRegularExpressionMatchIterator selfAttrs = selfAttrRegex.globalMatch(code);
    
    while (selfAttrs.hasNext()) {
        QRegularExpressionMatch match = selfAttrs.next();
        QString attrName = match.captured(1);
        
        // 找到包含这个属性的类
        int pos = match.capturedStart();
        QString className = findClassForPosition(code, pos);
        
        if (!className.isEmpty() && !attrName.isEmpty()) {
            // 添加到类成员中
            m_classMembers.insert(className, attrName);
        }
    }
    
    // 检查所有类，确保__init__方法被添加到类成员中
    QRegularExpression classRegex(R"(class\s+([A-Za-z_][A-Za-z0-9_]*)\s*(?:\(.*\))?:)");
    matches = classRegex.globalMatch(code);
    
    while (matches.hasNext()) {
        QRegularExpressionMatch match = matches.next();
        QString className = match.captured(1);
        
        // 确保__init__方法存在于成员中
        bool hasInit = false;
        for (auto it = m_classMembers.begin(); it != m_classMembers.end(); ++it) {
            if (it.key() == className && it.value() == "__init__") {
                hasInit = true;
                break;
            }
        }
        
        if (!hasInit) {
            m_classMembers.insert(className, "__init__");
        }
    }
}

QString QPythonExtracter::findClassForPosition(const QString& code, int position)
{
    // 获取position之前的代码段
    QString beforeCode = code.left(position);
    
    // 从position向前查找最近的类定义
    QRegularExpression classRegex(R"(class\s+([A-Za-z_][A-Za-z0-9_]*)\s*(?:\(.*\))?:)");
    QRegularExpressionMatchIterator matches = classRegex.globalMatch(beforeCode);
    
    QString lastClassName;
    int lastPos = -1;
    
    while (matches.hasNext()) {
        QRegularExpressionMatch match = matches.next();
        if (match.capturedStart() > lastPos) {
            lastPos = match.capturedStart();
            lastClassName = match.captured(1);
        }
    }
    
    // 检查position是否在该类的范围内
    if (!lastClassName.isEmpty() && lastPos >= 0) {
        int endPos = findClassEnd(code, lastPos);
        if (position < endPos) {
            return lastClassName;
        }
    }
    
    return QString();
}

QList<SymbolInfo> QPythonExtracter::getSymbolsInfo() const
{
    return m_symbolsInfo;
}

SymbolInfo QPythonExtracter::getSymbolInfoAtPosition(int line, int column) const
{
    // 查找位于给定行列位置的符号
    for (const SymbolInfo& info : m_symbolsInfo) {
        if (info.line == line && 
            column >= info.column && 
            column < (info.column + info.length)) {
            return info;
        }
    }
    
    // 没有找到，返回空的SymbolInfo
    return SymbolInfo();
}

void QPythonExtracter::extractSymbolPositions(const QString& code)
{
    // 清空现有的符号位置信息
    m_symbolsInfo.clear();
    
    // 用于跟踪已处理的符号，避免重复添加
    QSet<QString> processedSymbols;
    
    // 1. 添加关键字位置信息
    for (const QString& keyword : m_pythonKeywords) {
        // 关键字可以重复出现多次，不需要去重
        QRegularExpression keywordRegex(QString("\\b(%1)\\b").arg(keyword));
        QRegularExpressionMatchIterator matches = keywordRegex.globalMatch(code);
        
        while (matches.hasNext()) {
            QRegularExpressionMatch match = matches.next();
            int position = match.capturedStart(1);
            int length = match.capturedLength(1);
            
            int line = 0, column = 0;
            calculateLineAndColumn(code, position, line, column);
            
            m_symbolsInfo.append(SymbolInfo(keyword, SymbolInfo::Keyword, line, column, length));
        }
    }
    
    // 2. 添加类位置信息
    QRegularExpression classRegex(R"(class\s+([A-Za-z_][A-Za-z0-9_]*)\s*(?:\(.*\))?:)");
    QRegularExpressionMatchIterator classMatches = classRegex.globalMatch(code);
    
    while (classMatches.hasNext()) {
        QRegularExpressionMatch match = classMatches.next();
        QString className = match.captured(1);
        
        // 类名是唯一的，只添加一次
        if (processedSymbols.contains("c:" + className)) {
            continue;
        }
        processedSymbols.insert("c:" + className);
        
        int position = match.capturedStart(1);
        int length = match.capturedLength(1);
        
        int line = 0, column = 0;
        calculateLineAndColumn(code, position, line, column);
        
        m_symbolsInfo.append(SymbolInfo(className, SymbolInfo::Class, line, column, length));
    }
    
    // 3. 添加函数位置信息
    QRegularExpression funcRegex(R"(def\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(([^)]*)\))");
    QRegularExpressionMatchIterator funcMatches = funcRegex.globalMatch(code);
    
    while (funcMatches.hasNext()) {
        QRegularExpressionMatch match = funcMatches.next();
        QString funcName = match.captured(1);
        QString parameters = match.captured(2).trimmed();
        
        // 确定函数的作用域
        int position = match.capturedStart(1);
        QString scope = findClassForPosition(code, position);
        
        // 作用域前缀，避免不同作用域中同名函数的冲突
        QString scopedName = scope.isEmpty() ? funcName : scope + "." + funcName;
        
        // 检查是否已处理过此函数
        if (processedSymbols.contains("f:" + scopedName)) {
            continue;
        }
        processedSymbols.insert("f:" + scopedName);
        
        int length = match.capturedLength(1);
        
        int line = 0, column = 0;
        calculateLineAndColumn(code, position, line, column);
        
        m_symbolsInfo.append(SymbolInfo(funcName, SymbolInfo::Function, line, column, length, scope, parameters));
    }
    
    // 4. 添加变量位置信息
    QRegularExpression varRegex(R"(^(?:\s*)([A-Za-z_][A-Za-z0-9_]*)\s*=(?!=))", 
                                QRegularExpression::MultilineOption);
    QRegularExpressionMatchIterator varMatches = varRegex.globalMatch(code);
    
    while (varMatches.hasNext()) {
        QRegularExpressionMatch match = varMatches.next();
        QString varName = match.captured(1);
        
        // 过滤掉Python关键字
        if (!m_pythonKeywords.contains(varName)) {
            int position = match.capturedStart(1);
            
            // 确定变量的作用域
            QString scope = findClassForPosition(code, position);
            
            // 作用域前缀，避免不同作用域中同名变量的冲突
            QString scopedName = scope.isEmpty() ? varName : scope + "." + varName;
            
            // 检查是否已处理过此变量
            if (processedSymbols.contains("v:" + scopedName)) {
                continue;
            }
            processedSymbols.insert("v:" + scopedName);
            
            int length = match.capturedLength(1);
            
            int line = 0, column = 0;
            calculateLineAndColumn(code, position, line, column);
            
            m_symbolsInfo.append(SymbolInfo(varName, SymbolInfo::Variable, line, column, length, scope));
        }
    }
    
    // 5. 添加类属性位置信息
    QRegularExpression attrRegex(R"(self\.([A-Za-z_][A-Za-z0-9_]*)\s*=)");
    QRegularExpressionMatchIterator attrMatches = attrRegex.globalMatch(code);
    
    while (attrMatches.hasNext()) {
        QRegularExpressionMatch match = attrMatches.next();
        QString attrName = match.captured(1);
        
        int position = match.capturedStart(1);
        
        // 确定属性的作用域（肯定在某个类中）
        QString scope = findClassForPosition(code, position);
        
        // 作用域前缀
        QString scopedName = scope + "." + attrName;
        
        // 检查是否已处理过此属性
        if (processedSymbols.contains("v:" + scopedName)) {
            continue;
        }
        processedSymbols.insert("v:" + scopedName);
        
        int length = match.capturedLength(1);
        
        int line = 0, column = 0;
        calculateLineAndColumn(code, position, line, column);
        
        m_symbolsInfo.append(SymbolInfo(attrName, SymbolInfo::Variable, line, column, length, scope));
    }
    
    // 6. 添加导入位置信息
    QRegularExpression importRegex(R"(import\s+([A-Za-z_][A-Za-z0-9_]*))");
    QRegularExpressionMatchIterator importMatches = importRegex.globalMatch(code);
    
    while (importMatches.hasNext()) {
        QRegularExpressionMatch match = importMatches.next();
        QString importName = match.captured(1);
        
        // 检查是否已处理过此导入
        if (processedSymbols.contains("i:" + importName)) {
            continue;
        }
        processedSymbols.insert("i:" + importName);
        
        int position = match.capturedStart(1);
        int length = match.capturedLength(1);
        
        int line = 0, column = 0;
        calculateLineAndColumn(code, position, line, column);
        
        m_symbolsInfo.append(SymbolInfo(importName, SymbolInfo::Import, line, column, length));
    }
    
    // 7. 添加from import位置信息
    QRegularExpression fromImportRegex(R"(from\s+[A-Za-z0-9_.]+\s+import\s+([A-Za-z_][A-Za-z0-9_]*)(?:\s+as\s+([A-Za-z_][A-Za-z0-9_]*))?)");
    QRegularExpressionMatchIterator fromImportMatches = fromImportRegex.globalMatch(code);
    
    while (fromImportMatches.hasNext()) {
        QRegularExpressionMatch match = fromImportMatches.next();
        
        // 如果有别名，使用别名
        if (match.lastCapturedIndex() >= 2 && !match.captured(2).isEmpty()) {
            QString importName = match.captured(2);
            
            // 检查是否已处理过此导入
            if (processedSymbols.contains("i:" + importName)) {
                continue;
            }
            processedSymbols.insert("i:" + importName);
            
            int position = match.capturedStart(2);
            int length = match.capturedLength(2);
            
            int line = 0, column = 0;
            calculateLineAndColumn(code, position, line, column);
            
            m_symbolsInfo.append(SymbolInfo(importName, SymbolInfo::Import, line, column, length));
        } else {
            QString importName = match.captured(1);
            
            // 检查是否已处理过此导入
            if (processedSymbols.contains("i:" + importName)) {
                continue;
            }
            processedSymbols.insert("i:" + importName);
            
            int position = match.capturedStart(1);
            int length = match.capturedLength(1);
            
            int line = 0, column = 0;
            calculateLineAndColumn(code, position, line, column);
            
            m_symbolsInfo.append(SymbolInfo(importName, SymbolInfo::Import, line, column, length));
        }
    }
} 