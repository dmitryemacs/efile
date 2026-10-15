#include "util/Format.h"

#include <QDateTime>
#include <QFileInfo>
#include <QLocale>

QString Format::permissions(const QFileInfo& fi, bool isSymlink, bool isDir) {
    QString p;
    p += isSymlink ? QChar('l') : (isDir ? QChar('d') : QChar('-'));

    const QFile::Permissions perm = fi.permissions();
    auto bit = [&perm](QFile::Permission f, QChar ch) -> QChar {
        return perm.testFlag(f) ? ch : QChar('-');
    };

    p += bit(QFile::ReadOwner, QChar('r'));
    p += bit(QFile::WriteOwner, QChar('w'));
    p += bit(QFile::ExeOwner, QChar('x'));
    p += bit(QFile::ReadGroup, QChar('r'));
    p += bit(QFile::WriteGroup, QChar('w'));
    p += bit(QFile::ExeGroup, QChar('x'));
    p += bit(QFile::ReadOther, QChar('r'));
    p += bit(QFile::WriteOther, QChar('w'));
    p += bit(QFile::ExeOther, QChar('x'));
    return p;
}

QString Format::dateTime(const QDateTime& dt) {
    if (!dt.isValid())
        return QStringLiteral("-");
    return QLocale::c().toString(dt, QStringLiteral("MMM d HH:mm"));
}

QString Format::owner(const QFileInfo& fi) {
    const QString o = fi.owner();
    return o.isEmpty() ? QStringLiteral("-") : o;
}

QString Format::humanSize(qint64 bytes) {
    static const char* units[] = {"B", "K", "M", "G", "T"};
    double v = static_cast<double>(bytes);
    int i = 0;
    while (v >= 1024.0 && i < 4) {
        v /= 1024.0;
        ++i;
    }
    if (i == 0)
        return QString::number(bytes) + QStringLiteral("B");
    return QString::number(v, 'f', 1) + QString::fromLatin1(units[i]);
}
