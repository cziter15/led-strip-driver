/*
 *	Copyright (c) 2024, Krzysztof Strehlau
 *
 *	This file is part of the Energy Monitor firmware.
 *	All licensing information can be found inside LICENSE.md file.
 *
 *	https://github.com/cziter15/led-strip-driver/blob/master/LICENSE
 */


#include <ksIotFrameworkLib.h>

#include "board.h"
#include "LedDriverApp.h"
#include "../config/LedDriverConfig.h"

using namespace std::placeholders;

namespace apps::leddriver
{
	bool LedDriverApp::init()
	{
		/* Add WiFi connector component. */
		addComponent<ksf::comps::ksWifiConnector>(apps::config::LedDriverConfig::ledDriverDeviceName);

		/* Add MQTT components. */
		auto mqttWp{addComponent<ksf::comps::ksMqttConnector>()};
		addComponent<ksf::comps::ksDevStatMqttReporter>();

		/* Add LED indicator components. */
		statusLedWp = addComponent<ksf::comps::ksLed>(STATUS_LED_PIN);
		errorLedWp = addComponent<ksf::comps::ksLed>(ERROR_LED_PIN);

		/* Create Device Portal component. */
		addComponent<ksf::comps::ksDevicePortal>();

		/* Bind MQTT connect/disconnect events for LED status. */
		if (auto mqttSp{mqttWp.lock()})
		{
			mqttSp->onConnected->registerEvent(connEventHandleSp, std::bind(&LedDriverApp::onMqttConnected, this));
			mqttSp->onDisconnected->registerEvent(disEventHandleSp, std::bind(&LedDriverApp::onMqttDisconnected, this));
		}

		/* Start blinking status LED. */
		if (auto statusLedSp{statusLedWp.lock()})
			statusLedSp->setBlinking(500);

		return true;
	}

	void LedDriverApp::onMqttDisconnected()
	{
		if (auto statusLedSp{statusLedWp.lock()})
			statusLedSp->setBlinking(500);
	}

	void LedDriverApp::onMqttConnected()
	{
		if (auto statusLedSp{statusLedWp.lock()})
			statusLedSp->setBlinking(0);
	}

	bool LedDriverApp::loop()
	{
		return ksApplication::loop();
	}
}
