#ifndef _Scene3D_Core_Geom_h_
#define _Scene3D_Core_Geom_h_



struct GeomProgram;
struct GeomProject;
struct GeomScene;
struct GeomDirectory;
struct GeomWorldState;
struct Edit3D;


struct GeomKeypoint {
	int frame_id = -1;
	vec3 position;
	quat orientation;
	bool has_position = true;
	bool has_orientation = true;
	int position_ease = 0;
	int orientation_ease = 0;
	
	void Visit(Vis& v);
};

static inline bool IsVfsType(const VfsValue& v, hash_t h) {
	return v.type_hash == h || (v.ext && v.ext->GetTypeHash() == h);
}

struct GeomTimeline : VfsValueExt {
	ArrayMap<int, GeomKeypoint> keypoints;
	
	DEFAULT_EXT(GeomTimeline)
	GeomKeypoint& GetAddKeypoint(int i);
	int FindPre(int kp_i) const;
	int FindPost(int kp_i) const;
	int FindPrePosition(int kp_i) const;
	int FindPostPosition(int kp_i) const;
	int FindPreOrientation(int kp_i) const;
	int FindPostOrientation(int kp_i) const;
	
	void Visit(Vis& v) override;
};

struct GeomSceneTimeline : VfsValueExt {
	double time = 0;
	int position = 0;
	int length = 0;
	bool is_playing = false;
	bool repeat = false;
	double speed = 1.0;

	DEFAULT_EXT(GeomSceneTimeline)
	void Reset();
	void Play(int scene_length);
	void Pause();
	void Update(GeomWorldState& state, double dt);
	void ApplyAtPosition(GeomWorldState& state, int pos, double t);
	void Visit(Vis& v) override;
};

struct GeomTransform : VfsValueExt {
	vec3 position = vec3(0);
	quat orientation = Identity<quat>();
	vec3 scale = vec3(1);
	
	DEFAULT_EXT(GeomTransform)
	void Visit(Vis& v) override;
};

struct GeomScript : VfsValueExt {
	String file;
	bool enabled = true;
	bool run_on_load = true;
	bool run_every_frame = false;
	
	DEFAULT_EXT(GeomScript)
	void Visit(Vis& v) override;
};

struct GeomDynamicProperties : VfsValueExt {
	VectorMap<String, Value> props;

	DEFAULT_EXT(GeomDynamicProperties)
	void Visit(Vis& v) override;
};

struct GeomBone : VfsValueExt {
	String name;
	vec3 position = vec3(0);
	quat orientation = Identity<quat>();
	float length = 0.3f;

	DEFAULT_EXT(GeomBone)
	void Visit(Vis& v) override;
};

struct GeomSkeleton : VfsValueExt {
	String name;

	DEFAULT_EXT(GeomSkeleton)
	void Visit(Vis& v) override;
};

struct GeomSkinWeights : VfsValueExt {
	VectorMap<String, Vector<float>> weights;

	DEFAULT_EXT(GeomSkinWeights)
	void Visit(Vis& v) override;
};

struct GeomEdge {
	int a = -1;
	int b = -1;

	void Visit(Vis& v);
};

struct GeomFace {
	int a = -1;
	int b = -1;
	int c = -1;

	void Visit(Vis& v);
};

struct GeomEditableMesh : VfsValueExt {
	Vector<vec3> points;
	Vector<GeomEdge> lines;
	Vector<GeomFace> faces;

	DEFAULT_EXT(GeomEditableMesh)
	void Visit(Vis& v) override;
};

struct Geom2DShape : Moveable<Geom2DShape> {
	typedef enum {
		S_LINE,
		S_RECT,
		S_CIRCLE,
		S_POLY,
	} Type;
	Type type = S_LINE;
	Vector<vec2> points;
	float radius = 0;
	Color stroke = Color(220, 220, 220);
	float width = 1.0f;
	int stroke_cap = -1;
	int stroke_join = -1;
	bool closed = false;
	int tex_wrap = -1;
	int stroke_uv_mode = -1;
	float tex_repeat_x = 0;
	float tex_repeat_y = 0;

	void Visit(Vis& v);
};

struct Geom2DLayer : VfsValueExt {
	Vector<Geom2DShape> shapes;
	bool visible = true;
	Color stroke = Color(220, 220, 220);
	Color fill = Color(60, 60, 60);
	float width = 1.0f;
	float opacity = 1.0f;
	bool use_layer_style = true;
	int stroke_cap = 0;
	int stroke_join = 0;
	String texture_ref;
	int blend_mode = 0;
	float tex_offset_x = 0;
	float tex_offset_y = 0;
	float tex_repeat_x = 1;
	float tex_repeat_y = 1;
	float tex_rotate = 0;
	int tex_wrap = 0;
	int stroke_uv_mode = 0;

	DEFAULT_EXT(Geom2DLayer)
	void Visit(Vis& v) override;
};

struct Geom2DKeyframe : Moveable<Geom2DKeyframe> {
	int frame_id = -1;
	Vector<Geom2DShape> shapes;

	void Visit(Vis& v);
};

struct Geom2DAnimation : VfsValueExt {
	ArrayMap<int, Geom2DKeyframe> keyframes;

	DEFAULT_EXT(Geom2DAnimation)
	Geom2DKeyframe& GetAddKeyframe(int frame);
	int FindPre(int frame) const;
	int FindPost(int frame) const;
	void Visit(Vis& v) override;
};

struct GeomTextureEdit : VfsValueExt {
	String path;
	int width = 512;
	int height = 512;

	DEFAULT_EXT(GeomTextureEdit)
	void Visit(Vis& v) override;
};

struct GeomMeshKeyframe {
	int frame_id = -1;
	Vector<vec3> points;

	void Visit(Vis& v);
};

struct GeomMeshAnimation : VfsValueExt {
	ArrayMap<int, GeomMeshKeyframe> keyframes;

	DEFAULT_EXT(GeomMeshAnimation)
	GeomMeshKeyframe& GetAddKeyframe(int frame);
	int FindPre(int frame) const;
	int FindPost(int frame) const;
	void Visit(Vis& v) override;
};

struct GeomPointcloudEffectTransform : VfsValueExt {
	String name;
	bool enabled = true;
	bool locked = false;
	vec3 position = vec3(0);
	quat orientation = Identity<quat>();
	
	DEFAULT_EXT(GeomPointcloudEffectTransform)
	void Visit(Vis& v) override;
};

struct GeomObject : VfsValueExt {
	typedef enum {
		O_NULL,
		O_MODEL,
		O_CAMERA,
		O_OCTREE,
		
		O_COUNT
	} Type;
	
	hash_t key = 0;
	String name;
	Type type = O_NULL;
	String ui_type;
	String asset_ref;
	String pointcloud_ref;
	bool is_visible = true;
	bool is_locked = false;
	bool read_enabled = true;
	bool write_enabled = false;
	bool timeline_muted = false;
	bool timeline_solo = false;
	
	DEFAULT_EXT(GeomObject)
	One<Model> mdl;
	Camera cam;
	OctreePointModel octree;
	Octree* octree_ptr = 0;
	
	GeomTimeline& GetTimeline();
	GeomTimeline* FindTimeline() const;
	GeomTransform& GetTransform();
	GeomTransform* FindTransform() const;
	GeomDynamicProperties& GetDynamicProperties();
	GeomDynamicProperties* FindDynamicProperties() const;
	GeomEditableMesh& GetEditableMesh();
	GeomEditableMesh* FindEditableMesh() const;
	Geom2DLayer& Get2DLayer();
	Geom2DLayer* Find2DLayer() const;
	Geom2DAnimation& Get2DAnimation();
	Geom2DAnimation* Find2DAnimation() const;
	GeomTextureEdit& GetTextureEdit();
	GeomTextureEdit* FindTextureEdit() const;
	GeomMeshAnimation& GetMeshAnimation();
	GeomMeshAnimation* FindMeshAnimation() const;
	GeomSkeleton& GetSkeleton();
	GeomSkeleton* FindSkeleton() const;
	GeomSkinWeights& GetSkinWeights();
	GeomSkinWeights* FindSkinWeights() const;
	GeomPointcloudEffectTransform& GetAddPointcloudEffect(String name);
	void GetPointcloudEffects(Vector<GeomPointcloudEffectTransform*>& out) const;
	
	bool IsModel() const {return type == O_MODEL;}
	bool IsOctree() const {return type == O_OCTREE;}
	bool IsCamera() const {return type == O_CAMERA;}
	String GetPath() const;
	
	void Visit(Vis& v) override;
};

struct GeomPointcloudDataset : VfsValueExt {
	String name;
	String source_ref;
	
	DEFAULT_EXT(GeomPointcloudDataset)
	OctreePointModel octree;
	Octree* octree_ptr = 0;
	
	String GetId() const;
	void Visit(Vis& v) override;
};

struct GeomDirectory : VfsValueExt {
	virtual ~GeomDirectory() {}
	
	String name;
	
	DEFAULT_EXT(GeomDirectory)
	GeomProject& GetProject() const;
	int GetSubdirCount() const;
	GeomDirectory& GetSubdir(int i) const;
	int GetObjectCount() const;
	GeomObject& GetObject(int i) const;
	GeomDirectory& GetAddDirectory(String name);
	GeomObject& GetAddModel(String name);
	GeomObject& GetAddCamera(String name);
	GeomObject& GetAddOctree(String name);
	GeomObject* FindObject(String name);
	GeomObject* FindObject(String name, GeomObject::Type type);
	GeomObject* FindCamera(String name);
	GeomTransform& GetTransform();
	GeomTransform* FindTransform() const;
	GeomDynamicProperties& GetDynamicProperties();
	GeomDynamicProperties* FindDynamicProperties() const;
	GeomPointcloudDataset& GetAddPointcloudDataset(String id);
	GeomPointcloudDataset* FindPointcloudDataset(String id);
	
	void Visit(Vis& v) override;
};

struct GeomObjectIterator {
	static const int MAX_LEVELS = 128;
	int pos[MAX_LEVELS] = {0};
	VfsValue* addr[MAX_LEVELS];
	GeomObject* obj = 0;
	int level = -1;
	
	GeomObjectIterator() {}
	explicit GeomObjectIterator(GeomDirectory& d);
	bool Next();
	operator bool() const;
	GeomObject& operator*();
	GeomObject* operator->();
	bool operator==(const GeomObjectIterator& it) const {
		if (it.level != level || it.obj != obj)
			return false;
		for(int i = 0; i <= level; i++)
			if (pos[i] != it.pos[i])
				return false;
		return true;
	}
	void operator++() {Next();}
	void operator++(int) {Next();}
	
};

struct GeomObjectCollection {
	using Iterator = GeomObjectIterator;
	
	Iterator iter;
	
	
	GeomObjectCollection(GeomDirectory& d);
	Iterator begin() const {return iter;}
	Iterator end() const {return Iterator();}
	Iterator begin() {return iter;}
	Iterator end() {return Iterator();}
	
};

struct GeomScene : GeomDirectory {
	CLASSTYPE(GeomScene)
	GeomScene(VfsValue& n) : GeomDirectory(n) {}
	int length = 0;
	
	GeomSceneTimeline& GetTimeline();
	GeomSceneTimeline* FindTimeline() const;
	
	void Visit(Vis& v) override;
};

struct GeomProject : VfsValueExt {
	int kps = 5;
	int fps = 60;
	
	hash_t key_counter;
	
	DEFAULT_EXT(GeomProject)
	void Clear() {
		val.sub.Clear();
		kps = 5;
		fps = 60;
		key_counter = 1;
	}
	
	GeomScene& AddScene();
	int GetSceneCount() const;
	GeomScene& GetScene(int i);
	
	hash_t NewKey() {return key_counter++;}
	
	void Visit(Vis& v) override;
};

typedef enum {
	VIEWMODE_YZ,
	VIEWMODE_XZ,
	VIEWMODE_XY,
	VIEWMODE_PERSPECTIVE,
} ViewMode;

typedef enum {
	CAMSRC_FOCUS,
	CAMSRC_PROGRAM,
	CAMSRC_OBJECT,
	CAMSRC_VIDEOIMPORT_FOCUS,
	CAMSRC_VIDEOIMPORT_PROGRAM,
} CameraSource;

struct GeomCamera : VfsValueExt {
	vec3 position = vec3(0);
	quat orientation = Identity<quat>();
	float distance = 10;
	float fov = 120;
	float scale = 1;
	
	DEFAULT_EXT(GeomCamera)
	void LoadCamera(ViewMode m, Camera& cam, Size sz, float far=1000) const;
	mat4 GetViewMatrix(ViewMode m, Size sz) const;
	Frustum GetFrustum(ViewMode m, Size sz) const;
	
	void Visit(Vis& v) override;
};

struct GeomObjectState {
	GeomObject* obj;
	vec3 position;
	quat orientation;
	vec3 scale;
};

struct GeomWorldState : VfsValueExt {
	GeomProject* prj = 0;
	int active_scene = -1;
	int active_camera_obj_i = -1;
	int focus_mode = 0;
	hash_t focus_object_key = 0;
	bool program_visible = true;
	bool focus_visible = true;
	
	Array<GeomObjectState> objs;
	
	DEFAULT_EXT(GeomWorldState)
	GeomCamera& GetFocus();
	GeomCamera& GetProgram();
	GeomObject* FindObjectByKey(hash_t key) const;
	const GeomObjectState* FindObjectStateByKey(hash_t key) const;
	
	void Visit(Vis& v) override;
	void UpdateObjects();
	GeomScene& GetActiveScene();
	bool HasActiveScene() const {return active_scene >= 0;}
	
};

struct GeomAnim : VfsValueExt {
	GeomWorldState* state = 0;
	double time = 0;
	int position = 0;
	bool is_playing = false;
	
	DEFAULT_EXT(GeomAnim)
	void Reset();
	void Play();
	void Pause();
	void ApplyAtPosition(int pos, double t);
	void Update(double dt) override;
	void Visit(Vis& v) override;
	
	
	Callback WhenSceneEnd;
	
};

INITIALIZE(GeomTimeline)
INITIALIZE(GeomSceneTimeline)
INITIALIZE(GeomDynamicProperties)
INITIALIZE(GeomBone)
INITIALIZE(GeomSkeleton)
INITIALIZE(GeomSkinWeights)
INITIALIZE(GeomEditableMesh)
INITIALIZE(Geom2DLayer)
INITIALIZE(Geom2DAnimation)
INITIALIZE(GeomTextureEdit)
INITIALIZE(GeomMeshAnimation)
INITIALIZE(GeomObject)
INITIALIZE(GeomDirectory)
INITIALIZE(GeomScene)
INITIALIZE(GeomProject)
INITIALIZE(GeomCamera)
INITIALIZE(GeomWorldState)
INITIALIZE(GeomAnim)
#endif
