#ifndef GUI_CONTROLLER_H
#define GUI_CONTROLLER_H

#include <QObject>
#include <QVariantList>
extern "C" {
#include "execution_session.h"
}

class GuiController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString fmuPath READ FmuPath NOTIFY FmuChanged)
    Q_PROPERTY(QString modelName READ ModelName NOTIFY FmuChanged)
    Q_PROPERTY(int inputCount READ InputCount NOTIFY FmuChanged)
    Q_PROPERTY(int outputCount READ OutputCount NOTIFY FmuChanged)
    Q_PROPERTY(QString profilePath READ ProfilePath NOTIFY ProfileChanged)
    Q_PROPERTY(int profileId READ ProfileId NOTIFY ProfileChanged)
    Q_PROPERTY(int profileMappingCount READ ProfileMappingCount NOTIFY ProfileChanged)
    Q_PROPERTY(QString errorMessage READ ErrorMessage NOTIFY ErrorChanged)
public:
    explicit GuiController(QObject *parent = nullptr);
    ~GuiController() override;
    QString FmuPath() const;
    QString ModelName() const;
    int InputCount() const;
    int OutputCount() const;
    QString ProfilePath() const;
    int ProfileId() const;
    int ProfileMappingCount() const;
    QString ErrorMessage() const;
    Q_INVOKABLE bool LoadFmu(const QString &path);
    Q_INVOKABLE bool LoadProfile(const QString &path);
    Q_INVOKABLE QVariantList Outputs() const;
    Q_INVOKABLE bool SetOutputSelected(int index, bool selected);
signals:
    void FmuChanged();
    void ProfileChanged();
    void ErrorChanged();
private:
    execution_session_t *session_ = nullptr;
    QString error_message_;
    static QString NumericTypeName(int type);
};

#endif
