#pragma once
#include <QString>
namespace Platform {
bool isAdministrator();
bool restartElevated(const QString &executable);
void hideConsoleForChild(void *process);
void *createProcessJob();
void addProcessToJob(void *job, qint64 pid);
void closeProcessJob(void *job);
}
