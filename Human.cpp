#include "Level.h"

HumanInventory::HumanInventory() :Inventory(9) {

}

const std::string HumanInventory::getInventorySlotName(uint64_t slot) const {
	const char* inventorySlotNames[] = {
		"1 Right Hand",
		"2 Left Hand",
		"3 Back",
		"4 Waist",
		"5 Head",
		"6 Hands",
		"7 Chest",
		"8 Legs",
		"9 Feet"
	};

	return inventorySlotNames[slot];
}



Human::Human(Level* _level, uint64_t _posX, uint64_t _posY) :Entity(_level, _posX, _posY) {
	inventory.reset(new HumanInventory());
}
