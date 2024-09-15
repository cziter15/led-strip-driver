/*
 *	Copyright (c) 2021-2024, Krzysztof Strehlau
 *
 *	This file is part of the led strip driver firmware.
 *	All licensing information can be found inside LICENSE.md file.
 *
 *	https://github.com/cziter15/led-strip-driver/blob/master/LICENSE
 */

#pragma once

#include <ksIotFrameworkLib.h>

class WiFiUDP;

namespace apps::leddriver
{
	struct LedPixel
	{
		uint8_t green{};
		uint8_t red{};
		uint8_t blue{};
	};

	class LedDriverApp : public ksf::ksApplication
	{
		protected:
			std::weak_ptr<ksf::comps::ksLed> statusLedWp, errorLedWp;											// Weak pointer to LEDs.
			std::unique_ptr<ksf::evt::ksEventHandle> connEventHandleSp, disEventHandleSp, msgEventHandleSp;		// Event handlers for connect/disconnect.
			std::unique_ptr<WiFiUDP> udpPort;																	// Unique pointer to UDP.
			std::vector<LedPixel> stripPixels;																	// Strip pixel buffer.
			std::weak_ptr<ksf::comps::ksMqttConnector> mqttClientWp;

			bool correctGamma{false};

			class StaticColorMode
			{
				private:
					bool stripEnabled{false};				// Strip enabled flag.
					LedPixel current_rgb;					// Current color value.
					LedPixel target_rgb;					// Target color value.

					uint8_t brightnessOnDisabled{100};		// Brightness before disabling the LEDs.
					uint8_t current_brightness{0};			// Brightness multiplier.
					uint8_t target_brightness{0};			// Target brightness multiplier.
					uint16_t blendAlpha{1024};				// Blending alpha (0 - 1024)
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
				 	LedPixel getColor()
					{
						return LedPixel{
							.green = static_cast<uint8_t>(current_rgb.green * current_brightness / 100),
							.red = static_cast<uint8_t>(current_rgb.red * current_brightness / 100),
							.blue = static_cast<uint8_t>(current_rgb.blue * current_brightness / 100)
						};
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
						target_rgb.red = red;
						target_rgb.green = green;
						target_rgb.blue = blue;
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
						current_rgb.red = (current_rgb.red * (1024 - blendAlpha) + target_rgb.red * blendAlpha) >> 10;
						current_rgb.green = (current_rgb.green * (1024 - blendAlpha) + target_rgb.green * blendAlpha) >> 10;
						current_rgb.blue = (current_rgb.blue * (1024 - blendAlpha) + target_rgb.blue * blendAlpha) >> 10;
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