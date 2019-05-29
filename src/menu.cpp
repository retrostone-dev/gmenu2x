/***************************************************************************
 *   Copyright (C) 2006 by Massimiliano Torromeo                           *
 *   massimiliano.torromeo@gmail.com                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <algorithm>
#include <math.h>
#include <fstream>
#include <unistd.h>
#include <cassert>
#include <cerrno>
#include <cstring>

#ifdef HAVE_LIBOPK
#include <opk.h>
#endif

#include "gmenu2x.h"
#include "linkapp.h"
#include "menu.h"
#include "monitor.h"
#include "filelister.h"
#include "utilities.h"
#include "debug.h"

using namespace std;


Menu::Animation::Animation()
	: curr(0)
{
}

void Menu::Animation::adjust(int delta)
{
	curr += delta;
}

void Menu::Animation::step()
{
	if (curr == 0) {
		ERROR("Computing step past animation end\n");
	} else if (curr < 0) {
		const int v = ((1 << 16) - curr) / 32;
		curr = std::min(0, curr + v);
	} else {
		const int v = ((1 << 16) + curr) / 32;
		curr = std::max(0, curr - v);
	}
}

Menu::Menu(GMenu2X& gmenu2x)
	: gmenu2x(gmenu2x)
	, btnContextMenu(gmenu2x, "skin:imgs/menu.png", "",
			std::bind(&GMenu2X::showContextMenu, &gmenu2x))
{
	readSections(GMENU2X_SYSTEM_DIR "/sections");
	readSections(GMenu2X::getHome() + "/sections");

	setSectionIndex(0);
	readLinks();

#ifdef HAVE_LIBOPK
	{
		DIR *dirp = opendir(CARD_ROOT);
		if (dirp) {
			struct dirent *dptr;
			while ((dptr = readdir(dirp))) {
				if (dptr->d_type != DT_DIR)
					continue;

				if (!strcmp(dptr->d_name, ".") || !strcmp(dptr->d_name, ".."))
					continue;

				openPackagesFromDir((string) CARD_ROOT + "/" +
							dptr->d_name + "/apps");
				printf("%s/%s/apps\n", CARD_ROOT, dptr->d_name);
			}
			closedir(dirp);
		}
	}
#endif

	btnContextMenu.setPosition(gmenu2x.resX - 38, gmenu2x.bottomBarIconY);
}

Menu::~Menu()
{
}

void Menu::readSections(std::string const& parentDir)
{
	DIR *dirp = opendir(parentDir.c_str());
	if (!dirp) return;

	struct dirent *dptr;
	while ((dptr = readdir(dirp))) {
		if (dptr->d_name[0] != '.' && dptr->d_type == DT_DIR) {
			// Create section if it doesn't exist yet.
			sectionNamed(dptr->d_name);
		}
	}

	closedir(dirp);
}

string Menu::createSectionDir(string const& sectionName)
{
	string parentDir = GMenu2X::getHome() + "/sections";
	if (mkdir(parentDir.c_str(), 0755) && errno != EEXIST) {
		WARNING("Failed to create parent sections dir: %s\n", strerror(errno));
		return "";
	}

	string childDir = parentDir + "/" + sectionName;
	if (mkdir(childDir.c_str(), 0755) && errno != EEXIST) {
		WARNING("Failed to create child section dir: %s\n", strerror(errno));
		return "";
	}

	return childDir;
}

void Menu::skinUpdated() {
	ConfIntHash &skinConfInt = gmenu2x.skinConfInt;

	//recalculate some coordinates based on the new element sizes
	linkColumns = (gmenu2x.resX - 10) / skinConfInt["linkWidth"];
	linkRows = (gmenu2x.resY - 35 - skinConfInt["topBarHeight"]) / skinConfInt["linkHeight"];

	//reload section icons
	decltype(links)::size_type i = 0;
	for (auto& sectionName : sections) {
		gmenu2x.sc["skin:sections/" + sectionName + ".png"];

		for (auto& link : links[i]) {
			link->loadIcon();
		}

		i++;
	}
}

void Menu::calcSectionRange(int &leftSection, int &rightSection) {
	ConfIntHash &skinConfInt = gmenu2x.skinConfInt;
	const int linkWidth = skinConfInt["linkWidth"];
	const int screenWidth = gmenu2x.resX;
	const int numSections = sections.size();
	rightSection = min(
			max(1, (screenWidth - 20 - linkWidth) / (2 * linkWidth)),
			numSections / 2);
	leftSection = max(
			-rightSection,
			rightSection - numSections + 1);
}

bool Menu::runAnimations() {
	if (sectionAnimation.isRunning()) {
		sectionAnimation.step();
	}
	return sectionAnimation.isRunning();
}

void Menu::paint(Surface &s) {
	const uint width = s.width(), height = s.height();
	Font &font = *gmenu2x.font;
	SurfaceCollection &sc = gmenu2x.sc;

	ConfIntHash &skinConfInt = gmenu2x.skinConfInt;
	const int topBarHeight = skinConfInt["topBarHeight"];
	const int bottomBarHeight = skinConfInt["bottomBarHeight"];
	const int linkWidth = skinConfInt["linkWidth"];
	const int linkHeight = skinConfInt["linkHeight"];
	RGBAColor &selectionBgColor = gmenu2x.skinConfColors[COLOR_SELECTION_BG];

	// Apply section header animation.
	int leftSection, rightSection;
	calcSectionRange(leftSection, rightSection);
	int sectionFP = sectionAnimation.currentValue();
	int sectionDelta = (sectionFP * linkWidth + (1 << 15)) >> 16;
	int centerSection = iSection - sectionDelta / linkWidth;
	sectionDelta %= linkWidth;
	if (sectionDelta < 0) {
		rightSection++;
	} else if (sectionDelta > 0) {
		leftSection--;
	}

	// Paint section headers.
	s.box(width / 2  - linkWidth / 2, 0, linkWidth, topBarHeight, selectionBgColor);
	const uint sectionLinkPadding = (topBarHeight - 32 - font.getLineSpacing()) / 3;
	const uint numSections = sections.size();
	for (int i = leftSection; i <= rightSection; i++) {
		uint j = (centerSection + numSections + i) % numSections;
		string sectionIcon = "skin:sections/" + sections[j] + ".png";
		Surface *icon = sc.exists(sectionIcon)
				? sc[sectionIcon]
				: sc.skinRes("icons/section.png");
		int x = width / 2 + i * linkWidth + sectionDelta;
		if (i == leftSection) {
			int t = sectionDelta > 0 ? linkWidth - sectionDelta : -sectionDelta;
			x -= (((t * t) / linkWidth) * t) / linkWidth;
		} else if (i == rightSection) {
			int t = sectionDelta < 0 ? sectionDelta + linkWidth : sectionDelta;
			x += (((t * t) / linkWidth) * t) / linkWidth;
		}
		icon->blit(s, x - 16, sectionLinkPadding, 32, 32);
		font.write(s, sections[j], x, topBarHeight - sectionLinkPadding,
				Font::HAlignCenter, Font::VAlignBottom);
	}

	auto l_button = sc.skinRes("imgs/section-l.png");
	if (l_button)
		l_button->blit(s, 0, 0);
	auto r_button = sc.skinRes("imgs/section-r.png");
	if (r_button)
		r_button->blit(s, width - 10, 0);

	auto& sectionLinks = links[iSection];
	auto numLinks = sectionLinks.size();
	gmenu2x.drawScrollBar(
			linkRows, (numLinks + linkColumns - 1) / linkColumns, iFirstDispRow);

	//Links
	const uint linksPerPage = linkColumns * linkRows;
	const int linkSpacingX = (width - 10 - linkColumns * linkWidth) / linkColumns;
	const int linkMarginX = (
			width - linkWidth * linkColumns - linkSpacingX * (linkColumns - 1)
			) / 2;
	const int linkSpacingY = (height - 35 - topBarHeight - linkRows * linkHeight) / linkRows;
	for (uint i = iFirstDispRow * linkColumns; i < iFirstDispRow * linkColumns + linksPerPage && i < numLinks; i++) {
		const int ir = i - iFirstDispRow * linkColumns;
		const int x = linkMarginX + (ir % linkColumns) * (linkWidth + linkSpacingX);
		const int y = ir / linkColumns * (linkHeight + linkSpacingY) + topBarHeight + 2;
		sectionLinks.at(i)->setPosition(x, y);

		if (i == (uint)iLink) {
			sectionLinks.at(i)->paintHover();
		}

		sectionLinks.at(i)->paint();
	}

	if (selLink())
		selLink()->paintDescription(width / 2, height - bottomBarHeight + 2);

	LinkApp *linkApp = selLinkApp();
	if (linkApp && linkApp->isEditable()) {
#ifdef ENABLE_CPUFREQ
		font.write(s, gmenu2x.cpu.freqStr(linkApp->clock()),
				gmenu2x.cpuX, gmenu2x.bottomBarTextY,
				Font::HAlignLeft, Font::VAlignMiddle);
#endif
		//Manual indicator
		if (!linkApp->getManual().empty())
			sc.skinRes("imgs/manual.png")->blit(
					s, gmenu2x.manualX, gmenu2x.bottomBarIconY);
	}
}

bool Menu::handleButtonPress(InputManager::Button button) {
	switch (button) {
		case InputManager::ACCEPT:
			if (selLink() != NULL) selLink()->run();
			return true;
		case InputManager::UP:
			linkUp();
			return true;
		case InputManager::DOWN:
			linkDown();
			return true;
		case InputManager::LEFT:
			linkLeft();
			return true;
		case InputManager::RIGHT:
			linkRight();
			return true;
		case InputManager::ALTLEFT:
			decSectionIndex();
			return true;
		case InputManager::ALTRIGHT:
			incSectionIndex();
			return true;
		case InputManager::MENU:
			gmenu2x.showContextMenu();
			return true;
		default:
			return false;
	}
}

/*====================================
   SECTION MANAGEMENT
  ====================================*/

vector<unique_ptr<Link>> *Menu::sectionLinks(int i)
{
	if (i<0 || i>=(int)links.size()) {
		i = selSectionIndex();
	}

	if (i<0 || i>=(int)links.size()) {
		return nullptr;
	}

	return &links[i];
}

void Menu::decSectionIndex() {
	sectionAnimation.adjust(-1 << 16);
	setSectionIndex(iSection - 1);
}

void Menu::incSectionIndex() {
	sectionAnimation.adjust(1 << 16);
	setSectionIndex(iSection + 1);
}

int Menu::selSectionIndex() {
	return iSection;
}

const string &Menu::selSection() {
	return sections[iSection];
}

void Menu::setSectionIndex(int i) {
	if (i<0)
		i=sections.size()-1;
	else if (i>=(int)sections.size())
		i=0;
	iSection = i;

	iLink = 0;
	iFirstDispRow = 0;
}

/*====================================
   LINKS MANAGEMENT
  ====================================*/
void Menu::addActionLink(uint section, string const& title, Action action,
		string const& description, string const& icon)
{
	assert(section < sections.size());

	Link *link = new Link(gmenu2x, action);
	link->setSize(gmenu2x.skinConfInt["linkWidth"], gmenu2x.skinConfInt["linkHeight"]);
	link->setTitle(title);
	link->setDescription(description);
	if (gmenu2x.sc.exists(icon)
			|| (icon.substr(0,5)=="skin:"
				&& !gmenu2x.sc.getSkinFilePath(icon.substr(5,icon.length())).empty())
			|| fileExists(icon)) {
		link->setIcon(icon);
	}

	links[section].emplace_back(link);
}

bool Menu::addLink(string const& path, string const& file)
{
	string const& sectionName = selSection();

	string sectionDir = createSectionDir(sectionName);
	if (sectionDir.empty()) {
		return false;
	}

	//strip the extension from the filename
	string title = file;
	string::size_type pos = title.rfind(".");
	if (pos!=string::npos && pos>0) {
		string ext = title.substr(pos, title.length());
		transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
		title = title.substr(0, pos);
	}

	string linkpath = uniquePath(sectionDir, title);
	INFO("Adding link: '%s'\n", linkpath.c_str());

	string dirPath = path;
	if (dirPath.empty() || dirPath.back() != '/') dirPath += '/';

	//search for a manual
	pos = file.rfind(".");
	string exename = dirPath + file.substr(0, pos);
	string manual = "";
	if (fileExists(exename+".man.png")) {
		manual = exename+".man.png";
	} else if (fileExists(exename+".man.txt")) {
		manual = exename+".man.txt";
	} else {
		//scan directory for a file like *readme*
		FileLister fl;
		fl.setShowDirectories(false);
		fl.setFilter(".txt");
		fl.browse(dirPath);
		bool found = false;
		for (uint x=0; x<fl.size() && !found; x++) {
			string lcfilename = fl[x];

			if (lcfilename.find("readme") != string::npos) {
				found = true;
				manual = dirPath + fl.getFiles()[x];
			}
		}
	}

	INFO("Manual: '%s'\n", manual.c_str());

	string shorttitle=title, description="", exec=dirPath+file, icon="";
	if (fileExists(exename+".png")) icon = exename+".png";

	//Reduce title lenght to fit the link width
	if (gmenu2x.font->getTextWidth(shorttitle)>gmenu2x.skinConfInt["linkWidth"]) {
		while (gmenu2x.font->getTextWidth(shorttitle+"..")>gmenu2x.skinConfInt["linkWidth"])
			shorttitle = shorttitle.substr(0,shorttitle.length()-1);
		shorttitle += "..";
	}

	ofstream f(linkpath.c_str());
	if (f.is_open()) {
		f << "title=" << shorttitle << endl;
		f << "exec=" << exec << endl;
		if (!description.empty()) f << "description=" << description << endl;
		if (!icon.empty()) f << "icon=" << icon << endl;
		if (!manual.empty()) f << "manual=" << manual << endl;
		f.close();
 		sync();

		auto idx = sectionNamed(sectionName);
		auto link = new LinkApp(gmenu2x, linkpath, true);
		link->setSize(gmenu2x.skinConfInt["linkWidth"], gmenu2x.skinConfInt["linkHeight"]);
		links[idx].emplace_back(link);
	} else {

		ERROR("Error while opening the file '%s' for write.\n", linkpath.c_str());

		return false;
	}

	return true;
}

int Menu::sectionNamed(const char *sectionName)
{
	auto it = lower_bound(sections.begin(), sections.end(), sectionName);
	int idx = it - sections.begin();
	if (it == sections.end() || *it != sectionName) {
		sections.emplace(it, sectionName);
		links.emplace(links.begin() + idx);
		// Make sure the selected section doesn't change.
		if (idx <= iSection) {
			iSection++;
		}
	}
	return idx;
}

void Menu::deleteSelectedLink()
{
	string iconpath = selLink()->getIconPath();

	INFO("Deleting link '%s'\n", selLink()->getTitle().c_str());

	if (selLinkApp()!=NULL)
		unlink(selLinkApp()->getFile().c_str());
	sectionLinks()->erase( sectionLinks()->begin() + selLinkIndex() );
	setLinkIndex(selLinkIndex());

	bool icon_used = false;
	for (auto& section : links) {
		for (auto& link : section) {
			if (iconpath == link->getIconPath()) {
				icon_used = true;
			}
		}
	}
	if (!icon_used) {
		gmenu2x.sc.del(iconpath);
	}
}

void Menu::deleteSelectedSection()
{
	string const& sectionName = selSection();
	INFO("Deleting section '%s'\n", sectionName.c_str());

	gmenu2x.sc.del("sections/" + sectionName + ".png");
	auto idx = selSectionIndex();
	links.erase(links.begin() + idx);
	sections.erase(sections.begin() + idx);
	setSectionIndex(0); //reload sections

	string path = GMenu2X::getHome() + "/sections/" + sectionName;
	if (rmdir(path.c_str()) && errno != ENOENT) {
		WARNING("Removal of section dir \"%s\" failed: %s\n",
				path.c_str(), strerror(errno));
	}
}

bool Menu::moveSelectedLink(string const& newSection)
{
	LinkApp *linkApp = selLinkApp();
	if (!linkApp) {
		return false;
	}

	// Note: Get new index first, since it might move the selected index.
	auto const newSectionIndex = sectionNamed(newSection);
	auto const oldSectionIndex = iSection;

	string const& file = linkApp->getFile();
	string linkTitle = file.substr(file.rfind('/') + 1);

	string sectionDir = createSectionDir(newSection);
	if (sectionDir.empty()) {
		return false;
	}

	string newFileName = uniquePath(sectionDir, linkTitle);

	if (rename(file.c_str(), newFileName.c_str())) {
		WARNING("Link file move from '%s' to '%s' failed: %s\n",
				file.c_str(), newFileName.c_str(), strerror(errno));
		return false;
	}
	linkApp->setFile(newFileName);

	// Fetch sections.
	auto& newSectionLinks = links[newSectionIndex];
	auto& oldSectionLinks = links[oldSectionIndex];

	// Move link.
	auto it = oldSectionLinks.begin() + iLink;
	auto link = it->release();
	oldSectionLinks.erase(it);
	newSectionLinks.emplace_back(link);

	// Select the same link in the new section.
	setSectionIndex(newSectionIndex);
	setLinkIndex(newSectionLinks.size() - 1);

	return true;
}

void Menu::linkLeft() {
	if (iLink % linkColumns == 0)
		setLinkIndex(sectionLinks()->size() > iLink + linkColumns - 1
				? iLink + linkColumns - 1 : sectionLinks()->size() - 1);
	else
		setLinkIndex(iLink - 1);
}

void Menu::linkRight() {
	if (iLink % linkColumns == linkColumns - 1
			|| iLink == (int)sectionLinks()->size() - 1)
		setLinkIndex(iLink - iLink % linkColumns);
	else
		setLinkIndex(iLink + 1);
}

#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))

void Menu::linkUp() {
	int l = iLink - linkColumns;
	if (l < 0) {
		const auto numLinks = sectionLinks()->size();
		unsigned int rows = DIV_ROUND_UP(numLinks, linkColumns);
		l = (rows * linkColumns) + l;
		if (l >= static_cast<int>(numLinks))
			l -= linkColumns;
	}
	setLinkIndex(l);
}

void Menu::linkDown() {
	int l = iLink + linkColumns;
	const auto numLinks = sectionLinks()->size();
	if (l >= static_cast<int>(numLinks)) {
		unsigned int rows = DIV_ROUND_UP(numLinks, linkColumns);
		unsigned int curCol = DIV_ROUND_UP(iLink + 1, linkColumns);
		if (rows > curCol)
			l = numLinks - 1;
		else
			l %= linkColumns;
	}
	setLinkIndex(l);
}

int Menu::selLinkIndex() {
	return iLink;
}

Link *Menu::selLink() {
	auto curSectionLinks = sectionLinks();
	if (curSectionLinks->empty()) {
		return nullptr;
	} else {
		return curSectionLinks->at(iLink).get();
	}
}

LinkApp *Menu::selLinkApp() {
	return dynamic_cast<LinkApp*>(selLink());
}

void Menu::setLinkIndex(int i) {
	auto links = sectionLinks();
	if (!links) {
		// No sections.
		return;
	}
	const int numLinks = static_cast<int>(sectionLinks()->size());
	if (i < 0)
		i = numLinks - 1;
	else if (i >= numLinks)
		i = 0;
	iLink = i;

	int nbRows = static_cast<int>(DIV_ROUND_UP(numLinks, linkColumns));
	int row = i / linkColumns;

	if (row >= (int)(iFirstDispRow + linkRows))
		iFirstDispRow = min(row + 1, nbRows - 1) - linkRows + 1;
	else if (row < (int)iFirstDispRow)
		iFirstDispRow = max(row - 1, 0);
}

#ifdef HAVE_LIBOPK
void Menu::openPackagesFromDir(std::string const& path)
{
	DEBUG("Opening packages from directory: %s\n", path.c_str());
	if (readPackages(path)) {
#ifdef ENABLE_INOTIFY
		monitors.emplace_back(new Monitor(path.c_str()));
#endif
	}
}

void Menu::openPackage(std::string const& path, bool order)
{
#ifdef ENABLE_INOTIFY
	/* First try to remove existing links of the same OPK
	 * (needed for instance when an OPK is modified) */
	removePackageLink(path);
#endif

	struct OPK *opk = opk_open(path.c_str());
	if (!opk) {
		ERROR("Unable to open OPK %s\n", path.c_str());
		return;
	}

	for (;;) {
		bool has_metadata = false;
		const char *name;

		for (;;) {
			string::size_type pos;
			int ret = opk_open_metadata(opk, &name);
			if (ret < 0) {
				ERROR("Error while loading meta-data\n");
				break;
			} else if (!ret)
			  break;

			/* Strip .desktop */
			string metadata(name);
			pos = metadata.rfind('.');
			metadata = metadata.substr(0, pos);

			/* Keep only the platform name */
			pos = metadata.rfind('.');
			metadata = metadata.substr(pos + 1);

			if (metadata == PLATFORM || metadata == "all") {
				has_metadata = true;
				break;
			}
		}

		if (!has_metadata)
		  break;

		// Note: OPK links can only be deleted by removing the OPK itself,
		//       but that is not something we want to do in the menu,
		//       so consider this link undeletable.
		auto link = new LinkApp(gmenu2x, path, false, opk, name);
		link->setSize(gmenu2x.skinConfInt["linkWidth"], gmenu2x.skinConfInt["linkHeight"]);

		auto idx = sectionNamed(link->getCategory());
		links[idx].emplace_back(link);

		createSectionDir(link->getCategory());
	}

	opk_close(opk);

	if (order)
		orderLinks();
}

bool Menu::readPackages(std::string const& parentDir)
{
	DIR *dirp = opendir(parentDir.c_str());
	if (!dirp) {
		return false;
	}

	while (struct dirent *dptr = readdir(dirp)) {
		if (dptr->d_type != DT_REG)
			continue;

		char *c = strrchr(dptr->d_name, '.');
		if (!c) /* File without extension */
			continue;

		if (strcasecmp(c + 1, "opk"))
			continue;

		if (dptr->d_name[0] == '.') {
			// Ignore hidden files.
			// Mac OS X places these on SD cards, probably to store metadata.
			continue;
		}

		openPackage(parentDir + '/' + dptr->d_name, false);
	}

	closedir(dirp);
	orderLinks();

	return true;
}

#ifdef ENABLE_INOTIFY
/* Remove all links that correspond to the given path.
 * If "path" is a directory, it will remove all links that
 * correspond to an OPK present in the directory. */
void Menu::removePackageLink(std::string const& path)
{
	for (auto section = links.begin(); section != links.end(); ++section) {
		for (auto link = section->begin(); link != section->end(); ++link) {
			LinkApp *app = dynamic_cast<LinkApp *>(link->get());
			if (!app || !app->isOpk() || app->getOpkFile().empty())
				continue;

			if (app->getOpkFile().compare(0, path.size(), path) == 0) {
				DEBUG("Removing link corresponding to package %s\n",
							app->getOpkFile().c_str());
				section->erase(link);
				if (section - links.begin() == iSection
							&& iLink == (int) section->size()) {
					setLinkIndex(iLink - 1);
				}
				--link;
			}
		}
	}

	/* Remove registered monitors */
	for (auto it = monitors.begin(); it < monitors.end(); ++it) {
		if ((*it)->getPath().compare(0, path.size(), path) == 0) {
			monitors.erase(it);
		}
	}
}
#endif
#endif

static bool compare_links(unique_ptr<Link> const& a, unique_ptr<Link> const& b)
{
	LinkApp *app1 = dynamic_cast<LinkApp *>(a.get());
	LinkApp *app2 = dynamic_cast<LinkApp *>(b.get());
	bool app1_is_opk = app1 && app1->isOpk(),
		 app2_is_opk = app2 && app2->isOpk();

	if (app1_is_opk && !app2_is_opk) {
		return false;
	}
	if (app2_is_opk && !app1_is_opk) {
		return true;
	}
	return a->getTitle().compare(b->getTitle()) < 0;
}

void Menu::orderLinks()
{
	for (auto& section : links) {
		sort(section.begin(), section.end(), compare_links);
	}
}

void Menu::readLinks()
{
	iLink = 0;
	iFirstDispRow = 0;

	for (uint i=0; i<links.size(); i++) {
		links[i].clear();

		int correct = (i>sections.size() ? iSection : i);
		string const& section = sections[correct];

		readLinksOfSection(
				links[i], GMENU2X_SYSTEM_DIR "/sections/" + section, false);
		readLinksOfSection(
				links[i], GMenu2X::getHome() + "/sections/" + section, true);
	}

	orderLinks();
}

void Menu::readLinksOfSection(
		vector<unique_ptr<Link>>& links, string const& path, bool deletable)
{
	DIR *dirp = opendir(path.c_str());
	if (!dirp) return;

	while (struct dirent *dptr = readdir(dirp)) {
		if (dptr->d_type != DT_REG) continue;
		string linkfile = path + '/' + dptr->d_name;

		LinkApp *link = new LinkApp(gmenu2x, linkfile, deletable);
		if (link->targetExists()) {
			link->setSize(
					gmenu2x.skinConfInt["linkWidth"],
					gmenu2x.skinConfInt["linkHeight"]);
			links.emplace_back(link);
		} else {
			delete link;
		}
	}

	closedir(dirp);
}
