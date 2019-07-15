#include "battery.h"

#include "surfacecollection.h"

#include <SDL.h>
#include <cstdio>
#include <sstream>

extern volatile uint32_t *memregs;
extern int32_t memdev;

#ifdef PLATFORM_RS97
static int32_t getBatteryStatus() 
{
	char buf[32] = "-1";
	FILE *f = fopen("/proc/jz/battery", "r");
	if (f) {
		fgets(buf, sizeof(buf), f);
		fclose(f);
	}
	return atol(buf);
}
#endif


/**
 * Reads the current battery state and returns a number representing its level
 * of charge.
 * @return A number representing battery charge: 0 means fully discharged,
 * 5 means fully charged, 6 represents running on external power.
 */
static unsigned short getBatteryLevel()
{
	FILE *handle;

#if defined(PLATFORM_A320) || defined(PLATFORM_GCW0) || defined(PLATFORM_NANONOTE)
	handle = fopen("/sys/class/power_supply/usb/online", "r");
	if (handle) {
		int usbval = 0;
		fscanf(handle, "%d", &usbval);
		fclose(handle);
		if (usbval == 1)
			return 6;
	}

	handle = fopen("/sys/class/power_supply/battery/capacity", "r");
	if (handle) {
		int battval = 0;
		fscanf(handle, "%d", &battval);
		fclose(handle);

		if (battval>90) return 5;
		if (battval>70) return 4;
		if (battval>50) return 3;
		if (battval>30) return 2;
		if (battval>10) return 1;
	}

	return 0;
#endif

#ifdef PLATFORM_RS90
	unsigned long voltage_min, voltage_max, voltage_now;

	handle = fopen("/sys/class/power_supply/jz-battery/voltage_max_design", "r");
	if (handle) {
		fscanf(handle, "%lu", &voltage_max);
		fclose(handle);
	}

	handle = fopen("/sys/class/power_supply/jz-battery/voltage_min_design", "r");
	if (handle) {
		fscanf(handle, "%lu", &voltage_min);
		fclose(handle);
	}

	handle = fopen("/sys/class/power_supply/jz-battery/voltage_now", "r");
	if (handle) {
		fscanf(handle, "%lu", &voltage_now);
		fclose(handle);
	}

	return (voltage_now - voltage_min) * 6 / (voltage_max - voltage_min);
#endif


#ifdef PLATFORM_RS97
	uint32_t val = getBatteryStatus();
	if (val > 4000) return 5; // 100%
    else if (val > 3900) return 4; // 80%
    else if (val > 3800) return 3; // 60%
    else if (val > 3730) return 2; // 40%
    else if (val > 3600) return 1; // 20%
    else return 0; // 0% :(
#endif

}

Battery::Battery(SurfaceCollection& sc_)
	: sc(sc_)
{
	lastUpdate = SDL_GetTicks();
	update();
}

const OffscreenSurface *Battery::getIcon()
{
	// Check battery status every 60 seconds.
	unsigned int now = SDL_GetTicks();
	if (now - lastUpdate >= 60000) {
		lastUpdate = now;
		update();
	}

	return sc.skinRes(iconPath);
}

void Battery::update()
{
	unsigned short battlevel = getBatteryLevel();
	if (battlevel > 5) {
		iconPath = "imgs/battery/ac.png";
	} else {
		std::stringstream ss;
		ss << "imgs/battery/" << battlevel << ".png";
		ss >> iconPath;
	}
}
