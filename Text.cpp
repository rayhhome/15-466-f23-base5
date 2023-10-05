//author: Sirui (Ray) Huang, and all referenced sources (in comments as links)
//usage : Render text in quads!
#include "Text.hpp"

#include "GL.hpp"
#include "gl_errors.hpp"
#include "gl_compile_program.hpp"

#include <hb.h>
#include <hb-ft.h>
#include <freetype/freetype.h>

#include "data_path.hpp"
#include "ColorTextProgram.hpp"

#include <iostream>
#include <cmath> // for dichotomic aligned text support

#define DEFAULT_SIZE 24 // font size, either change me or use show_text with scale parameter 
#define DRAW_WIDTH 1280 // drawable space width, change me!
#define DRAW_HEIGHT 720 // drawable space height, change me!

//reference: https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-freetype.c
//FreeType objects to use
FT_Library ft_library;
FT_Face ft_face;
FT_Error ft_error;

//glu vertex array object/vertex buffer object
GLuint vao;
GLuint vbo;

Text::Text(std::string const &filename) {
  //https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-freetype.c
  //FreeType initialization
  ft_error = FT_Init_FreeType(&ft_library);
  if (ft_error) {
    std::cout << "Error: FreeType Library Initialization Failure" << std::endl;
		abort();
	}
  ft_error = FT_New_Face(ft_library, filename.c_str(), 0, &ft_face);
	if (ft_error) {
		std::cout << "Error: FreeType TypeFace Initialization Failure" << std::endl;
    abort();
	}
  ft_error = FT_Set_Char_Size(ft_face, 0, DEFAULT_SIZE << 6, 0, 0);  // 72dpi
	if (ft_error) {
    std::cout << "Error: FreeType Set Char Size Failure" << std::endl;
		abort();
	}
  
  //https://learnopengl.com/In-Practice/Text-Rendering
  //remove restriction on byte alignment
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1); 

  //https://learnopengl.com/code_viewer_gh.php?code=src/7.in_practice/2.text_rendering/text_rendering.cpp
  //gl setup
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  

  //set up program's projection matrix
  //for projection, no perspective needed and orthographic projection is used
  glUseProgram(color_text_program->program);
	glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(DRAW_WIDTH), 0.0f, static_cast<float>(DRAW_HEIGHT));
	glUniformMatrix4fv(glGetUniformLocation(color_text_program->program, "PROJECTION"), 1, GL_FALSE, &projection[0][0]);
  glUseProgram(0);

  //gl objects preparation
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);   

  //display any openGL errors
  GL_ERRORS();
}

Text::~Text() {
  //freetype objects used, free storage
  FT_Done_Face(ft_face);
  FT_Done_FreeType(ft_library);
}

void Text::show_text(std::string const &text, glm::uvec2 const &drawable_size, float const &x_in, float const &y_in, int const& size, float const &scale, glm::vec3 const &color) const {
	//https://learnopengl.com/In-Practice/Text-Rendering  
  //pull up shade program: color_text_program
  glUseProgram(color_text_program->program);
  glUniform3f(glGetUniformLocation(color_text_program->program, "TEXT_COLOR"), color.x, color.y, color.z);
  glActiveTexture(GL_TEXTURE0);
  glBindVertexArray(vao);

  if (size > 0) {
    ft_error = FT_Set_Char_Size(ft_face, 0, size << 6, 0, 0);  // 72dpi
    if (ft_error) {
      std::cout << "Error: FreeType Set Char Size Failure" << std::endl;
      abort();
    }
  }

  //Harfbuzz buffer initialization
  hb_buffer_t *hb_buffer;
	hb_buffer = hb_buffer_create();

	//add text to buffer, using c_str to ensure text null-terminated
	hb_buffer_add_utf8(hb_buffer, text.c_str(), -1, 0, -1);
  //guess segment properties is equivalent to the following three lines: (details may differ)
  // hb_buffer_set_direction(hb_buffer, HB_DIRECTION_LTR);
	// hb_buffer_set_script(hb_buffer, HB_SCRIPT_LATIN);
  // hb_buffer_set_language(hb_buffer, hb_language_from_string("en", -1));
  hb_buffer_guess_segment_properties(hb_buffer);

	//create font
	hb_font_t *hb_font;
	hb_font = hb_ft_font_create(ft_face, NULL);

	//shape!!
	hb_shape(hb_font, hb_buffer, NULL, 0);

	//get all glyph information
	unsigned int len = hb_buffer_get_length(hb_buffer);
	hb_glyph_info_t *info = hb_buffer_get_glyph_infos(hb_buffer, NULL);
	hb_glyph_position_t *glyph_position = hb_buffer_get_glyph_positions(hb_buffer, NULL);

  //get local copy of target drawing position
  float x = x_in;
  float y = y_in;

  {//dichotomic text alignment
    //if x is less than 0, find text position dichotomically, as in a heap structure
    if (x < 0.0f) {
      int dichotomy_parameter = -static_cast<int>(floor(x));
      x = 0.0f;

      // get x advance for all glyphs
      for (uint32_t i = 0; i < len; i++) {
        x += static_cast<float>((glyph_position[i].x_advance >> 6)) * scale;
      }
      //choose the center position
      int level = static_cast<int>(pow(2, 1 + floor(log2(dichotomy_parameter))));
      x = static_cast<float>((dichotomy_parameter % (level / 2)) * 2 + 1) * (DRAW_WIDTH / (float)(level)) - x / 2.0f;
    }
    
    //same for y
    if (y < 0.0f) {
      int dichotomy_parameter = -static_cast<int>(floor(y));
      y = 0.0f;

      // get y advance for all glyphs
      for (uint32_t i = 0; i < len; i++) {
        y += static_cast<float>((glyph_position[i].y_advance >> 6)) * scale;
      }
      //choose the center position
      int level = static_cast<int>(pow(2, 1 + floor(log2(dichotomy_parameter))));
      y = static_cast<float>((dichotomy_parameter % (level / 2)) * 2 + 1) * (DRAW_HEIGHT / (float)(level)) - y / 2.0f;
    }
  }

  //render all glyphs in buffer
	for (uint32_t i = 0; i < len; i++) {
    //load and render current glyph
    ft_error = FT_Load_Glyph(ft_face, info[i].codepoint, FT_LOAD_DEFAULT);
		if (ft_error) {
      std::cout << "Error: FreeType Load Glyph Failure" << std::endl;
      abort();
    }
    ft_error = FT_Render_Glyph(ft_face->glyph, FT_RENDER_MODE_NORMAL);
		if (ft_error) {
      std::cout << "Error: FreeType Render Glyph Failure" << std::endl;
      abort();
    } 

    //set texture for current glyph
    glTexImage2D(
      GL_TEXTURE_2D,
      0,
      GL_RED,
      ft_face->glyph->bitmap.width,
      ft_face->glyph->bitmap.rows,
      0,
      GL_RED,
      GL_UNSIGNED_BYTE,
      ft_face->glyph->bitmap.buffer
    );

    //set texture options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    //calculate size with scale
		float w = ft_face->glyph->bitmap.width * scale;
    float h = ft_face->glyph->bitmap.rows  * scale;

    //calculate position with scale
    //note: for y, we need to be careful with characters like g and y
    //      since they can extend below the baseline, and we should start drawing there
		float xpos = x + (glyph_position[i].x_offset + ft_face->glyph->bitmap_left) * scale;
    float ypos = y + (glyph_position[i].y_offset + ft_face->glyph->bitmap_top ) * scale - h; // here!

    //update VBO quad vertices value for the current glyph
    float vertices[6][4] = {
        {xpos  , ypos+h, 0.0f, 0.0f},            
        {xpos  , ypos  , 0.0f, 1.0f},
        {xpos+w, ypos  , 1.0f, 1.0f},
      
			  {xpos  , ypos+h, 0.0f, 0.0f},
        {xpos+w, ypos  , 1.0f, 1.0f},
        {xpos+w, ypos+h, 1.0f, 0.0f}           
    };

    //update content of VBO memory
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); 
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    //render!!
    glDrawArrays(GL_TRIANGLES, 0, 6);
    //advance cursors for next glyph, calculated in 1/64 pixels
    //bitshift by 6 to get value in pixels (2^6 = 64)
		x += (glyph_position[i].x_advance >> 6) * scale;
		y += (glyph_position[i].y_advance >> 6) * scale;
    //unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);
	}

  //unbind program and vertex array object
	glUseProgram(0);
  glBindVertexArray(0);

  //buffer use complete, free storage
	hb_buffer_destroy (hb_buffer);
  hb_font_destroy (hb_font);

  //display any openGL errors
  GL_ERRORS();
}
