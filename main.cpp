#include "sgct.h"

#include <glm/gtx/euler_angles.hpp>

#include <iostream>
#include <sstream>

sgct::Engine* _engine;

struct Box {
	sgct_utils::SGCTBox* box;

	size_t textureHandle;

	sgct::SharedFloat posX;
	sgct::SharedFloat posY;
	sgct::SharedFloat posZ;

	sgct::SharedFloat rotationAlpha;
	sgct::SharedFloat rotationBeta;
	sgct::SharedFloat rotationGamma;
};

const int nBoxes = 2;
Box boxes[nBoxes];

sgct_utils::SGCTSphere* environmentSphere;
size_t backgroundTexture;

GLint matrixLocation;

void draw();
void initOpenGL();
void encode();
void decode();
void cleanup();
void externalControl(const char* msg, int length, int index);
void keyboardControl(int key, int action);

int main(int argc, char* argv[]) {
	_engine = new sgct::Engine( argc, argv );

	_engine->setInitOGLFunction(initOpenGL);
	_engine->setDrawFunction(draw);
	_engine->setCleanUpFunction(cleanup);
	_engine->setExternalControlCallback(externalControl);
	_engine->setKeyboardCallbackFunction(keyboardControl);

	if(!_engine->init( sgct::Engine::OpenGL_3_3_Core_Profile ))	{
		delete _engine;
		return EXIT_FAILURE;
	}

	sgct::SharedData::instance()->setEncodeFunction(encode);
	sgct::SharedData::instance()->setDecodeFunction(decode);

	// Main loop
	_engine->render();

	// Clean up
	delete _engine;

	// Exit program
	exit(EXIT_SUCCESS);
}

void draw() {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	sgct::ShaderManager::instance()->bindShaderProgram("xform");

	for (int i = 0; i < nBoxes; ++i) {
		Box& box = boxes[i];

		glm::vec3 pos(box.posX.getVal(), box.posY.getVal(), box.posZ.getVal());
		glm::mat4 model = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, -3.f));
		model = glm::translate(model, pos);

		glm::mat4 rotation = glm::yawPitchRoll(
			box.rotationAlpha.getVal(),
			box.rotationBeta.getVal(),
			box.rotationGamma.getVal()
		);

		model = model * rotation;

		glm::mat4 MVP = _engine->getActiveModelViewProjectionMatrix() * model;

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, sgct::TextureManager::instance()->getTextureByHandle(box.textureHandle));
		glUniformMatrix4fv(matrixLocation, 1, GL_FALSE, &MVP[0][0]);

		box.box->draw();
	}

	sgct::ShaderManager::instance()->unBindShaderProgram();

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
}

void initOpenGL() {
	std::vector<std::string> textures = {
		"C-Research.png",
		"CoViDAg.png"
	};

	sgct::TextureManager::instance()->setAnisotropicFilterSize(8.0f);
	sgct::TextureManager::instance()->setCompression(sgct::TextureManager::S3TC_DXT);
	for (int i = 0; i < nBoxes; ++i)
		sgct::TextureManager::instance()->loadTexure(boxes[i].textureHandle, textures[i], textures[i], true);

	sgct::TextureManager::instance()->loadTexure(backgroundTexture, "background", "background.png", true);

	boxes[0].box = new sgct_utils::SGCTBox(2.f, sgct_utils::SGCTBox::Regular);
	boxes[1].box = new sgct_utils::SGCTBox(2.f, sgct_utils::SGCTBox::Regular);

	/*environmentSphere = new sgct_utils::SGCTSphere(14.7f, 10);*/
	environmentSphere = new sgct_utils::SGCTSphere(2.f, 10);

	//Set up backface culling
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW); //our polygon winding is counter clockwise

	sgct::ShaderManager::instance()->addShaderProgram("xform",
			"SimpleVertexShader.vertexshader",
			"SimpleFragmentShader.fragmentshader");

	sgct::ShaderManager::instance()->bindShaderProgram("xform");

	matrixLocation = sgct::ShaderManager::instance()->getShaderProgram("xform").getUniformLocation("MVP");
	GLint Tex_Loc = sgct::ShaderManager::instance()->getShaderProgram("xform").getUniformLocation("Tex");
	glUniform1i( Tex_Loc, 0 );

	sgct::ShaderManager::instance()->unBindShaderProgram();
}

void encode() {
	for (int i = 0; i < nBoxes; ++i) {
		sgct::SharedData::instance()->writeFloat(&(boxes[i].posX));
		sgct::SharedData::instance()->writeFloat(&(boxes[i].posY));
		sgct::SharedData::instance()->writeFloat(&(boxes[i].posZ));

		sgct::SharedData::instance()->writeFloat(&(boxes[i].rotationAlpha));
		sgct::SharedData::instance()->writeFloat(&(boxes[i].rotationBeta));
		sgct::SharedData::instance()->writeFloat(&(boxes[i].rotationGamma));
	}
}

void decode() {
	for (int i = 0; i < nBoxes; ++i) {
		sgct::SharedData::instance()->readFloat(&(boxes[i].posX));
		sgct::SharedData::instance()->readFloat(&(boxes[i].posY));
		sgct::SharedData::instance()->readFloat(&(boxes[i].posZ));
		sgct::SharedData::instance()->readFloat(&(boxes[i].rotationAlpha));
		sgct::SharedData::instance()->readFloat(&(boxes[i].rotationBeta));
		sgct::SharedData::instance()->readFloat(&(boxes[i].rotationGamma));
	}

	/**for (int i = nBoxes - 1; i >= 0; --i) {
		sgct::SharedData::instance()->readFloat(&(boxes[i].posX));
		sgct::SharedData::instance()->readFloat(&(boxes[i].posY));
		sgct::SharedData::instance()->readFloat(&(boxes[i].posZ));
		sgct::SharedData::instance()->readFloat(&(boxes[i].rotationAlpha));
		sgct::SharedData::instance()->readFloat(&(boxes[i].rotationBeta));
		sgct::SharedData::instance()->readFloat(&(boxes[i].rotationGamma));
	}
	*/
}

void cleanup() {
	for (int i = 0; i < nBoxes; ++i)
		delete boxes[i].box;
}

void externalControl(const char* msg, int length, int index) {
	std::stringstream s;
	if (length == 0)
		return;

	int id;
	switch (msg[0]) {
		case '0':
			id = 0;
			break;
		case '1':
			id = 1;
			break;
		default:
			id = -1;
	}
#ifdef _DEBUG
	std::cout << "ID (" << id << "): ";
#endif

	s.str(msg + 2);
	switch (msg[1]) {
		case '0':
		{
#ifdef _DEBUG
			std::cout << "Position: " << msg + 2 << std::endl;
#endif
			float x,y,z;
			float oldX, oldY, oldZ;

			s >> x;
			s >> y;
			s >> z;

			oldX = boxes[id].posX.getVal();
			oldY = boxes[id].posY.getVal();
			oldZ = boxes[id].posZ.getVal();

			boxes[id].posX.setVal(oldX + x);
			boxes[id].posY.setVal(oldY + y);
			boxes[id].posZ.setVal(oldZ + z);
			break;
		}
		case '1':
		{
#ifdef _DEBUG
			std::cout << "Rotation: " << msg + 2 << std::endl;
#endif
			float rotationAlpha, rotationBeta, rotationGamma;
			float oldRotationAlpha, oldRotationBeta, oldRotationGamma;

			s >> rotationAlpha;
			s >> rotationBeta;
			s >> rotationGamma;

			oldRotationAlpha = boxes[id].rotationAlpha.getVal();
			oldRotationBeta = boxes[id].rotationBeta.getVal();
			oldRotationGamma = boxes[id].rotationGamma.getVal();

			boxes[id].rotationAlpha.setVal(oldRotationAlpha + rotationAlpha);
			boxes[id].rotationBeta.setVal(oldRotationBeta + rotationBeta);
			boxes[id].rotationGamma.setVal(oldRotationGamma + rotationGamma);
			break;
		}
		default:
			std::cout << "Unknown command: " << msg << std::endl;
	}
}

void keyboardControl(int key, int action) {
	if (action != SGCT_PRESS)
		return;
	switch (key) {
		case SGCT_KEY_I:
			//_engine->setStatsGraphVisibility(!_engine->isDisplayInfoRendered());
			_engine->setDisplayInfoVisibility(!_engine->isDisplayInfoRendered());
			break;
		case SGCT_KEY_K:
		{
			static bool state = false;

			state = !state;
			_engine->setStatsGraphVisibility(state);
			break;
		}
		case SGCT_KEY_R:
		{
			for (int i = 0; i < nBoxes; ++i) {
				boxes[i].posX.setVal(0.f);
				boxes[i].posY.setVal(0.f);
				boxes[i].posZ.setVal(0.f);

				boxes[i].rotationAlpha.setVal(0.f);
				boxes[i].rotationBeta.setVal(0.f);
				boxes[i].rotationGamma.setVal(0.f);
			}
			break;
		}
	}
}
