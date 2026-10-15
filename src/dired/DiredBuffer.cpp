#include "dired/DiredBuffer.h"

#include "dired/DiredHighlighter.h"
#include "dired/DirModel.h"
#include "ui/BlockCursor.h"

#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextFormat>

DiredBuffer::DiredBuffer(QWidget* parent) : QPlainTextEdit(parent) {
    setReadOnly(true);
    setUndoRedoEnabled(false);
    setLineWrapMode(QPlainTextEdit::NoWrap);
    setContextMenuPolicy(Qt::NoContextMenu);
    setAcceptDrops(false);
    setFocusPolicy(Qt::StrongFocus);
    setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);

    QFont f(QStringLiteral("Consolas"));
    f.setStyleHint(QFont::Monospace);
    f.setFixedPitch(true);
    f.setPointSize(10);
    setFont(f);

    highlighter_ = new DiredHighlighter(document());

    // Read-only text edits do not paint the caret, so we draw our own block cursor.
    cursor_ = new BlockCursor(this);

    connect(this, &QPlainTextEdit::cursorPositionChanged, this,
            &DiredBuffer::updatePointHighlight);
    connect(this, &QPlainTextEdit::selectionChanged, this, &DiredBuffer::updatePointHighlight);
}

void DiredBuffer::updatePointHighlight() {
    // Full-width highlight of the line under the point (dired's hl-line feel).
    if (!textCursor().block().isValid()) {
        setExtraSelections({});
        return;
    }
    QTextEdit::ExtraSelection selection;
    selection.cursor = textCursor();
    selection.cursor.clearSelection();
    selection.format.setBackground(QColor(40, 52, 70));
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    setExtraSelections({selection});

    viewport()->update();
}

void DiredBuffer::paintEvent(QPaintEvent* event) {
    QPlainTextEdit::paintEvent(event);

    // Draw the point as a blinking block cursor on top of the listing.
    QPainter painter(viewport());
    cursor_->paint(painter);
}

void DiredBuffer::setModel(dired::DirModel* model) {
    model_ = model;
    highlighter_->setModel(model);
}

void DiredBuffer::refreshText(bool keepPosition) {
    if (!model_)
        return;

    QString nameAtPoint;
    int column = 0;
    if (keepPosition) {
        nameAtPoint = fileNameAt(currentLine());
        column = textCursor().positionInBlock();
    }

    setPlainText(model_->plainText());

    if (keepPosition) {
        const int idx = model_->indexOfName(nameAtPoint);
        if (idx >= 0) {
            gotoLine(idx);
            moveToColumn(column);
        } else {
            gotoLine(0);
        }
    } else {
        gotoLine(0);
    }
    highlighter_->rehighlight();
}

int DiredBuffer::currentLine() const { return textCursor().blockNumber(); }

void DiredBuffer::gotoLine(int line) {
    if (line < 0)
        return;
    const QTextBlock block = document()->findBlockByNumber(line);
    if (!block.isValid())
        return;
    QTextCursor c(textCursor());
    c.setPosition(block.position());
    setTextCursor(c);
    ensureCursorVisible();
}

void DiredBuffer::moveLine(int delta) {
    QTextCursor c = textCursor();
    const int line = c.blockNumber();
    const int column = c.positionInBlock();
    const int target = qBound(0, line + delta, qMax(0, blockCount() - 1));
    const QTextBlock block = document()->findBlockByNumber(target);
    if (!block.isValid())
        return;
    c.setPosition(block.position() + qMin(column, block.text().length()));
    setTextCursor(c);
    ensureCursorVisible();
}

void DiredBuffer::moveToColumn(int column) {
    QTextCursor c = textCursor();
    const QTextBlock block = c.block();
    c.setPosition(block.position() + qMin(column, block.text().length()));
    setTextCursor(c);
}

void DiredBuffer::updateMark(int line) {
    if (!model_ || line < 0 || line >= model_->count())
        return;
    const QTextBlock block = document()->findBlockByNumber(line);
    if (!block.isValid() || block.text().isEmpty())
        return;

    const QChar mark = model_->markChar(line);
    if (block.text().at(0) == mark)
        return;

    QTextCursor c(block);
    c.movePosition(QTextCursor::StartOfBlock);
    c.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
    c.insertText(QString(mark));
}

QString DiredBuffer::fileNameAt(int line) const {
    if (!model_ || line < 0 || line >= model_->count())
        return QString();
    return model_->at(line).name;
}

int DiredBuffer::visibleLineCount() const {
    const int lineHeight = qMax(1, fontMetrics().lineSpacing());
    return qMax(1, viewport()->height() / lineHeight);
}
