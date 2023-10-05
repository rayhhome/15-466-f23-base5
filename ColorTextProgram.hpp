//author: Sirui (Ray) Huang, and all referenced sources (in comments as links)
//https://learnopengl.com/In-Practice/Text-Rendering
//also referencing the other two openGL programs in https://github.com/15-466/15-466-f23-base4
#pragma once

#include "GL.hpp"
#include "Load.hpp"

//Shader program that draws projected, colored texts:
struct ColorTextProgram {
  ColorTextProgram();
  ~ColorTextProgram();

  GLuint program = 0;
  //Uniform (per-invocation variable) locations:
  GLuint PROJECTION_mat4 = -1U;
  GLuint TEXT_COLOR_vec3 = -1U;
  //Textures:
	//TEXTURE0 - texture that is accessed by TexCoord
};

extern Load< ColorTextProgram > color_text_program;
