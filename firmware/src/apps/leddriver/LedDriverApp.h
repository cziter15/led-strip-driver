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

class WiFiUDP;
class Adafruit_NeoPixel;

namespace apps::leddriver
{
	class LedDriverApp : public ksf::ksApplication
	{
		protected:
			std::weak_ptr<ksf::comps::ksLed> statusLedWp, errorLedWp;											// Weak pointer to LEDs.
			std::unique_ptr<ksf::evt::ksEventHandle> connEventHandleSp, disEventHandleSp, msgEventHandleSp;		// Event handlers for connect/disconnect.
			std::unique_ptr<WiFiUDP> udpPort;																	// Unique pointer to UDP.
			std::unique_ptr<Adafruit_NeoPixel> strip;															// Unique pointer to strip.

			std::weak_ptr<ksf::comps::ksMqttConnector> mqttClientWp;

			bool correctGamma{false};

			class StaticColorMode
			{
				private:
					bool stripEnabled{false};				// Strip enabled flag.
					uint8_t current_rgb[3]{255,255,255};	// Current color value.
					uint8_t target_rgb[3]{255,255,255};		// Target color value.

					uint8_t brightnessOnDisabled{100};		// Brightness before disabling the LEDs.
					uint8_t current_brightness{0};		// Brightness multiplier.
					uint8_t target_brightness{0};			// Target brightness multiplier.
					uint16_t blendAlpha{1024};					// Blending alpha (0 - 1024)
					uint32_t prevMillis{0};					// Previous millis value.

				private:
					void startBlend()
					{
						blendAlpha = 0;
						prevMillis = millis();
					}

				public:
					/*
						Returns final color value.
						@return Color RGB value.
					*/
				 	uint32_t getColor()
					{
						return (((uint32_t)current_rgb[0] * current_brightness / 100 << 16) |
							(((uint32_t)current_rgb[1] * current_brightness) / 100 << 8) |
							((uint32_t)current_rgb[2] * current_brightness) / 100);
					}

					/*
						Set target brightness.
					*/
					void setBrightness(uint8_t brightness)
					{
						target_brightness = brightness;
						startBlend();
					}

					void setEnabled(bool enabled)
					{
						if (stripEnabled == enabled)
							return;

						stripEnabled = enabled;

						if (stripEnabled)
						{
							setBrightness(brightnessOnDisabled);
						}
						else
						{
							brightnessOnDisabled = current_brightness;
							setBrightness(0);
						}
					}

					/*
						Set target color.
					*/
					void setRgb(uint8_t rgb[3])
					{
						setRgb(rgb[0], rgb[1], rgb[2]);
					}

					/*
						Set target color.
					*/
					void setRgb(uint8_t red, uint8_t green, uint8_t blue)
					{
						target_rgb[0] = red;
						target_rgb[1] = green;
						target_rgb[2] = blue;
						startBlend();
					}

					/*
						Update color and brightness.
					*/
					bool update()
					{
						/* Check if we should blend. */
						if (blendAlpha >= 1024)
							return false;

						/* Increase blend alpha. */
						auto timeNow{millis()};
						if (blendAlpha += timeNow - prevMillis; blendAlpha > 1024)
							blendAlpha = 1024;

						/* Handle color and brightness blending. */
						for (auto i{0}; i < 3; ++i)
							current_rgb[i] = (current_rgb[i] * (1024 - blendAlpha) + target_rgb[i] * blendAlpha) >> 10;
						current_brightness = (current_brightness * (1024 - blendAlpha) + target_brightness * blendAlpha) >> 10;
						
						/* Update previous millis. */
						prevMillis = timeNow;
						return true;
					}
			} staticColorMode;

			/*
				Called on MQTT connection established.
			*/
			void onMqttConnected();

			/*
				Called on MQTT connection lost.
			*/
			void onMqttDisconnected();

			/*
				Called on MQTT message received.

				@param topic Topic.
				@param payload Payload.
			*/
			void onMqttMessage(const std::string_view& topic, const std::string_view& payload);

		public:
			/*
				Initializes LedDriverApp.
			*/
			LedDriverApp();

			/*
				Deinitializes LedDriverApp.
			*/
			virtual ~LedDriverApp();

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