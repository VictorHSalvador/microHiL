#include <QDir>
#include <QGuiApplication>
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
    QGuiApplication application(argc, argv);
    if (argc != 4) return Require(false, "expected FMU, compatible YAML and incompatible YAML paths");

    GuiController controller;
    if (Require(controller.LoadFmu(FileUrl(argv[1])), "could not load FMU through a local file URL") != 0) return 1;
    if (Require(controller.LoadProfile(FileUrl(argv[2])), "could not load YAML through a local file URL") != 0) return 1;
    if (Require(controller.ProfilePath() == QDir::cleanPath(QString::fromLocal8Bit(argv[2])), "profile path was not normalized to a local path") != 0) return 1;
    if (Require(!controller.LoadProfile(FileUrl(argv[3])), "accepted a YAML profile with incompatible FMU mappings") != 0) return 1;
    return Require(controller.ErrorMessage().contains(QStringLiteral("FMU mapping")), "did not expose the incompatible FMU mapping diagnostic");
}
