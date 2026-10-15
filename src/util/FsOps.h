#pragma once

#include <QString>
#include <QStringList>

class FsOps {
public:
    static bool removeRecursively(const QString& path);
    static bool copyRecursively(const QString& srcPath, const QString& dstPath);
    static bool moveTo(const QString& srcPath, const QString& dstPath);

    // Split a command line into argv respecting double quotes.
    static QStringList splitCommand(const QString& command);
};
