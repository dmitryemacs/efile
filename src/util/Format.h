#pragma once

#include <QString>

class QFileInfo;
class QDateTime;

// Small formatting helpers used to build dired-style listing lines.
class Format {
public:
    static QString permissions(const QFileInfo& fi, bool isSymlink, bool isDir);
    static QString dateTime(const QDateTime& dt);
    static QString owner(const QFileInfo& fi);
    static QString humanSize(qint64 bytes);
};
