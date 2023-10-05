//author: Sirui (Ray) Huang, and all referenced sources (in comments as links)
//https://learnopengl.com/In-Practice/Text-Rendering
//also referencing the other two openGL programs in https://github.com/15-466/15-466-f23-base4
#include "ColorTextProgram.hpp"

#include "gl_compile_program.hpp"
#include "gl_errors.hpp"

Load< ColorTextProgram > color_text_program(LoadTagEarly);

ColorTextProgram::ColorTextProgram() {
	//Compile vertex and fragment shaders using the convenient 'gl_compile_program' helper function:
	program = gl_compile_program(
		//vertex shader:
		"#version 330\n"
		"layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>\n"
		"uniform mat4 PROJECTION;\n"
    "out vec2 TexCoords;\n"
		"void main() {\n"
		"	gl_Position = PROJECTION * vec4(vertex.xy, 0.0, 1.0);\n"
		"	TexCoords = vertex.zw;\n"
		"}\n"
	,
		//fragment shader:
		"#version 330\n"
		"uniform sampler2D TEX;\n"
		"uniform vec3 TEXT_COLOR;\n"
		"in vec2 TexCoords;\n"
		"out vec4 color;\n"
		"void main() {\n"
		"	vec4 sampled = vec4(1.0, 1.0, 1.0, texture(TEX, TexCoords).r);\n"
		"	color = vec4(TEXT_COLOR, 1.0) * sampled;\n"
		"}\n"
	);

	//look up the locations of uniforms:
	PROJECTION_mat4 = glGetUniformLocation(program, "PROJECTION");
  TEXT_COLOR_vec3 = glGetUniformLocation(program, "TEXT_COLOR");
	GLuint TEX_sampler2D = glGetUniformLocation(program, "TEX");

	//set TEX to always refer to texture binding zero:
	glUseProgram(program); //bind program -- glUniform* calls refer to this program now

	glUniform1i(TEX_sampler2D, 0); //set TEX to sample from GL_TEXTURE0

	glUseProgram(0); //unbind program -- glUniform* calls refer to ??? now
}

ColorTextProgram::~ColorTextProgram() {
	glDeleteProgram(program);
	program = 0;
}
