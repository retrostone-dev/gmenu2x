#include <algorithm>
#include <fstream>

#include "cpu.h"
#include "debug.h"
#include "utilities.h"

#define SYSFS_CPUFREQ_DIR "/sys/devices/system/cpu/cpu0/cpufreq"
#define SYSFS_CPUFREQ_LIST SYSFS_CPUFREQ_DIR "/scaling_available_frequencies"
#define SYSFS_CPUFREQ_SET SYSFS_CPUFREQ_DIR "/scaling_setspeed"
#define SYSFS_CPUFREQ_CUR SYSFS_CPUFREQ_DIR "/scaling_cur_freq"

extern volatile uint32_t *memregs;
extern int32_t memdev;

using namespace std;

Cpu::Cpu() : defaultAppClock(0)
{
	ifstream fd1(SYSFS_CPUFREQ_CUR, ios_base::in);

	if (fd1.is_open()) {
		string line;

		getline(fd1, line, '\n');
		defaultAppClock = stoul(line) / 1000;

		DEBUG("Running at %lu MHz\n", defaultAppClock);

#ifdef PLATFORM_RS97
		if (memdev > 0) 
		{
			uint32_t m = defaultAppClock / 6;
			memregs[0x10 >> 2] = (m << 24) | 0x090520;
		}
#endif

		fd1.close();
	}

	ifstream fd2(SYSFS_CPUFREQ_LIST, ios_base::in);
	vector<string> vect;

	if (fd2.is_open()) {
		string line;

		getline(fd2, line, '\n');
		fd2.close();

		split(vect, line, " ");
	}

	for (auto it = vect.begin(); it != vect.end(); it++) {
		if (!it->empty())
			frequencies.push_back(stoul(*it) / 1000);
	}
}

string Cpu::freqStr(unsigned long mhz)
{
	return to_string(mhz) + " MHz";
}

unsigned long Cpu::freqFromStr(const string& str)
{
	return stoul(str);
}

vector<string> Cpu::getFrequencies()
{
	vector<string> freqs;

	for (unsigned long& each: frequencies)
		freqs.push_back(freqStr(each));

	return freqs;
}

void Cpu::setCpuSpeed(unsigned long mhz)
{
#ifdef PLATFORM_RS97
	if (memdev > 0) 
	{
		uint32_t m = mhz / 6;
		memregs[0x10 >> 2] = (m << 24) | 0x090520;
	}
#else
	ofstream outf(SYSFS_CPUFREQ_SET);

	if (outf.is_open()) {
		DEBUG("Running app at %lu MHz\n", mhz);

		outf << to_string(mhz * 1000) << endl;

		outf.close();
	}
#endif
}
