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
#include <ESP8266WiFi.h>

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

			bool correctGamma{true}; // Correct gamma flag.

			class StaticColorMode
			{
				private:
					bool stripEnabled{false};				// Strip enabled flag.
					LedPixel current_rgb{255,255,255};		// Current color value.
					LedPixel target_rgb{255,255,255};		// Target color value.
					uint8_t brightnessOnDisabled{100};		// Brightness before disabling the LEDs.
					uint8_t current_brightness{0};			// Brightness multiplier.
					uint8_t target_brightness{0};			// Target brightness multiplier.
					uint16_t blendAlpha{1024};				// Blending alpha (0 - 1024)
					uint32_t startBlendTime{0};				// Time when blend started
					uint32_t blendDuration{500};			// Duration of blend in milliseconds (500ms default)
					bool needsUpdate{false};				// Flag to indicate if update is needed

					void startBlend()
					{
						blendAlpha = 0;
						startBlendTime = millis();
						needsUpdate = true;
					}

				public:
					/*
						Returns final color value.
						@return Color RGB value.
					*/
					LedPixel getColor()
					{
						return LedPixel{
							.green = static_cast<uint8_t>((static_cast<uint16_t>(current_rgb.green) * current_brightness) / 100),
							.red = static_cast<uint8_t>((static_cast<uint16_t>(current_rgb.red) * current_brightness) / 100),
							.blue = static_cast<uint8_t>((static_cast<uint16_t>(current_rgb.blue) * current_brightness) / 100)
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
						WiFi.setSleep(!enabled);

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
						Returns whether strip is enabled.
						@return Strip enabled flag.
					*/
					bool getEnabled() const
					{
						return stripEnabled;
					}

					/*
						Set blending duration
						@param duration Duration in milliseconds
					*/
					void setBlendDuration(uint32_t duration)
					{
						blendDuration = duration;
					}

					/*
						Update color and brightness.
					*/
					bool update()
					{
						/* Check if we need an update */
						if (!needsUpdate)
							return false;

						/* Calculate blend alpha based on elapsed time */
						auto currentTime = millis();
						uint32_t elapsedTime = currentTime - startBlendTime;
						
						/* Ensure we don't exceed the blend duration */
						if (elapsedTime >= blendDuration) 
						{
							/* If blend is complete, set current values to target values directly */
							current_rgb = target_rgb;
							current_brightness = target_brightness;
							blendAlpha = 1024;
							needsUpdate = false;
							return true;
						}
						
						/* Linear interpolation based on time */
						blendAlpha = (elapsedTime * 1024) / blendDuration;
						
						/* Ensure blend alpha is valid */
						blendAlpha = min(uint16_t(1024), blendAlpha);
						
						/* Perform blending with overflow protection */
						uint32_t invAlpha = 1024 - blendAlpha;
						
						current_rgb.red = static_cast<uint8_t>((current_rgb.red * invAlpha + target_rgb.red * blendAlpha) >> 10);
						current_rgb.green = static_cast<uint8_t>((current_rgb.green * invAlpha + target_rgb.green * blendAlpha) >> 10);
						current_rgb.blue = static_cast<uint8_t>((current_rgb.blue * invAlpha + target_rgb.blue * blendAlpha) >> 10);
						current_brightness = static_cast<uint8_t>((current_brightness * invAlpha + target_brightness * blendAlpha) >> 10);
						
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

			/*
				Update strip data.
				@return True if pixels should be updated, false otherwise.
			*/
			bool updateStripData();

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