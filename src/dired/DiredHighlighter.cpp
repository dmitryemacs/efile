#include "dired/DiredHighlighter.h"

#include "dired/DirModel.h"

#include <QColor>
#include <QTextCharFormat>

DiredHighlighter::DiredHighlighter(QTextDocument* parent) : QSyntaxHighlighter(parent) {}

void DiredHighlighter::setModel(dired::DirModel* model) {
    model_ = model;
    rehighlight();
}

void DiredHighlighter::highlightBlock(const QString& text) {
    if (!model_)
        return;
    const int line = currentBlock().blockNumber();
    if (line < 0 || line >= model_->count())
        return;

    const dired::DirEntry& e = model_->at(line);

    QColor c;
    if (model_->isFlagged(e.name))
        c = QColor(255, 90, 90); // marked for deletion
    else if (model_->isMarked(e.name))
        c = QColor(255, 200, 80); // marked
    else if (e.isDotEntry())
        c = QColor(140, 140, 140); // "." and ".."
    else if (e.isSymlink)
        c = QColor(200, 140, 255); // symbolic link
    else if (e.isDir)
        c = QColor(110, 180, 255); // directory
    else if (e.isExecutable)
        c = QColor(120, 210, 120); // executable file
    else
        return; // regular file: default color

    QTextCharFormat fmt;
    fmt.setForeground(c);
    setFormat(0, text.length(), fmt);
}
