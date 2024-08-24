/*
 *	Copyright (c) 2021-2023, Krzysztof Strehlau
 *
 *	This file is part of the Energy Monitor firmware.
 *	All licensing information can be found inside LICENSE.md file.
 *
 *	https://github.com/cziter15/led-strip-driver/blob/master/LICENSE
 */

#include "board.h"
#include "LedDriverConfig.h"

namespace apps::config
{
	const char LedDriverConfig::ledDriverDeviceName[] PROGMEM { "LedStripDriver"};

	bool LedDriverConfig::init()
	{
		addComponent<ksf::comps::ksLed>(STATUS_LED_PIN);
		addComponent<ksf::comps::ksLed>(ERROR_LED_PIN);

		addComponent<ksf::comps::ksWifiConfigurator>(ledDriverDeviceName);
		addComponent<ksf::comps::ksMqttConfigProvider>();

		return true;
	}

	bool LedDriverConfig::loop()
	{
		return ksApplication::loop();
	}
}
