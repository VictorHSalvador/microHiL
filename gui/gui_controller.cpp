#include "gui_controller.h"

#include <QFile>
#include <QSaveFile>
#include <QTemporaryFile>
#include <QTextStream>

GuiController::GuiController(QObject *parent) : QObject(parent), session_(ExecutionSessionCreate()) {}
GuiController::~GuiController() { ExecutionSessionDelete(session_); }
QString GuiController::FmuPath() const { return QString::fromUtf8(ExecutionSessionFmuPath(session_)); }
QString GuiController::ModelName() const { return QString::fromUtf8(ExecutionSessionModelName(session_)); }
int GuiController::InputCount() const { return static_cast<int>(ExecutionSessionInputCount(session_)); }
int GuiController::OutputCount() const { return static_cast<int>(ExecutionSessionListOutputs(session_, nullptr, 0U)); }
QString GuiController::ProfilePath() const { return QString::fromUtf8(ExecutionSessionProfilePath(session_)); }
int GuiController::ProfileId() const { return static_cast<int>(ExecutionSessionProfileId(session_)); }
int GuiController::ProfileMappingCount() const { return static_cast<int>(ExecutionSessionProfileMappingCount(session_)); }
QString GuiController::ErrorMessage() const { return error_message_; }
bool GuiController::LoadFmu(const QString &path) {
    const QByteArray native_path = path.toLocal8Bit();
    if (!session_) {
        error_message_ = QStringLiteral("could not create the execution session");
        emit ErrorChanged();
        return false;
    }
    const execution_session_status_t status = ExecutionSessionLoadFmu(session_, native_path.constData());
    error_message_ = status == EXECUTION_SESSION_OK ? QString() : QString::fromUtf8(ExecutionSessionStatusString(status));
    emit ErrorChanged();
    if (status == EXECUTION_SESSION_OK) {
        emit FmuChanged();
        emit ProfileChanged();
    }
    return status == EXECUTION_SESSION_OK;
}

bool GuiController::LoadProfile(const QString &path) {
    const QByteArray native_path = path.toLocal8Bit();
    if (!session_) {
        error_message_ = QStringLiteral("could not create the execution session");
        emit ErrorChanged();
        return false;
    }
    const execution_session_status_t status = ExecutionSessionLoadProfile(session_, native_path.constData());
    error_message_ = status == EXECUTION_SESSION_OK ? QString() : QString::fromUtf8(ExecutionSessionStatusString(status));
    emit ErrorChanged();
    if (status == EXECUTION_SESSION_OK) emit ProfileChanged();
    return status == EXECUTION_SESSION_OK;
}

QString GuiController::QuoteYaml(const QString &value) {
    QString escaped = value;
    escaped.replace(QStringLiteral("'"), QStringLiteral("''"));
    return QStringLiteral("'") + escaped + QStringLiteral("'");
}

bool GuiController::SaveProfile(const QString &path, double stepSizeSeconds, double stopTimeSeconds, int adcResolutionBits,
                                const QVariantList &adcAttenuations, const QVariantList &pwmConfigurations, const QVariantList &mappings) {
    if (!session_ || path.isEmpty() || adcAttenuations.size() != 6 || pwmConfigurations.size() != 2 || mappings.size() > PROFILE_CONFIG_MAX_MAPPINGS) {
        error_message_ = QStringLiteral("invalid YAML profile parameters");
        emit ErrorChanged();
        return false;
    }

    QString document;
    QTextStream stream(&document);
    stream.setRealNumberNotation(QTextStream::SmartNotation);
    stream.setRealNumberPrecision(17);
    stream << "version: 1\nprofile:\n  id: 1\nexecution:\n  step_size_s: " << stepSizeSeconds << "\n  stop_time_s: " << stopTimeSeconds;
    stream << "\nadc:\n  resolution_bits: " << adcResolutionBits << "\n  attenuation:\n";
    static const char *const adcChannels[] = {"GPIO32_AI", "GPIO33_AI", "GPIO34_AI", "GPIO35_AI", "GPIO36_AI", "GPIO39_AI"};
    for (int index = 0; index < adcAttenuations.size(); ++index) stream << "    " << adcChannels[index] << ": " << adcAttenuations.at(index).toInt() << "\n";
    stream << "pwm:\n";
    static const char *const pwmChannels[] = {"GPIO18_PWM", "GPIO19_PWM"};
    for (int index = 0; index < pwmConfigurations.size(); ++index) {
        const QVariantMap configuration = pwmConfigurations.at(index).toMap();
        stream << "  " << pwmChannels[index] << ":\n    frequency_hz: " << configuration.value(QStringLiteral("frequency_hz")).toUInt()
               << "\n    resolution_bits: " << configuration.value(QStringLiteral("resolution_bits")).toUInt() << "\n";
    }
    stream << "mappings:\n";
    for (const QVariant &mappingVariant : mappings) {
        const QVariantMap mapping = mappingVariant.toMap();
        const QString channel = mapping.value(QStringLiteral("channel")).toString();
        const QString variable = mapping.value(QStringLiteral("variable")).toString();
        const QString type = mapping.value(QStringLiteral("type")).toString().toLower();
        if (channel.isEmpty() || variable.isEmpty() || (type != QStringLiteral("real") && type != QStringLiteral("boolean"))) {
            error_message_ = QStringLiteral("invalid DAQC mapping");
            emit ErrorChanged();
            return false;
        }
        stream << "  - channel: " << QuoteYaml(channel) << "\n    variable: " << QuoteYaml(variable) << "\n    type: " << type
               << "\n    scale: " << mapping.value(QStringLiteral("scale")).toDouble() << "\n    offset: " << mapping.value(QStringLiteral("offset")).toDouble() << "\n";
    }

    QTemporaryFile candidate;
    candidate.setAutoRemove(true);
    if (!candidate.open() || candidate.write(document.toUtf8()) < 0 || !candidate.flush()) {
        error_message_ = QStringLiteral("could not prepare the YAML profile");
        emit ErrorChanged();
        return false;
    }
    candidate.close();
    if (ExecutionSessionValidateProfile(session_, candidate.fileName().toLocal8Bit().constData()) != EXECUTION_SESSION_OK) {
        error_message_ = QStringLiteral("YAML profile is incompatible with the FMU or ESP32 profile");
        emit ErrorChanged();
        return false;
    }

    QSaveFile destination(path);
    if (!destination.open(QIODevice::WriteOnly) || destination.write(document.toUtf8()) < 0 || !destination.commit()) {
        error_message_ = QStringLiteral("could not save the YAML profile");
        emit ErrorChanged();
        return false;
    }
    return LoadProfile(path);
}

QString GuiController::NumericTypeName(int type) {
    switch (type) {
        case 0: return QStringLiteral("Real");
        case 1: return QStringLiteral("Integer");
        case 2: return QStringLiteral("Boolean");
        case 3: return QStringLiteral("Enumeration");
        default: return QStringLiteral("Unsupported");
    }
}

QVariantList GuiController::Outputs() const {
    QVariantList outputs;
    const size_t count = ExecutionSessionOutputCount(session_);
    for (size_t index = 0U; index < count; ++index) {
        QVariantMap output;
        output.insert(QStringLiteral("index"), static_cast<int>(index));
        output.insert(QStringLiteral("name"), QString::fromUtf8(ExecutionSessionOutputName(session_, index)));
        output.insert(QStringLiteral("type"), NumericTypeName(ExecutionSessionOutputType(session_, index)));
        output.insert(QStringLiteral("typeCode"), ExecutionSessionOutputType(session_, index));
        output.insert(QStringLiteral("valueReference"), static_cast<qulonglong>(ExecutionSessionOutputValueReference(session_, index)));
        output.insert(QStringLiteral("selected"), ExecutionSessionOutputSelected(session_, index));
        outputs.append(output);
    }
    return outputs;
}

QVariantList GuiController::Inputs() const {
    QVariantList inputs;
    const size_t count = ExecutionSessionInputCount(session_);
    for (size_t index = 0U; index < count; ++index) {
        QVariantMap input;
        input.insert(QStringLiteral("index"), static_cast<int>(index));
        input.insert(QStringLiteral("name"), QString::fromUtf8(ExecutionSessionInputName(session_, index)));
        input.insert(QStringLiteral("type"), NumericTypeName(ExecutionSessionInputType(session_, index)));
        input.insert(QStringLiteral("typeCode"), ExecutionSessionInputType(session_, index));
        input.insert(QStringLiteral("valueReference"), static_cast<qulonglong>(ExecutionSessionInputValueReference(session_, index)));
        input.insert(QStringLiteral("physicalMapped"), ExecutionSessionInputHasPhysicalMapping(session_, index));
        inputs.append(input);
    }
    return inputs;
}

QVariantList GuiController::DaqcChannels(int numericType, bool inputDirection) const {
    static const char *const realInputs[] = {"GPIO32_AI", "GPIO33_AI", "GPIO34_AI", "GPIO35_AI", "GPIO36_AI", "GPIO39_AI"};
    static const char *const booleanInputs[] = {"GPIO4_DI", "GPIO13_DI", "GPIO14_DI", "GPIO27_DI"};
    static const char *const realOutputs[] = {"GPIO18_PWM", "GPIO19_PWM", "GPIO25_AO", "GPIO26_AO"};
    static const char *const booleanOutputs[] = {"GPIO16_DO", "GPIO17_DO", "GPIO21_DO", "GPIO22_DO", "GPIO23_DO"};
    const char *const *channels = nullptr;
    size_t count = 0U;
    if (numericType == 0 && inputDirection) { channels = realInputs; count = sizeof(realInputs) / sizeof(realInputs[0]); }
    else if (numericType == 2 && inputDirection) { channels = booleanInputs; count = sizeof(booleanInputs) / sizeof(booleanInputs[0]); }
    else if (numericType == 0) { channels = realOutputs; count = sizeof(realOutputs) / sizeof(realOutputs[0]); }
    else if (numericType == 2) { channels = booleanOutputs; count = sizeof(booleanOutputs) / sizeof(booleanOutputs[0]); }
    QVariantList result;
    for (size_t index = 0U; index < count; ++index) result.append(QString::fromLatin1(channels[index]));
    return result;
}

bool GuiController::SetOutputSelected(int index, bool selected) {
    if (index < 0) return false;
    const execution_session_status_t status = ExecutionSessionSetOutputSelected(session_, static_cast<size_t>(index), selected);
    error_message_ = status == EXECUTION_SESSION_OK ? QString() : QString::fromUtf8(ExecutionSessionStatusString(status));
    emit ErrorChanged();
    if (status == EXECUTION_SESSION_OK) emit FmuChanged();
    return status == EXECUTION_SESSION_OK;
}

bool GuiController::SetVirtualInput(int index, const QString &value) {
    bool converted = false;
    const double numericValue = value.toDouble(&converted);
    if (index < 0 || !converted) {
        error_message_ = QStringLiteral("invalid virtual input value");
        emit ErrorChanged();
        return false;
    }
    const execution_session_status_t status = ExecutionSessionSetVirtualInput(session_, static_cast<size_t>(index), numericValue);
    error_message_ = status == EXECUTION_SESSION_OK ? QString() : QString::fromUtf8(ExecutionSessionStatusString(status));
    emit ErrorChanged();
    return status == EXECUTION_SESSION_OK;
}
