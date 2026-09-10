#include "gui_controller.h"

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
        output.insert(QStringLiteral("valueReference"), static_cast<qulonglong>(ExecutionSessionOutputValueReference(session_, index)));
        output.insert(QStringLiteral("selected"), ExecutionSessionOutputSelected(session_, index));
        outputs.append(output);
    }
    return outputs;
}

bool GuiController::SetOutputSelected(int index, bool selected) {
    if (index < 0) return false;
    const execution_session_status_t status = ExecutionSessionSetOutputSelected(session_, static_cast<size_t>(index), selected);
    error_message_ = status == EXECUTION_SESSION_OK ? QString() : QString::fromUtf8(ExecutionSessionStatusString(status));
    emit ErrorChanged();
    if (status == EXECUTION_SESSION_OK) emit FmuChanged();
    return status == EXECUTION_SESSION_OK;
}
