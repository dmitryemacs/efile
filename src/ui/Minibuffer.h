#pragma once

#include <QWidget>

#include <functional>

class QLabel;
class QLineEdit;

// Emacs-style echo area / minibuffer: shows messages and reads a line or a
// y-or-n answer for commands.
class Minibuffer : public QWidget {
    Q_OBJECT
public:
    explicit Minibuffer(QWidget* parent = nullptr);

    void message(const QString& text);
    void prompt(const QString& label, std::function<void(const QString&)> onAccept);
    void confirm(const QString& question, std::function<void(bool)> onAnswer);

    bool isBusy() const { return prompting_; }

public slots:
    void cancel();

signals:
    // Emitted when a prompt / confirmation ends (accepted or cancelled).
    void finished();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void finishAnswer(bool value);

    QLabel* label_ = nullptr;
    QLineEdit* input_ = nullptr;

    std::function<void(const QString&)> accept_;
    std::function<void(bool)> answer_;
    bool prompting_ = false;
    bool confirming_ = false;
};
