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


#include "utils/mesh.h"
#include "utils/shaders.h"
#include "utils/vbo.h"
#include "MAIN.H"



/*----------------------------------------------------------------------------
MESH TO LOAD
----------------------------------------------------------------------------*/
// this mesh is a dae file format but you should be able to use any other format too, obj is typically what is used
// put the mesh in your project directory, or provide a filepath for it here
#define MESH_NEUTRAL "models/high-res2/neutral.obj"
#define MESH_0 "models/high-res2/Mery_jaw_open.obj"


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


using namespace std;

GLuint shaderProgramID;

unsigned int mesh_vao = 0;
int width = 800;
int height = 600;


ModelData mesh_data_neutral;
std::vector<glm::vec3> deltaM;
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


	glutSwapBuffers();
}



void updateScene() {

	static DWORD last_time = 0;
	DWORD curr_time = timeGetTime();
	if (last_time == 0)
		last_time = curr_time;
	float delta = (curr_time - last_time) * 0.001f;
	last_time = curr_time;

	

	// Draw the next frame
	glutPostRedisplay();
}

void getDeltaM(const char* MESH)
{
	mesh_data_jaw_open = load_mesh(MESH);
	

	for (unsigned int i = 0; i < mesh_data_neutral.mPointCount; i++) {
		glm::vec3 vertice;
		vertice.x = mesh_data_neutral.mVertices[i].x - mesh_data_jaw_open.mVertices[i].x;
		vertice.y = mesh_data_neutral.mVertices[i].y - mesh_data_jaw_open.mVertices[i].y;
		vertice.z = mesh_data_neutral.mVertices[i].z - mesh_data_jaw_open.mVertices[i].z;
		std::cout << "deltaM  " << i << " " << glm::to_string(vertice) << std::endl;

		deltaM.push_back(vertice);
	}
}

void init()
{
	// Set up the shaders
	shaderProgramID = CompileShaders();

	mesh_data_neutral = load_mesh(MESH_NEUTRAL);


	getDeltaM(MESH_0);

	
	std::cout << mesh_data_neutral.mVertices[1000].x << std::endl;

	//std::cout << glm::to_string(deltaM[0]) << std::endl;
	//mesh_data.mVertices = deltaM;
	
	
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

		applyDeltaM(mesh_data_neutral, deltaM,0.2f);
		
		
	}
	if (key == 'u') {

		applyDeltaM(mesh_data_neutral, deltaM, -0.2f);


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
	// Set up your objects and shaders
	init();
	// Begin infinite event loop
	glutMainLoop();
	return 0;
}
