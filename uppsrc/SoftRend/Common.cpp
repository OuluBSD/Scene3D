#include "SoftRend.h"

NAMESPACE_UPP

GenericFragmentShaderArgs::GenericFragmentShaderArgs() {
	light_dir.Clear();
	camera_pos.Clear();
	camera_dir.Clear();
	
	iNone = iDiffuse = iSpecular = iAmbient = iEmissive = iHeight = iNormals = iShininess = 0;
	iOpacity = iDisplacement = iLightmap = iReflection = iUnknown = iCubeDiffuse = iCubeIrradiance = iCubeDisplay = 0;
	
	iIsNone = iIsDiffuse = iIsSpecular = iIsAmbient = iIsEmissive = iIsHeight = iIsNormals = iIsShininess = false;
	iIsOpacity = iIsDisplacement = iIsLightmap = iIsReflection = iIsUnknown = iIsCubeDiffuse = iIsCubeIrradiance = iIsCubeDisplay = false;
	
	for(int i = 0; i < TEXTYPE_COUNT; i++)
		color_buf[i] = 0;
}

END_UPP_NAMESPACE

