#ifndef WANG_TILE_CLASS_H
#define WANG_TILE_CLASS_H
#include"mesh.h"
#include"vector"
#include"string"
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
};
#endif