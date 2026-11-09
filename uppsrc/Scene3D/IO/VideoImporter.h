#ifndef _Scene3D_IO_VideoImporter_h_
#define _Scene3D_IO_VideoImporter_h_


struct GeomVideo {
	GeomAnim* anim = 0;
	int video_length = 0;
	DescriptorImage l_dimg, r_dimg;
	StagedVirtualStereoUncamera uncam;
	OrbSystem orb;
	bool is_importing = false;
	int point_limit = 2048;
	float eye_dist = 0.06f;
	TimeStop ts;
	int position = 0;
	String directory;
	Octree* octree = 0;
	GeomObject* go = 0;
	GeomObject* cam = 0;
	
	
public:
	typedef GeomVideo CLASSNAME;
	GeomVideo();
	
	void LoadDirectory(String dir, int fps, String name="");
	void SetWmrCamera();
	void Next();
	 
	void Update(double dt);
	
};

#endif
