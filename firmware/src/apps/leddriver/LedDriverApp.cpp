/*
 *	Copyright (c) 2024, Krzysztof Strehlau
 *
 *	This file is part of the Energy Monitor firmware.
 *	All licensing information can be found inside LICENSE.md file.
 *
 *	https://github.com/cziter15/led-strip-driver/blob/master/LICENSE
 */


#include <ksIotFrameworkLib.h>
#include <WiFiUdp.h>
#include <Adafruit_NeoPixel.h>

#include "board.h"
#include "LedDriverApp.h"
#include "../config/LedDriverConfig.h"

using namespace std::placeholders;

namespace apps::leddriver
{
	LedDriverApp::LedDriverApp() = default;
	LedDriverApp::~LedDriverApp() = default;

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

		/* Add LED strip component. */
		udpPort = std::make_unique<WiFiUDP>();
		strip = std::make_unique<Adafruit_NeoPixel>(STRIP_LED_NUM, STRIP_DATA_PIN, NEO_GRB + NEO_KHZ400);
		strip->begin();

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
		
		if (udpPort)
			udpPort->begin(21324);

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
		if (udpPort)
		{
			if (auto packetSize{udpPort->parsePacket()}; packetSize > 0)
			{
				static uint8_t packetBuffer[1024];
				auto len{udpPort->read(packetBuffer, 1024)};

				if (len < 2)
					return true;

				if (packetBuffer[0] != 1)
					return true;

				for(int i = 2; i < len; i+=4) 
				{
					auto color {Adafruit_NeoPixel::Color(packetBuffer[i+1], packetBuffer[i+2], packetBuffer[i+3])};
					strip->setPixelColor(packetBuffer[i], color);
				}

				strip->show();
			}
		}

		return ksApplication::loop();
	}
}
