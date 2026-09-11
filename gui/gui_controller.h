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
    Q_INVOKABLE bool SaveProfile(const QString &path, double stepSizeSeconds, double stopTimeSeconds, int adcResolutionBits,
                                 const QVariantList &adcAttenuations, const QVariantList &pwmConfigurations, const QVariantList &mappings);
    Q_INVOKABLE QVariantList Inputs() const;
    Q_INVOKABLE QVariantList Outputs() const;
    Q_INVOKABLE QVariantList DaqcChannels(int numericType, bool inputDirection) const;
    Q_INVOKABLE bool SetOutputSelected(int index, bool selected);
    Q_INVOKABLE bool SetVirtualInput(int index, const QString &value);
    Q_INVOKABLE bool StartSimulation(double stepSizeSeconds, double stopTimeSeconds, bool loggingEnabled, bool plotEnabled);
    Q_INVOKABLE bool StopSimulation();
    Q_INVOKABLE QVariantList PollSamples();
    Q_INVOKABLE bool SimulationRunning() const;
    Q_INVOKABLE QVariantMap Result() const;
    Q_INVOKABLE bool ExportCsv(const QString &path);
signals:
    void FmuChanged();
    void ProfileChanged();
    void ErrorChanged();
private:
    execution_session_t *session_ = nullptr;
    QString error_message_;
    bool gui_run_started_ = false;
    static QString NumericTypeName(int type);
    static QString QuoteYaml(const QString &value);
};

#endif
