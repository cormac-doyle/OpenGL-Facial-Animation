// Windows includes (For Time, IO, etc.)
#include <windows.h>
#include <mmsystem.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <math.h>
#include <vector> // STL dynamic memory.

// OpenGL includes
#include <GL/glew.h>
#include <GL/freeglut.h>

// Assimp includes
#include <assimp/cimport.h> // scene importer
#include <assimp/scene.h> // collects data
#include <assimp/postprocess.h> // various extra operations

// Project includes
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include "glm/gtx/string_cast.hpp"

#include <imgui.h>
#include <imgui_impl_glut.h>
#include <imgui_impl_opengl2.h>


#include "utils/mesh.h"
#include "utils/shaders.h"
#include "utils/vbo.h"
#include "MAIN.H"

#include "string"



/*----------------------------------------------------------------------------
MESH TO LOAD
----------------------------------------------------------------------------*/
// this mesh is a dae file format but you should be able to use any other format too, obj is typically what is used
// put the mesh in your project directory, or provide a filepath for it here
#define MESH_NEUTRAL "models/high-res2/neutral.obj"


std::vector<std::string> mesh_file_names{
		"Mery_jaw_open.obj",
		"Mery_kiss.obj",
		"Mery_l_brow_lower.obj",
		"Mery_l_brow_narrow.obj",
		"Mery_l_brow_raise.obj",
		"Mery_l_eye_closed.obj",
		"Mery_l_eye_lower_open.obj",
		"Mery_l_eye_upper_open.obj",
		"Mery_l_nose_wrinkle.obj",
		"Mery_l_puff.obj",
		"Mery_l_sad.obj",
		"Mery_l_smile.obj",
		"Mery_l_suck.obj",
		"Mery_r_brow_lower.obj",
		"Mery_r_brow_narrow.obj",
		"Mery_r_brow_raise.obj",
		"Mery_r_eye_closed.obj",
		"Mery_r_eye_lower_open.obj",
		"Mery_r_eye_upper_open.obj",
		"Mery_r_nose_wrinkle.obj",
		"Mery_r_puff.obj",
		"Mery_r_sad.obj",
		"Mery_r_smile.obj",
		"Mery_r_suck.obj"
};

/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


using namespace std;

GLuint shaderProgramID;

unsigned int mesh_vao = 0;
int width = 800;
int height = 600;


ModelData mesh_data_neutral;
std::vector < std::vector<glm::vec3> > deltaMs;
ModelData mesh_data_jaw_open;


bool activate = false;
void loadNeutral(glm::mat4& modelNeutral, int matrix_location)
{

	modelNeutral = glm::mat4(1.0f);
	generateObjectBufferMesh(mesh_data_neutral, shaderProgramID);

	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, glm::value_ptr(modelNeutral));
	glDrawArrays(GL_TRIANGLES, 0, mesh_data_neutral.mPointCount);
}

void display() {
	ImGui_ImplOpenGL2_NewFrame();
	ImGui_ImplGLUT_NewFrame();
	ImGui::NewFrame();
	ImGui::Begin("Window");

	ImGui::End();


	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(shaderProgramID);

	// load mesh into a vertex buffer array

	//Declare your uniform variables that will be used in your shader
	int matrix_location = glGetUniformLocation(shaderProgramID, "model");
	int view_mat_location = glGetUniformLocation(shaderProgramID, "view");
	int proj_mat_location = glGetUniformLocation(shaderProgramID, "proj");


	// Root of the Hierarchy
	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 persp_proj = glm::perspective(45.0f, (float)width / (float)height, 0.1f, 1000.0f);
	
	view = glm::translate(view, glm::vec3(0.0, -20.0f, -50.0f));

	// update uniforms & draw
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, glm::value_ptr(persp_proj));
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, glm::value_ptr(view));


	glm::mat4 modelPlane;
	loadNeutral(modelPlane, matrix_location);

	ImGui::Render();
	glutSwapBuffers();
}



void updateScene() {

	static DWORD last_time = 0;
	DWORD curr_time = timeGetTime();
	if (last_time == 0)
		last_time = curr_time;
	float delta = (curr_time - last_time) * 0.001f;
	last_time = curr_time;

	ImGui::NewFrame();

	// Draw the next frame
	glutPostRedisplay();
}

void calcDeltaM(const char* MESH)
{
	mesh_data_jaw_open = load_mesh(MESH);
	
	std::vector<glm::vec3> deltaM;
	for (unsigned int i = 0; i < mesh_data_neutral.mPointCount; i++) {
		glm::vec3 vertice;
		vertice.x = mesh_data_neutral.mVertices[i].x - mesh_data_jaw_open.mVertices[i].x;
		vertice.y = mesh_data_neutral.mVertices[i].y - mesh_data_jaw_open.mVertices[i].y;
		vertice.z = mesh_data_neutral.mVertices[i].z - mesh_data_jaw_open.mVertices[i].z;

		deltaM.push_back(vertice);
	}
	deltaMs.push_back(deltaM);
}

void init()
{
	// Set up the shaders
	shaderProgramID = CompileShaders();

	mesh_data_neutral = load_mesh(MESH_NEUTRAL);

	std::cout << "Calculating deltaM vertices..." << std::endl;
	for (std::string name : mesh_file_names) {
		std::string filepath = "models/high-res2/" + name;
		calcDeltaM(filepath.c_str());

	}
	std::cout << "Finished loading deltaM vertices." << std::endl;

	

	
	
}

void applyDeltaM(ModelData& mesh_data_neutral, std::vector<glm::vec3> deltaM, float weight)
{
	for (unsigned int i = 0; i < mesh_data_neutral.mPointCount; i++) {
		mesh_data_neutral.mVertices[i].x -= deltaM[i].x * weight;
		mesh_data_neutral.mVertices[i].y -= deltaM[i].y * weight;
		mesh_data_neutral.mVertices[i].z -= deltaM[i].z * weight;
	}

}

// Placeholder code for the keypress
float rotate_speed = 10.0f;
void keypress(unsigned char key, int x, int y) {
	if (key == 'y') {

		applyDeltaM(mesh_data_neutral, deltaMs[0],0.1f);
		
	}
	if (key == 'u') {

		applyDeltaM(mesh_data_neutral, deltaMs[0], -0.1f);

	}
	if (key == 'h') {

		applyDeltaM(mesh_data_neutral, deltaMs[1], 0.1f);

	}
	if (key == 'j') {

		applyDeltaM(mesh_data_neutral, deltaMs[1], -0.1f);

	}
}



int main(int argc, char** argv) {

	// Set up the window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(width, height);
	glutCreateWindow("Hello Triangle");

	// Tell glut where the display function is
	glutDisplayFunc(display);
	glutIdleFunc(updateScene);
	glutKeyboardFunc(keypress);

	// A call to glewInit() must be done after glut is initialized!
	GLenum res = glewInit();
	// Check for any errors
	if (res != GLEW_OK) {
		fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
		return 1;
	}

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	// Setup Dear ImGui style
	ImGui::StyleColorsClassic();

	// Setup Platform/Renderer bindings
	ImGui_ImplGLUT_Init();
	ImGui_ImplGLUT_InstallFuncs(); // use the imgui glut funcs
	ImGui_ImplOpenGL2_Init();

	// Set up your objects and shaders
	init();
	// Begin infinite event loop
	glutMainLoop();

	// imgui cleanup
	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplGLUT_Shutdown();
	ImGui::DestroyContext();
	return 0;
}
 