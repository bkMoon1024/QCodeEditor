// QCodeEditor
#include <QLineNumberArea>
#include <QSyntaxStyle>
#include <QCodeEditor>
#include <QStyleSyntaxHighlighter>
#include <QFramedTextAttribute>
#include <QCXXHighlighter>
#include <QSymbolExtracter>


// Qt
#include <QTextBlock>
#include <QPaintEvent>
#include <QFontDatabase>
#include <QScrollBar>
#include <QAbstractTextDocumentLayout>
#include <QTextCharFormat>
#include <QCursor>
#include <QCompleter>
#include <QAbstractItemView>
#include <QShortcut>
#include <QMimeData>
#include <QMouseEvent>

static QVector<QPair<QString, QString>> parentheses = {
    {"(", ")"},
    {"{", "}"},
    {"[", "]"},
    {"\"", "\""},
    {"'", "'"}
};

QCodeEditor::QCodeEditor(QWidget* widget) :
    QTextEdit(widget),
    m_highlighter(nullptr),
    m_syntaxStyle(nullptr),
    m_lineNumberArea(new QLineNumberArea(this)),
    m_completer(nullptr),
    m_framedAttribute(new QFramedTextAttribute(this)),
    m_autoIndentation(true),
    m_autoParentheses(true),
    m_replaceTab(true),
    m_tabReplace(QString(4, ' ')),
    m_symbolExtracter(nullptr),
    m_symbolExtractionEnabled(false),
    m_symbolLinkEnabled(true),
    m_ctrlPressed(false),
    m_symbolLinkValid(false)
{
    initDocumentLayoutHandlers();
    initFont();
    performConnections();

    setSyntaxStyle(QSyntaxStyle::defaultStyle());
    
    // 设置鼠标跟踪，以便能够处理鼠标移动事件
    setMouseTracking(true);
}

void QCodeEditor::initDocumentLayoutHandlers()
{
    document()
        ->documentLayout()
        ->registerHandler(
            QFramedTextAttribute::type(),
            m_framedAttribute
        );
}

void QCodeEditor::initFont()
{
    auto fnt = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    fnt.setFixedPitch(true);
    fnt.setPointSize(10);

    setFont(fnt);
}

void QCodeEditor::performConnections()
{
    connect(
        document(),
        &QTextDocument::blockCountChanged,
        this,
        &QCodeEditor::updateLineNumberAreaWidth
    );

    connect(
        verticalScrollBar(),
        &QScrollBar::valueChanged,
        [this](int){ m_lineNumberArea->update(); }
    );

    connect(
        this,
        &QTextEdit::cursorPositionChanged,
        this,
        &QCodeEditor::updateExtraSelection
    );

    connect(
        this,
        &QTextEdit::selectionChanged,
        this,
        &QCodeEditor::onSelectionChanged
    );

    connect(
        document(),
        &QTextDocument::contentsChanged,
        this,
        &QCodeEditor::updateSymbols
    );
}

void QCodeEditor::setHighlighter(QStyleSyntaxHighlighter* highlighter)
{
    if (m_highlighter)
    {
        m_highlighter->setDocument(nullptr);
    }

    m_highlighter = highlighter;

    if (m_highlighter)
    {
        m_highlighter->setSyntaxStyle(m_syntaxStyle);
        m_highlighter->setDocument(document());
    }
}

void QCodeEditor::setSyntaxStyle(QSyntaxStyle* style)
{
    m_syntaxStyle = style;

    m_framedAttribute->setSyntaxStyle(m_syntaxStyle);
    m_lineNumberArea->setSyntaxStyle(m_syntaxStyle);

    if (m_highlighter)
    {
        m_highlighter->setSyntaxStyle(m_syntaxStyle);
    }

    updateStyle();
}

void QCodeEditor::updateStyle()
{
    if (m_highlighter)
    {
        m_highlighter->rehighlight();
    }

    if (m_syntaxStyle)
    {
        auto currentPalette = palette();

        // Setting text format/color
        currentPalette.setColor(
            QPalette::ColorRole::Text,
            m_syntaxStyle->getFormat("Text").foreground().color()
        );

        // Setting common background
        currentPalette.setColor(
            QPalette::Base,
            m_syntaxStyle->getFormat("Text").background().color()
        );

        // Setting selection color
        currentPalette.setColor(
            QPalette::Highlight,
            m_syntaxStyle->getFormat("Selection").background().color()
        );

        setPalette(currentPalette);
    }

    updateExtraSelection();
}

void QCodeEditor::onSelectionChanged()
{
    auto selected = textCursor().selectedText();

    auto cursor = textCursor();

    // Cursor is null if setPlainText was called.
    if (cursor.isNull())
    {
        return;
    }

    cursor.movePosition(QTextCursor::MoveOperation::Left);
    cursor.select(QTextCursor::SelectionType::WordUnderCursor);

    QSignalBlocker blocker(this);
    m_framedAttribute->clear(cursor);

    if (selected.size() > 1 &&
        cursor.selectedText() == selected)
    {
        auto backup = textCursor();

        // Perform search selecting
        handleSelectionQuery(cursor);

        setTextCursor(backup);
    }
}

void QCodeEditor::resizeEvent(QResizeEvent* e)
{
    QTextEdit::resizeEvent(e);

    updateLineGeometry();
}

void QCodeEditor::updateLineGeometry()
{
    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(
        QRect(cr.left(),
              cr.top(),
              m_lineNumberArea->sizeHint().width(),
              cr.height()
        )
    );
}

void QCodeEditor::updateLineNumberAreaWidth(int)
{
    setViewportMargins(m_lineNumberArea->sizeHint().width(), 0, 0, 0);
}

void QCodeEditor::updateLineNumberArea(const QRect& rect)
{
    m_lineNumberArea->update(
        0,
        rect.y(),
        m_lineNumberArea->sizeHint().width(),
        rect.height()
    );
    updateLineGeometry();

    if (rect.contains(viewport()->rect()))
    {
        updateLineNumberAreaWidth(0);
    }
}

void QCodeEditor::handleSelectionQuery(QTextCursor cursor)
{

    auto searchIterator = cursor;
    searchIterator.movePosition(QTextCursor::Start);
    searchIterator = document()->find(cursor.selectedText(), searchIterator);
    while (searchIterator.hasSelection())
    {
        m_framedAttribute->frame(searchIterator);

        searchIterator = document()->find(cursor.selectedText(), searchIterator);
    }
}

void QCodeEditor::updateExtraSelection()
{
    QList<QTextEdit::ExtraSelection> extra;

    highlightCurrentLine(extra);
    highlightParenthesis(extra);
    
    if (m_symbolLinkEnabled && m_ctrlPressed && m_symbolLinkValid) {
        highlightSymbolLink(extra);
    }

    setExtraSelections(extra);
}

void QCodeEditor::highlightParenthesis(QList<QTextEdit::ExtraSelection>& extraSelection)
{
    auto currentSymbol = charUnderCursor();
    auto prevSymbol = charUnderCursor(-1);

    for (auto& pair : parentheses)
    {
        int direction;

        QChar counterSymbol;
        QChar activeSymbol;
        auto position = textCursor().position();

        if (pair.first == currentSymbol)
        {
            direction = 1;
            counterSymbol = pair.second[0];
            activeSymbol = currentSymbol;
        }
        else if (pair.second == prevSymbol)
        {
            direction = -1;
            counterSymbol = pair.first[0];
            activeSymbol = prevSymbol;
            position--;
        }
        else
        {
            continue;
        }

        auto counter = 1;

        while (counter != 0 &&
               position > 0 &&
               position < (document()->characterCount() - 1))
        {
            // Moving position
            position += direction;

            auto character = document()->characterAt(position);
            // Checking symbol under position
            if (character == activeSymbol)
            {
                ++counter;
            }
            else if (character == counterSymbol)
            {
                --counter;
            }
        }

        auto format = m_syntaxStyle->getFormat("Parentheses");

        // Found
        if (counter == 0)
        {
            ExtraSelection selection{};

            auto directionEnum =
                 direction < 0 ?
                 QTextCursor::MoveOperation::Left
                 :
                 QTextCursor::MoveOperation::Right;

            selection.format = format;
            selection.cursor = textCursor();
            selection.cursor.clearSelection();
            selection.cursor.movePosition(
                directionEnum,
                QTextCursor::MoveMode::MoveAnchor,
                std::abs(textCursor().position() - position)
            );

            selection.cursor.movePosition(
                QTextCursor::MoveOperation::Right,
                QTextCursor::MoveMode::KeepAnchor,
                1
            );

            extraSelection.append(selection);

            selection.cursor = textCursor();
            selection.cursor.clearSelection();
            selection.cursor.movePosition(
                directionEnum,
                QTextCursor::MoveMode::KeepAnchor,
                1
            );

            extraSelection.append(selection);
        }

        break;
    }
}

void QCodeEditor::highlightCurrentLine(QList<QTextEdit::ExtraSelection>& extraSelection)
{
    if (!isReadOnly())
    {
        QTextEdit::ExtraSelection selection{};

        selection.format = m_syntaxStyle->getFormat("CurrentLine");
        selection.format.setForeground(QBrush());
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();

        extraSelection.append(selection);
    }
}

void QCodeEditor::paintEvent(QPaintEvent* e)
{
    updateLineNumberArea(e->rect());
    QTextEdit::paintEvent(e);
}

int QCodeEditor::getFirstVisibleBlock()
{
    // Detect the first block for which bounding rect - once translated
    // in absolute coordinated - is contained by the editor's text area

    // Costly way of doing but since "blockBoundingGeometry(...)" doesn't
    // exists for "QTextEdit"...

    QTextCursor curs = QTextCursor(document());
    curs.movePosition(QTextCursor::Start);
    for(int i=0; i < document()->blockCount(); ++i)
    {
        QTextBlock block = curs.block();

        QRect r1 = viewport()->geometry();
        QRect r2 = document()
            ->documentLayout()
            ->blockBoundingRect(block)
            .translated(
                viewport()->geometry().x(),
                viewport()->geometry().y() - verticalScrollBar()->sliderPosition()
            ).toRect();

        if (r1.intersects(r2))
        {
            return i;
        }

        curs.movePosition(QTextCursor::NextBlock);
    }

    return 0;
}

bool QCodeEditor::proceedCompleterBegin(QKeyEvent *e)
{
    if (m_completer &&
        m_completer->popup()->isVisible())
    {
        switch (e->key())
        {
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_Escape:
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            e->ignore();
            return true; // let the completer do default behavior
        default:
            break;
        }
    }

    // todo: Replace with modifiable QShortcut
    auto isShortcut = ((e->modifiers() & Qt::ControlModifier) && e->key() == Qt::Key_Space);

    return !(!m_completer || !isShortcut);

}

void QCodeEditor::proceedCompleterEnd(QKeyEvent *e)
{
    auto ctrlOrShift = e->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier);

    if (!m_completer ||
        (ctrlOrShift && e->text().isEmpty()) ||
        e->key() == Qt::Key_Delete)
    {
        return;
    }

    static QString eow(R"(~!@#$%^&*()_+{}|:"<>?,/;'[]\-=)");
    auto isShortcut = ((e->modifiers() & Qt::ControlModifier) && e->key() == Qt::Key_Space);
    auto completionPrefix = wordUnderCursor();
    
    // 特殊处理点号情况
    bool isDotInput = (e->text() == ".");
    
    if (!isShortcut &&
        !isDotInput &&  // 如果是点号输入，不要隐藏补全
        (e->text().isEmpty() ||
         completionPrefix.length() < 1 ||
         eow.contains(e->text().right(1))))
    {
        m_completer->popup()->hide();
        return;
    }
    
    // 如果是点号，尝试找到对象表达式
    if (isDotInput)
    {
        // 获取当前光标
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::Left);
        
        // 获取光标前面的内容，尝试查找完整表达式
        int blockPos = cursor.positionInBlock();
        QString lineText = cursor.block().text().left(blockPos);
        
        // 找到最后一个分隔符位置
        int lastWordStart = lineText.lastIndexOf(QRegExp("\\b"));
        if (lastWordStart >= 0)
        {
            // 提取对象名称
            QString objectName = lineText.mid(lastWordStart).trimmed();
            
            if (!objectName.isEmpty())
            {
                // 将对象名称+点号设置为补全前缀
                completionPrefix = objectName + ".";
                // 设置补全前缀
                m_completer->setCompletionPrefix(completionPrefix);
                // 立即显示补全
                auto cursRect = cursorRect();
                cursRect.setWidth(
                    m_completer->popup()->sizeHintForColumn(0) +
                    m_completer->popup()->verticalScrollBar()->sizeHint().width()
                );
                m_completer->complete(cursRect);
                return;
            }
        }
    }

    if (completionPrefix != m_completer->completionPrefix())
    {
        m_completer->setCompletionPrefix(completionPrefix);
        m_completer->popup()->setCurrentIndex(m_completer->completionModel()->index(0, 0));
    }

    auto cursRect = cursorRect();
    cursRect.setWidth(
        m_completer->popup()->sizeHintForColumn(0) +
        m_completer->popup()->verticalScrollBar()->sizeHint().width()
    );

    m_completer->complete(cursRect);
}

void QCodeEditor::keyPressEvent(QKeyEvent* e) {
    // Ctrl键按下处理
    if (e->key() == Qt::Key_Control) {
        m_ctrlPressed = true;
        
        // 如果鼠标在编辑器中，更新符号链接光标
        QPoint mousePos = viewport()->mapFromGlobal(QCursor::pos());
        if (viewport()->rect().contains(mousePos)) {
            updateSymbolLinkCursor(mousePos);
        }
    }

#if QT_VERSION >= 0x050A00
  const int defaultIndent = tabStopDistance() / fontMetrics().averageCharWidth();
#else
  const int defaultIndent = tabStopWidth() / fontMetrics().averageCharWidth();
#endif

  auto completerSkip = proceedCompleterBegin(e);

  if (!completerSkip) {
    // 特殊处理点号输入
    if (e->text() == "." && m_completer) {
      // 先让事件正常处理，插入点号
      QTextEdit::keyPressEvent(e);
      
      // 然后触发补全
      QTextCursor cursor = textCursor();
      cursor.movePosition(QTextCursor::Left);
      
      // 获取光标左侧的表达式
      int blockPos = cursor.positionInBlock();
      QString lineText = cursor.block().text().left(blockPos);
      
      // 尝试找到完整表达式
      QRegExp wordExp("\\b[A-Za-z0-9_]+$");
      int wordPos = wordExp.indexIn(lineText);
      
      if (wordPos >= 0) {
        QString objectName = wordExp.cap(0);
        
        if (!objectName.isEmpty()) {
          
          // 强制显示补全
          if (m_completer) {
            m_completer->setCompletionPrefix(objectName + ".");
            auto cursRect = cursorRect();
            cursRect.setWidth(
                m_completer->popup()->sizeHintForColumn(0) +
                m_completer->popup()->verticalScrollBar()->sizeHint().width()
            );
            m_completer->complete(cursRect);
          }
        }
      }
      
      // 已处理，直接返回
      return;
    }
    
    if (m_replaceTab && e->key() == Qt::Key_Tab &&
        e->modifiers() == Qt::NoModifier) {
      insertPlainText(m_tabReplace);
      return;
    }

    // Auto indentation
    int indentationLevel = getIndentationSpaces();

#if QT_VERSION >= 0x050A00
    int tabCounts =
        indentationLevel * fontMetrics().averageCharWidth() / tabStopDistance();
#else
    int tabCounts =
        indentationLevel * fontMetrics().averageCharWidth() / tabStopWidth();
#endif

    // Have Qt Edior like behaviour, if {|} and enter is pressed indent the two
    // parenthesis
    if (m_autoIndentation && 
       (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) &&
        charUnderCursor() == '}' && charUnderCursor(-1) == '{') 
    {
      int charsBack = 0;
      insertPlainText("\n");

      if (m_replaceTab)
        insertPlainText(QString(indentationLevel + defaultIndent, ' '));
      else
        insertPlainText(QString(tabCounts + 1, '\t'));

      insertPlainText("\n");
      charsBack++;

      if (m_replaceTab) 
      {
        insertPlainText(QString(indentationLevel, ' '));
        charsBack += indentationLevel;
      }
      else 
      {
        insertPlainText(QString(tabCounts, '\t'));
        charsBack += tabCounts;
      }

      while (charsBack--)
        moveCursor(QTextCursor::MoveOperation::Left);
      return;
    }

    // Shortcut for moving line to left
    if (m_replaceTab && e->key() == Qt::Key_Backtab) {
      indentationLevel = std::min(indentationLevel, m_tabReplace.size());

      auto cursor = textCursor();

      cursor.movePosition(QTextCursor::MoveOperation::StartOfLine);
      cursor.movePosition(QTextCursor::MoveOperation::Right,
                          QTextCursor::MoveMode::KeepAnchor, indentationLevel);

      cursor.removeSelectedText();
      return;
    }

    QTextEdit::keyPressEvent(e);

    if (m_autoIndentation && (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter)) {
      if (m_replaceTab)
        insertPlainText(QString(indentationLevel, ' '));
      else
        insertPlainText(QString(tabCounts, '\t'));
    }

    if (m_autoParentheses) 
    {
      for (auto&& el : parentheses) 
      {
                // Inserting closed brace
                if (el.first == e->text()) 
                {
                  insertPlainText(el.second);
                  moveCursor(QTextCursor::MoveOperation::Left);
                  break;
                }

                // If it's close brace - check parentheses
                if (el.second == e->text())
                {
                    auto symbol = charUnderCursor();

                    if (symbol == el.second)
                    {
                        textCursor().deletePreviousChar();
                        moveCursor(QTextCursor::MoveOperation::Right);
                    }

                    break;
                }
            }
        }
    }

    proceedCompleterEnd(e);
}

void QCodeEditor::setAutoIndentation(bool enabled)
{
    m_autoIndentation = enabled;
}

bool QCodeEditor::autoIndentation() const
{
    return m_autoIndentation;
}

void QCodeEditor::setAutoParentheses(bool enabled)
{
    m_autoParentheses = enabled;
}

bool QCodeEditor::autoParentheses() const
{
    return m_autoParentheses;
}

void QCodeEditor::setTabReplace(bool enabled)
{
    m_replaceTab = enabled;
}

bool QCodeEditor::tabReplace() const
{
    return m_replaceTab;
}

void QCodeEditor::setTabReplaceSize(int val)
{
    m_tabReplace.clear();

    m_tabReplace.fill(' ', val);
}

int QCodeEditor::tabReplaceSize() const
{
    return m_tabReplace.size();
}

void QCodeEditor::setCompleter(QCompleter *completer)
{
    if (m_completer)
    {
        disconnect(m_completer, nullptr, this, nullptr);
    }

    m_completer = completer;

    if (!m_completer)
    {
        return;
    }

    m_completer->setWidget(this);
    m_completer->setCompletionMode(QCompleter::CompletionMode::PopupCompletion);

    connect(
        m_completer,
        QOverload<const QString&>::of(&QCompleter::activated),
        this,
        &QCodeEditor::insertCompletion
    );
}

void QCodeEditor::focusInEvent(QFocusEvent *e)
{
    if (m_completer)
    {
        m_completer->setWidget(this);
    }

    QTextEdit::focusInEvent(e);
}

void QCodeEditor::insertCompletion(QString s)
{
    if (m_completer->widget() != this)
    {
        return;
    }

    auto tc = textCursor();
    tc.select(QTextCursor::SelectionType::WordUnderCursor);
    tc.insertText(s);
    setTextCursor(tc);
}

QCompleter *QCodeEditor::completer() const
{
    return m_completer;
}

void QCodeEditor::setSymbolExtracter(QSymbolExtracter* extracter)
{
    if (m_symbolExtracter == extracter)
    {
        return;
    }
    
    m_symbolExtracter = extracter;
    m_symbolExtractionEnabled = (m_symbolExtracter != nullptr);
    
    // 如果有符号提取器，立即更新符号
    if (m_symbolExtractionEnabled)
    {
        updateSymbols();
    }
}

QSymbolExtracter* QCodeEditor::symbolExtracter() const
{
    return m_symbolExtracter;
}

void QCodeEditor::updateSymbols()
{
    // 如果没有启用符号提取，则直接返回
    if (!m_symbolExtractionEnabled || !m_symbolExtracter)
    {
        return;
    }
    
    // 获取当前文档文本并进行符号提取
    QString text = toPlainText();
    m_symbolExtracter->extractSymbols(text);
}

QChar QCodeEditor::charUnderCursor(int offset) const
{
    auto block = textCursor().blockNumber();
    auto index = textCursor().positionInBlock();
    auto text = document()->findBlockByNumber(block).text();

    index += offset;

    if (index < 0 || index >= text.size())
    {
        return {};
    }

    return text[index];
}

QString QCodeEditor::wordUnderCursor() const
{
    auto tc = textCursor();
    tc.select(QTextCursor::WordUnderCursor);
    return tc.selectedText();
}

void QCodeEditor::insertFromMimeData(const QMimeData* source)
{
    insertPlainText(source->text());
}

int QCodeEditor::getIndentationSpaces()
{
    auto blockText = textCursor().block().text();

    int indentationLevel = 0;

    for (auto i = 0;
         i < blockText.size() && QString("\t ").contains(blockText[i]);
         ++i)
    {
        if (blockText[i] == ' ')
        {
            indentationLevel++;
        }
        else
        {
#if QT_VERSION >= 0x050A00
            indentationLevel += tabStopDistance() / fontMetrics().averageCharWidth();
#else
            indentationLevel += tabStopWidth() / fontMetrics().averageCharWidth();
#endif
        }
    }

    return indentationLevel;
}

void QCodeEditor::setSymbolLinkEnabled(bool enabled)
{
    m_symbolLinkEnabled = enabled;
}

bool QCodeEditor::symbolLinkEnabled() const
{
    return m_symbolLinkEnabled;
}

void QCodeEditor::mouseMoveEvent(QMouseEvent* e)
{
    if (m_symbolLinkEnabled && m_ctrlPressed) {
        updateSymbolLinkCursor(e->pos());
    } else if (m_symbolLinkValid) {
        // 恢复正常光标
        m_symbolLinkValid = false;
        viewport()->setCursor(Qt::IBeamCursor);
        updateExtraSelection();
    }
    
    QTextEdit::mouseMoveEvent(e);
}

void QCodeEditor::mousePressEvent(QMouseEvent* e)
{
    if (m_symbolLinkEnabled && m_ctrlPressed && m_symbolLinkValid && e->button() == Qt::LeftButton) {
        // 跳转到符号定义位置
        setTextCursor(m_symbolLinkCursor);
        return;
    }
    
    QTextEdit::mousePressEvent(e);
}

void QCodeEditor::keyReleaseEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Control) {
        m_ctrlPressed = false;
        
        if (m_symbolLinkValid) {
            // 恢复正常光标
            m_symbolLinkValid = false;
            viewport()->setCursor(Qt::IBeamCursor);
            updateExtraSelection();
        }
    }
    
    QTextEdit::keyReleaseEvent(e);
}

QString QCodeEditor::wordUnderMouse(const QPoint& pos) const
{
    QTextCursor cursor = cursorForPosition(pos);
    cursor.select(QTextCursor::WordUnderCursor);
    return cursor.selectedText();
}

void QCodeEditor::updateSymbolLinkCursor(const QPoint& pos)
{
    if (!m_symbolLinkEnabled || !m_symbolExtractionEnabled || !m_symbolExtracter) {
        return;
    }
    
    bool found = getSymbolInfoUnderMouse(pos);
    
    if (found) {
        if (!m_symbolLinkValid) {
            m_symbolLinkValid = true;
            viewport()->setCursor(Qt::PointingHandCursor);
            updateExtraSelection();
        }
    } else if (m_symbolLinkValid) {
        m_symbolLinkValid = false;
        viewport()->setCursor(Qt::IBeamCursor);
        updateExtraSelection();
    }
}

bool QCodeEditor::getSymbolInfoUnderMouse(const QPoint& pos)
{
    if (!m_symbolExtracter) {
        return false;
    }
    
    // 获取鼠标位置的光标
    QTextCursor cursor = cursorForPosition(pos);
    
    // 保存当前位置的行列号（用于调试）
    int line = cursor.blockNumber() + 1; // 行号从1开始
    int column = cursor.positionInBlock() + 1; // 列号从1开始
    
    // 获取光标下的单词
    cursor.select(QTextCursor::WordUnderCursor);
    QString word = cursor.selectedText();
    
    // 如果没有选中任何文本，直接返回
    if (word.isEmpty()) {
        return false;
    }
    
    // 获取符号列表
    const QList<SymbolInfo>& symbolsInfo = m_symbolExtracter->getSymbolsInfo();
    
    // 在符号列表中查找匹配的符号
    for (const auto& symbol : symbolsInfo) {
        if (symbol.name == word) {
            // 找到匹配符号，创建指向符号定义的光标
            QTextBlock block = document()->findBlockByLineNumber(symbol.line - 1); // 行号从1开始
            if (block.isValid()) {
                // 保存符号定义的光标位置（用于跳转）
                m_symbolLinkCursor = QTextCursor(document());
                m_symbolLinkCursor.setPosition(block.position() + symbol.column - 1);
                m_symbolLinkCursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, symbol.length);
                
                // 用当前位置下的单词作为高亮显示
                cursor = cursorForPosition(pos);
                cursor.select(QTextCursor::WordUnderCursor);
                
                // 高亮当前单词
                QList<QTextEdit::ExtraSelection> extraSelections;
                QTextEdit::ExtraSelection selection;
                
                QTextCharFormat linkFormat;
                linkFormat.setFontUnderline(true);
                linkFormat.setUnderlineStyle(QTextCharFormat::SingleUnderline);
                linkFormat.setForeground(QColor(0, 0, 255)); // 蓝色
                
                selection.format = linkFormat;
                selection.cursor = cursor;
                
                extraSelections.append(selection);
                setExtraSelections(extraSelections);
                
                return true;
            }
        }
    }
    
    return false;
}

void QCodeEditor::highlightSymbolLink(QList<QTextEdit::ExtraSelection>& extraSelection)
{
    if (!m_symbolLinkValid) {
        return;
    }
    
    QTextEdit::ExtraSelection selection;
    
    QTextCharFormat format;
    format.setFontUnderline(true);
    
    selection.format = format;
    selection.cursor = m_symbolLinkCursor;
    
    extraSelection.append(selection);
}
