#include <QCoreApplication>
#include <QDir>
#include <QString>

#include <cstdio>

#include "gui_controller.h"

static int Require(bool condition, const char *message) {
    if (condition) return 0;
    std::fprintf(stderr, "%s\n", message);
    return 1;
}

static QString FileUrl(const char *path) {
    return QStringLiteral("file://") + QDir::cleanPath(QString::fromLocal8Bit(path));
}

int main(int argc, char **argv) {
    QCoreApplication application(argc, argv);
    if (argc != 3) return Require(false, "expected FMU and YAML paths");

    GuiController controller;
    if (Require(controller.LoadFmu(FileUrl(argv[1])), "could not load FMU through a local file URL") != 0) return 1;
    if (Require(controller.LoadProfile(FileUrl(argv[2])), "could not load YAML through a local file URL") != 0) return 1;
    return Require(controller.ProfilePath() == QDir::cleanPath(QString::fromLocal8Bit(argv[2])), "profile path was not normalized to a local path");
}
