#ifndef _Scene3D_Render_SoftRendCtrl_h_
#define _Scene3D_Render_SoftRendCtrl_h_

#if 0


struct SoftRendCtrl : Ctrl {
	SoftRend rend;
	SoftFramebuffer fb;
	SoftPipeline pipe;
	SoftProgram prog;
	
public:
	typedef SoftRendCtrl CLASSNAME;
	SoftRendCtrl();
	
	void Paint(Draw& d);
	
	void DefaultPreFrame();
	void ProcessStage();
	
	
	Scene3DRenderConfig* conf = 0;
};


#endif
#endif
