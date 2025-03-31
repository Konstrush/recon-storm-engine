#pragma once

#include "../inode.h"
#include "../../model/src/modelr.h"
#include "../../../ship/src/ship.h"
#include "../../../rigging/src/rope.h"
#include "../../../rigging/src/flag.h"
#include "../../../rigging/src/sail.h"
#include "../../../rigging/src/vant.h"
#include "../../../weather/src/sky.h"
#include "../../../sea/src/sea.h"
#include "../../../sea_foam/src/seafoam.h"
#include "../../../sea_foam/src/seafoam.h"
#include "../../../ship/src/track.h"


class INIFILE;
class XINTERFACE;

class CXI_MODELVIEWER : public CINODE
{
    friend XINTERFACE;

  public:
    CXI_MODELVIEWER();
    ~CXI_MODELVIEWER() override;
    void Draw(bool bSelected, uint32_t Delta_Time) override;
    void DrawShipScene(uint32_t Delta_Time);
    void DrawCommonScene(uint32_t Delta_Time);
    bool Init(INIFILE *ini1, const char *name1, INIFILE *ini2, const char *name2, VDX9RENDER *rs, XYRECT &hostRect,
              XYPOINT &ScreenSize) override;
    void ReleaseAll() override;
    int CommandExecute(int wActCode) override;
    bool IsClick(int buttonID, int32_t xPos, int32_t yPos) override;

    void MouseThis(float fX, float fY) override
    {
    }

    void ChangePosition(XYRECT &rNewPos) override;
    void SaveParametersToIni() override;
    uint32_t MessageProc(int32_t msgcode, MESSAGE &message) override;
    virtual void ChangeUV(FXYRECT &frNewUV);
    void ChangeColor(uint32_t dwColor);
    void SetPictureSize(int32_t &nWidth, int32_t &nHeight);
    void CalculateNewViewport(D3DVIEWPORT9 vp, D3DVIEWPORT9 &res_vp);

  protected:
    void LoadIni(INIFILE *ini1, const char *name1, INIFILE *ini2, const char *name2) override;
    void SetNewPicture(bool video, const char *sNewTexName);
    void SetNewPictureFromDir(const char *dirName);
    void SetNewPictureByGroup(const char *groupName, const char *picName);
    void SetNewPictureByPointer(int32_t pTex);
    void ReleasePicture();
    void HandleMouseMove();
    void HandleMouseWheel(int32_t nWeel);
    void UpdateCameraPosition();
    

    char *m_pcGroupName;
    int32_t m_idTex;
    CVideoTexture *m_pTex;
    XI_ONETEX_VERTEX m_v[4];

    bool m_bMakeBlind;
    float m_fCurBlindTime;
    bool m_bBlindUp;
    float m_fBlindUpSpeed;
    float m_fBlindDownSpeed;
    uint32_t m_dwBlindMin;
    uint32_t m_dwBlindMax;

    bool m_bDrag;
    const char* m_pcDragControlName;
    FXYPOINT m_curMousePos;

    entid_t m_modelId;

    entid_t m_shipId;
    entid_t m_ropeId;
    entid_t m_sailId;
    entid_t m_flagId;

    entid_t m_vantId;
    entid_t m_vantLId;
    entid_t m_vantZId;
    float m_fGlobalWinAngle;

    entid_t seaEntId;
    entid_t skyEntId;
    entid_t seafoamEntId;
    entid_t shipTrackEntId;

    bool m_bRigSetted;
    bool m_bCamSetted;
    bool m_bRenderFlags;
    bool m_bUseSailRotation;

    float sensY;
    float sensZ;

    float xAngMin;
    float xAngMax;
    float xAng;
    float dxAng;

    float yAng;
    float dyAng;
    
    float baseZCamPos;
    float ZCamPos;
    CVECTOR camPos;

    float centerX;
    float centerY;
    float centerZ;

    bool m_bShipModelView;

    float minCamDistanceCoef;
    float maxCamDistanceCoef;
    float camZoomCoef;
    float translateCoef;

    VDATA *m_pMouseWeel;
};