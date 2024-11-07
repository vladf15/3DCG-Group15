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