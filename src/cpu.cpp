#include <algorithm>
#include <fstream>

#include "cpu.h"
#include "debug.h"
#include "utilities.h"

/*
 * Gameblabla 15th July :
 * - Added RS97 specific code. (PLATFORM_RS97)
 * In this case, it's for setting the CPU frequency.
*/


#ifdef PLATFORM_RS97
#define SYSFS_CPUFREQ_LIST "/mnt/int_sd/gmenu2x/frequencies"
#define SYSFS_CPUFREQ_CUR "/mnt/int_sd/gmenu2x/cur_freq"
#define SYSFS_CPUFREQ_SET SYSFS_CPUFREQ_DIR "/scaling_setspeed"
#define SYSFS_CPUFREQ_DIR "/sys/devices/system/cpu/cpu0/cpufreq"
#else
#define SYSFS_CPUFREQ_DIR "/sys/devices/system/cpu/cpu0/cpufreq"
#define SYSFS_CPUFREQ_SET SYSFS_CPUFREQ_DIR "/scaling_setspeed"
#define SYSFS_CPUFREQ_CUR SYSFS_CPUFREQ_DIR "/scaling_cur_freq"
#define SYSFS_CPUFREQ_LIST SYSFS_CPUFREQ_DIR "/scaling_available_frequencies"
#endif

extern volatile uint32_t *memregs;
extern int32_t memdev;

#if defined(PLATFORM_MIYOO)
uint32_t oc_table[] = {
  0x00c81802,
  0x00cc1013,
  0x00cc1001,
  0x00d01902,
  0x00d00c12,
  0x00d80b23,
  0x00d81101,
  0x00d80833,
  0x00d81a02,
  0x00d80811,
  0x00d81113,
  0x00d80220,
  0x00d80521,
  0x00d80822,
  0x00d80800,
  0x00e01b02,
  0x00e00d12,
  0x00e00632,
  0x00e41201,
  0x00e41213,
  0x00e81c02,
  0x00ea0c23,
  0x00f00922,
  0x00f00900,
  0x00f01301,
  0x00f00933,
  0x00f00911,
  0x00f01313,
  0x00f01d02,
  0x00f00431,
  0x00f00e12,
  0x00f00410,
  0x00f81e02,
  0x00fc0d23,
  0x00fc0621,
  0x00fc1413,
  0x00fc1401,
  0x01000732,
  0x01001f02,
  0x01000f12,
  0x01081501,
  0x01080a11,
  0x01080a22,
  0x01081513,
  0x01080a00,
  0x01080a33,
  0x010e0e23,
  0x01101012,
  0x01141601,
  0x01141613,
  0x01200531,
  0x01200f23,
  0x01200832,
  0x01200b11,
  0x01200230,
  0x01200721,
  0x01200b22,
  0x01200320,
  0x01201701,
  0x01200b33,
  0x01200b00,
  0x01201713,
  0x01201112,
  0x01200510,
  0x012c1801,
  0x012c1813,
  0x01301212,
  0x01321023,
  0x01380c11,
  0x01380c22,
  0x01381901,
  0x01380c00,
  0x01380c33,
  0x01381913,
  0x01401312,
  0x01400932,
  0x01441123,
  0x01441a13,
  0x01441a01,
  0x01440821,
  0x01501b13,
  0x01501412,
  0x01500631,
  0x01500610,
  0x01500d22,
  0x01500d00,
  0x01501b01,
  0x01500d33,
  0x01500d11,
  0x01561223,
  0x015c1c01,
  0x015c1c13,
  0x01601512,
  0x01600a32,
  0x01680921,
  0x01681d01,
  0x01680420,
  0x01680e22,
  0x01681d13,
  0x01680e00,
  0x01681323,
  0x01680e33,
  0x01680e11,
  0x01701612,
  0x01741e01,
  0x01741e13,
  0x017a1423,
  0x01800f33,
  0x01800710,
  0x01800731,
  0x01800f11,
  0x01801712,
  0x01800330,
  0x01800f00,
  0x01801f01,
  0x01800f22,
  0x01800b32,
  0x01801f13,
  0x018c0a21,
  0x018c1523,
  0x01901812,
  0x01981022,
  0x01981000,
  0x01981033,
  0x01981011,
  0x019e1623,
  0x01a01912,
  0x01a00c32,
  0x01b00520,
  0x01b00810,
  0x01b01111,
  0x01b01723,
  0x01b00831,
  0x01b01100,
  0x01b01122,
  0x01b00b21,
  0x01b01a12,
  0x01b01133,
  0x01c01b12,
  0x01c00d32,
  0x01c21823,
  0x01c81233,
  0x01c81211,
  0x01c81222,
  0x01c81200,
  0x01d01c12,
  0x01d41923,
  0x01d40c21,
  0x01e00931,
  0x01e01333,
  0x01e00430,
  0x01e00e32,
  0x01e01311,
  0x01e00910,
  0x01e01300,
  0x01e01322,
  0x01e01d12,
  0x01e61a23,
  0x01f01e12,
  0x01f81400,
  0x01f81b23,
  0x01f81433,
  0x01f81411,
  0x01f80d21,
  0x01f80620,
  0x01f81422,
  0x02001f12,
  0x02000f32,
  0x020a1c23,
  0x02101522,
  0x02101500,
  0x02101533,
  0x02101511,
  0x02100a10,
  0x02100a31,
  0x021c0e21,
  0x021c1d23,
  0x02201032,
  0x02281611,
  0x02281622,
  0x02281600,
  0x02281633,
  0x022e1e23,
  0x02401722,
  0x02400b10,
  0x02401733,
  0x02401700,
  0x02400b31,
  0x02400530,
  0x02401132,
  0x02401711,
  0x02400f21,
  0x02400720,
  0x02401f23,
  0x02581800,
  0x02581833,
  0x02581811,
  0x02581822,
  0x02601232,
  0x02641021,
  0x02701911,
  0x02700c10,
  0x02700c31,
  0x02701922,
  0x02701900,
  0x02701933,
  0x02801332,
  0x02881a11,
  0x02880820,
  0x02881121,
  0x02881a22,
  0x02881a00,
  0x02881a33,
  0x02a00d31,
  0x02a01432,
  0x02a01b11,
  0x02a00630,
  0x02a01b00,
  0x02a01b22,
  0x02a00d10,
  0x02a01b33,
  0x02ac1221,
  0x02b81c00,
  0x02b81c33,
  0x02b81c11,
  0x02b81c22,
  0x02c01532,
  0x02d01d00,
  0x02d01d22,
  0x02d00920,
  0x02d01d33,
  0x02d00e10,
  0x02d00e31,
  0x02d01d11,
  0x02d01321,
  0x02e01632,
  0x02e81e00,
  0x02e81e33,
  0x02e81e11,
  0x02e81e22,
  0x02f41421,
  0x03000730,
  0x03000f10,
  0x03001f11,
  0x03000f31,
  0x03001f00,
  0x03001f22,
  0x03001732,
  0x03001f33,
  0x03180a20,
  0x03181521,
  0x03201832,
  0x03301031,
  0x03301010,
  0x033c1621,
  0x03401932,
  0x03600830,
  0x03601721,
  0x03601a32,
  0x03600b20,
  0x03601110,
  0x03601131,
  0x03801b32,
  0x03841821,
};
#endif

using namespace std;

Cpu::Cpu() : defaultAppClock(0)
{
#if defined(PLATFORM_RS97)
	if (memdev > 0) 
	{
		uint32_t m = 600 / 6;
		memregs[0x10 >> 2] = (m << 24) | 0x090520;
	}
	defaultAppClock = 600;
#elif defined(PLATFORM_MIYOO)
	uint32_t x, v;
	uint32_t total=sizeof(oc_table)/sizeof(uint32_t);

	for(x=0; x<total; x++)
	{
		if((oc_table[x] >> 16) >= 533)
		{
			v = memregs[0];
			v&= 0xffff0000;
			v|= (oc_table[x] &  0x0000ffff);
			memregs[0] = v;
			break;
		}
	}
#else
	ifstream fd1(SYSFS_CPUFREQ_CUR, ios_base::in);

	if (fd1.is_open()) {
		string line;

		getline(fd1, line, '\n');
		defaultAppClock = stoul(line) / 1000;

		DEBUG("Running at %lu MHz\n", defaultAppClock);

		fd1.close();
	}
#endif

#if defined(PLATFORM_RS97)
	frequencies.push_back(600);
	frequencies.push_back(612);
	frequencies.push_back(624);
	frequencies.push_back(636);
	frequencies.push_back(648);
	frequencies.push_back(672);
	frequencies.push_back(688);
	frequencies.push_back(702);
	frequencies.push_back(718);
	frequencies.push_back(724);
	frequencies.push_back(732);
	frequencies.push_back(740);
	frequencies.push_back(748);
	frequencies.push_back(756);
#elif defined(PLATFORM_MIYOO)
	frequencies.push_back(533);
	frequencies.push_back(702);
	frequencies.push_back(732);
	frequencies.push_back(792);
	frequencies.push_back(798);
#else
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
#endif
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
	DEBUG("Running app at %lu MHz\n", mhz);
#elif defined(PLATFORM_MIYOO)
	uint32_t x, v;
	uint32_t total=sizeof(oc_table)/sizeof(uint32_t);

	for(x=0; x<total; x++)
	{
		if((oc_table[x] >> 16) >= mhz)
		{
			v = memregs[0];
			v&= 0xffff0000;
			v|= (oc_table[x] &  0x0000ffff);
			memregs[0] = v;
			break;
		}
	}
	DEBUG("Running app at %lu MHz\n", mhz);
#else
	ofstream outf(SYSFS_CPUFREQ_SET);

	if (outf.is_open()) {
		DEBUG("Running app at %lu MHz\n", mhz);

		outf << to_string(mhz * 1000) << endl;

		outf.close();
	}
#endif
}
