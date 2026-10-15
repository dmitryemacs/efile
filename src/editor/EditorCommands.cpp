#include "editor/EditorCommands.h"

#include "editor/EditorBuffer.h"
#include "ui/Minibuffer.h"

#include <QDir>
#include <QFileInfo>
#include <QTextCursor>
#include <QTextDocument>
#include <QWidget>

#include <utility>

using M = QTextCursor::MoveOperation;

EditorCommands::EditorCommands(EditorBuffer* buffer, Minibuffer* minibuffer, QWidget* window,
                               QObject* parent)
    : QObject(parent), buffer_(buffer), minibuffer_(minibuffer), window_(window) {
    bind(QStringLiteral("forward-char"), [this] { buffer_->movePoint(M::Right); });
    bind(QStringLiteral("backward-char"), [this] { buffer_->movePoint(M::Left); });
    bind(QStringLiteral("next-line"), [this] { buffer_->movePoint(M::Down); });
    bind(QStringLiteral("previous-line"), [this] { buffer_->movePoint(M::Up); });
    bind(QStringLiteral("move-beginning-of-line"),
         [this] { buffer_->movePoint(M::StartOfLine); });
    bind(QStringLiteral("move-end-of-line"), [this] { buffer_->movePoint(M::EndOfLine); });
    bind(QStringLiteral("beginning-of-buffer"), [this] { buffer_->movePoint(M::Start); });
    bind(QStringLiteral("end-of-buffer"), [this] { buffer_->movePoint(M::End); });
    bind(QStringLiteral("scroll-up"), [this] { buffer_->scroll(buffer_->visibleLineCount()); });
    bind(QStringLiteral("scroll-down"), [this] { buffer_->scroll(-buffer_->visibleLineCount()); });
    bind(QStringLiteral("recenter"), [this] { buffer_->centerCursor(); });

    bind(QStringLiteral("newline"), [this] { buffer_->newline(); });
    bind(QStringLiteral("open-line"), [this] { buffer_->openLine(); });
    bind(QStringLiteral("delete-char"), [this] { buffer_->deleteChar(); });
    bind(QStringLiteral("delete-backward-char"), [this] { buffer_->deleteBackwardChar(); });

    bind(QStringLiteral("set-mark-command"), [this] {
        buffer_->setMark();
        refreshStatus();
        minibuffer_->message(QStringLiteral("Mark set"));
    });
    bind(QStringLiteral("keyboard-quit"), [this] {
        buffer_->deactivateMark();
        updateStatus();
    });
    bind(QStringLiteral("kill-line"), [this] { buffer_->killLine(); });
    bind(QStringLiteral("kill-region"), [this] { buffer_->killRegion(); });
    bind(QStringLiteral("kill-ring-save"), [this] { buffer_->copyRegion(); });
    bind(QStringLiteral("yank"), [this] { buffer_->yank(); });
    bind(QStringLiteral("undo"), [this] { buffer_->undo(); });

    bind(QStringLiteral("search-forward"), [this] { searchForward(); });
    bind(QStringLiteral("save-buffer"), [this] { saveBuffer(); });
    bind(QStringLiteral("write-file"), [this] { writeFile(); });
    bind(QStringLiteral("kill-buffer"), [this] { closeBuffer(); });
    bind(QStringLiteral("save-buffers-kill-emacs"), [this] { closeBuffer(); });

    connect(buffer_, &QPlainTextEdit::cursorPositionChanged, this,
            &EditorCommands::updateStatus);
    connect(buffer_->document(), &QTextDocument::modificationChanged, this,
            &EditorCommands::updateStatus);
    connect(buffer_->document(), &QTextDocument::contentsChanged, this,
            &EditorCommands::updateStatus);
}

void EditorCommands::bind(const QString& name, std::function<void()> fn) {
    table_.insert(name, std::move(fn));
}

void EditorCommands::run(const QString& command) {
    const auto it = table_.find(command);
    if (it != table_.end())
        (*it)();
    else
        minibuffer_->message(QStringLiteral("Undefined command: ") + command);
}

void EditorCommands::refreshStatus() { updateStatus(); }

void EditorCommands::updateStatus() {
    if (minibuffer_->isBusy())
        return;
    const QTextCursor c = buffer_->textCursor();
    const QString mod = buffer_->document()->isModified() ? QStringLiteral(" **") : QString();
    minibuffer_->message(QStringLiteral("emacs: %1%2  (%3,%4)   [C-x C-s save, C-x k close]")
                             .arg(buffer_->fileName(), mod)
                             .arg(c.blockNumber() + 1)
                             .arg(c.columnNumber() + 1));
}

void EditorCommands::saveBuffer() {
    QString error;
    if (buffer_->saveFile(&error)) {
        window_->setWindowTitle(QStringLiteral("emacs: ") +
                                QDir::toNativeSeparators(buffer_->filePath()));
        minibuffer_->message(QStringLiteral("Wrote %1").arg(buffer_->filePath()));
    } else {
        minibuffer_->message(QStringLiteral("Save failed: ") + error);
    }
}

void EditorCommands::writeFile() {
    minibuffer_->prompt(QStringLiteral("Write file: "), [this](const QString& text) {
        if (text.isEmpty()) {
            updateStatus();
            return;
        }
        QString path = text;
        if (path.startsWith(QLatin1Char('~')))
            path = QDir::homePath() + path.mid(1);
        QString error;
        if (buffer_->saveAs(path, &error)) {
            window_->setWindowTitle(QStringLiteral("emacs: ") +
                                    QDir::toNativeSeparators(buffer_->filePath()));
            minibuffer_->message(QStringLiteral("Wrote %1").arg(buffer_->filePath()));
        } else {
            minibuffer_->message(QStringLiteral("Write failed: ") + error);
        }
    });
}

void EditorCommands::closeBuffer() {
    if (!buffer_->document()->isModified()) {
        if (onClose_)
            onClose_();
        return;
    }
    minibuffer_->confirm(QStringLiteral("Save file %1?").arg(buffer_->fileName()),
                         [this](bool yes) {
                             if (yes) {
                                 QString error;
                                 if (!buffer_->saveFile(&error)) {
                                     minibuffer_->message(QStringLiteral("Save failed: ") +
                                                          error);
                                     return;
                                 }
                             }
                             if (onClose_)
                                 onClose_();
                         });
}

void EditorCommands::searchForward() {
    minibuffer_->prompt(QStringLiteral("Search: "), [this](const QString& text) {
        if (text.isEmpty()) {
            updateStatus();
            return;
        }
        QTextDocument* doc = buffer_->document();
        QTextCursor found = doc->find(text, buffer_->textCursor());
        if (found.isNull()) {
            QTextCursor from(doc);
            from.movePosition(QTextCursor::Start);
            found = doc->find(text, from);
        }
        if (found.isNull()) {
            minibuffer_->message(QStringLiteral("Search failed: %1").arg(text));
            return;
        }
        buffer_->setTextCursor(found);
        minibuffer_->message(QStringLiteral("Found: %1").arg(text));
    });
}
