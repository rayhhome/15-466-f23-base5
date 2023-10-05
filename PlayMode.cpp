#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "Text.hpp"
#include "Sound.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <fstream>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include <random>
#include <string>

//load mesh program
GLuint chess_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > chess_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("data/chess.pnct"));
	chess_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

//load scene for drawing
Load< Scene > chess_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("data/chess.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = chess_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = chess_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;
	});
});

Load< Text > chess_text(LoadTagDefault, []() -> Text const * {
	return new Text(data_path("data/BebasNeue-Regular.ttf"));
});

//setup walkmesh
WalkMesh const *walkmesh = nullptr;
Load< WalkMeshes > chess_walkmeshes(LoadTagDefault, []() -> WalkMeshes const * {
	WalkMeshes *ret = new WalkMeshes(data_path("data/chess.w"));
	walkmesh = &ret->lookup("WalkMesh");
	return ret;
});

Load< Sound::Sample > chess_music(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("data/High_Off_Milky.wav"));
});

//background music
std::shared_ptr< Sound::PlayingSample > background_loop;

PlayMode::PlayMode() : scene(*chess_scene) {
	//create a player transform:
	scene.transforms.emplace_back();
	player.transform = &scene.transforms.back();
	player.transform->position = startingPos;

	//create a player camera attached to a child of the player transform:
	scene.transforms.emplace_back();
	scene.cameras.emplace_back(&scene.transforms.back());
	player.camera = &scene.cameras.back();
	player.camera->fovy = glm::radians(60.0f);
	player.camera->near = 0.01f;
	player.camera->transform->parent = player.transform;

	for (auto drawable : scene.drawables) {
    if (drawable.transform->name.substr(0,6) != "ChessB") {
      s2.emplace_back(drawable);
    }
  	// if (drawable.transform->name.substr(0,6) != "Avatar"){		
		// 	tmpDrawables.emplace_back(drawable);
    // }
  }


	//https://www.geeksforgeeks.org/cpp-remove-elements-from-a-list-while-iterating/

	for (auto it = scene.drawables.begin(); it != scene.drawables.end(); it++) { 
		if ((*it).transform->name.substr(0,6) == "ChessW"){
			it = scene.drawables.erase(it);
			it--;
		}
	}

	// for (auto it = scene.drawables.begin(); it != scene.drawables.end(); it++) { 
	// 	if ((*it).transform->name.substr(0,6) == "Avatar"){
	// 		it = scene.drawables.erase(it);
	// 		it--;
	// 	}
	// }

	for (auto &transform : scene.transforms) {
    if (transform.name == "GoalGoal"){
      goal = &transform;
  	} else if (transform.name.substr(0,3) == "Che"){
      auto stuff = &transform;
    	chess_position_list.emplace_back(stuff->position);
		} else if (transform.name == "Avatar"){
			avatar = &transform;
		}
	}

	//player's eyes are 2.0 units above the ground:
	player.camera->transform->position = glm::vec3(0.0f, 0.0f, 2.0f);

	//rotate camera facing direction (-z) to player facing direction (+y):
	player.camera->transform->rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

	//start player walking at nearest walk point:
	player.at = walkmesh->nearest_walk_point(player.transform->position);

	//setup camera state;
	camera_state = true;

	//setup avatar transform;
	avatar->parent = player.transform;

	//play music
	background_loop = Sound::loop(*chess_music, 0.8f, 0.0f);
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	//check user input
	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			space.downs += 1;
			space.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_i) {
			ib.downs += 1;
			ib.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_r) {
			reset.downs += 1;
			reset.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			space.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_r) {
			reset.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_i) {
			ib.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	} else if (evt.type == SDL_MOUSEMOTION) {
			if (SDL_GetRelativeMouseMode() == SDL_TRUE && camera_state) {
					glm::vec2 motion = glm::vec2(
						evt.motion.xrel / float(window_size.y),
						-evt.motion.yrel / float(window_size.y)
					);
					
					glm::vec3 upDir = walkmesh->to_world_smooth_normal(player.at);
					player.transform->rotation = glm::angleAxis(-motion.x * player.camera->fovy, upDir) * player.transform->rotation;

					float pitch = glm::pitch(player.camera->transform->rotation);
					pitch += motion.y * player.camera->fovy;
					//camera looks down -z (basically at the player's feet) when pitch is at zero.
					pitch = std::min(pitch, 0.95f * 3.1415926f);
					pitch = std::max(pitch, 0.05f * 3.1415926f);
					player.camera->transform->rotation = glm::angleAxis(pitch, glm::vec3(1.0f, 0.0f, 0.0f));

					return true;
			}	
	}

	return false;
}

void PlayMode::update(float elapsed) {
	//reset
	if (reset.pressed){
			win = false;
			player.camera->transform->position = glm::vec3(0.0f, 0.0f, 1.8f);
			camera_state = true;
			player.camera->transform->rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			player.transform->position = startingPos;
			player.at = walkmesh->nearest_walk_point(player.transform->position);
			SDL_SetRelativeMouseMode(SDL_FALSE);
	}
	
	//player walking:
	if (!win) {
		//check if camera is switched
		if (space.downs > 0) {	
			if (camera_state) {
				//player's eyes are 40,0 units above the ground:
				player.camera->transform->position = glm::vec3(0.0f, 0.0f, 25.0f);

				//let camera face down
				float pitch = glm::radians(-90.0f);
				pitch =	std::min(pitch, 0.95f * 3.1415926f);
				pitch = std::max(pitch, 0.05f * 3.1415926f);
				player.camera->transform->rotation = glm::inverse(player.transform->rotation);
				// auto huaban = tmpDrawables.front();
				// scene.drawables.emplace_back(huaban);
				// s2.emplace_back(huaban);			
		} else {
				//player's eyes are 1.8 units above the ground:
				player.camera->transform->position = glm::vec3(0.0f, 0.0f, 1.8f);

				//rotate camera facing direction (-z) to player facing direction (+y):
				player.camera->transform->rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    		// s2.pop_back();
				// scene.drawables.pop_back();
			}
			camera_state = !camera_state;			
		}

		//combine inputs into a move:
		constexpr float PlayerSpeed = 5.0f;
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x =-1.0f;
		if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed) move.y =-1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;
		
		//switch chess
		if (ib.downs > 0){
			scene.drawables.swap(s2);
		}

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		//get move in world coordinate system:
		glm::vec3 remain = player.transform->make_local_to_world() * glm::vec4(move.x, move.y, 0.0f, 0.0f);

		bool move_flag = true;
		for (auto pos: chess_position_list) {
      if (glm::distance(player.transform->position + remain, pos) < 1.65f){
        remain = player.transform->make_local_to_world() * glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
				move_flag = false;
			}
    }

		//using a for() instead of a while() here so that if walkpoint gets stuck in
		// some awkward case, code will not infinite loop:
		for (uint32_t iter = 0; iter < 10; ++iter) {
			if (remain == glm::vec3(0.0f)) break;
			WalkPoint end;
			float time;
			walkmesh->walk_in_triangle(player.at, remain, &end, &time);
			player.at = end;
			if (time == 1.0f) {
				//finished within triangle:
				remain = glm::vec3(0.0f);
				break;
			}
			//some step remains:
			remain *= (1.0f - time);
			//try to step over edge:
			glm::quat rotation;
			if (walkmesh->cross_edge(player.at, &end, &rotation)) {
				//stepped to a new triangle:
				player.at = end;
				//rotate step to follow surface:
				remain = rotation * remain;
			} else {
				//ran into a wall, bounce / slide along it:
				glm::vec3 const &a = walkmesh->vertices[player.at.indices.x];
				glm::vec3 const &b = walkmesh->vertices[player.at.indices.y];
				glm::vec3 const &c = walkmesh->vertices[player.at.indices.z];
				glm::vec3 along = glm::normalize(b-a);
				glm::vec3 normal = glm::normalize(glm::cross(b-a, c-a));
				glm::vec3 in = glm::cross(normal, along);

				//check how much 'remain' is pointing out of the triangle:
				float d = glm::dot(remain, in);
				if (d < 0.0f) {
					//bounce off of the wall:
					remain += (-1.25f * d) * in;
				} else {
					//if it's just pointing along the edge, bend slightly away from wall:
					remain += 0.01f * d * in;
				}
			}
		}

		if (remain != glm::vec3(0.0f)) {
			std::cout << "NOTE: code used full iteration budget for walking." << std::endl;
		}

		//update player's position to respect walking:
		player.transform->position = walkmesh->to_world_point(player.at);

		{ //update player's rotation to respect local (smooth) up-vector:
			
			glm::quat adjust = glm::rotation(
				player.transform->rotation * glm::vec3(0.0f, 0.0f, 1.0f), //current up vector
				walkmesh->to_world_smooth_normal(player.at) //smoothed up vector at walk location
			);
			player.transform->rotation = glm::normalize(adjust * player.transform->rotation);
		}

	
		glm::mat4x3 frame = player.transform->make_local_to_parent();
		glm::vec3 right = frame[0];
		//glm::vec3 up = frame[1];
		glm::vec3 forward = -frame[2];

		if (move_flag) {
			player.transform->position += move.x * right + move.y * forward;
		} 
	}

	//win condition
	float dis = glm::distance(player.transform->position, goal->position);
	if (dis < 1.5f){
		win = true;
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
	space.downs = 0;
	ib.downs = 0;
	reset.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	player.camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.15f, 0.15f, 0.3f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*player.camera);

	/* In case you are wondering if your walkmesh is lining up with your scene, try:
	{
		glDisable(GL_DEPTH_TEST);
		DrawLines lines(player.camera->make_projection() * glm::mat4(player.camera->transform->make_world_to_local()));
		for (auto const &tri : walkmesh->triangles) {
			lines.draw(walkmesh->vertices[tri.x], walkmesh->vertices[tri.y], glm::u8vec4(0x88, 0x00, 0xff, 0xff));
			lines.draw(walkmesh->vertices[tri.y], walkmesh->vertices[tri.z], glm::u8vec4(0x88, 0x00, 0xff, 0xff));
			lines.draw(walkmesh->vertices[tri.z], walkmesh->vertices[tri.x], glm::u8vec4(0x88, 0x00, 0xff, 0xff));
		}
	}
	*/

	{ //draw texts
		glDisable(GL_DEPTH_TEST);		
		chess_text->show_text(
			"I to switch visible chess genre; Space to switch view mode", 
			drawable_size, 
			25.0f, 25.0f, 
			48, 1.0f, glm::vec3(0.2f, 0.2f, 0.2f));
		chess_text->show_text(
			"I to switch visible chess genre; Space to switch view mode", 
			drawable_size, 
			26.5f, 26.5f, 
			48, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));

		chess_text->show_text(
			"Mouse motion looks; WASD moves; Escape ungrabs mouse", 
			drawable_size, 
			25.0f, 75.0f, 
			48, 1.0f, glm::vec3(0.2f, 0.2f, 0.2f));	
		chess_text->show_text(
			"Mouse motion looks; WASD moves; Escape ungrabs mouse", 
			drawable_size, 
			26.5f, 76.5f, 
			48, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));	

		chess_text->show_text(
			"A-Maze-In Chess The Game!!", 
			drawable_size, 
			25.0f, 645.0f, 
			64, 1.0f, glm::vec3(0.2f, 0.2f, 0.2f));	
		chess_text->show_text(
			"A-Maze-In Chess The Game!!", 
			drawable_size, 
			26.5f, 646.5f, 
			64, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));	

		if (win) {
			chess_text->show_text(
				"You win! Press R to restart", 
				drawable_size, 
				-1.0f, -1.0f, 
				68, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
			chess_text->show_text(
				"You win! Press R to restart", 
				drawable_size, 
				-1.0f, -1.0f, 
				64, 1.0f, glm::vec3(0.5f, 0.5f, 0.5f));		
		}
	}

	GL_ERRORS();
}
