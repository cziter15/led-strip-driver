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
		mqttClientWp = addComponent<ksf::comps::ksMqttConnector>();
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
		if (auto mqttClientSp{mqttClientWp.lock()})
		{
			mqttClientSp->onConnected->registerEvent(connEventHandleSp, std::bind(&LedDriverApp::onMqttConnected, this));
			mqttClientSp->onDisconnected->registerEvent(disEventHandleSp, std::bind(&LedDriverApp::onMqttDisconnected, this));
			mqttClientSp->onDeviceMessage->registerEvent(msgEventHandleSp, std::bind(&LedDriverApp::onMqttMessage, this, _1, _2));
		}

		/* Start blinking status LED. */
		if (auto statusLedSp{statusLedWp.lock()})
			statusLedSp->setBlinking(500);
		
		if (udpPort)
			udpPort->begin(21324);

		strip->clear();
		strip->show();

		return true;
	}

	void LedDriverApp::onMqttMessage(const std::string_view& topic, const std::string_view& payload)
	{
		if (payload.length() == 0)
			return;

		auto mqttClientSp{mqttClientWp.lock()};
		if (!mqttClientSp)
			return;

		if (topic == "set")
		{
			staticColorMode.setEnabled(payload[0] == '1');
		}
		else if (topic == "set_rgb")
		{
			auto first_comma{payload.find_first_of(',')};
			auto second_comma{payload.find_first_of(',', first_comma + 1)};
			
			if (first_comma == std::string_view::npos || second_comma == std::string_view::npos)
				return;

			auto str_red{payload.substr(0, first_comma)};
			auto str_green{payload.substr(first_comma + 1, second_comma - first_comma - 1)};
			auto str_blue{payload.substr(second_comma + 1)};
	
			uint8_t target_rgb[3]{0,0,0};
			ksf::from_chars(str_red, target_rgb[0]);
			ksf::from_chars(str_green, target_rgb[1]);
			ksf::from_chars(str_blue, target_rgb[2]);
			staticColorMode.setRgb(target_rgb);
		}
		else if (topic == "set_brightness")
		{
			uint8_t target_brightness{0};
			ksf::from_chars(payload, target_brightness);
			staticColorMode.setBrightness(target_brightness);
		}
		else if (topic == "set_cgamma")
		{
			correctGamma = payload[0] == '1';
		}
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

		if (auto mqttClientSp{mqttClientWp.lock()})
		{
			mqttClientSp->publish("set_rgb", "255,255,255");
			mqttClientSp->publish("set_brightness", "100");
			mqttClientSp->publish("set", "0");

			//mqttClientSp->subscribe("rgb_state_topic");
			mqttClientSp->subscribe("set");
			mqttClientSp->subscribe("set_rgb");
			mqttClientSp->subscribe("set_brightness");
			mqttClientSp->subscribe("set_cgamma");
		}
	}
	bool LedDriverApp::loop()
	{
		if (staticColorMode.update())
		{
			auto color{ staticColorMode.getColor() };
			auto finalColor{ correctGamma ? Adafruit_NeoPixel::gamma32(color) : color };
			strip->fill(finalColor);
			strip->show();
		}

		if (udpPort)
		{
			if (auto packetSize{udpPort->parsePacket()}; packetSize > 0)
			{
				static uint8_t packetBuffer[1024];
				auto len{udpPort->read(packetBuffer, 1024)};

				if (len < 2 || packetBuffer[0] != 1)
					return true;

				for(auto i{2}; i < len; i+=4) 
				{
					auto color{Adafruit_NeoPixel::Color(packetBuffer[i+1], packetBuffer[i+2], packetBuffer[i+3])};
					auto finalColor{ correctGamma ? Adafruit_NeoPixel::gamma32(color) : color };
					strip->setPixelColor(packetBuffer[i], color);
				}

				strip->show();
			}
		}
		
		return ksApplication::loop();
	}
}
