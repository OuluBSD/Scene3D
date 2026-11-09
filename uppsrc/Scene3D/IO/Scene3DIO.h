#ifndef _Scene3D_IO_Scene3DIO_h_
#define _Scene3D_IO_Scene3DIO_h_

struct Scene3DExternalFile : Moveable<Scene3DExternalFile> {
	String id;
	String type;
	String path;
	String note;
	String created_utc;
	String modified_utc;
	int64 size = -1;

	void Visit(Vis& v);
};

struct Scene3DMetaEntry {
	String key;
	String value;

	void Visit(Vis& v);
};

struct Scene3DDocument {
	int version = 2;
	String name;
	VfsValue project_val;
	GeomProject* project = 0;
	int active_scene = 0;
	VfsValue focus_val;
	GeomCamera* focus = 0;
	VfsValue program_val;
	GeomCamera* program = 0;
	String created_utc;
	String modified_utc;
	String data_dir;
	Array<Scene3DExternalFile> external_files;
	Array<Scene3DMetaEntry> meta;

	Scene3DDocument();
	void Init();
	void Visit(Vis& v);
};

static const int SCENE3D_VERSION = 2;

void FixupScene3DOwners(GeomProject& prj);

bool SaveScene3DJson(const String& path, Scene3DDocument& doc, bool pretty = true);
bool LoadScene3DJson(const String& path, Scene3DDocument& doc);
bool SaveScene3DBin(const String& path, Scene3DDocument& doc);
bool LoadScene3DBin(const String& path, Scene3DDocument& doc);

#endif
