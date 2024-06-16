#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}
void SceneManager::LoadSceneTextures()
{

	bool bReturn = false;

	bReturn = CreateGLTexture(
		"//apporto.com/dfs/SNHU/USERS/vyhuynh11_snhu/Documents/CS330Content/Utilities/textures/sharpies.png",
		"sharpie");

	bReturn = CreateGLTexture(
		"//apporto.com/dfs/SNHU/USERS/vyhuynh11_snhu/Documents/CS330Content/Utilities/textures/sbx.png",
		"starbucks");

	bReturn = CreateGLTexture(
		"//apporto.com/dfs/SNHU/USERS/vyhuynh11_snhu/Documents/CS330Content/Utilities/textures/ruler.png",
		"ruler");

	BindGLTextures();
}
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}
void SceneManager::SetupSceneLights()
{

	m_pShaderManager->setVec3Value("lightSources[0].position", -3.0f, 4.0f, 6.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.005f, 0.005f, 0.005f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.25f, 0.25f, 0.25f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.25f, 0.25f, 0.25f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.1f);

	m_pShaderManager->setVec3Value("lightSources[1].position", 3.0f, 4.0f, 6.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.005f, 0.005f, 0.005f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.25f, 0.25f, 0.25f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.25f, 0.25f, 0.25f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.1f);

	m_pShaderManager->setVec3Value("lightSources[2].position", 0.0f, 3.0f, 10.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.025f, 0.025f, 0.025f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.25f, 0.25f, 0.25f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.125f, 0.125f, 0.125f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 22.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.1f);

	m_pShaderManager->setBoolValue("bUseLighting", true);


}
void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/

	OBJECT_MATERIAL plasticMaterial;
	plasticMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	plasticMaterial.ambientStrength = 0.2f;
	plasticMaterial.diffuseColor = glm::vec3(0.6f, 0.6f, 0.6f);
	plasticMaterial.specularColor = glm::vec3(0.9f, 0.9f, 0.9f);
	plasticMaterial.shininess = 32.0f;
	plasticMaterial.tag = "plastic";
	m_objectMaterials.push_back(plasticMaterial);

	OBJECT_MATERIAL feltWoolMaterial;
	feltWoolMaterial.ambientColor = glm::vec3(0.3f, 0.3f, 0.3f);
	feltWoolMaterial.ambientStrength = 0.6f;
	feltWoolMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	feltWoolMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	feltWoolMaterial.shininess = 8.0f;
	feltWoolMaterial.tag = "felt_wool";
	m_objectMaterials.push_back(feltWoolMaterial);

	OBJECT_MATERIAL leatherMaterial;
	leatherMaterial.ambientColor = glm::vec3(0.3f, 0.3f, 0.1f);
	leatherMaterial.ambientStrength = 0.4f;
	leatherMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.4f);
	leatherMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.2f);
	leatherMaterial.shininess = 16.0f;
	leatherMaterial.tag = "leather";
	m_objectMaterials.push_back(leatherMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.4f, 0.4f, 0.4f);
	glassMaterial.ambientStrength = 0.3f;
	glassMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	glassMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
	glassMaterial.shininess = 55.0;
	glassMaterial.tag = "glass";
	m_objectMaterials.push_back(glassMaterial);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.6f, 0.4f, 0.2f);
	woodMaterial.ambientStrength = 0.5f;
	woodMaterial.diffuseColor = glm::vec3(0.7f, 0.5f, 0.3f);
	woodMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);
	woodMaterial.shininess = 20.0;
	woodMaterial.tag = "wood";
	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL metalMaterial;
	metalMaterial.ambientColor = glm::vec3(0.3f, 0.3f, 0.3f);
	metalMaterial.ambientStrength = 0.4f;
	metalMaterial.diffuseColor = glm::vec3(0.7f, 0.7f, 0.7f);
	metalMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	metalMaterial.shininess = 80.0;
	metalMaterial.tag = "metal";
	m_objectMaterials.push_back(metalMaterial);

	OBJECT_MATERIAL matteMaterial;
	matteMaterial.ambientColor = glm::vec3(0.5f, 0.5f, 0.5f);
	matteMaterial.ambientStrength = 0.6f;
	matteMaterial.diffuseColor = glm::vec3(0.6f, 0.6f, 0.6f);
	matteMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	matteMaterial.shininess = 10.0;
	matteMaterial.tag = "matte";
	m_objectMaterials.push_back(matteMaterial);





}


void SceneManager::PrepareScene()
{
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene
	LoadSceneTextures();
	DefineObjectMaterials();
	SetupSceneLights();
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadBoxMesh();

}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	RenderBackground();
	RenderSharpie();
	RenderCup();
	RenderRuler();
	RenderBattery();
}

void SceneManager::RenderSharpie()
{
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	scaleXYZ = glm::vec3(0.6f, 5.5f, 1.0f); //scale for sharpie base
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 30.0f; //rotate the sharpie 30 degree
	positionXYZ = glm::vec3(0.0f, 0.2f, 1.0f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("sharpie");
	SetShaderMaterial("plastic");

	m_basicMeshes->DrawCylinderMesh(); //draw the base of the sharpie

	scaleXYZ = glm::vec3(0.5f, 1.6f, 1.0f); //scale for the blue part
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 30.0f; //rotate the sharpie 30 degree
	positionXYZ = glm::vec3(-2.73f, 4.95f, 1.0f); //set pos
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0.282, 0.78, 1); //blue for the body
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawCylinderMesh(); //draw the cylinder tip

	scaleXYZ = glm::vec3(0.5f, .6f, 1.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 30.0f; //rotate the sharpie 30 degree
	positionXYZ = glm::vec3(-3.5f, 6.3f, 1.0f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0.282, 0.78, 1); //same blue
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawTaperedCylinderMesh(); //draw


	scaleXYZ = glm::vec3(.2f, 0.5f, .3f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 00.0f;
	ZrotationDegrees = 30.0f; //rotate the sharpie 30 degree
	positionXYZ = glm::vec3(-3.8f, 6.8f, 1.0f); //set pos
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.165, 0.188, 0.282, 1); //dark blue for tip
	SetShaderMaterial("felt_wool");
	m_basicMeshes->DrawConeMesh(); //draw the shape
}

void SceneManager::RenderBackground()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 10.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, -0.1f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.859, 0.627, 0.196, 1); //set color to yellow
	SetShaderMaterial("leather");

	m_basicMeshes->DrawPlaneMesh();
}

void SceneManager::RenderCup()
{
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	scaleXYZ = glm::vec3(2.6f, 5.5f, 1.0f); 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f; 
	positionXYZ = glm::vec3(-6.2f, 0.0f, 0.8f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("starbucks");
	SetShaderMaterial("glass");
	m_basicMeshes->DrawCylinderMesh(); //draw cup base


	scaleXYZ = glm::vec3(2.0f, 2.0f, 1.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -90.0f;
	positionXYZ = glm::vec3(-4.0f, 3.0f, 0.8f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderMaterial("glass"); 
	m_basicMeshes->DrawHalfTorusMesh();


	scaleXYZ = glm::vec3(2.6f, 0.1f, 1.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-6.2f, 5.5f, 0.8f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	m_basicMeshes->DrawCylinderMesh();
}

void SceneManager::RenderRuler()
{
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	scaleXYZ = glm::vec3(18.0f, 1.5f, 0.2f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = -5.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-1.2f, 0.8f, 2.2f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("ruler");
	SetShaderMaterial("wood");
	m_basicMeshes->DrawBoxMesh();


	scaleXYZ = glm::vec3(0.25f, 0.0f, .25f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = -5.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-2.4f, 0.9f, 2.25f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.929, 0.659, 0.161, 1);
	SetShaderMaterial("leather");

	m_basicMeshes->DrawCylinderMesh();


	scaleXYZ = glm::vec3(0.27f, 0.0f, .27f);
	XrotationDegrees = 91.0f;
	YrotationDegrees = -5.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-9.4f,0.95f, 1.7f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.929, 0.659, 0.161, 1);
	SetShaderMaterial("leather");

	m_basicMeshes->DrawCylinderMesh();

	scaleXYZ = glm::vec3(0.22f, 0.0f, .22f);
	XrotationDegrees = 91.0f;
	YrotationDegrees = -5.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-8.0f, 0.9f, 1.75f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.929, 0.659, 0.161, 1);
	SetShaderMaterial("leather");

	m_basicMeshes->DrawCylinderMesh();


	scaleXYZ = glm::vec3(0.22f, 0.0f, .22f);
	XrotationDegrees = 91.0f;
	YrotationDegrees = -5.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(5.0f, 0.9f, 2.87f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.929, 0.659, 0.161, 1);
	SetShaderMaterial("leather");

	m_basicMeshes->DrawCylinderMesh();
}

void SceneManager::RenderBattery()
{
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	scaleXYZ = glm::vec3(.5f, .1f, .5f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(1.2f, 0.0f, 3.2f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.22, 0.941, 0.157, 1);
	SetShaderMaterial("matte");
	m_basicMeshes->DrawCylinderMesh();


	scaleXYZ = glm::vec3(.5f, 2.1f, .5f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(1.2f, 0.1f, 3.2f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 0, 1);
	SetShaderMaterial("matte");
	m_basicMeshes->DrawCylinderMesh();

	scaleXYZ = glm::vec3(.5f, .1f, .5f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(1.2f, 2.2f, 3.2f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.22, 0.941, 0.157, 1);
	SetShaderMaterial("matte");
	m_basicMeshes->DrawCylinderMesh();


	scaleXYZ = glm::vec3(.2f, .1f, .2f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(1.2f, 2.3f, 3.2f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.667, 0.663, 0.678, 1);
	SetShaderMaterial("metal");
	m_basicMeshes->DrawCylinderMesh();

}