#include <fstream>
#include "brightnessmanager.h"
#include "gmenu2x.h"
#include "utilities.h"

BrightnessManager::BrightnessManager(GMenu2X *gmenu2x)
	: is_available(false)
{
	sysfs_dir = gmenu2x->confStr["brightnessSysfsFile"];

	if (sysfs_dir.empty())
		return;

	std::ifstream max_brightness_f(sysfs_dir + "/" + "max_brightness");
	std::ifstream cur_brightness_f(sysfs_dir + "/" + "brightness");

	if (!max_brightness_f.is_open() ||
	    !cur_brightness_f.is_open())
		return;

	std::string line;

	std::getline(max_brightness_f, line);
	max_brightness = std::stoul(line);

	std::getline(cur_brightness_f, line);
	current_brightness = std::stoul(line);
	is_available = true;
}

void BrightnessManager::setBrightness(unsigned int brightness)
{
	std::ofstream brightness_f(sysfs_dir + "/" + "brightness");

	brightness_f << std::to_string(brightness) << std::endl;
}
