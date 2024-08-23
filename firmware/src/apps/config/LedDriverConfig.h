/*
 *	Copyright (c) 2024, Krzysztof Strehlau
 *
 *	This file is part of the Energy Monitor firmware.
 *	All licensing information can be found inside LICENSE.md file.
 *
 *	https://github.com/cziter15/led-strip-driver/blob/master/LICENSE
 */

#pragma once

#include <ksIotFrameworkLib.h>

namespace apps::config
{
	class LedDriverConfig : public ksf::ksApplication
	{
		public:
			static const char ledDriverDeviceName[];		// Static table of characters with device name.

			/*
				Initializes LedDriverConfig application.

				@return True on success, false on fail.
			*/
			bool init() override;

			/*
				Handles LedDriverConfig realtime application logic.

				@return True on success, false on fail.
			*/
			bool loop() override;
	};
}
