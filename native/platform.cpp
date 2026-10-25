#include "platform.h"
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#endif

bool Platform::isAdministrator() {
#ifdef Q_OS_WIN
    SID_IDENTIFIER_AUTHORITY authority = SECURITY_NT_AUTHORITY;
    PSID group=nullptr; BOOL member=FALSE;
    if (AllocateAndInitializeSid(&authority,2,SECURITY_BUILTIN_DOMAIN_RID,DOMAIN_ALIAS_RID_ADMINS,0,0,0,0,0,0,&group)) {
        CheckTokenMembership(nullptr,group,&member); FreeSid(group);
    }
    return member!=FALSE;
#else
    return true;
#endif
}
bool Platform::restartElevated(const QString &executable) {
#ifdef Q_OS_WIN
    const auto path = executable.toStdWString();
    SHELLEXECUTEINFOW info{}; info.cbSize=sizeof(info); info.lpVerb=L"runas"; info.lpFile=path.c_str(); info.nShow=SW_SHOWNORMAL;
    return ShellExecuteExW(&info)!=FALSE;
#else
    Q_UNUSED(executable); return false;
#endif
}
void Platform::hideConsoleForChild(void *process) { Q_UNUSED(process); }
void *Platform::createProcessJob() {
#ifdef Q_OS_WIN
    HANDLE job = CreateJobObjectW(nullptr,nullptr);
    if (job) {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION info{};
        info.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if (!SetInformationJobObject(job,JobObjectExtendedLimitInformation,&info,sizeof(info))) { CloseHandle(job); return nullptr; }
    }
    return job;
#else
    return nullptr;
#endif
}
void Platform::addProcessToJob(void *job,qint64 pid) {
#ifdef Q_OS_WIN
    if (!job) return;
    HANDLE process=OpenProcess(PROCESS_SET_QUOTA|PROCESS_TERMINATE,FALSE,static_cast<DWORD>(pid));
    if (process) { AssignProcessToJobObject(static_cast<HANDLE>(job),process); CloseHandle(process); }
#else
    Q_UNUSED(job); Q_UNUSED(pid);
#endif
}
void Platform::closeProcessJob(void *job) {
#ifdef Q_OS_WIN
    if (job) CloseHandle(static_cast<HANDLE>(job));
#else
    Q_UNUSED(job);
#endif
}
