#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>

class QKeyEvent;

// Small Emacs-like key dispatcher. Multi-key sequences ("C-x C-f") and prefixes
// are supported through a trie. Install it as an event filter on the target widget.
class KeyMap : public QObject {
    Q_OBJECT
public:
    explicit KeyMap(QObject* parent = nullptr);
    ~KeyMap() override;

    // "sequence" is space separated, e.g. "C-x C-f", "M-<", "RET", "d".
    void bind(const QString& sequence, const QString& command);
    void reset();

    // When true, an unbound single key is passed to the widget (for text input).
    void setPassThroughUndefined(bool on) { passThroughUndefined_ = on; }

signals:
    void commandTriggered(const QString& command);
    void prefixChanged(const QString& keys); // empty when no prefix is pending
    void undefinedKey(const QString& key);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    struct Node {
        QString command;
        QHash<QString, Node*> children;
    };

    static QString describe(const QKeyEvent* event);
    static void freeNode(Node* node);

    Node root_;
    QStringList pending_;
    bool passThroughUndefined_ = false;
};
