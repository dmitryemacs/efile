#include "util/FsOps.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

bool FsOps::removeRecursively(const QString& path) {
    const QFileInfo fi(path);
    if (fi.isDir() && !fi.isSymLink())
        return QDir(path).removeRecursively();
    if (!fi.exists() && !fi.isSymLink())
        return false;
    return QFile::remove(path);
}

bool FsOps::copyRecursively(const QString& srcPath, const QString& dstPath) {
    const QFileInfo srcInfo(srcPath);

    if (srcInfo.isDir() && !srcInfo.isSymLink()) {
        if (!QDir().mkpath(dstPath))
            return false;
        const QFileInfoList children =
            QDir(srcPath).entryInfoList(QDir::AllEntries | QDir::Hidden | QDir::System |
                                            QDir::NoDotAndDotDot,
                                        QDir::NoSort);
        for (const QFileInfo& child : children) {
            const QString dst = QDir(dstPath).filePath(child.fileName());
            if (!copyRecursively(child.absoluteFilePath(), dst))
                return false;
        }
        return true;
    }

    const QString parentDir = QFileInfo(dstPath).absolutePath();
    if (!QDir().mkpath(parentDir))
        return false;
    if (QFileInfo::exists(dstPath))
        QFile::remove(dstPath);
    return QFile::copy(srcPath, dstPath);
}

bool FsOps::moveTo(const QString& srcPath, const QString& dstPath) {
    if (QFile::rename(srcPath, dstPath))
        return true;
    // Cross-device or existing target: fall back to copy + delete.
    if (copyRecursively(srcPath, dstPath))
        return removeRecursively(srcPath);
    return false;
}

QStringList FsOps::splitCommand(const QString& command) {
    QStringList out;
    QString cur;
    bool inQuotes = false;
    bool hasToken = false;

    for (const QChar c : command) {
        if (c == u'"') {
            inQuotes = !inQuotes;
            hasToken = true;
            continue;
        }
        if (c.isSpace() && !inQuotes) {
            if (hasToken || !cur.isEmpty()) {
                out << cur;
                cur.clear();
                hasToken = false;
            }
            continue;
        }
        cur += c;
        hasToken = true;
    }
    if (hasToken || !cur.isEmpty())
        out << cur;
    return out;
}
