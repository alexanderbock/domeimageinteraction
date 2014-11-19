#include <stdlib.h>
#include <stdio.h>
#include "sgct.h"
#include <iostream>
#include <sstream>

sgct::Engine * gEngine;

void myDrawFun();
void myPreSyncFun();
void myInitOGLFun();
void myEncodeFun();
void myDecodeFun();
void myCleanUpFun();

size_t myTextureHandle;
sgct_utils::SGCTBox * myBox = NULL;
GLint Matrix_Loc = -1;

//variables to share across cluster
sgct::SharedDouble curr_time(0.0);

sgct::SharedFloat posX(0.f);
sgct::SharedFloat posY(0.f);
sgct::SharedFloat posZ(0.f);

sgct::SharedFloat rotation(0.f);

void controlCallback(const char* msg, int length, int index);

int main( int argc, char* argv[] )
{
	gEngine = new sgct::Engine( argc, argv );

	gEngine->setInitOGLFunction( myInitOGLFun );
	gEngine->setDrawFunction( myDrawFun );
	gEngine->setPreSyncFunction( myPreSyncFun );
	gEngine->setCleanUpFunction( myCleanUpFun );
	gEngine->setExternalControlCallback(controlCallback);

	if( !gEngine->init( sgct::Engine::OpenGL_3_3_Core_Profile ) )
	{
		delete gEngine;
		return EXIT_FAILURE;
	}

	sgct::SharedData::instance()->setEncodeFunction(myEncodeFun);
	sgct::SharedData::instance()->setDecodeFunction(myDecodeFun);

	// Main loop
	gEngine->render();

	// Clean up
	delete gEngine;

	// Exit program
	exit( EXIT_SUCCESS );
}

void myDrawFun()
{
	glEnable( GL_DEPTH_TEST );
	glEnable( GL_CULL_FACE );

	double speed = 25.0;

	//create scene transform (animation)
	glm::mat4 scene_mat = glm::translate( glm::mat4(1.0f), glm::vec3( 0.0f, 0.0f, -3.0f) );
	scene_mat = glm::translate(scene_mat, glm::vec3(posX.getVal(), posY.getVal(), posZ.getVal()));
	scene_mat = glm::rotate(scene_mat, rotation.getVal(), glm::vec3(0.f, -1.f, 0.f));
	//scene_mat = glm::rotate( scene_mat, static_cast<float>( curr_time.getVal() * speed ), glm::vec3(0.0f, -1.0f, 0.0f));
	//scene_mat = glm::rotate( scene_mat, static_cast<float>( curr_time.getVal() * (speed/2.0) ), glm::vec3(1.0f, 0.0f, 0.0f));

	glm::mat4 MVP = gEngine->getActiveModelViewProjectionMatrix() * scene_mat;

	glActiveTexture(GL_TEXTURE0);
	//glBindTexture( GL_TEXTURE_2D, sgct::TextureManager::instance()->getTextureByName("box") );
	glBindTexture( GL_TEXTURE_2D, sgct::TextureManager::instance()->getTextureByHandle(myTextureHandle) );

	sgct::ShaderManager::instance()->bindShaderProgram( "xform" );

	glUniformMatrix4fv(Matrix_Loc, 1, GL_FALSE, &MVP[0][0]);

	//draw the box
	myBox->draw();

	sgct::ShaderManager::instance()->unBindShaderProgram();

	glDisable( GL_CULL_FACE );
	glDisable( GL_DEPTH_TEST );
}

void myPreSyncFun()
{
	if( gEngine->isMaster() )
	{
		curr_time.setVal( sgct::Engine::getTime() );
	}
}

void myInitOGLFun()
{
	sgct::TextureManager::instance()->setAnisotropicFilterSize(8.0f);
	sgct::TextureManager::instance()->setCompression(sgct::TextureManager::S3TC_DXT);
	sgct::TextureManager::instance()->loadTexure(myTextureHandle, "box", "box.png", true);

	myBox = new sgct_utils::SGCTBox(2.0f, sgct_utils::SGCTBox::Regular);
	//myBox = new sgct_utils::SGCTBox(2.0f, sgct_utils::SGCTBox::CubeMap);
	//myBox = new sgct_utils::SGCTBox(2.0f, sgct_utils::SGCTBox::SkyBox);

	//Set up backface culling
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW); //our polygon winding is counter clockwise

	sgct::ShaderManager::instance()->addShaderProgram( "xform",
			"SimpleVertexShader.vertexshader",
			"SimpleFragmentShader.fragmentshader" );

	sgct::ShaderManager::instance()->bindShaderProgram( "xform" );

	Matrix_Loc = sgct::ShaderManager::instance()->getShaderProgram( "xform").getUniformLocation( "MVP" );
	GLint Tex_Loc = sgct::ShaderManager::instance()->getShaderProgram( "xform").getUniformLocation( "Tex" );
	glUniform1i( Tex_Loc, 0 );

	sgct::ShaderManager::instance()->unBindShaderProgram();
}

void myEncodeFun()
{
	sgct::SharedData::instance()->writeDouble(&curr_time);
}

void myDecodeFun()
{
	sgct::SharedData::instance()->readDouble(&curr_time);
}

void myCleanUpFun()
{
	if(myBox != NULL)
		delete myBox;
}

void controlCallback(const char* msg, int length, int index) {
	std::stringstream s;
	if (length == 0)
		return;

	s.str(msg + 1);
	switch (msg[0]) {
		case '0':
		{
			std::cout << "Position: " << msg + 1 << std::endl;
			float v;
			float oldV;
			s >> v;
			oldV = posX.getVal();
			posX.setVal(oldV + v);
			s >> v;
			oldV = posY.getVal();
			posY.setVal(oldV + v);
			s >> v;
			oldV = posZ.getVal();
			posZ.setVal(oldV + v);
			break;
		}
		case '1':
		{
			std::cout << "Rotation: " << msg + 1 << std::endl;
			float v;
			float oldV;
			s >> v;
			oldV = rotation.getVal();
			rotation.setVal(oldV + v);
			break;
		}
		default:
			std::cout << "Unknown command: " << msg << std::endl;
	}
}
