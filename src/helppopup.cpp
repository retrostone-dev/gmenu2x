// Various authors.
// License: GPL version 2 or later.

#include "helppopup.h"

#include "gmenu2x.h"


HelpPopup::HelpPopup(GMenu2X& gmenu2x)
	: gmenu2x(gmenu2x)
{
}

void HelpPopup::paint(Surface& s) {
	Font& font = *gmenu2x.font;
	Translator &tr = gmenu2x.tr;

	static const char *const strings[] = {
		"CONTROLS",
		"A: Launch link / Confirm action",
		"B: Show this help menu",
		"L, R: Change section",
		"SELECT: Show contextual menu",
		"START: Show options menu",
	};

	unsigned int nb_strings = sizeof(strings) / sizeof(strings[0]);
	unsigned int spacing = font.getLineSpacing();
	unsigned int helpBoxHeight = 20 + nb_strings * spacing;

	unsigned int posY = gmenu2x.getContentArea().first;

	/* Center the popup */
	posY += (gmenu2x.height() - posY - helpBoxHeight) / 2;

	s.box(10, posY, gmenu2x.width() - 20, helpBoxHeight + 4,
			gmenu2x.skinConfColors[COLOR_MESSAGE_BOX_BG]);
	s.rectangle(12, posY + 2, gmenu2x.width() - 24, helpBoxHeight,
			gmenu2x.skinConfColors[COLOR_MESSAGE_BOX_BORDER]);

	posY += 10;

	for (unsigned int i = 0; i < nb_strings; i++) {
		font.write(s, tr[strings[i]], 20, posY);
		posY += spacing;
	}
}

bool HelpPopup::handleButtonPress(InputManager::Button button) {
	if (button == InputManager::CANCEL) {
		dismiss();
	}
	return true;
}
