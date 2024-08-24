/*
 *	Copyright (c) 2024, Krzysztof Strehlau
 *
 *	This file is part of the Energy Monitor firmware.
 *	All licensing information can be found inside LICENSE.md file.
 *
 *	https://github.com/cziter15/led-strip-driver/blob/master/LICENSE
 */

#include "apps/leddriver/LedDriverApp.h"
#include "apps/config/LedDriverConfig.h"

using namespace apps;

KSF_IMPLEMENT_APP_ROTATOR
(
	leddriver::LedDriverApp, 
	config::LedDriverConfig
)