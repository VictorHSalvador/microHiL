#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[]) {
    QGuiApplication application(argc, argv);
    QQmlApplicationEngine engine;
    const QUrl main_url(QStringLiteral("qrc:/gui/Main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &application, [main_url](QObject *object, const QUrl &url) {
        if (!object && url == main_url) QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(main_url);
    return application.exec();
}
