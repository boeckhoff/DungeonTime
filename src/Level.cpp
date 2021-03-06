#include "Level.h"

std::unique_ptr<Level> level;
LivingEntity* hero;
std::unique_ptr<Config> config;

Level::Level() :width(75), height(25), background((uint8_t*)malloc(width * height)) {
	generateRandom();
}

Level::~Level() {
}

LevelElement::LevelElement(uint64_t _posX, uint64_t _posY, uint64_t _width, uint64_t _height):
	posX(_posX), posY(_posY), width(_width), height(_height) {
	}

Room::Room(uint64_t _posX, uint64_t _posY, uint64_t _width, uint64_t _height, RoomType _type):
	LevelElement(_posX, _posY, _width, _height),
	type(_type) {
	}

Room::~Room() {
	for(auto& i : doors) {
		//TODO free memory for doors
	}
}

Corridor::Corridor(uint64_t _posX, uint64_t _posY, uint64_t _width, uint64_t _height, CorridorType _type):
	LevelElement(_posX, _posY, _width, _height),
	type(_type) {
	}

Door::Door(uint64_t _relPosX, uint64_t _relPosY, Room* _origin):
	Door(_relPosX, _relPosY, _origin) {
	}

void Level::doFrame() {
	uint64_t maxX = std::min(width, (uint64_t)System::screenSize.ws_col);
	uint64_t maxY = std::min(height, (uint64_t)System::screenSize.ws_row);
	uint16_t field [level->width*level->height];
	createVisibilityField(hero->posX, hero->posY, field);

	std::string line;
	for(int i=0; i < sizeof(field)/sizeof(field[0]); ++i) {
		line += field[i];
	}

	for(uint64_t y = 0; y < maxY; ++y) {
		System::setCursorPosition(0, y);
		for(uint64_t x = 0; x < maxX; ++x) {
			if(Level::isVisible(x, y, field)) {
				auto entity = getPrimeEntityAt(x, y);
				if(entity)
					entity->doFrame();
				else
					fwrite(background.get()+y*width+x, 1, 1, stdout);
			}
			else
				fwrite(background.get()+0, 1, 1, stdout);
		} 
	}
}

Entity* Level::getPrimeEntityAt(uint64_t posX, uint64_t posY) const {
	for(uint64_t i = 0; i < entities.size(); ++i)
		if(entities[i]->posX == posX && entities[i]->posY == posY) {
			Entity* entityAtPos = dynamic_cast<LivingEntity*>(entities[i].get());
			if(entityAtPos) return entityAtPos;
		 	entityAtPos = dynamic_cast<MovingItemContainer*>(entities[i].get());
			if(entityAtPos) return entityAtPos;
		}
	return getItemContainerAt(posX, posY);
}

LivingEntity* Level::getLivingEntityAt(uint64_t posX, uint64_t posY, Entity* exceptFrom) const {
	for(uint64_t i = 0; i < entities.size(); ++i)
		if(entities[i]->posX == posX && entities[i]->posY == posY) {
			auto livingEntityAtPos = dynamic_cast<LivingEntity*>(entities[i].get());
			if(livingEntityAtPos && livingEntityAtPos != exceptFrom)
				return livingEntityAtPos;
		}
	return NULL;
}

ItemContainer* Level::getItemContainerAt(uint64_t posX, uint64_t posY, Entity* exceptFrom) const {
	for(uint64_t i = 0; i < entities.size(); ++i)
		if(entities[i]->posX == posX && entities[i]->posY == posY) {
			auto itemContainerAtPos = dynamic_cast<ItemContainer*>(entities[i].get());
			if(dynamic_cast<MovingItemContainer*>(itemContainerAtPos)) continue;
			if(itemContainerAtPos && itemContainerAtPos != exceptFrom)
				return itemContainerAtPos;
		}
	return NULL;
}

uint8_t Level::getBackgroundAt(uint64_t posX, uint64_t posY) const {
	return *(background.get()+posY*width+posX);
}

std::vector<std::pair<uint64_t, uint64_t>> Level::getNeighborFields(uint64_t posX, uint64_t posY) {
	std::vector<std::pair<uint64_t, uint64_t>> neighbors;
	if(level->isInBounds(posX+1, posY))
		neighbors.push_back(std::pair<uint64_t, uint64_t>(posX+1, posY));
	if(level->isInBounds(posX, posY+1))
		neighbors.push_back(std::pair<uint64_t, uint64_t>(posX, posY+1));
	if(level->isInBounds(posX-1, posY))
		neighbors.push_back(std::pair<uint64_t, uint64_t>(posX-1, posY));
	if(level->isInBounds(posX, posY-1))
		neighbors.push_back(std::pair<uint64_t, uint64_t>(posX, posY-1));

	return neighbors;
}

bool Level::isInBounds(uint64_t posX, uint64_t posY) {
	return posX < level->width && posY < level->height;
}

bool Level::isVisible(uint64_t posX, uint64_t posY, uint16_t* field) {
	if(config->godMode)
		return true;
	uint16_t val = field[posY*level->width+posX];
	if(val != UINT16_MAX) {
		return true;
	}
	return false;
}

bool Level::isWalkable(uint64_t posX, uint64_t posY) {
	if(posX > level->width || posY > level->height)
		return false;

	if(getLivingEntityAt(posX, posY))
		return true;

	char backgroundAtPos = this->getBackgroundAt(posX, posY);
	switch(backgroundAtPos) {
		case BACKGROUD_WALLX:
		case BACKGROUD_WALLY:
		case BACKGROUD_CLOSED_DOOR:
		case BACKGROUD_EMPTY:
		case BACKGROUD_CORRIDOR:
			return false;
			break;
		default:
			return true;
			break;
	}
}

void Level::createVisibilityField(uint64_t posX, uint64_t posY, uint16_t* field) {
	for(size_t i = 0; i < level->width*level->height; ++i)
		field[i] = UINT16_MAX;

	field[posY*level->width+posX] = 0;
	std::queue<std::pair<uint64_t, uint64_t>> que;
	que.push(std::pair<uint64_t, uint64_t>(posX, posY));
	while(!que.empty()) {
		auto cur = que.front();
		que.pop();
		uint16_t distance = field[cur.second*level->width+cur.first];
	
		if(distance+1 < field[cur.second*level->width+cur.first+1]) {
			field[cur.second*level->width+cur.first+1] = distance+1;
			if(level->isWalkable(cur.first+1, cur.second)) {
				que.push(std::pair<uint64_t, uint64_t>(cur.first+1, cur.second));
			}
		}

		if(distance+1 < field[(cur.second+1)*level->width+cur.first]) {
			field[(cur.second+1)*level->width+cur.first] = distance+1;
			if(level->isWalkable(cur.first, cur.second+1)) {
				que.push(std::pair<uint64_t, uint64_t>(cur.first, cur.second+1));
			}
		}

		if(distance+1 < field[cur.second*level->width+cur.first-1]) {
			field[cur.second*level->width+cur.first-1] = distance+1;
			if(level->isWalkable(cur.first-1, cur.second)) {
				que.push(std::pair<uint64_t, uint64_t>(cur.first-1, cur.second));
			}
		}

		if(distance+1 < field[(cur.second-1)*level->width+cur.first]) {
			field[(cur.second-1)*level->width+cur.first] = distance+1;
			if(level->isWalkable(cur.first, cur.second-1)) {
				que.push(std::pair<uint64_t, uint64_t>(cur.first, cur.second-1));
			}
		}
	}
}

void Level::createDistanceField(uint64_t posX, uint64_t posY, uint16_t* field) {
	for(size_t i = 0; i < level->width*level->height; ++i)
		field[i] = UINT16_MAX;

	field[posY*level->width+posX] = 0;
	std::queue<std::pair<uint64_t, uint64_t>> que;
	que.push(std::pair<uint64_t, uint64_t>(posX, posY));
	while(!que.empty()) {
		auto cur = que.front();
		que.pop();
		uint16_t distance = field[cur.second*level->width+cur.first];
		//level->setBackgroundAt(cur.first, cur.second, '0' + distance);
		if(level->isWalkable(cur.first+1, cur.second) && distance+1 < field[cur.second*level->width+cur.first+1]) {
			que.push(std::pair<uint64_t, uint64_t>(cur.first+1, cur.second));
			field[cur.second*level->width+cur.first+1] = distance+1;
		}
		if(level->isWalkable(cur.first, cur.second+1) && distance+1 < field[(cur.second+1)*level->width+cur.first]) {
			que.push(std::pair<uint64_t, uint64_t>(cur.first, cur.second+1));
			field[(cur.second+1)*level->width+cur.first] = distance+1;
		}
		if(level->isWalkable(cur.first-1, cur.second) && distance+1 < field[cur.second*level->width+cur.first-1]) {
			que.push(std::pair<uint64_t, uint64_t>(cur.first-1, cur.second));
			field[cur.second*level->width+cur.first-1] = distance+1;
		}
		if(level->isWalkable(cur.first, cur.second-1) && distance+1 < field[(cur.second-1)*level->width+cur.first]) {
			que.push(std::pair<uint64_t, uint64_t>(cur.first, cur.second-1));
			field[(cur.second-1)*level->width+cur.first] = distance+1;
		}
	}
}


void Level::setBackgroundAt(uint64_t posX, uint64_t posY, uint8_t type) {
	*(background.get()+posY*width+posX) = type;
}

void Level::fillBackgroundRow(uint64_t posX, uint64_t posY, uint64_t length, uint8_t type) {
	for(uint64_t x = posX; x < posX+length; ++x)
		if(getBackgroundAt(x, posY) != BACKGROUD_FLOOR)
			setBackgroundAt(x, posY, type);
		else
			setBackgroundAt(x, posY, BACKGROUD_FLOOR);

}

void Level::fillBackgroundColumn(uint64_t posX, uint64_t posY, uint64_t length, uint8_t type) {
	for(uint64_t y = posY; y < posY+length; ++y)
		if(getBackgroundAt(posX, y)!= BACKGROUD_FLOOR)
			setBackgroundAt(posX, y, type);
		else
			setBackgroundAt(posX, y, BACKGROUD_FLOOR);
}

void Level::fillBackgroundRect(uint64_t posX, uint64_t posY, uint64_t w, uint64_t h, uint8_t type) {
	for(uint64_t y = posY; y < posY+h; ++y)
		for(uint64_t x = posX; x < posX+w; ++x)
			setBackgroundAt(x, y, type);
}

std::vector<std::pair<uint64_t, uint64_t>> Level::calculateLine(uint64_t fromX, uint64_t fromY, uint64_t toX, uint64_t toY) {
	//Bresenham Line
	int64_t dx =  std::abs((int64_t)toX-(int64_t)fromX), sx = fromX<toX ? 1LL : -1LL;
	int64_t dy = -std::abs((int64_t)toY-(int64_t)fromY), sy = fromY<toY ? 1LL : -1LL;
	int64_t err = dx+dy, e2;
	std::vector<std::pair<uint64_t, uint64_t>> path;

	while(true) {
		path.push_back(std::pair<uint64_t, uint64_t> (fromX, fromY));
		if(fromX == toX && fromY == toY)
			break;

		e2 = 2*err;
		if(e2 > dy) { err += dy; fromX += sx; }
		if(e2 < dx) { err += dx; fromY += sy; }
	}
	return path;
}

void Level::generateLine(uint64_t fromX, uint64_t fromY, uint64_t toX, uint64_t toY, uint8_t type) {
	std::vector<std::pair<uint64_t, uint64_t>> path = calculateLine(fromX, fromY, toX, toY);
	for(auto& i : path) {
		level->setBackgroundAt(i.first, i.second, type);
	}
}

void Level::generateEllipseRoom(uint64_t posX, uint64_t posY, uint64_t w, uint64_t h) {
	//Bresenham Ellipse

	int64_t dx = 0, dy = h, w2 = w*w, h2 = h*h, err = h2-(2*h-1)*w2, e2;

	do {
		setBackgroundAt(posX-dx, posY-dy, BACKGROUD_CORRIDOR);
		fillBackgroundColumn(posX-dx, posY-dy+1, dy*2-1, BACKGROUD_FLOOR);
		setBackgroundAt(posX-dx, posY+dy, BACKGROUD_CORRIDOR);

		setBackgroundAt(posX+dx, posY-dy, BACKGROUD_CORRIDOR);
		fillBackgroundColumn(posX+dx, posY-dy+1, dy*2-1, BACKGROUD_FLOOR);
		setBackgroundAt(posX+dx, posY+dy, BACKGROUD_CORRIDOR);

		e2 = 2*err;
		if(e2 <  (2*dx+1)*h2) err += (2*(++dx)+1)*h2;
		if(e2 > -(2*dy-1)*w2) err -= (2*(--dy)-1)*w2;
	} while(dy >= 0);

	while(dx++ < w) {
		setBackgroundAt(posX-dx, posY, BACKGROUD_CORRIDOR);
		setBackgroundAt(posX+dx, posY, BACKGROUD_CORRIDOR);
	}
	rooms.push_back(std::unique_ptr<Room>(new Room(posX, posY, w, h, Room::RoomType::EllipseRoom)));
}

void Level::drawRectRoom(const Room* room) {
	fillBackgroundRow(room->posX+1, room->posY, room->width-2, BACKGROUD_WALLX);
	fillBackgroundRow(room->posX+1, room->posY+room->height-1, room->width-2, BACKGROUD_WALLX);
	fillBackgroundColumn(room->posX, room->posY+1, room->height-2, BACKGROUD_WALLY);
	fillBackgroundColumn(room->posX+room->width-1, room->posY+1, room->width-2, BACKGROUD_WALLY);
	fillBackgroundRect(room->posX+1, room->posY+1, room->width-2, room->height-2, BACKGROUD_FLOOR);
	}
}

void Level::generateXSplitRoom(uint64_t posX, uint64_t posY, uint64_t w, uint64_t h) {
	uint64_t half = w >> 1;
	generateRectRoom(posX, posY, half+1, h, true);
	generateRectRoom(posX+half, posY, w-half, h, true);
	setBackgroundAt(posX+half, posY+h/2, BACKGROUD_FLOOR);
	auto newRoom = new Room(posX, posY, w, h, Room::RoomType::XSplitRoom);
	rooms.push_back(std::unique_ptr<Room>(newRoom));
}

void Level::generateYSplitRoom(uint64_t posX, uint64_t posY, uint64_t w, uint64_t h) {
	uint64_t half = h >> 1;
	generateRectRoom(posX, posY, w, half+1, true);
	generateRectRoom(posX, posY+half, w, h-half, true);
	setBackgroundAt(posX+w/2, posY+half, BACKGROUD_CLOSED_DOOR);
	auto newRoom = new Room(posX, posY, w, h, Room::RoomType::YSplitRoom);
	rooms.push_back(std::unique_ptr<Room>(newRoom));
}

void Level::generateXCorridor(uint64_t posX, uint64_t posY, uint64_t w, uint64_t h, std::unique_ptr<LevelElement>(element1) = NULL, std::unique_ptr<LevelElement>(element2) = NULL) {
	fillBackgroundRow(posX, posY, w, BACKGROUD_CORRIDOR);
	fillBackgroundRow(posX, posY+h-1, w, BACKGROUD_CORRIDOR);
	fillBackgroundRect(posX, posY+1, w, h-2, BACKGROUD_FLOOR);
	setBackgroundAt(posX, posY+1, BACKGROUD_CLOSED_DOOR);
	auto newCor = new Corridor(posX, posY, w, h, Corridor::CorridorType::XCorridor);
	corridors.push_back(std::unique_ptr<Corridor>(newCor));
}

void Level::generateYCorridor(uint64_t posX, uint64_t posY, uint64_t w, uint64_t h, std::unique_ptr<LevelElement>(element1) = NULL, std::unique_ptr<LevelElement>(element2) = NULL) {
	fillBackgroundColumn(posX, posY, h, BACKGROUD_CORRIDOR);
	fillBackgroundColumn(posX+w-1, posY, h, BACKGROUD_CORRIDOR);
	fillBackgroundRect(posX+1, posY, w-2, h, BACKGROUD_FLOOR);
	setBackgroundAt(posX+1, posY, BACKGROUD_CLOSED_DOOR);
	auto newCor = new Corridor(posX, posY, w, h, Corridor::CorridorType::YCorridor);
	corridors.push_back(std::unique_ptr<Corridor>(newCor));
}

void Level::generate() {

	//seed random
	timeval t1;
	gettimeofday(&t1, NULL);
	srand(t1.tv_usec * t1.tv_sec);

	//generate random values
	uint64_t w = 10 + (rand() % 12);
	uint64_t h = 5 + (rand() % 10);
	uint64_t posX = (rand() % (width-w));
	uint64_t posY = (rand() % (height-h));

	//generate and draw new Room
	auto newRoom = new Room(posX, posY, w, h, Room::RoomType::RectRoom);
	drawRectRoom(newRoom);
	auto newDoor = new Door(0, 2, newRoom);
	rooms.push_back(std::unique_ptr<Room>(newRoom));

}



void Level::openDoor() {

	//seed random
	timeval t1;
	gettimeofday(&t1, NULL);
	srand(t1.tv_usec * t1.tv_sec);

	//generate random values
	uint64_t w = 10 + (rand() % 12);
	uint64_t h = 5 + (rand() % 10);
	uint64_t posX = (rand() % (width-w));
	uint64_t posY = (rand() % (height-h));
}

void Level::generateRooms() {
	memset(background.get(), BACKGROUD_EMPTY, width * height);
	int roomNum = 10;
	int maxTries = 50;
	timeval t1;
	gettimeofday(&t1, NULL);
	srand(t1.tv_usec * t1.tv_sec);
	while(roomNum>0) {
		if (maxTries == 0)
			break;
		maxTries--;
		bool collide = false;

		uint64_t w = 10 + (rand() % 12);
		uint64_t h = 5 + (rand() % 10);
		uint64_t posX = (rand() % (width-w));
		uint64_t posY = (rand() % (height-h));

		for(int i = 0; i < rooms.size(); ++i) {
			auto room = rooms[i].get();
			if((posX < room->posX+room->width+2 && posX+width+2 > room->posX &&
				posY < room->posY+room->height+2 && posY+height+2 > room->posY) ||
				(posX == room->posX && posY == room->posY)) {
				collide = true;
				break;
			}
		}
		if(!collide) {
			int roomT = rand() % 13;
			switch(roomT) {
			case 0:
			case 1:
				if(w>12)
					generateXSplitRoom(posX, posY, w, h);
				else
					generateRectRoom(posX, posY, w,	h);
				break;
			case 2:
			case 3:
				if(h>8)
					generateYSplitRoom(posX, posY, w, h);
				else
					generateRectRoom(posX, posY, w,	h);
				break;
			default:
				generateRectRoom(posX, posY, w,	h);
				break;
			}
			roomNum--;
		}
	}
}

void Level::generateConnections() {
	//TODO When connecting split rooms set Corridor cords accordingly
	if(rooms.size() >= 2) {
		for(int i = 0; i < rooms.size()-1;) {
			for(int j = i+1; j < rooms.size();) {
				auto room1 = rooms[i].get();
				auto room2 = rooms[j].get();
				if(room1->posX+2 < room2->posX+room2->width-2 && room1->posX+room1->width-2 > room2->posX+2) { //X-Overlap
					int Xstart = std::max(room1->posX, room2->posX);
					int Xstop = std::min(room1->posX+room1->width, room2->posX+room2->width);
					int Xmid = (Xstart+Xstop)/2;
					bool overlap = false;
					int corX = Xmid;
					int corY = room1->posY+room1->height-1;
					int corW = 3;
					int corH = room2->posY-(room1->posY+room1->height)+2;
					for(int k = 0; k < rooms.size(); ++k) {
						auto testedRoom = rooms[k].get();
						if (testedRoom != room1 && testedRoom != room2) {
							if(testedRoom->posX < corX+corW && testedRoom->posX+testedRoom->width > corX &&
								testedRoom->posY < corY+corH && testedRoom->posY+testedRoom->height > corY)
								overlap = true;
						}
					}
					if(room1->type == Room::RoomType::XSplitRoom && (corX < room1->posX+(room1->width >> 1)
						&& corX+corW > room1->posX+(room1->width >> 1))) {
						overlap = true;
					}
					if(room2->type == Room::RoomType::XSplitRoom && (corX < room1->posX+(room1->width)
						&& corX+corW > room1->posX+(room1->width >> 1))) {
						overlap = true;
					}

					if (!overlap) {
						generateYCorridor(corX, corY, corW, corH);
						//newCor->connects = std::pair<std::unique_ptr<Room>, std::unique_ptr<Room>>(std::unique_ptr<Room>(room1),std::unique_ptr<Room>(room2));
					}
				}else{

					if(room1->posY+2 < room2->posY+room2->height-2 && room1->posY+room1->height-2 > room2->posY+2) { //Y-Overlap
						int Ystart = std::max(room1->posY,room2->posY);
						int Ystop = std::min(room1->posY+room1->height,room2->posY+room2->height);
						int Ymid = (Ystart+Ystop)/2;
						bool overlap = false;
						int corX = room1->posX+room1->width-1;
						int corY = Ymid;
						int corW = room2->posX - (room1->posX+room1->width)+2;
						int corH = 3;
						for(int k = 0; k < rooms.size(); ++k) {
							auto testedRoom = rooms[k].get();
							if (testedRoom != room1 && testedRoom != room2) {
								if(testedRoom->posX < corX+corW && testedRoom->posX+testedRoom->width > corX &&
									testedRoom->posY < corY+corH && testedRoom->posY+testedRoom->height > corY)
									overlap = true;
							}
						}
						if(room1->type == Room::RoomType::YSplitRoom && (corY < room1->posY+(room1->height >> 1)
							&& corY+corH > room1->posY+(room1->height >> 1))) {
							overlap = true;
						}
						if(room2->type == Room::RoomType::YSplitRoom && (corY < room2->posY+(room2->height >> 1)
							&& corY+corH > room2->posY+(room2->height >> 1))) {
							overlap = true;
						}

						if(!overlap) {
							generateXCorridor(corX, corY, corW, corH);
							//newCor->connects = std::pair<std::unique_ptr<Room>, std::unique_ptr<Room>>(std::unique_ptr<Room>(room1),std::unique_ptr<Room>(room2));
						}
					}
				}
			++j;
			}
		++i;
		}
	}
}

void Level::generateRandom() {
	generateRooms();
	generateConnections();
}
