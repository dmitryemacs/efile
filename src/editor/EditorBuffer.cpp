#include "editor/EditorBuffer.h"

#include "ui/BlockCursor.h"

#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QSaveFile>
#include <QTextBlock>
#include <QTextDocument>

namespace {
// QTextCursor uses U+2029 for paragraph separators; turn them into real newlines.
QString toPlain(const QString& cursorText) {
    QString s = cursorText;
    s.replace(QChar(0x2029), QLatin1Char('\n'));
    return s;
}
} // namespace

EditorBuffer::EditorBuffer(QWidget* parent) : QPlainTextEdit(parent) {
    setLineWrapMode(QPlainTextEdit::NoWrap);
    setUndoRedoEnabled(true);
    setTabChangesFocus(false);

    QFont f(QStringLiteral("Consolas"));
    f.setStyleHint(QFont::Monospace);
    f.setFixedPitch(true);
    f.setPointSize(10);
    setFont(f);

    cursor_ = new BlockCursor(this);

    // Any edit deactivates the mark (transient mark behaviour).
    connect(document(), &QTextDocument::contentsChanged, this,
            [this] { markActive_ = false; });
}

QString EditorBuffer::fileName() const {
    return filePath_.isEmpty() ? QStringLiteral("(new)") : QFileInfo(filePath_).fileName();
}

bool EditorBuffer::loadFile(const QString& path, QString* error) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (error)
            *error = f.errorString();
        return false;
    }
    const QByteArray data = f.readAll();
    if (data.contains('\0')) {
        if (error)
            *error = QStringLiteral("Binary file is not supported");
        return false;
    }

    QString text = QString::fromUtf8(data);
    text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));

    filePath_ = QFileInfo(path).absoluteFilePath();
    setPlainText(text);
    document()->setModified(false);
    markActive_ = false;

    QTextCursor c = textCursor();
    c.movePosition(QTextCursor::Start);
    setTextCursor(c);
    return true;
}

bool EditorBuffer::saveFile(QString* error) {
    if (filePath_.isEmpty()) {
        if (error)
            *error = QStringLiteral("No file name");
        return false;
    }
    return saveAs(filePath_, error);
}

bool EditorBuffer::saveAs(const QString& path, QString* error) {
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        if (error)
            *error = f.errorString();
        return false;
    }
    f.write(toPlainText().toUtf8());
    if (!f.commit()) {
        if (error)
            *error = f.errorString();
        return false;
    }
    filePath_ = QFileInfo(path).absoluteFilePath();
    document()->setModified(false);
    return true;
}

void EditorBuffer::setMark() {
    markActive_ = true;
    QTextCursor c = textCursor();
    c.clearSelection();
    setTextCursor(c);
}

void EditorBuffer::deactivateMark() {
    markActive_ = false;
    QTextCursor c = textCursor();
    c.clearSelection();
    setTextCursor(c);
}

void EditorBuffer::movePoint(QTextCursor::MoveOperation op, bool honorMark) {
    QTextCursor c = textCursor();
    c.movePosition(op, (honorMark && markActive_) ? QTextCursor::KeepAnchor
                                                  : QTextCursor::MoveAnchor);
    setTextCursor(c);
    ensureCursorVisible();
}

void EditorBuffer::scroll(int lines) {
    QTextCursor c = textCursor();
    c.movePosition(lines >= 0 ? QTextCursor::Down : QTextCursor::Up, QTextCursor::MoveAnchor,
                   qAbs(lines));
    setTextCursor(c);
    ensureCursorVisible();
    centerCursor();
}

void EditorBuffer::newline() { textCursor().insertText(QStringLiteral("\n")); }

void EditorBuffer::openLine() {
    QTextCursor c = textCursor();
    const int pos = c.position();
    c.insertText(QStringLiteral("\n"));
    QTextCursor restore = textCursor();
    restore.setPosition(pos);
    setTextCursor(restore);
}

void EditorBuffer::deleteChar() { textCursor().deleteChar(); }

void EditorBuffer::deleteBackwardChar() { textCursor().deletePreviousChar(); }

void EditorBuffer::killLine() {
    QTextCursor c = textCursor();
    c.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
    if (!c.hasSelection())
        c.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
    QApplication::clipboard()->setText(toPlain(c.selectedText()));
    c.removeSelectedText();
}

void EditorBuffer::killRegion() {
    QTextCursor c = textCursor();
    if (!c.hasSelection())
        return;
    QApplication::clipboard()->setText(toPlain(c.selectedText()));
    c.removeSelectedText();
}

void EditorBuffer::copyRegion() {
    QTextCursor c = textCursor();
    if (!c.hasSelection())
        return;
    QApplication::clipboard()->setText(toPlain(c.selectedText()));
}

void EditorBuffer::yank() {
    const QString text = QApplication::clipboard()->text();
    if (!text.isEmpty())
        textCursor().insertText(text);
}

int EditorBuffer::visibleLineCount() const {
    const int lineHeight = qMax(1, fontMetrics().lineSpacing());
    return qMax(1, viewport()->height() / lineHeight);
}

void EditorBuffer::paintEvent(QPaintEvent* event) {
    QPlainTextEdit::paintEvent(event);

    // Emacs-style blinking block cursor on top of the text.
    QPainter painter(viewport());
    cursor_->paint(painter);
}
