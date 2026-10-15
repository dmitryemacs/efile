#include "app/MainWindow.h"
#include "ui/Theme.h"

#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("dired-qt"));
    QCoreApplication::setApplicationName(QStringLiteral("file_manager"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    Theme::applyDark(app);

    QString startDir;
    const QStringList args = app.arguments();
    if (args.size() > 1)
        startDir = args.at(1);

    MainWindow window(startDir);
    window.resize(1000, 700);
    window.show();

    return app.exec();
}
