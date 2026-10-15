#pragma once

#include <QHash>
#include <QObject>
#include <QString>

#include <functional>

class EditorBuffer;
class Minibuffer;
class QWidget;

// Implements the Emacs editing commands dispatched by the editor key map.
class EditorCommands : public QObject {
    Q_OBJECT
public:
    EditorCommands(EditorBuffer* buffer, Minibuffer* minibuffer, QWidget* window,
                   QObject* parent = nullptr);

    void run(const QString& command);

    // Called by MainWindow when the editor becomes visible.
    void setCloseCallback(std::function<void()> callback) { onClose_ = std::move(callback); }
    void refreshStatus();

private slots:
    void updateStatus();

private:
    void bind(const QString& name, std::function<void()> fn);

    void saveBuffer();
    void writeFile();
    void closeBuffer();
    void searchForward();

    EditorBuffer* buffer_;
    Minibuffer* minibuffer_;
    QWidget* window_;
    std::function<void()> onClose_;
    QHash<QString, std::function<void()>> table_;
};
