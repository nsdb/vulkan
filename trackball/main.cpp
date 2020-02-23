#include "cgmath.h"			// slee's simple math library
#include "cgut.h"			// slee's OpenGL utility
#include "trackball.h"		// virtual trackball
#include "Planet.h"         // Planet info

//*******************************************************************
// global constants
static const char*	window_name = "cgbase - trackball";
static const char*	vert_shader_path = "./shaders/trackball.vert";
static const char*	frag_shader_path = "./shaders/trackball.frag";
static const uint	NUM_TESS = 72;		// initial tessellation factor of the "sphere" as a "polyhedron"
static const uint   NUM_TEXTURE = 12;  // number of texture

//*******************************************************************
// include stb_image with the implementation preprocessor definition
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//*******************************************************************
// common structures
struct camera
{
	vec3	eye = vec3( 0, 100, 0 );
	vec3	at = vec3( 0, 0, 0 );
	vec3	up = vec3( 0, 0, 1 );
	mat4	view_matrix = mat4::lookAt( eye, at, up );
		
	float	fovy = PI/4.0f; // must be in radian
	float	aspect_ratio;
	float	dNear = 1.0f;
	float	dFar = 1000.0f;
	mat4	projection_matrix;
};

struct light_t
{
	vec4	position = vec4(0.0f, 0.0f, 0.0f, 1.0f);   // non-directional light
	vec4	ambient = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	vec4	diffuse = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	vec4	specular = vec4(1.0f, 1.0f, 1.0f, 1.0f);
};


//*******************************************************************
// window objects
GLFWwindow*	window = nullptr;
ivec2		window_size = ivec2( 1280, 720 );	// initial window size

//*******************************************************************
// OpenGL objects
GLuint	program	= 0;	// ID holder for GPU program
GLuint	planet_vertex_buffer = 0;	// ID holder for vertex buffer (Planet)
GLuint	planet_index_buffer = 0;	// ID holder for index buffer (Planet)
GLuint	ring_vertex_buffer = 0;	// ID holder for vertex buffer (Ring)
GLuint	ring_index_buffer = 0;	// ID holder for index buffer (Ring)

//*******************************************************************
// global variables
int		frame = 0;	// index of rendering frames
float	radius = 1.0f;
bool	bWireframe = false;			// this is the default
bool    bShiftKeyPressed = false;      // state of shift key pressed
bool    bCtrlKeyPressed = false;      // state of ctrl key pressed
float   current_time = 0.0f;
GLuint  textures[NUM_TEXTURE]; // texture array


//*******************************************************************
// holder of vertices and indices
std::vector<vertex>	planet_vertex_list;	    // host-side vertices (Planet)
std::vector<uint>	planet_index_list;		// host-side indices (Planet)
std::vector<vertex>	ring_vertex_list;	    // host-side vertices (Ring)
std::vector<uint>	ring_index_list;		// host-side indices (Ring)

//*******************************************************************
// scene objects
camera		cam;
trackball	tb;
light_t		light;
std::vector<Planet> planet_list;    // planet list
float planet_shininess = 1000.0f;   // shininess of planet
uint saturn_ring_parent_index = -1;// parent index of saturn ring (saturn index)
float saturn_ring_radius = 0.0f;    // radius of saturn ring


//*******************************************************************
void update()
{
	// update projection matrix
	cam.aspect_ratio = window_size.x/float(window_size.y);
	cam.projection_matrix = mat4::perspective( cam.fovy, cam.aspect_ratio, cam.dNear, cam.dFar );

	// update planet rotation, revolution
	float elapsed_time = (float)glfwGetTime() - current_time;
	for (int i = 0; i < (int)planet_list.size(); i++) {
		planet_list.at(i).time_process(elapsed_time);
	}
	current_time += elapsed_time;

	// update uniform variables in vertex/fragment shaders
	GLint uloc;
	uloc = glGetUniformLocation( program, "view_matrix" );			if(uloc>-1) glUniformMatrix4fv( uloc, 1, GL_TRUE, cam.view_matrix );
	uloc = glGetUniformLocation( program, "projection_matrix" );	if(uloc>-1) glUniformMatrix4fv( uloc, 1, GL_TRUE, cam.projection_matrix );

	// setup light properties
	uloc = glGetUniformLocation(program, "light_position");			if (uloc>-1) glUniform4fv(uloc, 1, light.position);
	uloc = glGetUniformLocation(program, "Ia");						if (uloc>-1) glUniform4fv(uloc, 1, light.ambient);
	uloc = glGetUniformLocation(program, "Id");						if (uloc>-1) glUniform4fv(uloc, 1, light.diffuse);
	uloc = glGetUniformLocation(program, "Is");						if (uloc>-1) glUniform4fv(uloc, 1, light.specular);

	// setup material properties
	uloc = glGetUniformLocation(program, "shininess");				if (uloc>-1) glUniform1f(uloc, planet_shininess);


}

void render()
{
	// clear screen (with background color) and clear depth buffer
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	
	// notify GL that we use our own program
	glUseProgram( program );

	// bind "planet" vertex attributes to your shader program
	const char*	vertex_attrib[]	= { "position", "normal", "texcoord" };
	size_t		attrib_size[]	= { sizeof(vertex().pos), sizeof(vertex().norm), sizeof(vertex().tex) };
	for( size_t k=0, kn=std::extent<decltype(vertex_attrib)>::value, byte_offset=0; k<kn; k++, byte_offset+=attrib_size[k-1] )
	{
		GLuint loc = glGetAttribLocation( program, vertex_attrib[k] ); if(loc>=kn) continue;
		glEnableVertexAttribArray( loc );
		glBindBuffer( GL_ARRAY_BUFFER, planet_vertex_buffer );
		glVertexAttribPointer( loc, attrib_size[k]/sizeof(GLfloat), GL_FLOAT, GL_FALSE, sizeof(vertex), (GLvoid*) byte_offset );
	}
	
	// use "planet" index buffer
	if (planet_index_buffer) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planet_index_buffer);

	// render each "planet"(sphere)
	GLint uloc;
	for (int i = 0; i < (int)planet_list.size(); i++) {

		// radius, model matrix for planet
		mat4 model_matrix = mat4::translate(0, 0, 0);

		// child planet process
		if (planet_list.at(i).parent_index != -1) {

			uint parent_index = planet_list.at(i).parent_index;

			// parent position, revolution process
			model_matrix = model_matrix * mat4::rotate(vec3(0, 0, 1), planet_list.at(parent_index).revolution_theta) * mat4::translate(planet_list.at(parent_index).distance, 0, 0);
			// child position, revolution process
			model_matrix = model_matrix * mat4::rotate(vec3(0, 0, 1), planet_list.at(i).revolution_theta) * mat4::translate(planet_list.at(i).distance, 0, 0);
			// parent rotation process
			model_matrix = model_matrix * mat4::rotate(vec3(0, 0, 1), planet_list.at(parent_index).rotation_theta);
			// child rotation process
			model_matrix = model_matrix * mat4::rotate(vec3(0, 0, 1), planet_list.at(i).rotation_theta);
		}
		// normal planet process
		else {
			// position, revolution process
			model_matrix = model_matrix * mat4::rotate(vec3(0, 0, 1), planet_list.at(i).revolution_theta) * mat4::translate(planet_list.at(i).distance, 0, 0); 
			// rotation process
			model_matrix = model_matrix * mat4::rotate(vec3(0, 0, 1), planet_list.at(i).rotation_theta);
		}

		uloc = glGetUniformLocation(program, "use_alpha_tex");				if (uloc > -1) glUniform1i(uloc, false); // do not use alpha tex
		uloc = glGetUniformLocation(program, "use_shader");					if (uloc > -1) glUniform1i(uloc, i != 0); // do not apply shader to the Sun
		uloc = glGetUniformLocation(program, "planet_radius");				if (uloc > -1) glUniform1f(uloc, planet_list.at(i).radius);
		uloc = glGetUniformLocation(program, "model_matrix");				if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, model_matrix);
		uloc = glGetUniformLocation(program, "TEX");						if (uloc > -1) glUniform1i(uloc, planet_list.at(i).planet_texture_index);

		// render vertices: trigger shader programs to process vertex data
		glDrawElements(GL_TRIANGLES, planet_index_list.size(), GL_UNSIGNED_INT, nullptr);
	}


	// bind "ring" vertex attributes to your shader program
	for (size_t k = 0, kn = std::extent<decltype(vertex_attrib)>::value, byte_offset = 0; k<kn; k++, byte_offset += attrib_size[k - 1])
	{
		GLuint loc = glGetAttribLocation(program, vertex_attrib[k]); if (loc >= kn) continue;
		glEnableVertexAttribArray(loc);
		glBindBuffer(GL_ARRAY_BUFFER, ring_vertex_buffer);
		glVertexAttribPointer(loc, attrib_size[k] / sizeof(GLfloat), GL_FLOAT, GL_FALSE, sizeof(vertex), (GLvoid*)byte_offset);
	}

	// use "ring" index buffer
	if (ring_index_buffer) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ring_index_buffer);

	// render "ring"
	mat4 model_matrix = mat4::translate(0, 0, 0);
	// position, revolution process
	model_matrix = model_matrix * mat4::rotate(vec3(0, 0, 1), planet_list.at(saturn_ring_parent_index).revolution_theta) * mat4::translate(planet_list.at(saturn_ring_parent_index).distance, 0, 0);
	// rotation process
	model_matrix = model_matrix * mat4::rotate(vec3(0, 0, 1), planet_list.at(saturn_ring_parent_index).rotation_theta);

	uloc = glGetUniformLocation(program, "use_alpha_tex");				if (uloc > -1) glUniform1i(uloc, true); // use alpha tex
	uloc = glGetUniformLocation(program, "use_shader");					if (uloc > -1) glUniform1i(uloc, true); // use shader
	uloc = glGetUniformLocation(program, "planet_radius");				if (uloc > -1) glUniform1f(uloc, saturn_ring_radius);
	uloc = glGetUniformLocation(program, "model_matrix");				if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, model_matrix);
	uloc = glGetUniformLocation(program, "TEX");						if (uloc > -1) glUniform1i(uloc, 10); // index of the saturn ring
	uloc = glGetUniformLocation(program, "TEX_ALPHA");					if (uloc > -1) glUniform1i(uloc, 11); // index of the saturn ring alpha

	// render vertices: trigger shader programs to process vertex data
	glDrawElements(GL_TRIANGLES, ring_index_list.size(), GL_UNSIGNED_INT, nullptr);


	// swap front and back buffers, and display to screen
	glfwSwapBuffers( window );
}

void reshape( GLFWwindow* window, int width, int height )
{
	// set current viewport in pixels (win_x, win_y, win_width, win_height)
	// viewport: the window area that are affected by rendering 
	window_size = ivec2(width,height);
	glViewport( 0, 0, width, height );
}

void print_help()
{
	printf( "[help]\n" );
	printf( "- press ESC or 'q' to terminate the program\n" );
	printf( "- press F1 or 'h' to see help\n" );
	printf( "- press 'w' to toggle wireframe\n" );
	printf( "- press Home to reset camera\n" );
	printf( "\n" );
}

void keyboard( GLFWwindow* window, int key, int scancode, int action, int mods )
{
	if(action==GLFW_PRESS)
	{
		if(key==GLFW_KEY_ESCAPE||key==GLFW_KEY_Q)	glfwSetWindowShouldClose( window, GL_TRUE );
		else if(key==GLFW_KEY_H||key==GLFW_KEY_F1)	print_help();
		else if(key==GLFW_KEY_HOME)					memcpy(&cam,&camera(),sizeof(camera));
		else if (key == GLFW_KEY_W)
		{
			bWireframe = !bWireframe;
			glPolygonMode(GL_FRONT_AND_BACK, bWireframe ? GL_LINE : GL_FILL);
			printf("> using %s mode\n", bWireframe ? "wireframe" : "solid");
		}
		else if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT) {
			bShiftKeyPressed = true;
		}
		else if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL) {
			bCtrlKeyPressed = true;
		}
	}
	else if (action == GLFW_RELEASE) {
		if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT) {
			bShiftKeyPressed = false;
		}
		else if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL) {
			bCtrlKeyPressed = false;
		}
	}
}

void mouse( GLFWwindow* window, int button, int action, int mods )
{
	int mode;
	switch (button) {

	case GLFW_MOUSE_BUTTON_LEFT:
		if (bShiftKeyPressed) {
			mode = MODE_ZOOM;
		}
		else if (bCtrlKeyPressed) {
			mode = MODE_PANNING;
		}
		else {
			mode = MODE_ROTATION;
		}
		break;

	case GLFW_MOUSE_BUTTON_RIGHT:
		mode = MODE_ZOOM;
		break;

	case GLFW_MOUSE_BUTTON_MIDDLE:
		mode = MODE_PANNING;
		break;

	default:
		return;

	}

	dvec2 pos; glfwGetCursorPos(window,&pos.x,&pos.y);
	vec2 npos = vec2( float(pos.x)/float(window_size.x-1), float(pos.y)/float(window_size.y-1) );
	if(action==GLFW_PRESS)			tb.begin( cam.view_matrix, npos.x, npos.y, mode);
	else if(action==GLFW_RELEASE)	tb.end();

}

void motion( GLFWwindow* window, double x, double y )
{
	if(!tb.bTracking) return;
	vec2 npos = vec2( float(x)/float(window_size.x-1), float(y)/float(window_size.y-1) );
	cam.view_matrix = tb.update( npos.x, npos.y );
}

void update_vertex_buffer()
{
	// clear and create new buffers
	if (planet_vertex_buffer)	glDeleteBuffers(1, &planet_vertex_buffer);	planet_vertex_buffer = 0;
	if (planet_index_buffer)	glDeleteBuffers(1, &planet_index_buffer);	planet_index_buffer = 0;
	if (ring_vertex_buffer)	glDeleteBuffers(1, &ring_vertex_buffer);	ring_vertex_buffer = 0;
	if (ring_index_buffer)	glDeleteBuffers(1, &ring_index_buffer);	ring_index_buffer = 0;

	// check exceptions
	if (planet_vertex_list.empty() || ring_vertex_list.empty()) { printf("[error] vertex_list is empty.\n"); return; }

	// create buffers
	// i : longitude, k : latitude
	planet_index_list.clear();
	for (uint i = 0; i <= NUM_TESS + 1; i++) {
		for (uint k = 0; k < NUM_TESS / 2; k++) {
			planet_index_list.push_back(i * (NUM_TESS / 2) + k);
			planet_index_list.push_back(i * (NUM_TESS / 2) + k + 1);
			planet_index_list.push_back((i + 1) * (NUM_TESS / 2) + k + 1);

			planet_index_list.push_back((i + 1) * (NUM_TESS / 2) + k + 1);
			planet_index_list.push_back((i + 1) * (NUM_TESS / 2) + k);
			planet_index_list.push_back(i * (NUM_TESS / 2) + k);
		}
	}
	ring_index_list.clear();
	for (uint i = 0; i <= NUM_TESS; i++) {

		// like flatten doughnut

		ring_index_list.push_back(i * 2);
		ring_index_list.push_back(i * 2 + 1);
		ring_index_list.push_back((i + 1) * 2 + 1);

		ring_index_list.push_back((i + 1) * 2 + 1);
		ring_index_list.push_back(i * 2 + 1);
		ring_index_list.push_back(i * 2);

		ring_index_list.push_back((i + 1) * 2 + 1);
		ring_index_list.push_back((i + 1) * 2);
		ring_index_list.push_back(i * 2);

		ring_index_list.push_back(i * 2);
		ring_index_list.push_back((i + 1) * 2);
		ring_index_list.push_back((i + 1) * 2 + 1);
	}


	// generation of vertex buffer: use vertex_list as it is
	glGenBuffers(1, &planet_vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, planet_vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex)*planet_vertex_list.size(), &planet_vertex_list[0], GL_STATIC_DRAW);
	glGenBuffers(1, &ring_vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, ring_vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex)*ring_vertex_list.size(), &ring_vertex_list[0], GL_STATIC_DRAW);

	// geneation of index buffer
	glGenBuffers(1, &planet_index_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planet_index_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint)*planet_index_list.size(), &planet_index_list[0], GL_STATIC_DRAW);
	glGenBuffers(1, &ring_index_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ring_index_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint)*ring_index_list.size(), &ring_index_list[0], GL_STATIC_DRAW);
}

void mapping_texture(GLuint* target, const char* filePath) {

	// Texture
	int mip_levels = get_mip_levels(window_size.x, window_size.y);

	// load image
	int width, height, comp = 3;
	unsigned char* image0 = stbi_load(filePath, &width, &height, &comp, 3);
	if (comp == 1) comp = 3;

	// vertical flip
	int stride0 = width*comp, stride1 = (stride0 + 3)&(~3);	// 4-byte aligned stride
	unsigned char* image = (unsigned char*)malloc(sizeof(unsigned char)*stride1*height);
	for (int y = 0; y < height; y++) memcpy(image + (height - 1 - y)*stride1, image0 + y*stride0, stride0); // vertical flip

	glGenTextures(1, target);
	glBindTexture(GL_TEXTURE_2D, *target);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);

	for (int k = 1, w = width >> 1, h = height >> 1; k<mip_levels; k++, w = max(1, w >> 1), h = max(1, h >> 1))
		glTexImage2D(GL_TEXTURE_2D, k, GL_RGB8 /* GL_RGB for legacy GL */, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
	glGenerateMipmap(GL_TEXTURE_2D);

	free(image0);
	free(image);

}


void update_circle_vertices()
{
	planet_vertex_list.clear();
	ring_vertex_list.clear();

	// Sphere Init
	// i : longitude, k : latitude
	for (uint i = 0; i <= NUM_TESS; i++) {

		// t : theta - angle of longitude
		float t = PI*2.0f / float(NUM_TESS) * float(i);

		for (uint k = 0; k <= NUM_TESS / 2; k++) {

			// p : pi - angle of latitude
			float p = PI*2.0f / float(NUM_TESS) * float(k);

			// position, texcoord
			float x = radius * sin(p) * cos(t), y = radius * sin(p) * sin(t), z = radius * cos(p);
			float c1 = t / 2 / PI;
			float c2 = 1 - p / PI;

			planet_vertex_list.push_back({ vec3(x, y, z), vec3(x, y, z), vec2(c1, c2) });
		}
	}

	// Ring Init
	// i : longitude
	for (uint i = 0; i <= NUM_TESS; i++) {

		// t : theta - angle of longitude
		float t = PI*2.0f / float(NUM_TESS) * float(i);

		float x = radius * cos(t), y = radius * sin(t);

		ring_vertex_list.push_back({ vec3(x * 1.0f, y * 1.0f, 0), vec3(x * 1.0f, y * 1.0f, 0), vec2(0, t) });
		ring_vertex_list.push_back({ vec3(x * 0.6f, y * 0.6f, 0), vec3(x * 0.6f, y * 0.6f, 0), vec2(1, t) });
	}

	// texture initialize
	mapping_texture(&textures[0], "./textures/sun.jpg");
	mapping_texture(&textures[1], "./textures/mercury.jpg");
	mapping_texture(&textures[2], "./textures/venus.jpg");
	mapping_texture(&textures[3], "./textures/earth.jpg");
	mapping_texture(&textures[4], "./textures/mars.jpg");
	mapping_texture(&textures[5], "./textures/jupiter.jpg");
	mapping_texture(&textures[6], "./textures/saturn.jpg");
	mapping_texture(&textures[7], "./textures/uranus.jpg");
	mapping_texture(&textures[8], "./textures/neptune.jpg");
	mapping_texture(&textures[9], "./textures/moon.jpg");
	mapping_texture(&textures[10], "./textures/saturn-ring.jpg");
	mapping_texture(&textures[11], "./textures/saturn-ring-alpha.jpg");

	// texture bind
	for (int i = 0; i < NUM_TEXTURE; i++) {
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, textures[i]);
	}

	// Planet instance
	planet_list.push_back(Planet(-1, 0, 0.0f, 5.4f, 5.2f, 0.0f));       // Sun
	planet_list.push_back(Planet(-1, 1, 9.9f, 0.6f, 7.7f, 3.1f));       // Mercury
	planet_list.push_back(Planet(-1, 2, 15.8f, 1.0f, 15.6f, 3.9f));     // Venus
	planet_list.push_back(Planet(-1, 3, 19.3f, 1.0f, 1.0f, 4.4f));      // Earth
	planet_list.push_back(Planet(-1, 4, 24.2f, 0.7f, 1.0f, 5.1f));      // Mars
	planet_list.push_back(Planet(-1, 5, 36.8f, 3.3f, 0.6f, 8.1f));      // Jupiter
	planet_list.push_back(Planet(-1, 6, 61.4f, 3.1f, 0.6f, 10.2f));     // Saturn
	planet_list.push_back(Planet(-1, 7, 82.6f, 2.0f, 0.8f, 13.2f));     // Uranus
	planet_list.push_back(Planet(-1, 8, 103.6f, 2.0f, 0.8f, 15.6f));     // Neptune

	// Setellite instance
	planet_list.push_back(Planet(3, 9, 2.5f, 0.3f, 27.3f, 1.0f));  // Moon (Setellite of the Earth)

	planet_list.push_back(Planet(5, 9, 4.0f, 0.4f, 0.4f, 0.4f));  // Io (Setellite of the Jupiter)
	planet_list.push_back(Planet(5, 9, 8.5f, 0.5f, 4.0f, 4.0f));  // Callisto (Setellite of the Jupiter)
	planet_list.push_back(Planet(5, 9, 5.0f, 0.3f, 0.8f, 0.8f));  // Europa (Setellite of the Jupiter)
	planet_list.push_back(Planet(5, 9, 7.0f, 0.6f, 2.0f, 2.0f));  // Ganymede (Setellite of the Jupiter)

	planet_list.push_back(Planet(7, 9, 3.8f, 0.3f, 0.4f, 0.4f));  // Miranda (Setellite of the Uranus)
	planet_list.push_back(Planet(7, 9, 5.0f, 0.5f, 0.5f, 0.5f));  // Ariel (Setellite of the Uranus)
	planet_list.push_back(Planet(7, 9, 6.5f, 0.5f, 0.6f, 0.6f));  // Umbriel (Setellite of the Uranus)
	planet_list.push_back(Planet(7, 9, 8.0f, 0.7f, 0.8f, 0.8f));  // Titania (Setellite of the Uranus)
	planet_list.push_back(Planet(7, 9, 10.0f, 0.7f, 1.3f, 1.3f));  // Oberon (Setellite of the Uranus)

	planet_list.push_back(Planet(8, 9, 5.0f, 0.8f, -0.6f, -0.6f));  // Triton (Setellite of the Uranus)
	planet_list.push_back(Planet(8, 9, 7.0f, 0.5f, 1.1f, 30.0f));  // Oberon (Setellite of the Uranus)

	// Ring info
	saturn_ring_parent_index = 6;
	saturn_ring_radius = planet_list.at(6).radius * 2.0f;

}



bool user_init()
{
	// log hotkeys
	print_help();

	// init GL states
	glClearColor( 39/255.0f, 40/255.0f, 34/255.0f, 1.0f );	// set clear color
	glEnable( GL_CULL_FACE );								// turn on backface culling
	glEnable( GL_DEPTH_TEST );								// turn on depth tests
	glEnable(GL_TEXTURE_2D);                                // enable texturing for 2D
	glEnable(GL_BLEND);										// enable alpha
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// configure texture parameters
	// wrapping mode
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// filter mode
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	// define the position of four corner vertices
	update_circle_vertices();

	// create vertex buffer
	update_vertex_buffer();

	return true;
}

void user_finalize()
{
}

void main( int argc, char* argv[] )
{
	// initialization
	if(!glfwInit()){ printf( "[error] failed in glfwInit()\n" ); return; }

	// create window and initialize OpenGL extensions
	if(!(window = cg_create_window( window_name, window_size.x, window_size.y ))){ glfwTerminate(); return; }
	if(!cg_init_extensions( window )){ glfwTerminate(); return; }	// version and extensions

	// initializations and validations
	if(!(program=cg_create_program( vert_shader_path, frag_shader_path ))){ glfwTerminate(); return; }	// create and compile shaders/program
	if(!user_init()){ printf( "Failed to user_init()\n" ); glfwTerminate(); return; }					// user initialization

	// register event callbacks
	glfwSetWindowSizeCallback( window, reshape );	// callback for window resizing events
    glfwSetKeyCallback( window, keyboard );			// callback for keyboard events
	glfwSetMouseButtonCallback( window, mouse );	// callback for mouse click inputs
	glfwSetCursorPosCallback( window, motion );		// callback for mouse movement

	// enters rendering/event loop
	for( frame=0; !glfwWindowShouldClose(window); frame++ )
	{
		glfwPollEvents();	// polling and processing of events
		update();			// per-frame update
		render();			// per-frame render
	}
	
	// normal termination
	user_finalize();
	cg_destroy_window(window);
}