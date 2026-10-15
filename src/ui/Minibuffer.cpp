#include "ui/Minibuffer.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>

Minibuffer::Minibuffer(QWidget* parent) : QWidget(parent) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(6);

    label_ = new QLabel(this);
    label_->setTextInteractionFlags(Qt::NoTextInteraction);
    label_->setMinimumWidth(80);

    input_ = new QLineEdit(this);
    input_->installEventFilter(this);
    input_->hide();

    layout->addWidget(label_);
    layout->addWidget(input_, 1);

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

void Minibuffer::message(const QString& text) {
    if (prompting_)
        return; // never clobber an active prompt
    label_->setText(text);
}

void Minibuffer::prompt(const QString& label, std::function<void(const QString&)> onAccept) {
    accept_ = std::move(onAccept);
    answer_ = nullptr;
    prompting_ = true;
    confirming_ = false;
    label_->setText(label);
    input_->clear();
    input_->show();
    input_->setFocus();
}

void Minibuffer::confirm(const QString& question, std::function<void(bool)> onAnswer) {
    answer_ = std::move(onAnswer);
    accept_ = nullptr;
    prompting_ = true;
    confirming_ = true;
    label_->setText(question + QStringLiteral(" (y or n) "));
    input_->clear();
    input_->show();
    input_->setFocus();
}

void Minibuffer::cancel() {
    if (!prompting_)
        return;
    prompting_ = false;
    confirming_ = false;
    accept_ = nullptr;
    answer_ = nullptr;
    input_->clear();
    input_->hide();
    label_->setText(QStringLiteral("Quit"));
    emit finished();
}

void Minibuffer::finishAnswer(bool value) {
    auto callback = answer_;
    answer_ = nullptr;
    accept_ = nullptr;
    prompting_ = false;
    confirming_ = false;
    input_->clear();
    input_->hide();
    emit finished();
    if (callback)
        callback(value);
}

bool Minibuffer::eventFilter(QObject* obj, QEvent* event) {
    if (obj != input_ || event->type() != QEvent::KeyPress)
        return QWidget::eventFilter(obj, event);

    auto* keyEvent = static_cast<QKeyEvent*>(event);
    const int k = keyEvent->key();
    const bool ctrl = keyEvent->modifiers().testFlag(Qt::ControlModifier);

    if (confirming_) {
        if (k == Qt::Key_Y) {
            finishAnswer(true);
        } else if (k == Qt::Key_N || k == Qt::Key_Backspace || k == Qt::Key_Delete) {
            finishAnswer(false);
        } else if (k == Qt::Key_Return || k == Qt::Key_Enter) {
            finishAnswer(true);
        } else if (k == Qt::Key_Escape || (ctrl && k == Qt::Key_G)) {
            cancel();
        }
        return true; // swallow everything while confirming
    }

    if (prompting_) {
        if (k == Qt::Key_Escape || (ctrl && k == Qt::Key_G)) {
            cancel();
            return true;
        }
        if (k == Qt::Key_Return || k == Qt::Key_Enter) {
            const QString text = input_->text();
            auto callback = accept_;
            accept_ = nullptr;
            prompting_ = false;
            input_->clear();
            input_->hide();
            emit finished();
            if (callback)
                callback(text);
            return true;
        }
    }

    return QWidget::eventFilter(obj, event);
}
