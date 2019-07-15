#ifndef BRIGHTNESSMANAGER_H
#define BRIGHTNESSMANAGER_H

#include <string>

class GMenu2X;

class BrightnessManager {
public:
	BrightnessManager(GMenu2X *gmenu2x);

	bool available() const { return is_available; }
	void setBrightness(unsigned int brightness);

	unsigned int maxBrightness() const { return max_brightness; }
	unsigned int currentBrightness() const { return current_brightness; }

private:
	std::string sysfs_dir;
	unsigned int current_brightness;
	unsigned int max_brightness;
	unsigned int is_available;
};

#endif /* BRIGHTNESSMANAGER_H */
