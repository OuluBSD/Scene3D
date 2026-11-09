#ifndef _ModelerApp_ModelerApp_h_
#define _ModelerApp_ModelerApp_h_

#include <Docking/Docking.h>
#include <CtrlLib/CtrlLib.h>
#include <CodeEditor/CodeEditor.h>
#include <Geometry/Geometry.h>
#include <ComputerVision/ComputerVision.h>
#include <ByteVM/ByteVM.h>
#include <Draw/Camera/Camera.h>
#include <Ctrl/Camera/Camera.h>
#include <Scene3D/Core/Core.h>
#include <Scene3D/IO/IO.h>
#include <Scene3D/Render/Render.h>
#include <Scene3D/Exec/Exec.h>
#if defined(flagPOSIX) && defined(flagSOFTHMD)
#include <SoftHMD/SoftHMD.h>
#endif
#include <plugin/enet/enet.h>

#define IMAGECLASS ImagesImg
#define IMAGEFILE <ModelerApp/Images.iml>
#include <Draw/iml_header.h>

NAMESPACE_UPP

#include "VideoImportCtrl.h"
#include "Remote.h"
#include "RibbonCtrl.h"
#include "Editor.h"
#include "EditClientService.h"

END_UPP_NAMESPACE

#endif
