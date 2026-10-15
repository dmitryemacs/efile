#include "dired/KeyMap.h"

#include <QKeyEvent>

KeyMap::KeyMap(QObject* parent) : QObject(parent) {}

KeyMap::~KeyMap() { freeNode(&root_); }

void KeyMap::freeNode(Node* node) {
    for (Node* child : node->children)
        freeNode(child);
    node->children.clear();
}

void KeyMap::bind(const QString& sequence, const QString& command) {
    const QStringList parts = sequence.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (parts.isEmpty())
        return;
    Node* node = &root_;
    for (const QString& part : parts) {
        auto it = node->children.find(part);
        if (it == node->children.end()) {
            Node* child = new Node;
            node->children.insert(part, child);
            node = child;
        } else {
            node = it.value();
        }
    }
    node->command = command;
}

void KeyMap::reset() {
    if (pending_.isEmpty())
        return;
    pending_.clear();
    emit prefixChanged(QString());
}

QString KeyMap::describe(const QKeyEvent* event) {
    const int k = event->key();
    const Qt::KeyboardModifiers m = event->modifiers();
    const bool ctrl = m.testFlag(Qt::ControlModifier);
    const bool alt = m.testFlag(Qt::AltModifier);
    const bool shift = m.testFlag(Qt::ShiftModifier);

    QString base;

    if (k >= Qt::Key_A && k <= Qt::Key_Z) {
        QChar ch(static_cast<char16_t>('a' + (k - Qt::Key_A)));
        if (shift && !ctrl && !alt)
            ch = ch.toUpper();
        base = QString(ch);
    } else if (k >= Qt::Key_0 && k <= Qt::Key_9) {
        base = QString(QChar(static_cast<char16_t>('0' + (k - Qt::Key_0))));
    } else {
        switch (k) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
            base = QStringLiteral("RET");
            break;
        case Qt::Key_Backspace:
            base = QStringLiteral("DEL");
            break;
        case Qt::Key_Space:
            base = QStringLiteral("SPC");
            break;
        case Qt::Key_Tab:
            base = QStringLiteral("TAB");
            break;
        case Qt::Key_Escape:
            base = QStringLiteral("ESC");
            break;
        case Qt::Key_Less:
            base = QStringLiteral("<");
            break;
        case Qt::Key_Greater:
            base = QStringLiteral(">");
            break;
        case Qt::Key_Comma:
            base = QStringLiteral(",");
            break;
        case Qt::Key_Period:
            base = QStringLiteral(".");
            break;
        case Qt::Key_Slash:
            base = QStringLiteral("/");
            break;
        case Qt::Key_AsciiCircum:
            base = QStringLiteral("^");
            break;
        case Qt::Key_Plus:
            base = QStringLiteral("+");
            break;
        case Qt::Key_Question:
            base = QStringLiteral("?");
            break;
        case Qt::Key_Semicolon:
            base = QStringLiteral(";");
            break;
        case Qt::Key_Percent:
            base = QStringLiteral("%");
            break;
        case Qt::Key_Asterisk:
            base = QStringLiteral("*");
            break;
        case Qt::Key_Up:
            base = QStringLiteral("<up>");
            break;
        case Qt::Key_Down:
            base = QStringLiteral("<down>");
            break;
        case Qt::Key_Left:
            base = QStringLiteral("<left>");
            break;
        case Qt::Key_Right:
            base = QStringLiteral("<right>");
            break;
        case Qt::Key_Home:
            base = QStringLiteral("<home>");
            break;
        case Qt::Key_End:
            base = QStringLiteral("<end>");
            break;
        case Qt::Key_PageUp:
            base = QStringLiteral("<prior>");
            break;
        case Qt::Key_PageDown:
            base = QStringLiteral("<next>");
            break;
        default:
            base = event->text();
            if (base.isEmpty())
                base = QStringLiteral("key_") + QString::number(k);
            break;
        }
    }

    QString out;
    if (ctrl)
        out += QStringLiteral("C-");
    if (alt)
        out += QStringLiteral("M-");
    out += base;
    return out;
}

bool KeyMap::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() != QEvent::KeyPress)
        return QObject::eventFilter(obj, event);

    auto* keyEvent = static_cast<QKeyEvent*>(event);
    const int k = keyEvent->key();
    if (k == Qt::Key_Control || k == Qt::Key_Alt || k == Qt::Key_Shift ||
        k == Qt::Key_Meta || k == Qt::Key_CapsLock || k == Qt::Key_NumLock)
        return true;

    const QString key = describe(keyEvent);
    if (key.isEmpty())
        return false;

    // C-g cancels a pending prefix.
    if (!pending_.isEmpty() && key == QLatin1String("C-g")) {
        reset();
        return true;
    }

    const bool wasPending = !pending_.isEmpty();
    pending_.append(key);

    Node* node = &root_;
    for (const QString& part : pending_) {
        auto it = node->children.find(part);
        if (it == node->children.end()) {
            const QString full = pending_.join(QLatin1Char(' '));
            pending_.clear();
            emit prefixChanged(QString());
            if (passThroughUndefined_ && !wasPending)
                return false; // let the widget handle it (e.g. insert text)
            emit undefinedKey(full);
            return true;
        }
        node = it.value();
    }

    if (!node->command.isEmpty()) {
        const QString command = node->command;
        pending_.clear();
        emit prefixChanged(QString());
        emit commandTriggered(command);
        return true;
    }

    if (!node->children.isEmpty()) {
        emit prefixChanged(pending_.join(QLatin1Char(' ')));
        return true;
    }

    const QString full = pending_.join(QLatin1Char(' '));
    pending_.clear();
    emit prefixChanged(QString());
    if (passThroughUndefined_ && !wasPending)
        return false;
    emit undefinedKey(full);
    return true;
}
