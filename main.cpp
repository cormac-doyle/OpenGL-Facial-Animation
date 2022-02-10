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


#include "utils/mesh.h"
#include "utils/shaders.h"
#include "utils/vbo.h"



/*----------------------------------------------------------------------------
MESH TO LOAD
----------------------------------------------------------------------------*/
// this mesh is a dae file format but you should be able to use any other format too, obj is typically what is used
// put the mesh in your project directory, or provide a filepath for it here
#define MESH_PLANE "models/simplePlane.dae"
#define MESH_PROPELLOR "models/propellor.dae"

/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/



using namespace std;

GLuint shaderProgramID;

unsigned int mesh_vao = 0;
int width = 800;
int height = 600;

GLfloat rotate_propellor = 0.0f;
float propellor_pos_x = 2.1f;

GLfloat plane_rotate_x = 0.0f;
GLfloat plane_rotate_y = 0.0f;
GLfloat plane_rotate_z = 0.0f;


ModelData mesh_data;

void loadPropellor(glm::mat4& modelPlane, int matrix_location, float propellor_pos)
{
	mesh_data = generateObjectBufferMesh(MESH_PROPELLOR, shaderProgramID);
	// Set up the child matrix
	glm::mat4 modelPropellor = glm::mat4(1.0f);
	modelPropellor = glm::translate(modelPropellor, glm::vec3(propellor_pos, -0.4f, 2.2f));
	modelPropellor = glm::rotate(modelPropellor, glm::radians(rotate_propellor), glm::vec3(0, 0, 1));

	// Apply the root matrix to the child matrix
	modelPropellor = modelPlane * modelPropellor;

	// Update the appropriate uniform and draw the mesh again
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, glm::value_ptr(modelPropellor));
	glDrawArrays(GL_TRIANGLES, 0, mesh_data.mPointCount);
}

void loadPlane(glm::mat4& modelPlane, int matrix_location)
{
	mesh_data = generateObjectBufferMesh(MESH_PLANE, shaderProgramID);

	modelPlane = glm::mat4(1.0f);
	modelPlane = glm::yawPitchRoll(glm::radians(plane_rotate_x), glm::radians(plane_rotate_y), glm::radians(plane_rotate_z));

	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, glm::value_ptr(modelPlane));
	glDrawArrays(GL_TRIANGLES, 0, mesh_data.mPointCount);
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
	
	view = glm::translate(view, glm::vec3(0.0, 0.0, -10.0f));

	// update uniforms & draw
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, glm::value_ptr(persp_proj));
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, glm::value_ptr(view));


	glm::mat4 modelPlane;
	loadPlane(modelPlane, matrix_location);

	loadPropellor(modelPlane, matrix_location, propellor_pos_x);
	loadPropellor(modelPlane, matrix_location, propellor_pos_x*-1);


	glutSwapBuffers();
}



void updateScene() {

	static DWORD last_time = 0;
	DWORD curr_time = timeGetTime();
	if (last_time == 0)
		last_time = curr_time;
	float delta = (curr_time - last_time) * 0.001f;
	last_time = curr_time;

	// Rotate the model slowly around the y axis at 20 degrees per second
	rotate_propellor += 200.0f * delta;
	rotate_propellor = fmodf(rotate_propellor, 360.0f);

	// Draw the next frame
	glutPostRedisplay();
}


void init()
{
	// Set up the shaders
	shaderProgramID = CompileShaders();
	
}

// Placeholder code for the keypress
float rotate_speed = 10.0f;
void keypress(unsigned char key, int x, int y) {
	if (key == 'y') {
		plane_rotate_x += rotate_speed;
	}
	if (key == '6') {
		plane_rotate_x -= rotate_speed;
	}
	if (key == 'p') {
		plane_rotate_y += rotate_speed;
	}
	if (key == '0') {
		plane_rotate_y -= rotate_speed;
	}
	if (key == 'r') {
		plane_rotate_z += rotate_speed;
	}
	if (key == '4') {
		plane_rotate_z -= rotate_speed;
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
