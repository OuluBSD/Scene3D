#include "SoftRend.h"

NAMESPACE_UPP



SoftCompiler::SoftCompiler() {
	
}


bool SoftCompiler::Compile(SoftShader& s) {
	LOG("SoftCompiler::Compile: warning: software shaders are not currently compiled");
	
	return true;
}



END_UPP_NAMESPACE

