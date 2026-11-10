#include "SoftRend.h"

NAMESPACE_UPP


VectorMap<String, SoftShaderLibrary::ShaderFactory>& SoftShaderLibrary::GetMap(int i) {
	static VectorMap<String, typename SoftShaderLibrary::ShaderFactory> shader_classes[GVar::SHADERTYPE_COUNT];
	ASSERT(i >= 0 && i < GVar::SHADERTYPE_COUNT);
	return shader_classes[i];
}






SoftShader::SoftShader() {
	
}


void SoftShader::Clear() {
	s.Clear();
}


void SoftShader::SetSource(String s) {
	src = s;
}




END_UPP_NAMESPACE

