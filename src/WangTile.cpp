#include"WangTile.h"

WangTile::WangTile(int pId, bool pUp, bool pRight, bool pDown, bool pLeft) {
	id = pId;

	up = pUp;
	down = pDown;
	left = pLeft;
	right = pRight;
	std::string path = "resources/Tiles/Tile" + std::to_string(pId)+".obj";
	mesh = GPUMesh::loadMeshGPU(RESOURCE_ROOT + path);
	std::cout << "Created tile: " << get_string() << std::endl;

}
std::string WangTile::get_string() {
	return "ID: " + std::to_string(id) + "=>" + std::to_string(up) + "-" + std::to_string(right) + "-" + std::to_string(down) + "-" + std::to_string(left);
}

bool WangTile::matches_tile(Tile t){
	bool match = true;
	if(t.up >= 0){
		if((up && (t.up == 0)) || (!up && (t.up == 1))){
			match = false;
		}
	}
	if(t.right >= 0){
		if((right && (t.right == 0)) || (!right && (t.right == 1))){
			match = false;
		}
	}
	if(t.down >= 0){
		if((down && (t.down == 0)) || (!down && (t.down == 1))){
			match = false;
		}
	}
	if(t.left >= 0){
		if((left && (t.left == 0)) || (!left && (t.left == 1))){
			match = false;
		}
	}

	return match;
}