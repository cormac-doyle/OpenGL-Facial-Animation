// Windows includes (For Time, IO, etc.)
#include <windows.h>
#include <mmsystem.h>
#include <iostream>
#include <string>
#include <fstream>
#include <stdio.h>
#include <math.h>
#include <vector> // STL dynamic memory.

// OpenGL includes
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/glut.h>

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

#include<Eigen/Dense>

#include <imgui.h>
#include <imgui_impl_glut.h>
#include <imgui_impl_opengl3.h>

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

std::vector<std::string> labels;
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
int width = 1400;
int height = 1050;
static float aspect = width/ (float) height;

ModelData mesh_data_neutral_original;
ModelData mesh_data_neutral;
std::vector<ModelData> facialExpressions;


std::vector < std::vector<glm::vec3> > deltaMs;
std::vector<float> mWeights;
ModelData mesh_data;

bool playAnim = false;
vector<vector<float>> animationWeights;



void removeWordFromLine(std::string& line, const std::string& word)
{
	auto n = line.find(word);
	if (n != std::string::npos)
	{
		line.erase(n, word.length());
	}
}

bool activate = false;
glm::vec3 rotate_face = glm::vec3(0, -10.0f, 0);
void loadNeutral(glm::mat4& modelNeutral, int matrix_location)
{

	modelNeutral = glm::mat4(1.0f);
	modelNeutral = glm::rotate(modelNeutral, glm::radians(rotate_face.y), glm::vec3(0, 1, 0));

	generateObjectBufferMesh(mesh_data_neutral, shaderProgramID);

	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, glm::value_ptr(modelNeutral));
	glDrawArrays(GL_TRIANGLES, 0, mesh_data_neutral.mPointCount);
}

void applyDeltaM(ModelData& mesh_data_neutral, std::vector<glm::vec3> deltaM, float weight)
{
	
	for (unsigned int i = 0; i < mesh_data_neutral.mPointCount; i++) {
		mesh_data_neutral.mVertices[i].x -= deltaM[i].x * weight;
		mesh_data_neutral.mVertices[i].y -= deltaM[i].y * weight;
		mesh_data_neutral.mVertices[i].z -= deltaM[i].z * weight;
	}

}


glm::vec3 vertexPicker(int x, int y, glm::mat4 VM, glm::mat4 P, std::vector<ModelData> meshes, int& mesh_index, int& vertex_index)
{
	glm::vec3 window;
	window.x = x;
	window.y = height - y - 1;
	glReadPixels(x, height - y - 1, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &window.z);

	glm::vec3 object = glm::unProject(window, VM, P, glm::vec4(0.0f, 0.0f, width, height));
	std::cout << "object coords are " << glm::to_string(object) << endl;

	// find nearest vertex 
	GLfloat dist = 10;
	GLfloat temp = 0.0f;
	GLuint m_index = 0; // index of mesh with closest vertex
	GLuint v_index = 0; // index of closest vertex

	// brute force through all vertices to find nearest one 
	for (int i = 0; i < meshes.size(); i++) {
		for (int j = 0; j < meshes[i].mVertices.size(); j++) {

			temp = glm::distance(object, meshes[i].mVertices[j]);
			if (temp <= dist) {
				dist = temp;
				m_index = i;
				v_index = j;
			}
			
		}
	}
	mesh_index = m_index;
	vertex_index = v_index;
	glm::vec3 vertex = meshes[m_index].mVertices[v_index];
	//constraints.push_back(v_index); //add index of constrained vertex to list of constraints
	//mo.conservativeResize(constraints.size() * 3);
	//mo(3 * constraints.size() - 3) = vertex.x;
	//mo(3 * constraints.size() - 2) vertex.y;
	//mo(3 * constraints.size() - 1) vertex.z;
	std::cout << "vertex position: " << glm::to_string(vertex) << std::endl;
	return vertex;

}

int findClosestExpression(int x, int y, glm::mat4 VM, glm::mat4 P, int chosenVertexIndex, std::vector<ModelData> expressionMeshes) {
	glm::vec3 window;
	window.x = x;
	window.y = height - y - 1;
	glReadPixels(x, height - y - 1, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &window.z);

	glm::vec3 object = glm::unProject(window, VM, P, glm::vec4(0.0f, 0.0f, width, height));
	std::cout << "object coords are " << glm::to_string(object) << endl;
	GLfloat dist = 100000000;
	GLfloat temp = 0.0f;
	GLuint m_index = 0; // index of mesh with closest vertex
	GLuint v_index = 0; // index of closest vertex

	std::cout << "findClosestExpression Debugging "<< std::endl;
	std::cout << "chosenVertexIndex: " << chosenVertexIndex<<std::endl;

	
	for (int i = 0; i < expressionMeshes.size(); i++) {
		

			temp = glm::distance(object, expressionMeshes[i].mVertices[chosenVertexIndex]);
			std::cout << mesh_file_names[i] <<" distance " << temp << std::endl;
			if (temp <= dist) {
				dist = temp;
				m_index = i;
				
			}

		
	}
	return m_index;
}


glm::mat4 view;
glm::mat4 persp_proj;
void display() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGLUT_NewFrame();
	
	ImGui::Begin("Facial Feature Weight Controls");
	for (int i = 0; i < labels.size();i++) {
		
		ImGui::SliderFloat(labels[i].c_str(), &mWeights[i], 0.0f, 1.0f);
		
	}
	ImGui::Text("Rotate Face:");
	ImGui::SliderFloat(" ", &rotate_face.y, -180.0f, 180.0f);
	if (ImGui::Button("Play Animation")) {
		playAnim = true;
	}
	ImGui::End();

	ImGui::Render();
	ImGuiIO& io = ImGui::GetIO();
	glViewport(0, 0, (GLsizei)io.DisplaySize.x, (GLsizei)io.DisplaySize.y);

	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


	glUseProgram(shaderProgramID);

	// load mesh into a vertex buffer array

	//Declare your uniform variables that will be used in your shader
	int matrix_location = glGetUniformLocation(shaderProgramID, "model");
	int view_mat_location = glGetUniformLocation(shaderProgramID, "view");
	int proj_mat_location = glGetUniformLocation(shaderProgramID, "proj");


	// Root of the Hierarchy
	view = glm::mat4(1.0f);
	persp_proj = glm::perspective(45.0f, (float)width / (float)height, 0.1f, 1000.0f);
	
	view = glm::translate(view, glm::vec3(10.0, -15.0f, -50.0f));

	// update uniforms & draw
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, glm::value_ptr(persp_proj));
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, glm::value_ptr(view));

	glm::mat4 modelNeutralFace;
	loadNeutral(modelNeutralFace, matrix_location);

	glutSwapBuffers();
	
}


int frame_num = 0;
int mouse_x;
bool mouseClickedDown = false;
bool mouseClickedUp = false;

glm::vec2 mousePosDown;
glm::vec2 mousePosUp;
int chosen_vertex_index = -1;
void updateScene() {

	static DWORD last_time = 0;
	DWORD curr_time = timeGetTime();
	if (last_time == 0)
		last_time = curr_time;
	float delta = (curr_time - last_time) * 0.001f;
	last_time = curr_time;

	mesh_data_neutral = mesh_data_neutral_original;
	
	
	if (mouseClickedDown) {
		std::cout << "MOUSE DOWN X: " << mousePosDown.x << std::endl;
		std::cout << "MOUSE DOWN Y: " << mousePosDown.y << std::endl;
		std::vector<ModelData> neutral;
		neutral.push_back(mesh_data_neutral);
		int mesh_index = -1;
		

		glm::vec3 mouseVertex = vertexPicker((int) mousePosDown.x, (int) mousePosDown.y, view, persp_proj, neutral, mesh_index, chosen_vertex_index);
		std::cout << "mesh index " << mesh_index << std::endl;
		std::cout << "chosen vertix is: " << glm::to_string(mouseVertex) << std::endl;

		mouseClickedDown = false;
	}
	if (mouseClickedUp) {
		std::cout << "MOUSE UP X: " << mousePosUp.x << std::endl;
		std::cout << "MOUSE UP Y: " << mousePosUp.y << std::endl;
		//std::vector<ModelData> fa;
		//neutral.push_back(mesh_data_neutral);
		

		int mesh_index =findClosestExpression( (int) mousePosUp.x, (int) mousePosUp.y, view, persp_proj, chosen_vertex_index, facialExpressions);
		mWeights[mesh_index]=1.0f;
		std::cout << "mesh index " << mesh_index << std::endl;
		
		mouseClickedUp = false;
	}

	if (playAnim) {

		for (int i = 0; i < mesh_file_names.size(); i++) {
			applyDeltaM(mesh_data_neutral, deltaMs[i], animationWeights[frame_num][i]);
		}
		frame_num++;
		if (frame_num == animationWeights.size()) {
			playAnim = false;
		}
	}
	else {
		for (int i = 0; i < mesh_file_names.size(); i++) {
			applyDeltaM(mesh_data_neutral, deltaMs[i], mWeights[i]);
			frame_num = 0;
		}
	}

	// Draw the next frame
	glutPostRedisplay();
}

//std::vector
void calcDeltaM(ModelData mesh_data)
{
	
	
	std::vector<glm::vec3> deltaM;
	for (unsigned int i = 0; i < mesh_data_neutral.mPointCount; i++) {
		glm::vec3 vertice;
		vertice.x = mesh_data_neutral.mVertices[i].x - mesh_data.mVertices[i].x;
		vertice.y = mesh_data_neutral.mVertices[i].y - mesh_data.mVertices[i].y;
		vertice.z = mesh_data_neutral.mVertices[i].z - mesh_data.mVertices[i].z;

		deltaM.push_back(vertice);
	}
	deltaMs.push_back(deltaM);
	float weight = 0.0f;
	mWeights.push_back(weight);
}


//read file code found here
//https://www.tutorialspoint.com/how-to-read-a-text-file-with-cplusplus#:~:text=close()%20method.-,Call%20open()%20method%20to%20open%20a%20file%20%E2%80%9Ctpoint.,it%20into%20the%20string%20tp.
void read_anim_text_file() {
	fstream newfile;
	std::string delimiter = " ";
	std::string weight;
	newfile.open("blendshape_animation.txt", ios::in); //open a file to perform read operation using file object
	if (newfile.is_open()) { //checking whether the file is open
		string line;
		while (getline(newfile, line)) { //read data from file object and put it into string.
			//cout << line << "\n"; //print the data of the string
		
			size_t pos = 0;
			vector<float> lineWeights;
		
			//split line by a delimiter found here: https://stackoverflow.com/questions/14265581/parse-split-a-string-in-c-using-string-delimiter-standard-c
			while ((pos = line.find(delimiter)) != std::string::npos) {
				std::string weight = line.substr(0, pos);
				//std::cout << weight << std::endl;
				lineWeights.push_back(std::stof(weight));

				line.erase(0, pos + delimiter.length());
			}
			animationWeights.push_back(lineWeights);

		}
	newfile.close(); //close the file object.
	}
}

//found in power point real-time animation slides



void init()
{
	read_anim_text_file();

	for (std::string name: mesh_file_names) {
		std::string label = name;
		removeWordFromLine(label,".obj");
		removeWordFromLine(label, "Mery_");

		labels.push_back(label);
	}
	// Set up the shaders
	shaderProgramID = CompileShaders();

	mesh_data_neutral = load_mesh(MESH_NEUTRAL);
	mesh_data_neutral_original = load_mesh(MESH_NEUTRAL);

	std::cout << "Calculating deltaM vertices..." << std::endl;
	for (std::string name : mesh_file_names) {
		std::string filepath = "models/high-res2/" + name;

		mesh_data = load_mesh(filepath.c_str());
		facialExpressions.push_back(mesh_data);

		calcDeltaM(mesh_data);

	}
	std::cout << "Finished loading deltaM vertices." << std::endl;

}



// Placeholder code for the keypress
float rotate_speed = 10.0f;


void reshape(GLint w, GLint h) {
	// imgui reshape func
	ImGui_ImplGLUT_ReshapeFunc(w, h);

	glViewport(0, 0, w, h);
	aspect = float(w / h);
}


void mouseFunction(int button, int state, int x, int y) {
	if (x > 490) {
		if (button == 0 && state == GLUT_DOWN) {

			mousePosDown.x = x;
			mousePosDown.y = y;

			mouseClickedDown = true;
		}
		if (button == 0 && state == GLUT_UP) {

			mousePosUp.x = x;
			mousePosUp.y = y;

			mouseClickedUp = true;
		}
	}
	
	ImGui_ImplGLUT_MouseFunc(button,state,x,y);
}

int main(int argc, char** argv) {

	// Set up the window
	glutInit(&argc, argv);

	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_MULTISAMPLE);
	glutInitWindowSize(width, height);
	glutCreateWindow("Facial Animation");

	// Tell glut where the display function is
	glutDisplayFunc(display);
	glutIdleFunc(updateScene);
	
	//glutReshapeFunc(reshape);
	//glutSetOption();

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
	//ImGuiIO& io = ImGui::GetIO(); (void)io;
	// Setup Dear ImGui style
	ImGui::StyleColorsClassic();

	// Setup Platform/Renderer bindings
	ImGui_ImplGLUT_Init();
	ImGui_ImplGLUT_InstallFuncs(); // use the imgui glut funcs
	glutMouseFunc(mouseFunction);
	ImGui_ImplOpenGL3_Init();

	// Set up your objects and shaders
	init();
	// Begin infinite event loop
	glutMainLoop();

	// imgui cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGLUT_Shutdown();
	ImGui::DestroyContext();
	return 0;
}

 