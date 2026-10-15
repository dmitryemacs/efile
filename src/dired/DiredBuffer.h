#pragma once

#include <QPlainTextEdit>

namespace dired {
class DirModel;
}
class DiredHighlighter;

// Read-only text view that renders one directory entry per line ("dired buffer").
class DiredBuffer : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit DiredBuffer(QWidget* parent = nullptr);

    void setModel(dired::DirModel* model);
    dired::DirModel* model() const { return model_; }

    // Full re-render of the listing. Optionally keeps the point on the same file.
    void refreshText(bool keepPosition = true);

    int currentLine() const;
    void gotoLine(int line);
    void moveLine(int delta);
    void moveToColumn(int column);

    // Redraw the two-character mark column of a single line.
    void updateMark(int line);

    QString fileNameAt(int line) const;
    int visibleLineCount() const;

public slots:
    // Repaint the point indicator (current-line highlight + caret).
    void updatePointHighlight();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    dired::DirModel* model_ = nullptr;
    DiredHighlighter* highlighter_ = nullptr;
};
