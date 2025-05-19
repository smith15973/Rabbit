// HSHandler.h
#ifndef HS_HANDLER_H
#define HS_HANDLER_H

#include <arduino.h>
#include "config.h"

void setupHS();
void hsUpdate(float *, float *, float *);
void hsStart();

#endif