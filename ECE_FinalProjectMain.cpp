/*
Author: Ketaki Jain
Class: ECE6122
Last Date Modified: 03rd December 2021
Description: This is a submission for the Final Project-GaTEch Buzzy Bowl. It consists of a football field with
			 15 UAVS.

			 They begin motion, towards  the centre of the sphere of radius 10 m(33 feet), located at (0,0,150) after 5 s of start of simulation.
			 Once they reach the surface, they undergo random motion along the surface of the virtual sphere.
			 Motion path is generated using a tangential vector, to the surface of the sphere, and the position vector of the current point,
			 towards the centre.
			 Surface Motion: A random point on the sphere is assigned to every UAV. The UAV tries to reach this point. Once 
			 it is close enough, another random point is generated and assigned, and the UAV chases that point along the sphere.

			 The implementation is multithreaded-every UAV has one thread that describes its motion and updates parameters.
			 the thread fuction is in the ECE_UAV.cpp file, and the ECE_UAV class definition  is in the ECE_UAV.h header file.

			 Each UAV can undergo elastic collisions with other UAVs.
			 The elastic collision is when the UAVs are very close. This may be difficult to see, since the sphere is 3D, and almost
			 transperent. If seen carefully, you can make out the collision and the exchange of velocity.

			 This happens for a period of 60s, post which the simulation stops.
			 The UAVs have been textured with "Suzzanne" texture.
			 The UAVs are changing the intensity of their colour from 50% to 100%, giving a blinking effect, throughout the simulation.
			 
*/



// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include<iostream>
#include<random>
#include<cmath>
#include<chrono>

using namespace std;

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include <common/shader.hpp>
#include <common/texture.hpp>
#include <common/controls.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>

#include "E:\APT0\BuildStuff2\ECE_UAV.h"

ECE_UAV uavs[15];										//UAVs global

int main(void)
{
	//Initialize beginning positions on the football field
	ECE_UAV uavs[15];
	float fNumX = 65.0f;
	float fNumY = 70.0f;

	double uav_pos[3] = { 0.0, 0.0, 0.0 };

	int ijk = 0;
	for (int i = -1; i < 2; i++)
	{
		float offsetY = fNumY * float(i);
		for (int j = -2; j < 3; j++)
		{
			float offsetX = float(j) * fNumX;
			uav_pos[0] = offsetX;
			uav_pos[1] = offsetY;
			uavs[ijk].position(uav_pos);
			ijk++;
		}
	}

	// Initialise GLFW
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		//getchar();
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(1024, 768, "GaTech Buzzy Bowl", NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "Failed to open GLFW window.\n");
		//getchar();
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		//getchar();
		glfwTerminate();
		return -1;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	// Set the mouse at the center of the screen
	glfwPollEvents();
	glfwSetCursorPos(window, 1024 / 2, 768 / 2);

	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.2f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);

	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	// Create and compile our GLSL program from the shaders
	GLuint programID = LoadShaders("StandardShading.vertexshader", "StandardShading.fragmentshader");
	GLuint programID_2 = LoadShaders("StandardShading.vertexshader", "StandardShading.fragmentshader");

	// Get a handle for our "MVP" uniform
	GLuint MatrixID = glGetUniformLocation(programID, "MVP");
	GLuint ViewMatrixID = glGetUniformLocation(programID, "V");
	GLuint ModelMatrixID = glGetUniformLocation(programID, "M");
	GLfloat intensityScale = glGetUniformLocation(programID, "IntensityScale");
	GLfloat transpScale = glGetUniformLocation(programID, "Transperency");
	float tmpIntensity = 0.5f;
	intensityScale = 1.0;

	// Load the texture
	GLuint Texture = loadDDS("uvmap.DDS");					//texture for monkeys
	GLuint TextureF = loadBMP_custom("ff.bmp");				//load texture for football field

	GLuint TextureID = glGetUniformLocation(programID, "myTextureSampler");


	// Read our .obj file-FOOTBALL FIELD
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;
	bool res = loadOBJ("footballfield.obj", vertices, uvs, normals);

	std::vector<unsigned short> indices;
	std::vector<glm::vec3> indexed_vertices;
	std::vector<glm::vec2> indexed_uvs;
	std::vector<glm::vec3> indexed_normals;
	indexVBO(vertices, uvs, normals, indices, indexed_vertices, indexed_uvs, indexed_normals);

	// Load it into a VBO

	GLuint vertexbuffer;
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_vertices.size() * sizeof(glm::vec3), &indexed_vertices[0], GL_STATIC_DRAW);

	GLuint uvbuffer;
	glGenBuffers(1, &uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_uvs.size() * sizeof(glm::vec2), &indexed_uvs[0], GL_STATIC_DRAW);

	GLuint normalbuffer;
	glGenBuffers(1, &normalbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_normals.size() * sizeof(glm::vec3), &indexed_normals[0], GL_STATIC_DRAW);

	// Generate a buffer for the indices as well
	GLuint elementbuffer;
	glGenBuffers(1, &elementbuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0], GL_STATIC_DRAW);

	////Read obj- SUZIES

	std::vector<glm::vec3> vertices_Suz;
	std::vector<glm::vec2> uvs_Suz;
	std::vector<glm::vec3> normals_Suz;
	res = loadOBJ("suzanne.obj", vertices_Suz, uvs_Suz, normals_Suz);

	std::vector<unsigned short> indices_Suz;
	std::vector<glm::vec3> indexed_vertices_Suz;
	std::vector<glm::vec2> indexed_uvs_Suz;
	std::vector<glm::vec3> indexed_normals_Suz;
	indexVBO(vertices_Suz, uvs_Suz, normals_Suz, indices_Suz, indexed_vertices_Suz, indexed_uvs_Suz, indexed_normals_Suz);

	// Load it into a VBO

	GLuint vertexbuffer_Suz;
	glGenBuffers(1, &vertexbuffer_Suz);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_Suz);
	glBufferData(GL_ARRAY_BUFFER, indexed_vertices_Suz.size() * sizeof(glm::vec3), &indexed_vertices_Suz[0], GL_STATIC_DRAW);

	GLuint uvbuffer_Suz;
	glGenBuffers(1, &uvbuffer_Suz);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer_Suz);
	glBufferData(GL_ARRAY_BUFFER, indexed_uvs_Suz.size() * sizeof(glm::vec2), &indexed_uvs_Suz[0], GL_STATIC_DRAW);

	GLuint normalbuffer_Suz;
	glGenBuffers(1, &normalbuffer_Suz);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer_Suz);
	glBufferData(GL_ARRAY_BUFFER, indexed_normals_Suz.size() * sizeof(glm::vec3), &indexed_normals_Suz[0], GL_STATIC_DRAW);

	// Generate a buffer for the indices as well
	GLuint elementbuffer_Suz;
	glGenBuffers(1, &elementbuffer_Suz);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer_Suz);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_Suz.size() * sizeof(unsigned short), &indices_Suz[0], GL_STATIC_DRAW);


	//VIRTUAL SPHERE obj

	std::vector<glm::vec3> vertices_Vsph;
	std::vector<glm::vec2> uvs_Vsph;
	std::vector<glm::vec3> normals_Vsph;
	res = loadOBJ("sphere.obj", vertices_Vsph, uvs_Vsph, normals_Vsph);

	std::vector<unsigned short> indices_Vsph;
	std::vector<glm::vec3> indexed_vertices_Vsph;
	std::vector<glm::vec2> indexed_uvs_Vsph;
	std::vector<glm::vec3> indexed_normals_Vsph;
	indexVBO(vertices_Vsph, uvs_Vsph, normals_Vsph, indices_Vsph, indexed_vertices_Vsph, indexed_uvs_Vsph, indexed_normals_Vsph);

	// Load it into a VBO

	GLuint vertexbuffer_Vsph;
	glGenBuffers(1, &vertexbuffer_Vsph);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_Vsph);
	glBufferData(GL_ARRAY_BUFFER, indexed_vertices_Vsph.size() * sizeof(glm::vec3), &indexed_vertices_Vsph[0], GL_STATIC_DRAW);

	GLuint uvbuffer_Vsph;
	glGenBuffers(1, &uvbuffer_Vsph);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer_Vsph);
	glBufferData(GL_ARRAY_BUFFER, indexed_uvs_Vsph.size() * sizeof(glm::vec2), &indexed_uvs_Vsph[0], GL_STATIC_DRAW);

	GLuint normalbuffer_Vsph;
	glGenBuffers(1, &normalbuffer_Vsph);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer_Vsph);
	glBufferData(GL_ARRAY_BUFFER, indexed_normals_Vsph.size() * sizeof(glm::vec3), &indexed_normals_Vsph[0], GL_STATIC_DRAW);

	// Generate a buffer for the indices as well
	GLuint elementbuffer_Vsph;
	glGenBuffers(1, &elementbuffer_Vsph);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer_Vsph);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_Vsph.size() * sizeof(unsigned short), &indices_Vsph[0], GL_STATIC_DRAW);


	// Get a handle for our "LightPosition" uniform
	glUseProgram(programID);
	GLuint LightID = glGetUniformLocation(programID, "LightPosition_worldspace");
	GLuint TranspID = glGetUniformLocation(programID, "Transperency");

	// For speed computation
	double lastTime = glfwGetTime();
	int nbFrames = 0;

	float rotAngle = (90.f / (2.0f * 3.14156f));
	//start UAVs
	for (int jj = 0; jj < 15; jj++)
	{
		uavs[jj].start();
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	do {
		// Measure speed
		double currentTime = glfwGetTime();
		nbFrames++;
		if (currentTime - lastTime >= 1.0) { // If last prinf() was more than 1sec ago
			// printf and reset
			printf("%f ms/frame\n", 1000.0 / double(nbFrames));
			nbFrames = 0;
			lastTime += 1.0;
		}

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		// Compute the MVP matrix from keyboard and mouse input
		computeMatricesFromInputs();
		glm::mat4 ProjectionMatrix = getProjectionMatrix();
		glm::mat4 ViewMatrix = getViewMatrix();


		////// Start of the rendering of Football field //////

		// Use our shader
		glUseProgram(programID);
		intensityScale = 1.0;

		glm::vec3 lightPos = glm::vec3(4, 40, 40);	//(4,4,4)
		glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]); // This one doesn't change between objects, so this can be done once for all objects that use "programID"

		//DRAW FOOTBALL FIELD
		glm::mat4 ModelMatrix1 = glm::mat4(1.0);
		ModelMatrix1 = glm::translate(ModelMatrix1, vec3(4.0f, 0.0f, 0.0f));
		ModelMatrix1 = glm::scale(ModelMatrix1, vec3(1.0f, 1.0f, 1.0f));
		glm::mat4 MVP1 = ProjectionMatrix * ViewMatrix * ModelMatrix1;

		// Send our transformation to the currently bound shader, 
		// in the "MVP" uniform
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP1[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix1[0][0]);


		// Bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, TextureF);
		// Set our "myTextureSampler" sampler to use Texture Unit 0
		glUniform1i(TextureID, 0);

		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(
			0,                  // attribute
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		// 2nd attribute buffer : UVs
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
		glVertexAttribPointer(1,2,GL_FLOAT, GL_FALSE,0, (void*)0);

		// 3rd attribute buffer : normals
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
		glVertexAttribPointer(2, 3,GL_FLOAT,GL_FALSE,0,(void*)0);

		// Index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);

		// Draw the triangles !
		glDrawElements(
			GL_TRIANGLES,      // mode
			indices.size(),    // count
			GL_UNSIGNED_SHORT,   // type
			(void*)0           // element array buffer offset
		);

		//Render UAVs
		double* dCurPos;
		tmpIntensity += 0.005;
		if (tmpIntensity >= 1.0)
		{
			tmpIntensity = 0.5;
		}

		for (int j = 0; j < 15; j++)
		{

			dCurPos = uavs[j].getPosition();

			glm::mat4 ModelMatrixX = glm::mat4(1.0);
			ModelMatrixX = translate(ModelMatrixX, vec3(dCurPos[0], dCurPos[1], dCurPos[2]));
			ModelMatrixX = scale(ModelMatrixX, vec3(3.5f, 3.5f, 3.5f));
			ModelMatrixX = rotate(ModelMatrixX, rotAngle, vec3(1.0f, 0.0f, 0.0f));
			glm::mat4 MVPX = ProjectionMatrix * ViewMatrix * ModelMatrixX;

			// Send our transformation to the currently bound shader, 
			// in the "MVP" uniform
			glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVPX[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrixX[0][0]);
			glUniform1f(TranspID, tmpIntensity);

			// Bind our texture in Texture Unit 0
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, Texture);
			// Set our "myTextureSampler" sampler to use Texture Unit 0
			glUniform1i(TextureID, 0);

			///******
			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_Suz);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

			// 2nd attribute buffer : UVs
			glEnableVertexAttribArray(1);
			glBindBuffer(GL_ARRAY_BUFFER, uvbuffer_Suz);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

			// 3rd attribute buffer : normals
			glEnableVertexAttribArray(2);
			glBindBuffer(GL_ARRAY_BUFFER, normalbuffer_Suz);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

			// Index buffer
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer_Suz);

			//***********
			// Draw the triangles !
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC0_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glDrawElements(GL_TRIANGLES, indices_Suz.size(), GL_UNSIGNED_SHORT, (void*)0);
			glDisable(GL_BLEND);
		}

		//RENDER VIRTUAL SPHERE

		glm::mat4 ModelMatrixVsph = glm::mat4(1.0);
		ModelMatrixVsph = translate(ModelMatrixVsph, vec3(0.0f, 0.0f, 150.0f));
		ModelMatrixVsph = scale(ModelMatrixVsph, vec3(10.0f, 10.0f, 10.0f));
		glm::mat4 MVPVsph = ProjectionMatrix * ViewMatrix * ModelMatrixVsph;

		// Send our transformation to the currently bound shader, 
		// in the "MVP" uniform
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVPVsph[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrixVsph[0][0]);
		glUniform1f(TranspID, 0.3);
		// Bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture);
		// Set our "myTextureSampler" sampler to use Texture Unit 0
		glUniform1i(TextureID, 0);

		///******
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_Vsph);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		// 2nd attribute buffer : UVs
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer_Vsph);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

		// 3rd attribute buffer : normals
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, normalbuffer_Vsph);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		// Index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer_Vsph);

		//glDisable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC0_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		//***********
		// Draw the triangles !
		glDrawElements(GL_TRIANGLES, indices_Vsph.size(), GL_UNSIGNED_SHORT, (void*)0);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDisable(GL_BLEND);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0);

	for (int ii = 0; ii < 15; ii++)
	{
		uavs[ii].stop();
	}

	// Cleanup VBO and shader
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteBuffers(1, &uvbuffer);
	glDeleteBuffers(1, &normalbuffer);
	glDeleteBuffers(1, &elementbuffer);
	glDeleteProgram(programID);
	glDeleteTextures(1, &Texture);
	glDeleteVertexArrays(1, &VertexArrayID);

	glDeleteBuffers(1, &vertexbuffer_Suz);
	glDeleteBuffers(1, &uvbuffer_Suz);
	glDeleteBuffers(1, &normalbuffer_Suz);
	glDeleteBuffers(1, &elementbuffer_Suz);


	glDeleteBuffers(1, &vertexbuffer_Vsph);
	glDeleteBuffers(1, &uvbuffer_Vsph);
	glDeleteBuffers(1, &normalbuffer_Vsph);
	glDeleteBuffers(1, &elementbuffer_Vsph);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

