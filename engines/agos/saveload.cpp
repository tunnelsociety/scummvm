/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "common/file.h"
#include "common/savefile.h"
#include "common/textconsole.h"
#include "common/translation.h"

#include "gui/message.h"

#include "agos/agos.h"
#include "agos/intern.h"

namespace AGOS {


// FIXME: This code counts savegames, but callers in many cases assume
// that the return value + 1 indicates an empty slot.
int AGOSEngine::countSaveGames() {
	uint numSaveGames = 1;
	int slotNum;
	bool marks[256];

	// Get the name of (possibly non-existent) savegame slot 998, and replace
	// the extension by * to get a pattern.
	Common::String tmp = genSaveName(998);
	assert(tmp.size() >= 4 && tmp[tmp.size()-4] == '.');
	Common::String prefix = Common::String(tmp.c_str(), tmp.size()-3) + "*";
	Common::StringArray filenames = _saveFileMan->listSavefiles(prefix);

	memset(marks, false, 256 * sizeof(bool));	//assume no savegames for this title

	for (const auto &filename : filenames){
		// Obtain the last 3 digits of the filename, since they correspond to the save slot
		assert(filename.size() >= 4);
		slotNum = atoi(filename.c_str() + filename.size() - 3);
		if (slotNum >= 0 && slotNum < 256)
			marks[slotNum] = true;	//mark this slot as valid
	}

	// locate first empty slot
	for (uint s = 1; s < 256; s++) {
		if (marks[s])
			numSaveGames++;
	}

	return numSaveGames;
}

#ifdef ENABLE_AGOS2
Common::String AGOSEngine_PuzzlePack::genSaveName(int slot) const {
	if (getGameId() == GID_DIMP)
		return "dimp.sav";
	else
		return "swampy.sav";
}

Common::String AGOSEngine_Feeble::genSaveName(int slot) const {
	return Common::String::format("feeble.%.3d", slot);
}
#endif

Common::String AGOSEngine_Simon2::genSaveName(int slot) const {
	return Common::String::format("simon2.%.3d", slot);
}

Common::String AGOSEngine_Simon1::genSaveName(int slot) const {
	if (_gameDescription->desc.flags & ADGF_DEMO)
		return Common::String::format("simon1-demo.%.3d", slot);
	else
		return Common::String::format("simon1.%.3d", slot);
}

Common::String AGOSEngine_Waxworks::genSaveName(int slot) const {
	if (getPlatform() == Common::kPlatformDOS)
		return Common::String::format("waxworks-pc.%.3d", slot);
	else
		return Common::String::format("waxworks.%.3d", slot);
}

Common::String AGOSEngine_Elvira2::genSaveName(int slot) const {
	if (getPlatform() == Common::kPlatformDOS)
		return Common::String::format("elvira2-pc.%.3d", slot);
	else
		return Common::String::format("elvira2.%.3d", slot);
}

Common::String AGOSEngine_Elvira1::genSaveName(int slot) const {
	return Common::String::format("elvira1.%.3d", slot);
}

Common::String AGOSEngine::genSaveName(int slot) const {
	return Common::String::format("pn.%.3d", slot);
}

#ifdef ENABLE_AGOS2
void AGOSEngine_Feeble::quickLoadOrSave() {
	// Quick loading and saving isn't possible in The Feeble Files or Puzzle Pack.
}
#endif

// The function uses segments of code from the original game scripts
// to allow quick loading and saving, but isn't perfect.
//
// Unfortuntely this allows loading and saving in locations,
// which aren't supported, and will not restore correctly:
// Various locations in Elvira 1/2 and Waxworks where saving
// was disabled
void AGOSEngine::quickLoadOrSave() {
	bool success;
	Common::U32String buf;

	// Disable loading and saving when it was not possible in the original:
	// In overhead maps areas in Simon the Sorcerer 2
	// In the floppy disk demo of Simon the Sorcerer 1
	// In copy protection, conversations and cut scenes
	if ((getGameType() == GType_SIMON2 && _boxStarHeight == 200) ||
		(getGameType() == GType_SIMON1 && (getFeatures() & GF_DEMO)) ||
		_mouseHideCount || _showPreposition) {
		buf = _("Quick load or save game isn't supported in this location");
		GUI::MessageDialog dialog(buf);
		dialog.runModal();
		return;
	}

	// Check if Simon is walking, and stop when required
	if (getGameType() == GType_SIMON1 && getBitFlag(11)) {
		vcStopAnimation(11, 1122);
		animate(4, 11, 1122, 0, 0, 2);
		waitForSync(1122);
	} else if (getGameType() == GType_SIMON2 && getBitFlag(11)) {
		vcStopAnimation(11, 232);
		animate(4, 11, 232, 0, 0, 2);
		waitForSync(1122);
	}

	Common::String filename = genSaveName(_saveLoadSlot);
	if (_saveLoadType == 2) {
		Subroutine *sub;
		success = loadGame(genSaveName(_saveLoadSlot));
		if (!success) {
			buf = Common::U32String::format(_("Failed to load saved game from file:\n\n%s"), filename.c_str());
		} else if (getGameType() == GType_SIMON1 || getGameType() == GType_SIMON2) {
			drawIconArray(2, me(), 0, 0);
			setBitFlag(97, true);
			sub = getSubroutineByID(100);
			startSubroutine(sub);
		} else if (getGameType() == GType_WW) {
			sub = getSubroutineByID(66);
			startSubroutine(sub);
		} else if (getGameType() == GType_ELVIRA2) {
			sub = getSubroutineByID(87);
			startSubroutine(sub);
			setBitFlag(7, false);
			sub = getSubroutineByID(19);
			startSubroutine(sub);
			printStats();
			sub = getSubroutineByID(28);
			startSubroutine(sub);
			setBitFlag(17, false);
			sub = getSubroutineByID(207);
			startSubroutine(sub);
			sub = getSubroutineByID(71);
			startSubroutine(sub);
		} else if (getGameType() == GType_ELVIRA1) {
			drawIconArray(2, me(), 0, 0);
			sub = getSubroutineByID(265);
			startSubroutine(sub);
			sub = getSubroutineByID(129);
			startSubroutine(sub);
			sub = getSubroutineByID(131);
			startSubroutine(sub);
		}
	} else {
		success = saveGame(_saveLoadSlot, _saveLoadName);
		if (!success)
			buf = Common::U32String::format(_("Failed to save game to file:\n\n%s"), filename.c_str());
	}

	if (!success) {
		GUI::MessageDialog dialog(buf);
		dialog.runModal();

	} else if (_saveLoadType == 1) {
		buf = Common::U32String::format(_("Successfully saved game in file:\n\n%s"), filename.c_str());
		GUI::TimedMessageDialog dialog(buf, 1500);
		dialog.runModal();

	}

	_saveLoadType = 0;
}

bool AGOSEngine_Waxworks::confirmOverWrite(WindowBlock *window) {
	Subroutine *sub = getSubroutineByID(80);
	if (sub != nullptr)
		startSubroutineEx(sub);

	if (_variableArray[253] == 0)
		return true;

	return false;
}

bool AGOSEngine_Elvira2::confirmOverWrite(WindowBlock *window) {
	// Original version never confirmed
	return true;
}

bool AGOSEngine::confirmOverWrite(WindowBlock *window) {
	const char *message1, *message2, *message3;

	switch (_language) {
	case Common::FR_FRA:
		message1 = "\rFichier d/j; existant.\r\r";
		message2 = "  Ecrire pardessus ?\r\r";
		message3 = "     Oui      Non";
		break;
	case Common::DE_DEU:
		message1 = "\rDatei existiert bereits.\r\r";
		message2 = "   Ueberschreiben ?\r\r";
		message3 = "     Ja        Nein";
		break;
	case Common::JA_JPN:
		message1 = "\r   ""\x82\xbb\x82\xcc\x83""t""\x83""@""\x83""C""\x83\x8b\x82\xcd\x82\xb7\x82\xc5\x82\xc9\x91\xb6\x8d\xdd\x82\xb5\x82\xdc\x82\xb7""\r\r";
		message2 = "     ""\x8f\xe3\x8f\x91\x82\xab\x82\xb5\x82\xc4\x82\xe6\x82\xeb\x82\xb5\x82\xa2\x82\xc5\x82\xb7\x82\xa9\x81""H\r\r";
		message3 = "       ""\x82\xcd\x82\xa2""           ""\x82\xa2\x82\xa2\x82\xa6";
		break;
	default:
		message1 = "\r File already exists.\r\r";
		message2 = "    Overwrite it ?\r\r";
		message3 = "     Yes       No";
		break;
	}

	printScroll();
	window->textColumn = 0;
	window->textRow = 0;
	window->textColumnOffset = 0;
	window->textLength = 0;		// Difference

	for (; *message1; message1++)
		windowPutChar(window, *message1);
	for (; *message2; message2++)
		windowPutChar(window, *message2);
	for (; *message3; message3++)
		windowPutChar(window, *message3);

	if (confirmYesOrNo(120, 78) == 0x7FFF)
		return true;

	return false;
}

int16 AGOSEngine::matchSaveGame(const char *name, uint16 max) {
	Common::InSaveFile *in;
	char dst[10];
	uint16 slot;

	memset(dst, 0, sizeof(dst));
	for (slot = 0; slot < max; slot++) {
		if ((in = _saveFileMan->openForLoading(genSaveName(slot)))) {
			in->read(dst, 8);
			delete in;

			if (!scumm_stricmp(name, dst)) {
				return slot;
			}
		}
	}

	return -1;
}

void AGOSEngine::enterSaveLoadScreen(bool entering) {
	_system->setFeatureState(OSystem::kFeatureVirtualKeyboard, entering);
	getEventManager()->getKeymapper()->getKeymap("game-shortcuts")->setEnabled(!entering);
}

void AGOSEngine::userGame(bool load) {
	WindowBlock *window = _windowArray[4];
	const char *message1;
	int i = 0, numSaveGames;
	char *name;
	memset(_saveBuf, 0, sizeof(_saveBuf));

	numSaveGames = countSaveGames();

	uint32 saveTime = getTime();
	haltAnimation();

restart:
	printScroll();
	window->textColumn = 0;
	window->textRow = 0;
	window->textColumnOffset = 0;
	window->textLength = 0;		// Difference

	switch (_language) {
	case Common::FR_FRA:
		message1 = "\rIns/rez disquette de\rsauvegarde de jeux &\rentrez nom de fichier:\r\r   ";
		break;
	case Common::DE_DEU:
		message1 = "\rLege Spielstandsdiskette ein. Dateinamen eingeben:\r\r   ";
		break;
	case Common::JA_JPN:
		message1 = "\r  ""\x83""t""\x83""@""\x83""C""\x83\x8b\x96\xbc\x82\xf0\x93\xfc\x97\xcd\x82\xb5\x82\xc4\x82\xad\x82\xbe\x82\xb3\x82\xa2\x81""F\r\r\r   ";
		break;
	default:
		message1 = "\r Insert savegame data disk & enter filename:\r\r   ";
		break;
	}

	clearHiResTextLayer();
	for (; *message1; message1++)
		windowPutChar(window, *message1);

	memset(_saveBuf, 0, 10);
	name = _saveBuf;
	_saveGameNameLen = 0;
	_forceAscii = true;

	while (!shouldQuit()) {
		windowPutChar(window, 128);
		_keyPressed.reset();

		while (!shouldQuit()) {
			delay(10);
			if (_keyPressed.ascii && _keyPressed.ascii < 128) {
				i = _keyPressed.ascii;
				break;
			}
		}

		userGameBackSpace(_windowArray[4], 8);
		if (i == 10 || i == 13) {
			break;
		} else if (i == 8) {
			// do_backspace
			if (_saveGameNameLen) {
				_saveGameNameLen--;
				name[_saveGameNameLen] = 0;
				userGameBackSpace(_windowArray[4], 8);
			}
		} else if (i >= 32 && _saveGameNameLen != 8) {
			name[_saveGameNameLen++] = i;
			windowPutChar(_windowArray[4], i);
		}
	}

	_forceAscii = false;

	if (_saveGameNameLen != 0) {
		int16 slot = matchSaveGame(name, numSaveGames);
		if (!load) {
			if (slot >= 0 && !confirmOverWrite(window))
				goto restart;

			if (slot < 0)
				slot = numSaveGames;

			if (!saveGame(slot, name))
				fileError(_windowArray[4], true);
		} else {
			if (slot < 0) {
				fileError(_windowArray[4], false);
			} else {
				if (!loadGame(genSaveName(slot)))
					fileError(_windowArray[4], false);
			}
		}

		printStats();
	}

	clearHiResTextLayer();
	restartAnimation();
	_gameStoppedClock = getTime() - saveTime + _gameStoppedClock;
}

void AGOSEngine_Elvira2::listSaveGames() {
	Common::InSaveFile *in;
	uint y, slot;
	char *dst = _saveBuf;

	const uint8 num = (getGameType() == GType_WW) ? 3 : 4;

	disableFileBoxes();

	WindowBlock *window = _windowArray[num];
	window->textRow = 0;
	window->textColumn = 0;
	window->textColumnOffset = 4;

	windowPutChar(window, 12);

	memset(dst, 0, 200);

	slot = _saveLoadRowCurPos;
	for (y = 0; y < 8; y++) {
		window->textColumn = 0;
		window->textColumnOffset = (getGameType() == GType_ELVIRA2) ? 4 : 0;
		window->textLength = 0;
		if ((in = _saveFileMan->openForLoading(genSaveName(slot++)))) {
			in->read(dst, 8);
			delete in;

			const char *name = dst;
			for (; *name; name++)
				windowPutChar(window, *name);

			enableBox(200 + y * 3 + 0);
		}
		dst+= 8;

		if (getGameType() == GType_WW) {
			window->textColumn = 7;
			window->textColumnOffset = 4;
		} else if (getGameType() == GType_ELVIRA2) {
			window->textColumn = 8;
			window->textColumnOffset = 0;
		}
		window->textLength = 0;
		if ((in = _saveFileMan->openForLoading(genSaveName(slot++)))) {
			in->read(dst, 8);
			delete in;

			const char *name = dst;
			for (; *name; name++)
				windowPutChar(window, *name);

			enableBox(200 + y * 3 + 1);
		}
		dst+= 8;

		window->textColumn = 15;
		window->textColumnOffset = (getGameType() == GType_ELVIRA2) ? 4 : 0;
		window->textLength = 0;
		if ((in = _saveFileMan->openForLoading(genSaveName(slot++)))) {
			in->read(dst, 8);
			delete in;

			const char *name = dst;
			for (; *name; name++)
				windowPutChar(window, *name);

			enableBox(200 + y * 3 + 2);
		}
		dst+= 8;

		windowPutChar(window, 13);
	}

	window->textRow = 9;
	window->textColumn = 0;
	window->textColumnOffset = 4;
	window->textLength = 0;

	_saveGameNameLen = 0;
}

void AGOSEngine_Elvira2::userGame(bool load) {
	uint32 saveTime;
	int i, numSaveGames;
	char *name;
	bool b;
	memset(_saveBuf, 0, sizeof(_saveBuf));

	_saveOrLoad = load;

	saveTime = getTime();

	if (getGameType() == GType_ELVIRA2)
		haltAnimation();

	numSaveGames = countSaveGames();
	_numSaveGameRows = numSaveGames;
	_saveLoadRowCurPos = 1;
	_saveLoadEdit = false;

	const uint8 num = (getGameType() == GType_WW) ? 3 : 4;

	listSaveGames();

	if (!load) {
		WindowBlock *window = _windowArray[num];
		int16 slot = -1;

		name = _saveBuf + 192;

		while (!shouldQuit()) {
			windowPutChar(window, 128);

			_saveLoadEdit = true;

			i = userGameGetKey(&b, 128);
			if (b) {
				if (i <= 23) {
					if (!confirmOverWrite(window)) {
						listSaveGames();
						continue;
					}

					if (!saveGame(_saveLoadRowCurPos + i, _saveBuf + i * 8))
						fileError(_windowArray[num], true);
				}

				goto get_out;
			}

			userGameBackSpace(_windowArray[num], 8);
			if (i == 10 || i == 13) {
				slot = matchSaveGame(name, numSaveGames);
				if (slot >= 0) {
					if (!confirmOverWrite(window)) {
						listSaveGames();
						continue;
					}
				}
				break;
			} else if (i == 8) {
				// do_backspace
				if (_saveGameNameLen) {
					_saveGameNameLen--;
					name[_saveGameNameLen] = 0;
					userGameBackSpace(_windowArray[num], 8);
				}
			} else if (i >= 32 && _saveGameNameLen != 8) {
				name[_saveGameNameLen++] = i;
				windowPutChar(_windowArray[num], i);
			}
		}

		if (_saveGameNameLen != 0) {
			if (slot < 0)
				slot = numSaveGames;

			if (!saveGame(slot, _saveBuf + 192))
				fileError(_windowArray[num], true);
		}
	} else {
		i = userGameGetKey(&b, 128);
		if (i != 225) {
			if (!loadGame(genSaveName(_saveLoadRowCurPos + i)))
				fileError(_windowArray[num], false);
		}
	}

get_out:;
	disableFileBoxes();

	_gameStoppedClock = getTime() - saveTime + _gameStoppedClock;

	if (getGameType() == GType_ELVIRA2)
		restartAnimation();
}

int AGOSEngine_Elvira2::userGameGetKey(bool *b, uint maxChar) {
	HitArea *ha;
	*b = true;

	_keyPressed.reset();

	while (!shouldQuit()) {
		_lastHitArea = nullptr;
		_lastHitArea3 = nullptr;

		do {
			if (_saveLoadEdit && _keyPressed.ascii && _keyPressed.ascii < maxChar) {
				*b = false;
				return _keyPressed.ascii;
			}
			delay(10);
		} while (_lastHitArea3 == nullptr && !shouldQuit());

		ha = _lastHitArea;
		if (ha == nullptr || ha->id < 200) {
		} else if (ha->id == 225) {
			return ha->id;
		} else if (ha->id == 224) {
			_saveGameNameLen = 0;
			_saveLoadRowCurPos += 24;
			if (_saveLoadRowCurPos >= _numSaveGameRows)
				_saveLoadRowCurPos = 1;

			listSaveGames();
		} else if (ha->id < 224) {
			return ha->id - 200;
		}
	}

	return 225;
}

void AGOSEngine_Simon1::listSaveGames() {
	Common::InSaveFile *in;
	uint16 i, slot, lastSlot;
	char *dst = _saveBuf;

	disableFileBoxes();

	showMessageFormat("\xC");

	memset(dst, 0, 108);

	slot = _saveLoadRowCurPos;
	while (_saveLoadRowCurPos + 6 > slot) {
		if (!(in = _saveFileMan->openForLoading(genSaveName(slot))))
			break;

		in->read(dst, 18);
		delete in;

		lastSlot = slot;
		if (slot < 10) {
			showMessageFormat(" ");
		} else if (_language == Common::HE_ISR) {
			lastSlot = (slot % 10) * 10;
			lastSlot += slot / 10;
		}

		showMessageFormat("%d", lastSlot);
		if (_language == Common::HE_ISR && !(slot % 10))
			showMessageFormat("0");
		showMessageFormat(".%s\n", dst);
		dst += 18;
		slot++;
	}

	if (!_saveOrLoad) {
		if (_saveLoadRowCurPos + 6 == slot) {
			slot++;
		} else {
			if (slot < 10)
				showMessageFormat(" ");
			showMessageFormat("%d.\n", slot);
		}
	} else {
		if (_saveLoadRowCurPos + 6 == slot) {
			if ((in = _saveFileMan->openForLoading(genSaveName(slot)))) {
				slot++;
				delete in;
			}
		}
	}

	_saveDialogFlag = true;

	i = slot - _saveLoadRowCurPos;
	if (i != 7) {
		i++;
		if (!_saveOrLoad)
			i++;
		_saveDialogFlag = false;
	}

	if (!--i)
		return;

	do {
		enableBox(208 + i - 1);
	} while (--i);
}

const byte hebrewKeyTable[96] = {
	32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 90, 45, 85, 47, 48, 49, 50,
	51, 52, 53, 54, 55, 56, 57, 83, 83, 90, 61, 85, 63, 35, 89, 80, 65, 66, 87,
	75, 82, 73, 79, 71, 76, 74, 86, 78, 77, 84, 47, 88, 67, 64, 69, 68, 44, 81,
	72, 70, 91, 92, 93, 94, 95, 96, 89, 80, 65, 66, 87, 75, 82, 73, 79, 71, 76,
	74, 86, 78, 77, 84, 47, 88, 67, 64, 69, 68, 44, 81, 72, 70,
	123, 124, 125, 126, 127,
};

void AGOSEngine_Simon1::userGame(bool load) {
	uint32 saveTime;
	int i, numSaveGames, result;
	WindowBlock *window;
	char *name;
	bool b;
	memset(_saveBuf, 0, sizeof(_saveBuf));
	int maxChar = (_language == Common::HE_ISR) ? 155: 128;

	_saveOrLoad = load;

	saveTime = getTime();

	numSaveGames = countSaveGames();
	if (!load)
		numSaveGames++;
	numSaveGames -= 6;
	if (numSaveGames < 0)
		numSaveGames = 0;
	numSaveGames++;
	_numSaveGameRows = numSaveGames;

	_saveLoadRowCurPos = 1;
	if (!load)
		_saveLoadRowCurPos = numSaveGames;

	_saveLoadEdit = false;

restart:;
	i = userGameGetKey(&b, maxChar);

	if (i == 205)
		goto get_out;
	if (!load) {
		// if_1
	if_1:;
		result = i;

		disableBox(208 + i);
		leaveHitAreaById(208 + i);

		window = _windowArray[5];

		window->textRow = result;

		// init x offset with a 2 character savegame number + a period (18 pix)
		if (_language == Common::HE_ISR) {
			window->textColumn = 3;
			window->textColumnOffset = 6;
		} else {
			window->textColumn = 2;
			window->textColumnOffset = 2;
		}
		window->textLength = 3;

		name = _saveBuf + i * 18;

		// now process entire savegame name to get correct x offset for cursor
		_saveGameNameLen = 0;
		while (name[_saveGameNameLen]) {
			if (_language == Common::HE_ISR) {
				byte width = 6;
				if (name[_saveGameNameLen] >= 64 && name[_saveGameNameLen] < 91)
					width = _hebrewCharWidths [name[_saveGameNameLen] - 64];
				window->textLength++;
				window->textColumnOffset -= width;
				if (window->textColumnOffset < width) {
					window->textColumnOffset += 8;
					window->textColumn++;
				}
			} else {
				window->textLength++;
				window->textColumnOffset += 6;
				if (name[_saveGameNameLen] == 'i' || name[_saveGameNameLen] == 'l')
					window->textColumnOffset -= 2;
				if (window->textColumnOffset >= 8) {
					window->textColumnOffset -= 8;
					window->textColumn++;
				}
			}
			_saveGameNameLen++;
		}

		while (!shouldQuit()) {
			windowPutChar(window, 127);

			_saveLoadEdit = true;

			i = userGameGetKey(&b, maxChar);

			if (b) {
				if (i == 205)
					goto get_out;
				enableBox(208 + result);
				if (_saveLoadEdit) {
					userGameBackSpace(_windowArray[5], 8);
				}
				goto if_1;
			}

			if (!_saveLoadEdit) {
				enableBox(208 + result);
				goto restart;
			}

			if (_language == Common::HE_ISR) {
				if (i >= 128)
					i -= 64;
				else if (i >= 32)
					i = hebrewKeyTable[i - 32];
			}

			userGameBackSpace(_windowArray[5], 8);
			if (i == 10 || i == 13) {
				break;
			} else if (i == 8) {
				// do_backspace
				if (_saveGameNameLen) {
					byte m, x;

					_saveGameNameLen--;
					m = name[_saveGameNameLen];

					if (_language == Common::HE_ISR)
						x = 8;
					else
						x = (name[_saveGameNameLen] == 'i' || name[_saveGameNameLen] == 'l') ? 1 : 8;

					name[_saveGameNameLen] = 0;

					userGameBackSpace(_windowArray[5], x, m);
				}
			} else if (i >= 32 && _saveGameNameLen != 17) {
				name[_saveGameNameLen++] = i;

				windowPutChar(_windowArray[5], i);
			}
		}

		if (!saveGame(_saveLoadRowCurPos + result, _saveBuf + result * 18))
			fileError(_windowArray[5], true);
	} else {
		if (!loadGame(genSaveName(_saveLoadRowCurPos + i)))
			fileError(_windowArray[5], false);
	}

get_out:;
	disableFileBoxes();

	_gameStoppedClock = getTime() - saveTime + _gameStoppedClock;
}

int AGOSEngine_Simon1::userGameGetKey(bool *b, uint maxChar) {
	HitArea *ha;
	*b = true;

	if (!_saveLoadEdit) {
		listSaveGames();
	}

	_keyPressed.reset();

	while (!shouldQuit()) {
		_lastHitArea = nullptr;
		_lastHitArea3 = nullptr;

		do {
			if (_saveLoadEdit && _keyPressed.ascii && _keyPressed.ascii < maxChar) {
				*b = false;
				return _keyPressed.ascii;
			}
			delay(10);
		} while (_lastHitArea3 == nullptr && !shouldQuit());

		ha = _lastHitArea;
		if (ha == nullptr || ha->id < 205) {
		} else if (ha->id == 205) {
			return ha->id;
		} else if (ha->id == 206) {
			if (_saveLoadRowCurPos != 1) {
				if (_saveLoadRowCurPos < 7)
					_saveLoadRowCurPos = 1;
				else
					_saveLoadRowCurPos -= 6;

				_saveLoadEdit = false;
				listSaveGames();
			}
		} else if (ha->id == 207) {
			if (_saveDialogFlag) {
				_saveLoadRowCurPos += 6;
				if (_saveLoadRowCurPos >= _numSaveGameRows)
					_saveLoadRowCurPos = _numSaveGameRows;

				_saveLoadEdit = false;
				listSaveGames();
			}
		} else if (ha->id < 214) {
			return ha->id - 208;
		}
	}

	return 205;
}

void AGOSEngine::disableFileBoxes() {
	int i;

	if (getGameType() == GType_SIMON1 || getGameType() == GType_SIMON2) {
		for (i = 208; i != 214; i++)
			disableBox(i);
	} else {
		for (i = 200; i != 224; i++)
			disableBox(i);
	}
}

void AGOSEngine::userGameBackSpace(WindowBlock *window, int x, byte b) {
	byte oldTextColor;

	windowPutChar(window, x, b);
	oldTextColor = window->textColor;
	window->textColor = window->fillColor;

	if (_language == Common::HE_ISR) {
		x = 128;
	} else {
		x += 120;
		if (x != 128)
			x = 129;
	}

	windowPutChar(window, x);

	window->textColor = oldTextColor;
	windowPutChar(window, 8);
}

void AGOSEngine::fileError(WindowBlock *window, bool saveError) {
	const char *message1, *message2;

	if (saveError) {
		switch (_language) {
		case Common::RU_RUS:
			if (getGameType() == GType_SIMON2) {
				message1 = "\r   Mf sowrap+fts+.";
				message2 = "\r  Nzjb#a ejs#a.";
			} else {
				message1 = "\r   Mf sowrap]fts].";
				message2 = "\r   Nzjb_a ejs_a.";
			}
			break;
		case Common::PL_POL:
			message1 = "\r      Blad zapisu.    ";
			message2 = "\rBlad dysku.                       ";
			break;
		case Common::ES_ESP:
			message1 = "\r     Error al salvar";
			message2 = "\r  Intenta con otro disco";
			break;
		case Common::IT_ITA:
			message1 = "\r  Salvataggio non riuscito";
			message2 = "\r    Prova un""\x27""altro disco";
			break;
		case Common::FR_FRA:
			message1 = "\r    Echec sauvegarde";
			message2 = "\rEssayez une autre disquette";
			break;
		case Common::DE_DEU:
			message1 = "\r  Sicherung erfolglos.";
			message2 = "\rVersuche eine andere     Diskette.";
			break;
		case Common::JA_JPN:
			message1 = "\r       ""\x83""Z""\x81""[""\x83""u""\x82\xc9\x8e\xb8\x94""s""\x82\xb5\x82\xdc\x82\xb5\x82\xbd";
			message2 = "\r   ""\x95\xca\x82\xcc\x83""f""\x83""B""\x83""X""\x83""N""\x82\xf0\x8e""g""\x97""p""\x82\xb5\x82\xc4\x82\xad\x82\xbe\x82\xb3\x82\xa2";
			break;
		default:
			message1 = "\r       Save failed.";
			message2 = "\r       Disk error.";
			break;
		}
	} else {
		switch (_language) {
		case Common::RU_RUS:
			if (getGameType() == GType_SIMON2) {
				message1 = "\r  Mf ^adruhafts+.";
				message2 = "\r   Takm pf pakefp.";
			} else {
				message1 = "\r   Mf ^adruhafts].";
				message2 = "\r   Takm pf pakefp.";
			}
			break;
		case Common::PL_POL:
			message1 = "\r   Blad odczytu.    ";
			message2 = "\r  Nie znaleziono pliku.";
			break;
		case Common::ES_ESP:
			message1 = "\r     Error al cargar";
			message2 = "\r  Archivo no encontrado";
			break;
		case Common::IT_ITA:
			message1 = "\r  Caricamento non riuscito";
			message2 = "\r      File non trovato";
			break;
		case Common::FR_FRA:
			message1 = "\r    Echec chargement";
			message2 = "\r  Fichier introuvable";
			break;
		case Common::DE_DEU:
			message1 = "\r    Laden erfolglos.";
			message2 = "\r  Datei nicht gefunden.";
			break;
		case Common::JA_JPN:
			message1 = "\r       ""\x83\x8d\x81""[""\x83""h""\x82\xc9\x8e\xb8\x94""s""\x82\xb5\x82\xdc\x82\xb5\x82\xbd";
			message2 = "\r     ""\x83""t""\x83""@""\x83""C""\x83\x8b\x82\xaa\x8c\xa9\x82\xc2\x82\xa9\x82\xe8\x82\xdc\x82\xb9\x82\xf1";
			break;
		default:
			message1 = "\r       Load failed.";
			message2 = "\r     File not found.";
			break;
		}
	}

	if (getGameType() == GType_ELVIRA1) {
		printScroll();
		window->textColumn = 0;
		window->textRow = 0;
		window->textColumnOffset = 0;
		window->textLength = 0;		// Difference
	} else {
		windowPutChar(window, 12);
	}

	for (; *message1; message1++)
		windowPutChar(window, *message1);
	for (; *message2; message2++)
		windowPutChar(window, *message2);

	waitWindow(window);
}

uint16 readItemID(Common::SeekableReadStream *f) {
	uint32 val = f->readUint32BE();
	if (val == 0xFFFFFFFF)
		return 0;
	return val + 1;
}

void writeItemID(Common::WriteStream *f, uint16 val) {
	if (val == 0)
		f->writeUint32BE(0xFFFFFFFF);
	else
		f->writeUint32BE(val - 1);
}

bool AGOSEngine::loadGame(const Common::String &filename, bool restartMode) {
	char ident[100];
	Common::SeekableReadStream *f = nullptr;
	uint num, item_index, i;

	_videoLockOut |= 0x100;

	if (restartMode) {
		// Load restart state
		if (getPlatform() == Common::kPlatformPC98 && !filename.compareToIgnoreCase("start")) {
			f = createPak98FileStream("START.PAK");
		} else {
			Common::File *file = new Common::File();
			if (!file->open(Common::Path(filename))) {
				delete file;
				file = nullptr;
			}
			f = file;
		}
	} else {
		f = _saveFileMan->openForLoading(filename);
	}

	if (f == nullptr) {
		_videoLockOut &= ~0x100;
		return false;
	}

	if (!restartMode) {
		f->read(ident, 8);
	}

	num = f->readUint32BE();

	if (f->readUint32BE() != 0xFFFFFFFF || num != _itemArrayInited - 1) {
		delete f;
		_videoLockOut &= ~0x100;
		return false;
	}

	f->readUint32BE();
	f->readUint32BE();
	_noParentNotify = true;

	// add all timers
	killAllTimers();
	for (num = f->readUint32BE(); num; num--) {
		// See comment below inAGOSEngine_Elvira2::loadGame(): The timers are just as broken for Elvira as for the other games.
		int32 timeout = (int16)f->readSint32BE();
		uint16 subroutine_id = f->readUint16BE();
		addTimeEvent(timeout, subroutine_id);
	}

	item_index = 1;
	for (num = _itemArrayInited - 1; num; num--) {
		Item *item = _itemArrayPtr[item_index++], *parent_item;

		parent_item = derefItem(readItemID(f));
		setItemParent(item, parent_item);

		item->state = f->readUint16BE();
		item->classFlags = f->readUint16BE();

		SubObject *o = (SubObject *)findChildOfType(item, kObjectType);
		if (o) {
			o->objectSize = f->readUint16BE();
			o->objectWeight = f->readUint16BE();
		}

		SubPlayer *p = (SubPlayer *)findChildOfType(item, kPlayerType);
		if (p) {
			p->score = f->readUint32BE();
			p->level = f->readUint16BE();
			p->size = f->readUint16BE();
			p->weight = f->readUint16BE();
			p->strength = f->readUint16BE();
		}

		SubUserFlag *u = (SubUserFlag *)findChildOfType(item, kUserFlagType);
		if (u) {
			for (i = 0; i != 8; i++) {
				u->userFlags[i] = f->readUint16BE();
			}
			u->userItems[0] = readItemID(f);
		}
	}

	// read the variables
	for (i = 0; i != _numVars; i++) {
		writeVariable(i, f->readUint16BE());
	}

	if (f->err()) {
		error("load failed");
	}

	delete f;

	_noParentNotify = false;

	_videoLockOut &= ~0x100;

	return true;
}

bool AGOSEngine::saveGame(uint slot, const char *caption) {
	Common::OutSaveFile *f;
	uint item_index, num_item, i;
	TimeEvent *te;
	uint32 curTime = getTime();
	uint32 gsc = _gameStoppedClock;

	_videoLockOut |= 0x100;

	f = _saveFileMan->openForSaving(genSaveName(slot));
	if (f == nullptr) {
		_videoLockOut &= ~0x100;
		return false;
	}

	f->write(caption, 8);

	f->writeUint32BE(_itemArrayInited - 1);
	f->writeUint32BE(0xFFFFFFFF);
	f->writeUint32BE(curTime);
	f->writeUint32BE(0);

	i = 0;
	for (te = _firstTimeStruct; te; te = te->next)
		i++;
	f->writeUint32BE(i);

	for (te = _firstTimeStruct; te; te = te->next) {
		f->writeUint32BE(te->time - curTime + gsc);
		f->writeUint16BE(te->subroutine_id);
	}

	item_index = 1;
	for (num_item = _itemArrayInited - 1; num_item; num_item--) {
		Item *item = _itemArrayPtr[item_index++];

		writeItemID(f, item->parent);

		f->writeUint16BE(item->state);
		f->writeUint16BE(item->classFlags);

		SubObject *o = (SubObject *)findChildOfType(item, kObjectType);
		if (o) {
			f->writeUint16BE(o->objectSize);
			f->writeUint16BE(o->objectWeight);
		}

		SubPlayer *p = (SubPlayer *)findChildOfType(item, kPlayerType);
		if (p) {
			f->writeUint32BE(p->score);
			f->writeUint16BE(p->level);
			f->writeUint16BE(p->size);
			f->writeUint16BE(p->weight);
			f->writeUint16BE(p->strength);
		}

		SubUserFlag *u = (SubUserFlag *)findChildOfType(item, kUserFlagType);
		if (u) {
			for (i = 0; i != 8; i++) {
				f->writeUint16BE(u->userFlags[i]);
			}
			writeItemID(f, u->userItems[0]);
		}
	}

	// write the variables
	for (i = 0; i != _numVars; i++) {
		f->writeUint16BE(readVariable(i));
	}

	f->finalize();
	bool result = !f->err();

	delete f;
	_videoLockOut &= ~0x100;

	return result;
}

bool AGOSEngine_Elvira2::loadGame(const Common::String &filename, bool restartMode) {
	char ident[100];
	Common::SeekableReadStream *f = nullptr;
	uint num, item_index, i, j;

	_videoLockOut |= 0x100;

	if (restartMode) {
		// Load restart state
		Common::File *file = new Common::File();
		if (!file->open(Common::Path(filename))) {
			delete file;
			file = nullptr;
		}
		f = file;
	} else {
		f = _saveFileMan->openForLoading(filename);
	}

	if (f == nullptr) {
		_videoLockOut &= ~0x100;
		return false;
	}

	if (getGameType() == GType_PP) {
		// No caption
	} else if (getGameType() == GType_FF) {
		f->read(ident, 100);
	} else if (getGameType() == GType_SIMON1 || getGameType() == GType_SIMON2) {
		f->read(ident, 18);
	} else if (!restartMode) {
		f->read(ident, 8);
	}

	num = f->readUint32BE();

	if (f->readUint32BE() != 0xFFFFFFFF || num != _itemArrayInited - 1) {
		delete f;
		_videoLockOut &= ~0x100;
		return false;
	}

	f->readUint32BE();
	f->readUint32BE();
	_noParentNotify = true;

	// add all timers
	killAllTimers();
	for (num = f->readUint32BE(); num; num--) {
		// WORKAROUND for older (corrupt) savegames. Games with short timer intervals may write negative timeouts into the save files. The
		// original interpreter does that, too. I have checked it in the DOSBox debugger. We didn't handle this well, treating the negative
		// values as very large positive values. This effectively disabled the timers. In most cases this seems to have gone unnoticed, but
		// it also caused bug #14886 ("Waxworks crashing at Egypt Level 3, corrupting save file"). Waxworks runs a timer every 10 seconds
		// that cleans up the items chain and failure to do so causes that bug. The design of the timers in the original interpreter is poor,
		// but at least it somehow survives. Now, unfortunately, we don't have savegame versioning in this engine, so I can't simply limit
		// a fix to old savegames. However, it is so highly unlikely that a valid timer would exceed 32767 seconds (= 9 hours) that I
		// consider this safe.
		int32 timeout = (int16)f->readSint32BE();
		uint16 subroutine_id = f->readUint16BE();
		addTimeEvent(timeout, subroutine_id);
	}

	if (getGameType() == GType_WW && getPlatform() == Common::kPlatformDOS) {
		for (uint s = 0; s < _numRoomStates; s++) {
			_roomStates[s].state = f->readUint16BE();
			_roomStates[s].classFlags = f->readUint16BE();
			_roomStates[s].roomExitStates = f->readUint16BE();
		}
		f->readUint16BE();

		uint16 room = _currentRoom;
		_currentRoom = f->readUint16BE();
		if (_roomsListPtr) {
			byte *p = _roomsListPtr;
			if (room == _currentRoom) {
				for (;;) {
					uint16 minNum = READ_BE_UINT16(p); p += 2;
					if (minNum == 0)
						break;

					uint16 maxNum = READ_BE_UINT16(p); p += 2;

					 for (uint16 z = minNum; z <= maxNum; z++) {
						uint16 itemNum = z + 2;
						Item *item = derefItem(itemNum);

						num = (itemNum - _itemArrayInited);
						item->state = _roomStates[num].state;
						item->classFlags = _roomStates[num].classFlags;
						SubRoom *subRoom = (SubRoom *)findChildOfType(item, kRoomType);
						subRoom->roomExitStates = _roomStates[num].roomExitStates;
					}
				}
			} else {
				for (;;) {
					uint16 minNum = READ_BE_UINT16(p); p += 2;
					if (minNum == 0)
						break;

					uint16 maxNum = READ_BE_UINT16(p); p += 2;

					 for (uint16 z = minNum; z <= maxNum; z++) {
						uint16 itemNum = z + 2;
						_itemArrayPtr[itemNum] = nullptr;
					}
				}
			}
		}

		if (room != _currentRoom) {
			_roomsListPtr = nullptr;
			loadRoomItems(_currentRoom);
		}
	}

	item_index = 1;
	for (num = _itemArrayInited - 1; num; num--) {
		Item *item = _itemArrayPtr[item_index++], *parent_item;

		if ((getGameType() == GType_WW && getPlatform() == Common::kPlatformAmiga) ||
			getGameType() == GType_ELVIRA2) {
			parent_item = derefItem(readItemID(f));
			setItemParent(item, parent_item);
		} else {
			uint parent = f->readUint16BE();
			uint next = f->readUint16BE();

			if (getGameType() == GType_WW && getPlatform() == Common::kPlatformDOS && derefItem(item->parent) == nullptr)
				item->parent = 0;

			parent_item = derefItem(parent);
			setItemParent(item, parent_item);

			if (parent_item == nullptr) {
				item->parent = parent;
				item->next = next;
			}
		}

		item->state = f->readUint16BE();
		item->classFlags = f->readUint16BE();

		SubRoom *r = (SubRoom *)findChildOfType(item, kRoomType);
		if (r) {
			r->roomExitStates = f->readUint16BE();
		}

		SubSuperRoom *sr = (SubSuperRoom *)findChildOfType(item, kSuperRoomType);
		if (sr) {
			uint16 n = sr->roomX * sr->roomY * sr->roomZ;
			for (i = j = 0; i != n; i++)
				sr->roomExitStates[j++] = f->readUint16BE();
		}

		SubObject *o = (SubObject *)findChildOfType(item, kObjectType);
		if (o) {
			o->objectFlags = f->readUint32BE();
			i = o->objectFlags & 1;

			for (j = 1; j < 16; j++) {
				if (o->objectFlags & (1 << j)) {
					o->objectFlagValue[i++] = f->readUint16BE();
				}
			}
		}

		SubUserFlag *u = (SubUserFlag *)findChildOfType(item, kUserFlagType);
		if (u) {
			for (i = 0; i != 4; i++) {
				u->userFlags[i] = f->readUint16BE();
			}
		}
	}

	// read the variables
	for (i = 0; i != _numVars; i++) {
		writeVariable(i, f->readUint16BE());
	}

	// read the items in item store
	for (i = 0; i != _numItemStore; i++) {
		if (getGameType() == GType_WW && getPlatform() == Common::kPlatformAmiga) {
			_itemStore[i] = derefItem(f->readUint16BE() / 16);
		} else if (getGameType() == GType_ELVIRA2) {
			if (getPlatform() == Common::kPlatformDOS) {
				_itemStore[i] = derefItem(readItemID(f));
			} else {
				_itemStore[i] = derefItem(f->readUint16BE() / 18);
			}
		} else {
			_itemStore[i] = derefItem(f->readUint16BE());
		}
	}

	// Read the bits in array 1
	for (i = 0; i != _numBitArray1; i++)
		_bitArray[i] = f->readUint16BE();

	// Read the bits in array 2
	for (i = 0; i != _numBitArray2; i++)
		_bitArrayTwo[i] = f->readUint16BE();

	// Read the bits in array 3
	for (i = 0; i != _numBitArray3; i++)
		_bitArrayThree[i] = f->readUint16BE();

	if (getGameType() == GType_ELVIRA2 || getGameType() == GType_WW) {
		_superRoomNumber = f->readUint16BE();
	}

	if (f->err()) {
		error("load failed");
	}

	delete f;

	_noParentNotify = false;

	_videoLockOut &= ~0x100;

	// The floppy disk versions of Simon the Sorcerer 2 block changing
	// to scrolling rooms, if the copy protection fails. But the copy
	// protection flags are never set in the CD version.
	// Setting this copy protection flag, allows saved games to be shared
	// between all versions of Simon the Sorcerer 2.
	if (getGameType() == GType_SIMON2) {
		setBitFlag(135, 1);
	}

	return true;
}

bool AGOSEngine_Elvira2::saveGame(uint slot, const char *caption) {
	Common::OutSaveFile *f;
	uint item_index, num_item, i, j;
	TimeEvent *te;
	uint32 curTime = getTime();
	uint32 gsc = _gameStoppedClock;

	_videoLockOut |= 0x100;

	f = _saveFileMan->openForSaving(genSaveName(slot));
	if (f == nullptr) {
		_videoLockOut &= ~0x100;
		return false;
	}

	if (getGameType() == GType_PP) {
		// No caption
	} else if (getGameType() == GType_FF) {
		f->write(caption, 100);
	} else if (getGameType() == GType_SIMON1 || getGameType() == GType_SIMON2) {
		f->write(caption, 18);
	} else {
		f->write(caption, 8);
	}

	f->writeUint32BE(_itemArrayInited - 1);
	f->writeUint32BE(0xFFFFFFFF);
	f->writeUint32BE(curTime);
	f->writeUint32BE(0);

	i = 0;
	for (te = _firstTimeStruct; te; te = te->next)
		i++;
	f->writeUint32BE(i);

	if (getGameType() == GType_FF && _clockStopped)
		gsc += (getTime() - _clockStopped);
	for (te = _firstTimeStruct; te; te = te->next) {
		f->writeUint32BE(te->time - curTime + gsc);
		f->writeUint16BE(te->subroutine_id);
	}

	if (getGameType() == GType_WW && getPlatform() == Common::kPlatformDOS) {
		if (_roomsListPtr) {
			byte *p = _roomsListPtr;
			for (;;) {
				uint16 minNum = READ_BE_UINT16(p); p += 2;
				if (minNum == 0)
					break;

				uint16 maxNum = READ_BE_UINT16(p); p += 2;

				 for (uint16 z = minNum; z <= maxNum; z++) {
					uint16 itemNum = z + 2;
					Item *item = derefItem(itemNum);

					uint16 num = (itemNum - _itemArrayInited);
					_roomStates[num].state = item->state;
					_roomStates[num].classFlags = item->classFlags;
					SubRoom *subRoom = (SubRoom *)findChildOfType(item, kRoomType);
					_roomStates[num].roomExitStates = subRoom->roomExitStates;
				}
			}
		}

		for (uint s = 0; s < _numRoomStates; s++) {
			f->writeUint16BE(_roomStates[s].state);
			f->writeUint16BE(_roomStates[s].classFlags);
			f->writeUint16BE(_roomStates[s].roomExitStates);
		}
		f->writeUint16BE(0);
		f->writeUint16BE(_currentRoom);
	}

	item_index = 1;
	for (num_item = _itemArrayInited - 1; num_item; num_item--) {
		Item *item = _itemArrayPtr[item_index++];

		if ((getGameType() == GType_WW && getPlatform() == Common::kPlatformAmiga) ||
			getGameType() == GType_ELVIRA2) {
			writeItemID(f, item->parent);
		} else {
			f->writeUint16BE(item->parent);
			f->writeUint16BE(item->next);
		}

		f->writeUint16BE(item->state);
		f->writeUint16BE(item->classFlags);

		SubRoom *r = (SubRoom *)findChildOfType(item, kRoomType);
		if (r) {
			f->writeUint16BE(r->roomExitStates);
		}

		SubSuperRoom *sr = (SubSuperRoom *)findChildOfType(item, kSuperRoomType);
		if (sr) {
			uint16 n = sr->roomX * sr->roomY * sr->roomZ;
			for (i = j = 0; i != n; i++)
				f->writeUint16BE(sr->roomExitStates[j++]);
		}

		SubObject *o = (SubObject *)findChildOfType(item, kObjectType);
		if (o) {
			f->writeUint32BE(o->objectFlags);
			i = o->objectFlags & 1;

			for (j = 1; j < 16; j++) {
				if (o->objectFlags & (1 << j)) {
					f->writeUint16BE(o->objectFlagValue[i++]);
				}
			}
		}

		SubUserFlag *u = (SubUserFlag *)findChildOfType(item, kUserFlagType);
		if (u) {
			for (i = 0; i != 4; i++) {
				f->writeUint16BE(u->userFlags[i]);
			}
		}
	}

	// write the variables
	for (i = 0; i != _numVars; i++) {
		f->writeUint16BE(readVariable(i));
	}

	// write the items in item store
	for (i = 0; i != _numItemStore; i++) {
		if (getGameType() == GType_WW && getPlatform() == Common::kPlatformAmiga) {
			f->writeUint16BE(itemPtrToID(_itemStore[i]) * 16);
		} else if (getGameType() == GType_ELVIRA2) {
			if (getPlatform() == Common::kPlatformDOS) {
				writeItemID(f, itemPtrToID(_itemStore[i]));
			} else {
				f->writeUint16BE(itemPtrToID(_itemStore[i]) * 18);
			}
		} else {
			f->writeUint16BE(itemPtrToID(_itemStore[i]));
		}
	}

	// Write the bits in array 1
	for (i = 0; i != _numBitArray1; i++)
		f->writeUint16BE(_bitArray[i]);

	// Write the bits in array 2
	for (i = 0; i != _numBitArray2; i++)
		f->writeUint16BE(_bitArrayTwo[i]);

	// Write the bits in array 3
	for (i = 0; i != _numBitArray3; i++)
		f->writeUint16BE(_bitArrayThree[i]);

	if (getGameType() == GType_ELVIRA2 || getGameType() == GType_WW) {
		f->writeUint16BE(_superRoomNumber);
	}

	f->finalize();
	bool result = !f->err();

	delete f;
	_videoLockOut &= ~0x100;

	return result;
}

// Personal Nightmare specific
bool AGOSEngine_PN::badload(int8 errorNum) {
	if (errorNum == -2)
		return 0;
	// Load error recovery routine

	// Clear any stack
	while (_stackbase != nullptr) {
		dumpstack();
	}

	// Restart from process 1
	_tagOfActiveDoline = 1;
	_dolineReturnVal = 3;
	return 1;
}

void AGOSEngine_PN::getFilename() {
	_noScanFlag = 1;
	clearInputLine();

	memset(_saveFile, 0, sizeof(_saveFile));
	while (!shouldQuit() && !strlen(_saveFile)) {
		const char *msg = "File name : ";
		pcf((unsigned char)'\n');
		while (*msg)
			pcf((unsigned char)*msg++);

		interact(_saveFile, 8);
		pcf((unsigned char)'\n');
		_noScanFlag = 0;
	}
}

int AGOSEngine_PN::loadFile(const Common::String &name) {
	Common::InSaveFile *f;
	haltAnimation();

	f = _saveFileMan->openForLoading(name);
	if (f == nullptr) {
		restartAnimation();
		return -2;
	}
	f->read(_saveFile, 8);

	if (f->readByte() != 41) {
		restartAnimation();
		delete f;
		return -2;
	}
	if (f->readByte() != 33) {
		restartAnimation();
		delete f;
		return -2;
	}
	// TODO: Make endian safe
	if (!f->read(_dataBase + _quickptr[2], (int)(_quickptr[6] - _quickptr[2]))) {
		restartAnimation();
		delete f;
		return -1;
	}
	delete f;
	restartAnimation();
	dbtosysf();
	return 0;
}

int AGOSEngine_PN::saveFile(const Common::String &name) {
	Common::OutSaveFile *f;
	sysftodb();
	haltAnimation();

	f = _saveFileMan->openForSaving(name);
	if (f == nullptr) {
		restartAnimation();

		const char *msg = "Couldn't save. ";
		pcf((unsigned char)'\n');
		while (*msg)
			pcf((unsigned char)*msg++);

		return 0;
	}
	f->write(_saveFile, 8);

	f->writeByte(41);
	f->writeByte(33);
	// TODO: Make endian safe
	if (!f->write(_dataBase + _quickptr[2], (int)(_quickptr[6] - _quickptr[2]))) {
		delete f;
		restartAnimation();
		error("Couldn't save ");
		return 0;	// for compilers that don't support NORETURN
	}
	f->finalize();
	delete f;

	restartAnimation();
	return 1;
}

void AGOSEngine_PN::sysftodb() {
	uint32 pos = _quickptr[2];
	int ct = 0;

	while (ct < (getptr(49L) / 2)) {
		_dataBase[pos] = (uint8)(_variableArray[ct] % 256);
		_dataBase[pos + 1] = (uint8)(_variableArray[ct] / 256);
		pos+=2;
		ct++;
	}
}

void AGOSEngine_PN::dbtosysf() {
	uint32 pos = _quickptr[2];
	int ct = 0;

	while (ct < (getptr(49L) / 2)) {
		_variableArray[ct] = _dataBase[pos] + 256 * _dataBase[pos + 1];
		pos += 2;
		ct++;
	}
}

} // End of namespace AGOS
