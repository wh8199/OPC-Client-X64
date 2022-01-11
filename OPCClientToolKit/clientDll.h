#pragma once

#include <string>
#include <iostream>
#include <vector>

#include "OPCHost.h"
#include "string.h"

char *Readitem(char *progID, char *host, char *item);
char *Readitems(char *progID, char *host, char *params);
void Freeresult(char *ret);
