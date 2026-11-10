#ifndef _SoftRend_Compiler_h_
#define _SoftRend_Compiler_h_

NAMESPACE_UPP



struct SoftCompiler {
	typedef SoftCompiler CLASSNAME;
	SoftCompiler();
	
	
	bool Compile(SoftShader& s);
	
};


END_UPP_NAMESPACE

#endif
