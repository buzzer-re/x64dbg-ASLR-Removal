#pragma once

#include "pluginmain.h"

using namespace Script; // For access Module options

//plugin data
#define PLUGIN_NAME "ASLR Removal"
#define PLUGIN_VERSION 1

//functions
bool pluginInit(PLUG_INITSTRUCT* initStruct);
void pluginStop();
void pluginSetup();
