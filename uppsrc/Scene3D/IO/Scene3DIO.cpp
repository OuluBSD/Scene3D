#include "IO.h"

NAMESPACE_UPP

void Scene3DExternalFile::Visit(Vis& v) {
	v.Ver(1)
	(1) VIS_(id)
	    VIS_(type)
	    VIS_(path)
	    VIS_(note)
	    VIS_(created_utc)
	    VIS_(modified_utc)
	    VIS_(size);
}

void Scene3DMetaEntry::Visit(Vis& v) {
	v.Ver(1)
	(1) VIS_(key)
	    VIS_(value);
}

Scene3DDocument::Scene3DDocument() {
	Init();
}

void Scene3DDocument::Init() {
	project = &project_val.CreateExt<GeomProject>();
	focus = &focus_val.CreateExt<GeomCamera>();
	program = &program_val.CreateExt<GeomCamera>();
}

void Scene3DDocument::Visit(Vis& v) {
	v VIS_(version)
	  VIS_(name);
	if (!project)
		Init();
	v("project", *project, VISIT_NODE)
	  VIS_(active_scene);
	v("focus", *focus, VISIT_NODE);
	v("program", *program, VISIT_NODE);
	if (version >= 2) {
		v VIS_(created_utc)
		  VIS_(modified_utc)
		  VIS_(data_dir)
		  VISV(external_files)
		  VISV(meta);
	}
}

void FixupScene3DOwners(GeomProject& prj) {
	hash_t max_key = 0;
	for (GeomScene& scene : prj.val.Sub<GeomScene>())
		for (GeomObject& obj : GeomObjectCollection(scene))
			if (obj.key > max_key)
				max_key = obj.key;
	if (max_key >= prj.key_counter)
		prj.key_counter = max_key + 1;
}

static void PruneEmptyObjects(GeomDirectory& dir) {
	for (int i = 0; i < dir.val.sub.GetCount(); ) {
		VfsValue& sub = dir.val.sub[i];
		if (IsVfsType(sub, AsTypeHash<GeomDirectory>())) {
			PruneEmptyObjects(sub.GetExt<GeomDirectory>());
			i++;
			continue;
		}
		if (IsVfsType(sub, AsTypeHash<GeomObject>())) {
			GeomObject& obj = sub.GetExt<GeomObject>();
			bool empty = obj.type == GeomObject::O_NULL &&
			             obj.name.IsEmpty() &&
			             obj.asset_ref.IsEmpty() &&
			             obj.pointcloud_ref.IsEmpty();
			GeomTimeline* tl = obj.FindTimeline();
			if (tl && !tl->keypoints.IsEmpty())
				empty = false;
			if (empty) {
				dir.val.sub.Remove(i);
				continue;
			}
		}
		i++;
	}
}

static void PruneEmptyObjects(GeomProject& prj) {
	for (GeomScene& scene : prj.val.Sub<GeomScene>())
		PruneEmptyObjects(scene);
}

static bool CheckScene3DVersion(int version) {
	return version > 0 && version <= SCENE3D_VERSION;
}

bool SaveScene3DJson(const String& path, Scene3DDocument& doc, bool pretty) {
	if (!CheckScene3DVersion(doc.version))
		doc.version = SCENE3D_VERSION;
	String json;
	if (!DoVisitToJson(doc, json, pretty))
		return false;
	return SaveFile(path, json);
}

bool LoadScene3DJson(const String& path, Scene3DDocument& doc) {
	String json = LoadFile(path);
	if (json.IsEmpty())
		return false;
	if (!VisitFromJson(doc, json.Begin()))
		return false;
	if (!CheckScene3DVersion(doc.version))
		return false;
	doc.project_val.FixParent();
	FixupScene3DOwners(*doc.project);
	PruneEmptyObjects(*doc.project);
	return true;
}

bool SaveScene3DBin(const String& path, Scene3DDocument& doc) {
	if (!CheckScene3DVersion(doc.version))
		doc.version = SCENE3D_VERSION;
	FileOut out(path);
	if (!out.IsOpen())
		return false;
	Vis vis(out);
	doc.Visit(vis);
	out.Close();
	return !vis.IsError();
}

bool LoadScene3DBin(const String& path, Scene3DDocument& doc) {
	FileIn in(path);
	if (!in.IsOpen())
		return false;
	Vis vis(in);
	doc.Visit(vis);
	if (vis.IsError())
		return false;
	if (!CheckScene3DVersion(doc.version))
		return false;
	doc.project_val.FixParent();
	FixupScene3DOwners(*doc.project);
	PruneEmptyObjects(*doc.project);
	return true;
}

END_UPP_NAMESPACE
