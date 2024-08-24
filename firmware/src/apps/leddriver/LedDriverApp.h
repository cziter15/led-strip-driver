/*
 *	Copyright (c) 2021-2023, Krzysztof Strehlau
 *
 *	This file is part of the Energy Monitor firmware.
 *	All licensing information can be found inside LICENSE.md file.
 *
 *	https://github.com/cziter15/led-strip-driver/blob/master/LICENSE
 */

#pragma once

#include <ksIotFrameworkLib.h>

namespace apps::leddriver
{
	class LedDriverApp : public ksf::ksApplication
	{
		protected:
			std::weak_ptr<ksf::comps::ksLed> statusLedWp, errorLedWp;							// Weak pointer to LEDs.
			std::unique_ptr<ksf::evt::ksEventHandle> connEventHandleSp, disEventHandleSp;		// Event handlers for connect/disconnect.

			/*
				Called on MQTT connection established.
			*/
			void onMqttConnected();

			/*
				Called on MQTT connection lost.
			*/
			void onMqttDisconnected();
		public:
			/*
				Initializes LedDriverApp.

				@return True on success, false on fail.
			*/
			bool init() override;

			/*
				Main application loop.

				@return True on success, false on fail.
			*/
			bool loop() override;
	};
}