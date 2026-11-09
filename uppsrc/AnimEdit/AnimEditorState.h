#ifndef _AnimEdit_AnimEditorState_h_
#define _AnimEdit_AnimEditorState_h_

#include <Core/Core.h>
#include <AnimEditLib/AnimCore.h>

using namespace Upp;

struct AnimEditorState {
    Upp::AnimationProject project;
    String                current_path;
    bool                  dirty = false;

    void Clear() {
        project = AnimationProject();
        current_path.Clear();
        dirty = false;
    }
};

#endif