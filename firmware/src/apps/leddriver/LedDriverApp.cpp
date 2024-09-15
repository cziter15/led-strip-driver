/*
 *	Copyright (c) 2021-2024, Krzysztof Strehlau
 *
 *	This file is part of the led strip driver firmware.
 *	All licensing information can be found inside LICENSE.md file.
 *
 *	https://github.com/cziter15/led-strip-driver/blob/master/LICENSE
 */

#include <ksIotFrameworkLib.h>
#include <WiFiUdp.h>

#include "board.h"
#include "LedDriverApp.h"
#include "../config/LedDriverConfig.h"

using namespace std::placeholders;

namespace apps::leddriver
{
	LedDriverApp::LedDriverApp() = default;
	LedDriverApp::~LedDriverApp() = default;

	static IRAM_ATTR inline uint32_t _getCycleCount(void)
	{
		uint32_t cycles;
		__asm__ __volatile__("rsr %0,ccount":"=a" (cycles));
		return cycles;
	}
	IRAM_ATTR void ws2812_write(uint8_t pin, uint8_t *pixels, uint32_t length) 
	{
		#define CYCLES_T0H  (F_CPU / 2000000) // 0.5uS
		#define CYCLES_T1H  (F_CPU /  833333) // 1.2us
		#define CYCLES      (F_CPU /  400000) // 2.5us per bit

		uint32_t t{}, c{}, time0{CYCLES_T0H}, time1{CYCLES_T1H}, period{CYCLES}, startTime{0}, pinMask(_BV(pin));
		uint8_t *p{pixels}, *end{p + length}, pix{*p++}, mask{0x80};
		ets_intr_lock();
		for(t = time0;; t = time0)
		{
			if(pix & mask) 
				t = time1;
			while(((c = _getCycleCount()) - startTime) < period);
			GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, pinMask);
			startTime = c;
			while(((c = _getCycleCount()) - startTime) < t);
			GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, pinMask);

			if(!(mask >>= 1))
			{
				if(p >= end)
					break;
				pix  = *p++;
				mask = 0x80;
			}
		}
		while((_getCycleCount() - startTime) < period);
		ets_intr_unlock();
	}

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

		stripPixels.resize(STRIP_LED_NUM);

		pinMode(STRIP_DATA_PIN, OUTPUT);
		digitalWrite(STRIP_DATA_PIN, LOW);

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

	bool LedDriverApp::updateStripData()
	{
		if (staticColorMode.update())
		{
			auto color{ staticColorMode.getColor() };
			for (auto& pix : stripPixels)
				pix = color;
			return true;
		} 

		if (!udpPort || !staticColorMode.getEnabled())
			return false;

		if (auto packetSize{udpPort->parsePacket()}; packetSize > 0)
		{
			static uint8_t packetBuffer[1024];
			auto len{udpPort->read(packetBuffer, 1024)};

			if (len < 2 || packetBuffer[0] != 1)
				return true;

			for(auto i{2}; i < len; i+=4) 
				stripPixels[packetBuffer[i]] = {packetBuffer[i+2], packetBuffer[i+1], packetBuffer[i+3]};

			return true;
		}

		return false;
	}

	bool LedDriverApp::loop()
	{
		if (updateStripData())
			ws2812_write(STRIP_DATA_PIN, (uint8_t*)&stripPixels[0], stripPixels.size()*sizeof(LedPixel));

		return ksApplication::loop();
	}
}
