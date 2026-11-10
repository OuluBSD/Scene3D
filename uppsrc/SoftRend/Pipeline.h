#ifndef _SoftRend_Pipeline_h_
#define _SoftRend_Pipeline_h_

NAMESPACE_UPP



class SoftPipeline {
	bool inited = false;
	
public:
	struct Stage : Moveable<Stage> {
		SoftProgram* prog = 0;
		uint32 bmask = 0;
	};
	Vector<Stage> stages;
	
	
public:
	typedef SoftPipeline CLASSNAME;
	SoftPipeline();
	
	bool Create();
	void Clear();
	void Begin();
	void Use(SoftProgram& prog, uint32 bmask);
	
	void operator=(int i) {if (i == 0) Clear();}
	void operator=(const Nuller&) {Clear();}
	operator bool() const {return inited;}
	
};


END_UPP_NAMESPACE

#endif
