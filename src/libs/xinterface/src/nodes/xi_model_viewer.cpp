#include "xi_model_viewer.h"

#include "storm_assert.h"
#include "string_compare.hpp"
#include "v_file_service.h"
#include <math_inlines.h>



CXI_MODELVIEWER::CXI_MODELVIEWER()
{
    m_rs = nullptr;
    m_idTex = -1;
    m_pTex = nullptr;
    m_nNodeType = NODETYPE_MODELVIEWER;
    m_pcGroupName = nullptr;
    m_bMakeBlind = false;
    m_fCurBlindTime = 0.f;
    m_bBlindUp = true;
    m_fBlindUpSpeed = 0.001f;
    m_fBlindDownSpeed = 0.001f;
    m_dwBlindMin = ARGB(255, 128, 128, 128);
    m_dwBlindMax = ARGB(255, 255, 255, 255);

    m_modelId = invalid_entity;

    m_shipId = invalid_entity;
    m_ropeId = invalid_entity;
    m_sailId = invalid_entity;
    m_flagId = invalid_entity;

    m_vantId = invalid_entity;
    m_vantLId = invalid_entity;
    m_vantZId = invalid_entity;

    seaEntId = invalid_entity;
    skyEntId = invalid_entity;
    seafoamEntId = invalid_entity;
    shipTrackEntId = invalid_entity;

    m_bCamSetted = false;
    m_bRigSetted = false;
    m_bShipModelView = false;
    m_fGlobalWinAngle = PI / 6;
    m_bRenderFlags = true;
    m_bUseSailRotation = false;

    dxAng = 0.015;
    dyAng = 0.015;
    sensY = 1.0;
    sensZ = 1.0;
    xAngMin = 0.01;
    xAngMax = PI / 3;
    xAng = PI / 36;
    yAng = 0.0;

    centerX = 0.0;
    centerY = 0.0;
    centerZ = 0.0;

    baseZCamPos = 0.0;
    ZCamPos = 0.0;
    camPos = 0.0;

    minCamDistanceCoef = 0.3;
    maxCamDistanceCoef = 1.7;
    camZoomCoef = 1.0;
    m_bDrag = false;
    m_pcDragControlName = nullptr;
    m_pMouseWeel = nullptr;

    translateCoef = 0.0;
}

CXI_MODELVIEWER::~CXI_MODELVIEWER()
{
    ReleaseAll();
}

void CXI_MODELVIEWER::Draw(bool bSelected, uint32_t Delta_Time)
{
    if (m_bUse)
    {
        if (m_bMakeBlind)
        {
            if (m_bBlindUp)
            {
                m_fCurBlindTime += m_fBlindUpSpeed * Delta_Time;
                if (m_fCurBlindTime >= 1.f)
                {
                    m_fCurBlindTime = 1.f;
                    m_bBlindUp = false;
                }
            }
            else
            {
                m_fCurBlindTime -= m_fBlindDownSpeed * Delta_Time;
                if (m_fCurBlindTime <= 0.f)
                {
                    m_fCurBlindTime = 0.f;
                    m_bBlindUp = true;
                }
            }
            ChangeColor(ptrOwner->GetBlendColor(m_dwBlindMin, m_dwBlindMax, m_fCurBlindTime));
        }

        if (m_idTex != -1 || m_pTex)
        {
            if (m_idTex != -1)
                m_rs->TextureSet(0, m_idTex);
            else
                m_rs->SetTexture(0, m_pTex ? m_pTex->m_pTexture : nullptr);
            m_rs->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, XI_ONETEX_FVF, 2, m_v, sizeof(XI_ONETEX_VERTEX), "iVideo");
        }

        if (m_bDrag && m_pcDragControlName && m_bCamSetted)
        {
            CONTROL_STATE cs;
            core.Controls->GetControlState(m_pcDragControlName, cs);
            if (cs.state == CST_INACTIVE || cs.state == CST_INACTIVATED)
                m_bDrag = false;

            if (m_bDrag)
                HandleMouseMove();
        }

        if (m_bCamSetted)
        {
            if (m_bShipModelView)
            {
                DrawShipScene(Delta_Time);
            }
            else
            {
                DrawCommonScene(Delta_Time);
            }
        }

        
    }
}

void CXI_MODELVIEWER::DrawShipScene(uint32_t Delta_Time)
{
    if (m_shipId && m_bRigSetted)
    {
        auto _shipEntPtr = core.GetEntityPointerSafe(m_shipId);
        if (_shipEntPtr == nullptr)
        {
            return;
        }
        auto _ship = static_cast<SHIP *>(_shipEntPtr);

        //CMatrix _viewMatrix;
        CMatrix _projMatrix;

        //CVECTOR t_camPos, t_camAng;
        //float t_camPers;
        //m_rs->GetCamera(t_camPos, t_camAng, t_camPers);

        CMatrix t_projMtx = m_rs->GetProjection();
        CMatrix t_viewMtx = m_rs->GetView();
        CMatrix t_worldMtx = m_rs->GetWorld();

        D3DVIEWPORT9 vp;
        m_rs->GetViewport(&vp);

        D3DVIEWPORT9 viewer_vp;
        CalculateNewViewport(vp, viewer_vp);
        m_rs->SetViewport(&viewer_vp);

        _ship->ExecuteForMV(Delta_Time);

        float newYCenter = centerY + _ship->mtx.Pos().y;
        if (seaEntId)
        {
            // Limit the height from the bottom
            auto _sea = static_cast<SEA_BASE *>(core.GetEntityPointer(seaEntId));
            const float _waveY = _sea->WaveXZ(camPos.x, camPos.z);
            if (camPos.y - _waveY < 1.0f)
            {
                camPos.y = _waveY + 1.0f;
            }
        }

        float newXCenter = _ship->State.vPos.x + centerX;
        float newZCenter = _ship->State.vPos.z + centerZ;
        
        float newXCamPos = _ship->State.vPos.x + camPos.x;
        float newZCamPos = _ship->State.vPos.z + camPos.z;

        //_viewMatrix.BuildViewMatrix(CVECTOR(newXCamPos, camPos.y, newZCamPos), // точка, в которой находится камера
        //                            CVECTOR(newXCenter, newYCenter, newZCenter), // точка, в которую мы смотрим
        //                            CVECTOR(0.0f, 1.0f, 0.0f));            // верх объекта
        m_rs->SetCamera(CVECTOR(newXCamPos, camPos.y, newZCamPos),
                          CVECTOR(newXCenter, newYCenter, newZCenter),
                          CVECTOR(0.0f, 1.0f, 0.0f));
        //m_rs->SetView(&_viewMatrix);
        _projMatrix.BuildProjectionMatrix(D3DX_PI / 4, static_cast<float>(vp.Width), static_cast<float>(vp.Height),
                                          1.0f, 4000.0f);
        m_rs->SetRenderState(D3DRS_LIGHTING, true);
        m_rs->SetProjection(&_projMatrix);

        if (skyEntId)
        {
            auto _sky = static_cast<SKY *>(core.GetEntityPointer(skyEntId));
            _sky->Realize(Delta_Time);
        }
        if (seaEntId)
        {
            auto _sea = static_cast<SEA *>(core.GetEntityPointer(seaEntId));
            _sea->Realize(Delta_Time);

            if (seafoamEntId)
            {
                auto _seafoamEntPtr = core.GetEntityPointerSafe(seafoamEntId);
                if (_seafoamEntPtr)
                {
                    auto _seafoam = static_cast<SEAFOAM *>(_seafoamEntPtr);
                    _seafoam->Execute(Delta_Time);
                    _seafoam->Realize(Delta_Time);
                }
            }
            if (shipTrackEntId)
            {
                auto _shipTrackntPtr = core.GetEntityPointerSafe(shipTrackEntId);
                if (_shipTrackntPtr)
                {
                    auto shipTrack = static_cast<ShipTracks *>(_shipTrackntPtr);
                    shipTrack->Execute(Delta_Time);
                    shipTrack->Realize(Delta_Time);
                }
            }
        }
        
        auto _sailEntPtr = core.GetEntityPointerSafe(m_sailId);
        if (_sailEntPtr)
        {
            auto _sail = static_cast<SAIL *>(_sailEntPtr);
            _sail->ExecuteForMV(Delta_Time, m_fGlobalWinAngle, m_bUseSailRotation);
            _sail->Realize(Delta_Time);
        }

        auto _ropeEntPtr = core.GetEntityPointerSafe(m_ropeId);
        if (_ropeEntPtr)
        {
            auto _rope = static_cast<ROPE *>(_ropeEntPtr);
            _rope->Execute(Delta_Time);
            _rope->Realize(Delta_Time);
        }

        auto _flagEntPtr = core.GetEntityPointerSafe(m_flagId);
        if (m_bRenderFlags && _flagEntPtr)
        {
            auto _flag = static_cast<FLAG *>(_flagEntPtr);
            _flag->ExecuteForMV(Delta_Time, m_fGlobalWinAngle);
            _flag->Realize(Delta_Time);
        }

        auto _vantEntPtr = core.GetEntityPointerSafe(m_vantId);
        if (_vantEntPtr)
        {
            auto _vant = static_cast<VANT *>(_vantEntPtr);
            _vant->Execute(Delta_Time);
            _vant->Realize(Delta_Time);
        }

        auto _vantLEntPtr = core.GetEntityPointerSafe(m_vantLId);
        if (_vantLEntPtr)
        {
            auto _vantL = static_cast<VANTL *>(_vantLEntPtr);
            _vantL->Execute(Delta_Time);
            _vantL->Realize(Delta_Time);
        }

        auto _vantZEntPtr = core.GetEntityPointerSafe(m_vantZId);
        if (_vantZEntPtr)
        {
            auto _vantZ = static_cast<VANTZ *>(_vantZEntPtr);
            _vantZ->Execute(Delta_Time);
            _vantZ->Realize(Delta_Time);
        }

        // ship modelr
        _ship->Realize(Delta_Time);

        m_rs->SetRenderState(D3DRS_LIGHTING, false);

        m_rs->SetViewport(&vp);
        //m_rs->SetCamera(t_camPos, t_camAng, t_camPers);
        m_rs->SetView(&t_viewMtx);
        
        m_rs->SetProjection(&t_projMtx);
        m_rs->SetWorld(&t_worldMtx);
    }
}

void CXI_MODELVIEWER::DrawCommonScene(uint32_t Delta_Time)
{
    if (m_modelId)
    {
        auto c = static_cast<MODELR *>(core.GetEntityPointer(m_modelId));

        //CMatrix _viewMatrix;
        CMatrix _projMatrix;

        CMatrix t_projMtx = m_rs->GetProjection();
        CMatrix t_viewMtx = m_rs->GetView();
        CMatrix t_worldMtx = m_rs->GetWorld();

        D3DVIEWPORT9 vp;
        m_rs->GetViewport(&vp);

        D3DVIEWPORT9 viewer_vp;
        CalculateNewViewport(vp, viewer_vp);
        m_rs->SetViewport(&viewer_vp);

        m_rs->SetCamera(CVECTOR(camPos.x, camPos.y, camPos.z), // точка, в которой находится камера
                                    CVECTOR(centerX, centerY, centerZ), // точка, в которую мы смотрим
                                    CVECTOR(0.0f, 1.0f, 0.0f));            // верх объекта
        _projMatrix.BuildProjectionMatrix(D3DX_PI / 4, static_cast<float>(vp.Width), static_cast<float>(vp.Height),
                                          1.0f, 4000.0f);

        m_rs->SetRenderState(D3DRS_LIGHTING, true);
        //m_rs->SetView(&_viewMatrix);
        m_rs->SetProjection(&_projMatrix);

        // modelr
        c->Realize(Delta_Time);

        m_rs->SetRenderState(D3DRS_LIGHTING, false);

        m_rs->SetViewport(&vp);
        m_rs->SetView(&t_viewMtx);
        m_rs->SetProjection(&t_projMtx);
        m_rs->SetWorld(&t_worldMtx);
    }
}

void CXI_MODELVIEWER::CalculateNewViewport(D3DVIEWPORT9 vp, D3DVIEWPORT9 &res_vp)
{
    float coefScaleX = static_cast<float>(vp.Width) / m_screenSize.x;
    float coefScaleY = static_cast<float>(vp.Height) / m_screenSize.y;
    
    int newX = static_cast<int>(m_rect.left * coefScaleX);
    int newY = static_cast<int>(m_rect.top * coefScaleY);
    int newWidth = static_cast<int>((m_rect.right - m_rect.left) * coefScaleX);
    int newHeight = static_cast<int>((m_rect.bottom - m_rect.top) * coefScaleY);

    res_vp.MaxZ = vp.MaxZ;
    res_vp.MinZ = vp.MinZ;
    res_vp.X = newX < 0 ? vp.X : newX;
    res_vp.Y = newY < 0 ? vp.Y : newY;
    res_vp.Width = newWidth > vp.Width ? vp.Width : newWidth;
    res_vp.Height = newHeight > vp.Height ? vp.Height : newHeight;
}

bool CXI_MODELVIEWER::Init(INIFILE *ini1, const char *name1, INIFILE *ini2, const char *name2, VDX9RENDER *rs,
                       XYRECT &hostRect, XYPOINT &ScreenSize)
{
    if (!CINODE::Init(ini1, name1, ini2, name2, rs, hostRect, ScreenSize))
        return false;

    m_pMouseWeel = core.Event("evGetMouseWeel");
    return true;
}

void CXI_MODELVIEWER::LoadIni(INIFILE *ini1, const char *name1, INIFILE *ini2, const char *name2)
{
    char param[255];

    m_idTex = -1;

    auto texRect = FXYRECT(0.f, 0.f, 1.f, 1.f);

    if (ReadIniString(ini1, name1, ini2, name2, "groupName", param, sizeof(param), ""))
    {
        m_idTex = pPictureService->GetTextureID(param);
        const auto len = strlen(param) + 1;
        m_pcGroupName = new char[len];
        Assert(m_pcGroupName);
        memcpy(m_pcGroupName, param, len);

        if (ReadIniString(ini1, name1, ini2, name2, "picName", param, sizeof(param), ""))
            pPictureService->GetTexturePos(m_pcGroupName, param, texRect);
    }
    else
    {
        if (ReadIniString(ini1, name1, ini2, name2, "textureName", param, sizeof(param), ""))
            m_idTex = m_rs->TextureCreate(param);
        texRect = GetIniFloatRect(ini1, name1, ini2, name2, "textureRect", texRect);
    }

    m_pTex = nullptr;
    if (ReadIniString(ini1, name1, ini2, name2, "videoName", param, sizeof(param), ""))
        m_pTex = m_rs->GetVideoTexture(param);

    const auto color = GetIniARGB(ini1, name1, ini2, name2, "color", ARGB(255, 128, 128, 128));

    // Create rectangle
    ChangePosition(m_rect);
    ChangeUV(texRect);
    for (auto i = 0; i < 4; i++)
    {
        m_v[i].color = color;
        m_v[i].pos.z = 1.f;
    }

    m_bMakeBlind = GetIniBool(ini1, name1, ini2, name2, "blind", false);
    m_fCurBlindTime = 0.f;
    m_bBlindUp = true;
    auto fTmp = GetIniFloat(ini1, name1, ini2, name2, "blindUpTime", 1.f);
    if (fTmp > 0.f)
        m_fBlindUpSpeed = 0.001f / fTmp;
    fTmp = GetIniFloat(ini1, name1, ini2, name2, "blindDownTime", 1.f);
    if (fTmp > 0.f)
        m_fBlindDownSpeed = 0.001f / fTmp;
    m_dwBlindMin = GetIniARGB(ini1, name1, ini2, name2, "blindMinColor", ARGB(255, 128, 128, 128));
    m_dwBlindMax = GetIniARGB(ini1, name1, ini2, name2, "blindMaxColor", ARGB(255, 255, 255, 255));
}

void CXI_MODELVIEWER::ReleaseAll()
{
    ReleasePicture();

    //core.EraseEntity(seaEntId);
}

int CXI_MODELVIEWER::CommandExecute(int wActCode)
{
    int i;
    CONTROL_STATE cs;
    if (m_bUse)
    {
        int32_t nWeel = 0;
        if (m_pMouseWeel)
            m_pMouseWeel->Get(nWeel);
        if (nWeel != 0 && m_bDrag)
        {
            HandleMouseWheel(nWeel);
        }
    }
    
    if (m_bUse)
    {
        switch (wActCode)
        {
            case ACTION_MOUSECLICK:
                core.Controls->GetControlState("ILClick", cs);
                if (cs.state == CST_ACTIVATED)
                {
                    m_bDrag = true;
                    m_pcDragControlName = "ILClick";
                    const auto pntMouse = ptrOwner->GetMousePoint();
                    m_curMousePos = pntMouse;
                    HandleMouseMove();
                }
                break;
            case ACTION_MOUSERCLICK:
                core.Controls->GetControlState("IRClick", cs);
                if (cs.state == CST_ACTIVATED)
                {
                    m_bDrag = true;
                    m_pcDragControlName = "IRClick";
                    const auto pntMouse = ptrOwner->GetMousePoint();
                    m_curMousePos = pntMouse;
                    HandleMouseMove();
                }
            break;
        }
    }

    return -1;
}

void CXI_MODELVIEWER::HandleMouseMove()
{
    const auto pntMouse = ptrOwner->GetMousePoint();
    if (pntMouse.x != m_curMousePos.x || pntMouse.y != m_curMousePos.y)
    {
        float pntDx = pntMouse.x - m_curMousePos.x;
        float pntDy = pntMouse.y - m_curMousePos.y;

        yAng += dyAng * sensY * pntDx;
        if (yAng > PI)
            yAng = -PIm2 + yAng;
        if (yAng < -PI)
            yAng = PIm2 - yAng;

        xAng += dxAng * sensZ * pntDy;
        if (xAng > xAngMax)
            xAng = xAngMax;
        if (xAng < xAngMin)
            xAng = xAngMin;

        UpdateCameraPosition();

        m_curMousePos = pntMouse;
    }
}

void CXI_MODELVIEWER::HandleMouseWheel(int32_t nWeel)
{
    int step;
    if (nWeel > 0)
        step = 1;
    else
        step = -1;

    ZCamPos -= step * camZoomCoef;
    if (ZCamPos < minCamDistanceCoef * baseZCamPos)
    {
        ZCamPos = minCamDistanceCoef * baseZCamPos;
    }
    if (ZCamPos > maxCamDistanceCoef * baseZCamPos)
    {
        ZCamPos = maxCamDistanceCoef * baseZCamPos;
    }

    UpdateCameraPosition();
}

void CXI_MODELVIEWER::UpdateCameraPosition()
{
    float curYCamPos = ZCamPos * sinf(xAng);

    float curZCamPos = ZCamPos * cosf(xAng) * cosf(yAng);
    float curXCamPos = ZCamPos * cosf(xAng) * sinf(yAng);

    float zTranslate = ZCamPos * translateCoef * cosf(yAng + PId2);
    float xTranslate = ZCamPos * translateCoef * sinf(yAng + PId2);

    CVECTOR _camPos(curXCamPos + xTranslate, curYCamPos, curZCamPos + zTranslate);
    centerX = xTranslate;
    centerZ = zTranslate;
    camPos = _camPos;
}

bool CXI_MODELVIEWER::IsClick(int buttonID, int32_t xPos, int32_t yPos)
{
    if (m_bClickable)
    {
        if (xPos >= m_rect.left && xPos <= m_rect.right && yPos >= m_rect.top && yPos <= m_rect.bottom)
            return true;
    }
    return false;
}

void CXI_MODELVIEWER::ChangePosition(XYRECT &rNewPos)
{
    m_rect = rNewPos;
    m_v[0].pos.x = static_cast<float>(m_rect.left);
    m_v[0].pos.y = static_cast<float>(m_rect.top);
    m_v[1].pos.x = static_cast<float>(m_rect.left);
    m_v[1].pos.y = static_cast<float>(m_rect.bottom);
    m_v[2].pos.x = static_cast<float>(m_rect.right);
    m_v[2].pos.y = static_cast<float>(m_rect.top);
    m_v[3].pos.x = static_cast<float>(m_rect.right);
    m_v[3].pos.y = static_cast<float>(m_rect.bottom);
}

void CXI_MODELVIEWER::SaveParametersToIni()
{
    char pcWriteParam[2048];

    auto pIni = fio->OpenIniFile(ptrOwner->m_sDialogFileName.c_str());
    if (!pIni)
    {
        core.Trace("Warning! Can`t open ini file name %s", ptrOwner->m_sDialogFileName.c_str());
        return;
    }

    // save position
    sprintf_s(pcWriteParam, sizeof(pcWriteParam), "%d,%d,%d,%d", m_rect.left, m_rect.top, m_rect.right, m_rect.bottom);
    pIni->WriteString(m_nodeName, "position", pcWriteParam);
}

void CXI_MODELVIEWER::SetNewPicture(bool video, const char *sNewTexName)
{
    ReleasePicture();
    if (video)
        m_pTex = m_rs->GetVideoTexture(sNewTexName);
    else
        m_idTex = m_rs->TextureCreate(sNewTexName);

    FXYRECT uv;
    uv.left = uv.top = 0.f;
    uv.right = uv.bottom = 1.f;
    ChangeUV(uv);
}

void CXI_MODELVIEWER::SetNewPictureFromDir(const char *dirName)
{
    char param[512];
    sprintf(param, "resource\\textures\\%s", dirName);

    const auto vFilenames = fio->_GetPathsOrFilenamesByMask(param, "*.tx", false);
    if (!vFilenames.empty())
    {
        int findQ = rand() % vFilenames.size();
        sprintf(param, "%s\\%s", dirName, vFilenames[findQ].c_str());
        const int paramlen = strlen(param);
        if (paramlen < sizeof(param) && paramlen >= 3)
        {
            param[paramlen - 3] = 0;
        }
        SetNewPicture(false, param);
    }
}

void CXI_MODELVIEWER::SetNewPictureByGroup(const char *groupName, const char *picName)
{
    if (!m_pcGroupName || !storm::iEquals(m_pcGroupName, groupName))
    {
        ReleasePicture();
        if (groupName)
        {
            const auto len = strlen(groupName) + 1;
            m_pcGroupName = new char[strlen(groupName) + 1];
            Assert(m_pcGroupName);
            memcpy(m_pcGroupName, groupName, len);
            m_idTex = pPictureService->GetTextureID(groupName);
        }
    }

    if (m_pcGroupName && picName)
    {
        FXYRECT texRect;
        pPictureService->GetTexturePos(m_pcGroupName, picName, texRect);
        ChangeUV(texRect);
    }
}

uint32_t CXI_MODELVIEWER::MessageProc(int32_t msgcode, MESSAGE &message)
{
    switch (msgcode)
    {
        case 0: // Set model for scene
        {
            auto msgName = message.String();
            if (msgName == "ship")
            {
                m_shipId = message.EntityID();
                //core.Trace("%s", "ship setted");
                m_bShipModelView = true;
            }
            else
            {
                m_modelId = message.EntityID();
                //core.Trace("%s", "model setted");
            }
        }
        break;

        case 1: // Set camera for scene (required for drawing)
        {
            if (!m_modelId && !m_shipId)
            {
                return 0;
            }
            sensY = message.Float();
            sensZ = message.Float();

            yAng = message.Float();
            xAngMax = message.Float();
            xAng = message.Float();

            if (m_bShipModelView)
            {
                auto s = static_cast<SHIP *>(core.GetEntityPointer(m_shipId));
                auto realShipHeight = s->GetRealBoxsize().y;
                centerY = realShipHeight / 3.0f;
                ZCamPos = realShipHeight * 2.85f;
                baseZCamPos = ZCamPos;
            }
            else
            {
                auto m = static_cast<MODELR *>(core.GetEntityPointer(m_modelId));
                GEOS::INFO gi;
                m->root->geo->GetInfo(gi);
                centerY = gi.boxcenter.y;
                ZCamPos = gi.boxsize.y * 2.85f;
                baseZCamPos = ZCamPos;
            }

            minCamDistanceCoef = message.Float();
            maxCamDistanceCoef = message.Float();
            camZoomCoef = message.Float();
            translateCoef = message.Float();
            
            UpdateCameraPosition();
            m_bCamSetted = true;
        }
        break;

        case 2: // Set ship rig for scene (required for ship drawing)
        {
            m_bRigSetted = false;

            // set ropes
            m_ropeId = message.EntityID();
            // set sails
            m_sailId = message.EntityID();
            // set flags
            m_flagId = message.EntityID();
            // set vants
            m_vantId = message.EntityID();
            m_vantLId = message.EntityID();
            m_vantZId = message.EntityID();

            m_bRigSetted = true;
        }
        break;

        case 3: // Set Environment (sea, sky, etc) for scene
        {
            seaEntId = message.EntityID();
            skyEntId = message.EntityID();
            // seafoam
            seafoamEntId = message.EntityID();
            // track
            shipTrackEntId = message.EntityID();
        }
        break;
        case 4: //Update camera settings (zoom, translation)
        {
            auto comName = message.String();
            if (comName == "translate")
            {
                translateCoef = message.Float();
                UpdateCameraPosition();
            }
            else if (comName == "zoom_in")
            {
                HandleMouseWheel(1);
            }
            else if (comName == "zoom_out")
            {
                HandleMouseWheel(-1);
            }
            else if (comName == "v_translate")
            {
                float v_tcoef = message.Float();
                if (v_tcoef > 1.0)
                    v_tcoef = 1.0;
                if (v_tcoef < 0)
                    v_tcoef = 0.0;

                float _modelHeight = 0;
                if (m_bShipModelView)
                {
                    auto s = static_cast<SHIP *>(core.GetEntityPointer(m_shipId));
                    _modelHeight = s->GetRealBoxsize().y;
                }
                else
                {
                    auto m = static_cast<MODELR *>(core.GetEntityPointer(m_modelId));
                    GEOS::INFO gi;
                    m->root->geo->GetInfo(gi);
                    _modelHeight = gi.boxcenter.y;
                }

                centerY = _modelHeight * v_tcoef;
            }
        }
        break;
        case 5: //External settings
        {
            auto comName = message.String();
            if (comName == "enableFlags")
            {
                m_bRenderFlags = message.Long();
            }
            else if (comName == "setGlobalWindAngle")
            {
                float _globalWindAngle = message.Float();
                m_fGlobalWinAngle = _globalWindAngle;
            }
            else if (comName == "setShipSpeed")
            {
                auto s = static_cast<SHIP *>(core.GetEntityPointer(m_shipId));
                float _newShipSpeed = message.Float();
                if (_newShipSpeed > 50.f)
                    _newShipSpeed = 50.f;
                if (_newShipSpeed < 0.f)
                    _newShipSpeed = 0.f;

                s->State.vSpeed.z = _newShipSpeed;
            }
            else if (comName == "useSailRotation")
            {
                m_bUseSailRotation = message.Long();
            }
        }
        break;
        case 100: // Move the picture to a new position
        {
            m_rect.left = message.Long();
            m_rect.top = message.Long();
            m_rect.right = message.Long();
            m_rect.bottom = message.Long();
            ChangePosition(m_rect);
        }
        break;

        case 101: // Set the texture coordinates of the image
        {
            FXYRECT texRect;
            texRect.left = message.Float();
            texRect.right = message.Float();
            texRect.top = message.Float();
            texRect.bottom = message.Float();
            ChangeUV(texRect);
        }
        break;

        case 102: // Set a new picture or video picture
        {
            const auto bVideo = message.Long() != 0;
            const std::string &param = message.String();
            SetNewPicture(bVideo, param.c_str());
        }
        break;

        case 103: // Get a random picture from the directory
        {
            const std::string &param = message.String();
            SetNewPictureFromDir(param.c_str());
        }
        break;

        case 104: // Set a new color
        {
            const uint32_t color = message.Long();
            for (auto i = 0; i < 4; i++)
                m_v[i].color = color;
        }
        break;

        case 105: // set / remove blinking
        {
            const bool bBlind = message.Long() != 0;
            if (m_bMakeBlind != bBlind)
            {
                m_bMakeBlind = bBlind;
                if (!m_bMakeBlind)
                    ChangeColor(m_dwBlindMin);
                else
                {
                    m_fCurBlindTime = 0.f;
                    m_bBlindUp = true;
                }
            }
        }
        break;

        case 106: // set new picture by group and picture name
        {
            const std::string &groupName = message.String();
            const std::string &picName = message.String();
            SetNewPictureByGroup(groupName.c_str(), picName.c_str());
        }
        break;

        case 107: // set new picture by pointer to IDirect3DTexture9
        {
            int32_t pTex = -1;
            if (message.GetCurrentFormatType() == 'p')
            {
                // DEPRECATED
                core.Trace("Warning! Setting an interface picture by pointer is deprecated. Please use integers instead.");
                pTex = message.Pointer();
            }
            else
            {
                pTex = message.Long();
            }
            SetNewPictureByPointer(pTex);
        }
        break;

        case 108: // remove texture from other picture to this
        {
            const std::string &srcNodeName = message.String();
            auto *pNod = static_cast<CINODE *>(ptrOwner->FindNode(srcNodeName.c_str(), nullptr));
            if (pNod->m_nNodeType != NODETYPE_MODELVIEWER)
            {
                core.Trace("Warning! XINTERFACE:: node with name %s have not picture type.", srcNodeName.c_str());
            }
            else
            {
                ReleasePicture();
                auto *pOtherPic = static_cast<CXI_MODELVIEWER *>(pNod);
                if (pOtherPic->m_pcGroupName)
                {
                    m_pcGroupName = pOtherPic->m_pcGroupName;
                    pOtherPic->m_pcGroupName = nullptr;
                }
                if (pOtherPic->m_idTex != -1)
                {
                    m_idTex = pOtherPic->m_idTex;
                    pOtherPic->m_idTex = -1;
                }
                for (int32_t n = 0; n < 4; n++)
                {
                    m_v[n].tu = pOtherPic->m_v[n].tu;
                    m_v[n].tv = pOtherPic->m_v[n].tv;
                }
                pOtherPic->ReleasePicture();
            }
        }
        break;
    }

    return 0;
}

void CXI_MODELVIEWER::ChangeUV(FXYRECT &frNewUV)
{
    m_v[0].tu = frNewUV.left;
    m_v[0].tv = frNewUV.top;
    m_v[1].tu = frNewUV.left;
    m_v[1].tv = frNewUV.bottom;
    m_v[2].tu = frNewUV.right;
    m_v[2].tv = frNewUV.top;
    m_v[3].tu = frNewUV.right;
    m_v[3].tv = frNewUV.bottom;
}

void CXI_MODELVIEWER::ChangeColor(uint32_t dwColor)
{
    m_v[0].color = m_v[1].color = m_v[2].color = m_v[3].color = dwColor;
}

void CXI_MODELVIEWER::SetPictureSize(int32_t &nWidth, int32_t &nHeight)
{
    if (!m_pTex && m_idTex == -1)
    {
        m_bUse = false;
        nWidth = nHeight = 0;
        return;
    }

    if (nWidth <= 0)
    {
        // find the real width
        nWidth = 128;
    }
    if (nHeight <= 0)
    {
        // find the real height
        nHeight = 128;
    }

    if (nWidth < 0 || nHeight < 0)
    {
        m_bUse = false;
        nWidth = nHeight = 0;
        return;
    }

    XYRECT rNewPos = m_rect;
    if (rNewPos.right - rNewPos.left != nWidth)
    {
        rNewPos.left = (m_rect.left + m_rect.right - nWidth) / 2;
        rNewPos.right = rNewPos.left + nWidth;
    }
    if (rNewPos.bottom - rNewPos.top != nHeight)
    {
        rNewPos.top = (m_rect.top + m_rect.bottom - nHeight) / 2;
        rNewPos.bottom = rNewPos.top + nHeight;
    }
    ChangePosition(rNewPos);
}

void CXI_MODELVIEWER::SetNewPictureByPointer(int32_t textureId)
{
    IDirect3DBaseTexture9 *texture = m_rs->GetTextureFromID(textureId);
    m_rs->TextureIncReference(textureId);
    if (texture)
        texture->AddRef();
    ReleasePicture();
    m_idTex = textureId;

    FXYRECT uv;
    uv.left = uv.top = 0.f;
    uv.right = uv.bottom = 1.f;
    ChangeUV(uv);
}

void CXI_MODELVIEWER::ReleasePicture()
{
    PICTURE_TEXTURE_RELEASE(pPictureService, m_pcGroupName, m_idTex);

    STORM_DELETE(m_pcGroupName);
    TEXTURE_RELEASE(m_rs, m_idTex);
    VIDEOTEXTURE_RELEASE(m_rs, m_pTex);
}
