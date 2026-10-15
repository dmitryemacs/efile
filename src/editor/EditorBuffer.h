#pragma once

#include <QPlainTextEdit>
#include <QTextCursor>

class BlockCursor;

// Editable text buffer used by the embedded Emacs-like editor.
class EditorBuffer : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit EditorBuffer(QWidget* parent = nullptr);

    bool loadFile(const QString& path, QString* error = nullptr);
    bool saveFile(QString* error = nullptr);
    bool saveAs(const QString& path, QString* error = nullptr);

    QString filePath() const { return filePath_; }
    QString fileName() const;

    bool markActive() const { return markActive_; }
    void setMark();
    void deactivateMark();

    // Move the point, extending the region when the mark is active.
    void movePoint(QTextCursor::MoveOperation op, bool honorMark = true);
    void scroll(int lines);

    void newline();
    void openLine();
    void deleteChar();
    void deleteBackwardChar();
    void killLine();
    void killRegion();
    void copyRegion();
    void yank();

    int visibleLineCount() const;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QString filePath_;
    bool markActive_ = false;
    BlockCursor* cursor_ = nullptr;
};
