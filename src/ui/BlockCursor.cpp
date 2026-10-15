#include "ui/BlockCursor.h"

#include <QApplication>
#include <QEvent>
#include <QFontMetrics>
#include <QPainter>
#include <QPalette>
#include <QPlainTextEdit>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextFormat>

#include <algorithm>

namespace {

// Rectangle (in viewport coordinates) where the character at point is drawn.
// cursorRect() is used even though the native caret has zero width: its top and
// height are still meaningful, only the width is meaningless.
QRect caretRect(QPlainTextEdit* edit) {
    if (!edit->textCursor().block().isValid())
        return QRect();
    return edit->cursorRect();
}

// Width to use for the block: the advance of the character at point.
int blockWidth(QPlainTextEdit* edit, const QChar& ch, bool hasChar) {
    const QFontMetrics fm(edit->font());
    int w = hasChar ? fm.horizontalAdvance(ch) : 0;
    if (w <= 0)
        w = fm.horizontalAdvance(QLatin1Char(' '));
    if (w <= 0)
        w = fm.averageCharWidth();
    return std::max(1, w);
}

// Colours that the widget paints at the point cell when no cursor is drawn:
// an active region is painted with Highlight/HighlightedText, a full-width line
// highlight (dired's hl-line) with its own background, otherwise plain Base.
void cellColors(QPlainTextEdit* edit, QColor* background, QColor* foreground) {
    const QPalette pal = edit->palette();
    *background = pal.color(QPalette::Base);
    *foreground = pal.color(QPalette::Text);

    const QTextCursor cursor = edit->textCursor();
    const int pos = cursor.position();
    if (cursor.hasSelection() && pos >= cursor.selectionStart()
        && pos < cursor.selectionEnd()) {
        *background = pal.color(QPalette::Highlight);
        *foreground = pal.color(QPalette::HighlightedText);
        return;
    }
    for (const QTextEdit::ExtraSelection& sel : edit->extraSelections()) {
        const QTextCursor& c = sel.cursor;
        if (c.isNull() || pos < c.selectionStart() || pos > c.selectionEnd())
            continue;
        if (sel.format.property(QTextFormat::FullWidthSelection).toBool()
            && sel.format.background().style() != Qt::NoBrush) {
            *background = sel.format.background().color();
            return;
        }
    }
}

} // namespace

BlockCursor::BlockCursor(QPlainTextEdit* edit, QObject* parent)
    : QObject(parent), edit_(edit) {
    edit_->setCursorWidth(0); // hide the native thin caret

    const int flash = QApplication::cursorFlashTime();
    timer_.setInterval(std::max(200, flash / 2));
    connect(&timer_, &QTimer::timeout, this, &BlockCursor::tick);

    edit_->installEventFilter(this);
    edit_->viewport()->installEventFilter(this);

    connect(edit_, &QPlainTextEdit::cursorPositionChanged, this, &BlockCursor::restart);
    connect(edit_, &QPlainTextEdit::selectionChanged, this, &BlockCursor::restart);
    connect(edit_, &QPlainTextEdit::textChanged, this, &BlockCursor::restart);
}

void BlockCursor::setEnabled(bool on) {
    if (enabled_ == on)
        return;
    enabled_ = on;
    if (on)
        restart();
    else
        stop();
}

void BlockCursor::restart() {
    if (!enabled_ || !edit_ || !edit_->hasFocus())
        return;
    on_ = true;
    timer_.start();
    updateViewport();
}

void BlockCursor::stop() {
    timer_.stop();
    on_ = true;
    updateViewport();
}

void BlockCursor::tick() {
    on_ = !on_;
    updateViewport();
}

void BlockCursor::updateViewport() {
    if (edit_ && edit_->viewport())
        edit_->viewport()->update();
}

bool BlockCursor::eventFilter(QObject* obj, QEvent* event) {
    switch (event->type()) {
    case QEvent::FocusIn:
    case QEvent::KeyPress:
    case QEvent::MouseButtonPress:
        restart();
        break;
    case QEvent::FocusOut:
        stop();
        break;
    default:
        break;
    }
    return QObject::eventFilter(obj, event);
}

void BlockCursor::paint(QPainter& painter) {
    if (!enabled_ || !edit_)
        return;

    const QRect r = caretRect(edit_);
    if (r.height() <= 0)
        return;

    const QTextCursor cursor = edit_->textCursor();
    const QString line = cursor.block().text();
    const int col = cursor.positionInBlock();
    const bool hasChar = col < line.size();
    const QChar ch = hasChar ? line.at(col) : QChar(QLatin1Char(' '));

    const int width = blockWidth(edit_, ch, hasChar);
    const QPalette pal = edit_->palette();
    const QFont font = edit_->font();
    const QFontMetrics fm(font);

    // Even with setCursorWidth(0) Qt 6 still paints its own thin caret over the
    // text; it is only hidden while our block is drawn. Erase it by restoring
    // the left edge of the character cell underneath (background + glyph),
    // clipped so the rest of the text keeps Qt's own rendering.
    QColor bg, fg;
    cellColors(edit_, &bg, &fg);
    const QRect strip(r.left(), r.top(), std::min(2, width), r.height());
    painter.save();
    painter.setClipRect(strip);
    painter.fillRect(strip, bg);
    if (hasChar) {
        painter.setPen(fg);
        painter.setFont(font);
        painter.drawText(QPointF(r.left(), r.top() + fm.ascent()), QString(ch));
    }
    painter.restore();

    if (!on_ || !edit_->hasFocus())
        return;

    const QRect block(r.left(), r.top(), width, r.height());
    painter.fillRect(block, pal.color(QPalette::Text));
    if (hasChar) {
        painter.setPen(pal.color(QPalette::Base));
        painter.setFont(font);
        painter.drawText(QPointF(r.left(), r.top() + fm.ascent()), QString(ch));
    }
}
