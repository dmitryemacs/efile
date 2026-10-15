#pragma once

#include <QObject>
#include <QTimer>

class QPainter;
class QPlainTextEdit;

// Emacs-style block cursor: a filled rectangle over the character at point,
// which blinks. Qt's own caret is hidden with setCursorWidth(0), but Qt 6 still
// paints a thin 1-2px caret on top of the text; paint() therefore also erases
// that caret by restoring the character cell underneath, so during the
// blink-off phase nothing (not even a stick) remains at the point.
class BlockCursor : public QObject {
    Q_OBJECT
public:
    explicit BlockCursor(QPlainTextEdit* edit, QObject* parent = nullptr);

    // Turn drawing on/off (e.g. off while the widget is not showing the point).
    void setEnabled(bool on);
    bool isEnabled() const { return enabled_; }

    // Draw the block cursor. Call after the widget painted its own contents.
    void paint(QPainter& painter);

    // Show the cursor solid and restart the blink cycle (used on activity).
    void restart();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void tick();
    void updateViewport();
    void stop();
    QPlainTextEdit* edit() const { return edit_; }

    QPlainTextEdit* edit_ = nullptr;
    QTimer timer_;
    bool on_ = true;
    bool enabled_ = true;
};
