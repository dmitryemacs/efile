#pragma once

#include <QSyntaxHighlighter>

namespace dired {
class DirModel;
}

// Applies dired "faces" (colors) to listing lines.
class DiredHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    explicit DiredHighlighter(QTextDocument* parent = nullptr);

    void setModel(dired::DirModel* model);

protected:
    void highlightBlock(const QString& text) override;

private:
    dired::DirModel* model_ = nullptr;
};
