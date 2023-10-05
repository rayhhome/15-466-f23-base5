#include "Mode.hpp"

#include "Scene.hpp"
#include "WalkMesh.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, space, ib, reset;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	//player info:
	struct Player {
		WalkPoint at;
		//transform is at player's feet and will be yawed by mouse left/right motion:
		Scene::Transform *transform = nullptr;
		//camera is at player's head and will be pitched by mouse up/down motion:
		Scene::Camera *camera = nullptr;
	} player;

	//player avator:
	Scene::Transform *avatar = nullptr;

	//goal transform:
	Scene::Transform *goal = nullptr;

	//chess transform:
	std::vector<glm::vec3> chess_position_list = {};

	//camera state
	bool camera_state = true;

	std::list<Scene::Drawable> s2;
	// std::vector<Scene::Drawable> tmpDrawables;
	bool win = false;
	glm::vec3 startingPos = glm::vec3(-36.0f, 20.0f, 0.0f);
};
