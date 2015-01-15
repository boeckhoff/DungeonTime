#include "Entity.h"

#define BACKGROUD_EMPTY ' '
#define BACKGROUD_FLOOR '.'
#define BACKGROUD_WALLX '-'
#define BACKGROUD_WALLY '|'
#define BACKGROUD_OPEN_DOOR 'x'
#define BACKGROUD_CLOSED_DOOR '+'
#define BACKGROUD_CORRIDOR '#'

class Level {
	public:
	uint64_t width, height;
	std::unique_ptr<uint8_t> background;
	std::vector<std::unique_ptr<Entity>> entities;
	Level();
	~Level();
	void doFrame();
	Entity* getEntityAt(uint64_t posX, uint64_t posY);
	uint8_t getBackgroundAt(uint64_t posX, uint64_t posY);
	void setBackgroundAt(uint64_t posX, uint64_t posY, uint8_t type);
	void fillBackgroundRow(uint64_t posX, uint64_t posY, uint64_t length, uint8_t type);
	void fillBackgroundColumn(uint64_t posX, uint64_t posY, uint64_t length, uint8_t type);
	void fillBackgroundRect(uint64_t posX, uint64_t posY, uint64_t w, uint64_t h, uint8_t type);
	void generateLine(uint64_t fromX, uint64_t fromY, uint64_t toX, uint64_t toY, uint8_t type);
	void generateEllipseRoom(uint64_t posX, uint64_t posY, uint64_t w, uint64_t h);
	void generateRectRoom(uint64_t posX, uint64_t posY, uint64_t w, uint64_t h);
	void generateXSplitRoom(uint64_t posX, uint64_t posY, uint64_t w, uint64_t h);
	void generateYSplitRoom(uint64_t posX, uint64_t posY, uint64_t w, uint64_t h);
	void generateXCorridor(uint64_t posX, uint64_t posY, uint64_t w, uint64_t h);
	void generateYCorridor(uint64_t posX, uint64_t posY, uint64_t w, uint64_t h);
	void generate();
};

extern std::unique_ptr<Level> level;
extern Entity* hero;
