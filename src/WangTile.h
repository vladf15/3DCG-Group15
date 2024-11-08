#ifndef WANG_TILE_CLASS_H
#define WANG_TILE_CLASS_H
#include"mesh.h"
#include"vector"
#include"string"
struct Tile {
	// 0 - no path, 1 - path, -1 - undefined
	int up;
	int right;
	int down;
	int left;

};
class WangTile {
	public:
		//Id of the tile
		int id;
		//Mesh of the tile
		std::vector<GPUMesh> mesh;
		//Booleans to indicate whether there is a 'path' in that specific direction.
		bool up;
		bool down;
		bool left;
		bool right;

		WangTile(int pId, bool pUp, bool pRight, bool pDown, bool pLeft);
		std::string get_string();
		bool matches_tile(Tile t);
};
#endif