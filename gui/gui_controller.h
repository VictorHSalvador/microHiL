#ifndef GUI_CONTROLLER_H
#define GUI_CONTROLLER_H

#include <QObject>
extern "C" {
#include "execution_session.h"
}

class GuiController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString fmuPath READ FmuPath NOTIFY FmuChanged)
    Q_PROPERTY(QString modelName READ ModelName NOTIFY FmuChanged)
    Q_PROPERTY(int inputCount READ InputCount NOTIFY FmuChanged)
    Q_PROPERTY(int outputCount READ OutputCount NOTIFY FmuChanged)
    Q_PROPERTY(QString errorMessage READ ErrorMessage NOTIFY ErrorChanged)
public:
    explicit GuiController(QObject *parent = nullptr);
    ~GuiController() override;
    QString FmuPath() const;
    QString ModelName() const;
    int InputCount() const;
    int OutputCount() const;
    QString ErrorMessage() const;
    Q_INVOKABLE bool LoadFmu(const QString &path);
signals:
    void FmuChanged();
    void ErrorChanged();
private:
    execution_session_t *session_ = nullptr;
    QString error_message_;
};

#endif
