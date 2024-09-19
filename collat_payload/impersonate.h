#pragma once

#include <winsock2.h>
#include "win_defs.h"

BOOL ImpersonateProcess(LPWSTR processName);
BOOL RevertImpersonation();