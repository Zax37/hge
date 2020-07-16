/*
** Haaf's Game Engine 1.8
** Copyright (C) 2003-2007, Relish Games
** hge.relishgames.com
** edited by kvakvs@yandex.ru, see https://github.com/kvakvs/hge
**
** Core functions implementation: graphics
*/


#include "hge_impl.h"
// GAPI dependent includes and defines (DX8/DX9 switch) by kvakvs@yandex.ru
#include "hge_gapi.h"
#include "notimplemented.h"


void HGE_CALL HGE_Impl::Gfx_Clear(const DWORD color) {
    if (cur_target_) {
        if (cur_target_->pDepth) {
            d3d_device_->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 
                               color, 1.0f, 0);
        }
        else {
            d3d_device_->Clear(0, nullptr, D3DCLEAR_TARGET, color, 1.0f, 0);
        }
    }
    else {
        if (z_buffer_) {
            d3d_device_->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                               color, 1.0f, 0);
        }
        else {
            d3d_device_->Clear(0, nullptr, D3DCLEAR_TARGET, color, 1.0f, 0);
        }
    }
}

int clipping_x, clipping_y, clipping_w, clipping_h;

void HGE_Impl::Gfx_GetClipping(int *x, int *y, int *w, int *h) {
    *x = clipping_x;
    *y = clipping_y;
    *w = clipping_w;
    *h = clipping_h;
}

void HGE_CALL HGE_Impl::Gfx_SetClipping(int x, int y, int w, int h) {
    hgeGAPIViewport vp;
    int scr_width, scr_height;

    if (!cur_target_) {
        scr_width = pHGE->System_GetStateInt(HGE_SCREENWIDTH);
        scr_height = pHGE->System_GetStateInt(HGE_SCREENHEIGHT);
    }
    else {
        scr_width = Texture_GetWidth(reinterpret_cast<HTEXTURE>(cur_target_->pTex));
        scr_height = Texture_GetHeight(reinterpret_cast<HTEXTURE>(cur_target_->pTex));
    }

    if (!w) {
        vp.X = 0;
        vp.Y = 0;
        vp.Width = scr_width;
        vp.Height = scr_height;
    }
    else {
        if (x < 0) {
            w += x;
            x = 0;
        }
        if (y < 0) {
            h += y;
            y = 0;
        }

        if (x + w > scr_width) {
            w = scr_width - x;
        }
        if (y + h > scr_height) {
            h = scr_height - y;
        }

        vp.X = x;
        vp.Y = y;
        vp.Width = w;
        vp.Height = h;
    }

    clipping_x = vp.X;
    clipping_y = vp.Y;
    clipping_w = vp.Width;
    clipping_h = vp.Height;

    vp.MinZ = 0.0f;
    vp.MaxZ = 1.0f;

    render_batch();
    d3d_device_->SetViewport(&vp);

    D3DXMATRIX tmp;
    D3DXMatrixScaling(&proj_matrix_, 1.0f, -1.0f, 1.0f);
    D3DXMatrixTranslation(&tmp, -0.5f, +0.5f, 0.0f);
    D3DXMatrixMultiply(&proj_matrix_, &proj_matrix_, &tmp);
    D3DXMatrixOrthoOffCenterLH(
        &tmp,
        static_cast<float>(vp.X),
        static_cast<float>(vp.X + vp.Width),
        -static_cast<float>(vp.Y + vp.Height),
        -static_cast<float>(vp.Y),
        vp.MinZ, vp.MaxZ
    );
    D3DXMatrixMultiply(&proj_matrix_, &proj_matrix_, &tmp);
    d3d_device_->SetTransform(D3DTS_PROJECTION, &proj_matrix_);
}

void HGE_CALL HGE_Impl::Gfx_SetTransform(const float x, const float y,
                                         const float dx, const float dy,
                                         const float rot, const float hscale,
                                         const float vscale) {
    D3DXMATRIX tmp;

    if (vscale == 0.0f) {
        D3DXMatrixIdentity(&view_matrix_);
    }
    else {
        D3DXMatrixTranslation(&view_matrix_, -x, -y, 0.0f);
        D3DXMatrixScaling(&tmp, hscale, vscale, 1.0f);
        D3DXMatrixMultiply(&view_matrix_, &view_matrix_, &tmp);
        D3DXMatrixRotationZ(&tmp, -rot);
        D3DXMatrixMultiply(&view_matrix_, &view_matrix_, &tmp);
        D3DXMatrixTranslation(&tmp, x + dx, y + dy, 0.0f);
        D3DXMatrixMultiply(&view_matrix_, &view_matrix_, &tmp);
    }

    render_batch();
    d3d_device_->SetTransform(D3DTS_VIEW, &view_matrix_);
}

bool HGE_CALL HGE_Impl::Gfx_BeginScene(const HTARGET targ) {
    hgeGAPISurface* p_surf = nullptr;
#if HGE_DIRECTX_VER == 8
    hgeGAPISurface *p_depth = nullptr;
#endif

    D3DDISPLAYMODE Mode;
    CRenderTargetList* target = reinterpret_cast<CRenderTargetList *>(targ);

    const HRESULT hr = d3d_device_->TestCooperativeLevel();
    if (hr == D3DERR_DEVICELOST) {
        return false;
    }
    if (hr == D3DERR_DEVICENOTRESET) {
        if (windowed_) {
            if (FAILED(d3d_->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &Mode))
                || Mode.Format == D3DFMT_UNKNOWN) {
                post_error("Can't determine desktop video mode");
                return false;
            }

            d3dpp_windowed_.BackBufferFormat = Mode.Format;
            if (format_id(Mode.Format) < 4) {
                screen_bpp_ = 16;
            }
            else {
                screen_bpp_ = 32;
            }
        }

        if (!gfx_restore()) {
            return false;
        }
    }

    if (vert_array_) {
        post_error("Gfx_BeginScene: Scene is already being rendered");
        return false;
    }

    if (target != cur_target_) {
        if (target) {
            target->pTex->GetSurfaceLevel(0, &p_surf);
#if HGE_DIRECTX_VER == 8
            p_depth = target->pDepth;
#endif
        }
        else {
            p_surf = screen_surf_;
#if HGE_DIRECTX_VER == 8
            p_depth = pScreenDepth;
#endif
        }
#if HGE_DIRECTX_VER == 8
        if(FAILED(pD3DDevice->SetRenderTarget(pSurf, p_depth)))
#endif
#if HGE_DIRECTX_VER == 9
        if (FAILED(d3d_device_->SetRenderTarget(0, p_surf)))
#endif
        {
            if (target) {
                p_surf->Release();
            }
            post_error("Gfx_BeginScene: Can't set render target");
            return false;
        }
        if (target) {
            p_surf->Release();
            if (target->pDepth) {
                d3d_device_->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
            }
            else {
                d3d_device_->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
            }
            set_projection_matrix(target->width, target->height);
        }
        else {
            if (z_buffer_) {
                d3d_device_->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
            }
            else {
                d3d_device_->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
            }
            set_projection_matrix(screen_width_, screen_height_);
        }

        d3d_device_->SetTransform(D3DTS_PROJECTION, &proj_matrix_);
        D3DXMatrixIdentity(&view_matrix_);
        d3d_device_->SetTransform(D3DTS_VIEW, &view_matrix_);

        cur_target_ = target;
    }

    d3d_device_->BeginScene();
#if HGE_DIRECTX_VER == 8
    pVB->Lock( 0, 0, (uint8_t**)&VertArray, D3DLOCK_DISCARD );
#endif
#if HGE_DIRECTX_VER == 9
    vertex_buf_->Lock(0, 0, reinterpret_cast<void **>(&vert_array_), D3DLOCK_DISCARD);
#endif
    return true;
}

void HGE_CALL HGE_Impl::Gfx_EndScene() {
    render_batch(true);
    d3d_device_->EndScene();
    if (!cur_target_) {
        d3d_device_->Present(nullptr, nullptr, nullptr, nullptr);
    }
}

void HGE_CALL HGE_Impl::Gfx_RenderLine(const float x1, const float y1,
                                       const float x2, const float y2,
                                       const DWORD color, const float z) {
    if (vert_array_) {
        if (cur_prim_type_ != HGEPRIM_LINES
            || n_prim_ >= VERTEX_BUFFER_SIZE / HGEPRIM_LINES
            || cur_texture_ || cur_blend_mode_ != BLEND_DEFAULT) {
            render_batch();

            cur_prim_type_ = HGEPRIM_LINES;
            if (cur_blend_mode_ != BLEND_DEFAULT) {
                set_blend_mode(BLEND_DEFAULT);
            }
            if (cur_texture_) {
                d3d_device_->SetTexture(0, nullptr);
                cur_texture_ = 0;
            }
        }

        const int i = n_prim_ * HGEPRIM_LINES;
        vert_array_[i].x = x1;
        vert_array_[i + 1].x = x2;
        vert_array_[i].y = y1;
        vert_array_[i + 1].y = y2;
        vert_array_[i].z = vert_array_[i + 1].z = z;
        vert_array_[i].col = vert_array_[i + 1].col = color;
        vert_array_[i].tx = vert_array_[i + 1].tx =
                vert_array_[i].ty = vert_array_[i + 1].ty = 0.0f;

        n_prim_++;
    }
}

void HGE_CALL HGE_Impl::Gfx_RenderTriple(const hgeTriple* triple) {
    if (vert_array_) {
        if (cur_prim_type_ != HGEPRIM_TRIPLES 
            || n_prim_ >= VERTEX_BUFFER_SIZE / HGEPRIM_TRIPLES
            || cur_texture_ != triple->tex
            || cur_blend_mode_ != triple->blend) 
        {
            render_batch();

            cur_prim_type_ = HGEPRIM_TRIPLES;
            if (cur_blend_mode_ != triple->blend) {
                set_blend_mode(triple->blend);
            }
            if (triple->tex != cur_texture_) {
                d3d_device_->SetTexture(0, reinterpret_cast<hgeGAPITexture *>(triple->tex));
                cur_texture_ = triple->tex;
            }
        }

        memcpy(&vert_array_[n_prim_ * HGEPRIM_TRIPLES], triple->v,
               sizeof(hgeVertex) * HGEPRIM_TRIPLES);
        n_prim_++;
    }
}

void HGE_CALL HGE_Impl::Gfx_RenderQuad(const hgeQuad* quad, bool filled) {
    if (vert_array_) {
        if (filled) {
            if (cur_prim_type_ != HGEPRIM_QUADS || n_prim_ >= VERTEX_BUFFER_SIZE / HGEPRIM_QUADS ||
                cur_texture_ != quad->tex
                || cur_blend_mode_ != quad->blend) {
                render_batch();

                cur_prim_type_ = HGEPRIM_QUADS;
                if (cur_blend_mode_ != quad->blend) {
                    set_blend_mode(quad->blend);
                }
                if (quad->tex != cur_texture_) {
                    d3d_device_->SetTexture(0, reinterpret_cast<hgeGAPITexture *>(quad->tex));
                    cur_texture_ = quad->tex;
                }
            }

            memcpy(&vert_array_[n_prim_ * HGEPRIM_QUADS],
                   quad->v,
                   sizeof(hgeVertex) * HGEPRIM_QUADS);
            n_prim_++;
        } else {
            Gfx_RenderLine(quad->v[0].x - 1, quad->v[0].y, quad->v[1].x, quad->v[1].y, quad->v->col);
            Gfx_RenderLine(quad->v[1].x, quad->v[1].y, quad->v[2].x, quad->v[2].y, quad->v->col);
            Gfx_RenderLine(quad->v[2].x, quad->v[2].y, quad->v[3].x, quad->v[3].y, quad->v->col);
            Gfx_RenderLine(quad->v[3].x, quad->v[3].y, quad->v[0].x, quad->v[0].y, quad->v->col);
        }
    }
}

void HGE_Impl::Gfx_RenderBumpedQuad(const hgeBumpQuad *quad, int, int) {
    throw NotImplemented();
}

IDirect3DDevice9 *HGE_Impl::Gfx_GetDevice() {
    return d3d_device_;
}

HSURFACE HGE_Impl::Target_GetSurface(HTARGET hTarget) {
    throw NotImplemented();
    return 0;
}

void HGE_Impl::Surface_Free(HSURFACE) {
    throw NotImplemented();
}

DWORD *HGE_Impl::Surface_Lock(HSURFACE, bool, int, int, int, int) {
    throw NotImplemented();
    return nullptr;
}

void HGE_Impl::Surface_Unlock(HSURFACE) {
    throw NotImplemented();
}

void HGE_Impl::Gfx_FlushBuffer() {
    throw NotImplemented();
}

hgeVertex* HGE_CALL HGE_Impl::
Gfx_StartBatch(const int prim_type, const HTEXTURE tex, const int blend, int* max_prim) {
    if (vert_array_) {
        render_batch();

        cur_prim_type_ = prim_type;
        if (cur_blend_mode_ != blend) {
            set_blend_mode(blend);
        }
        if (tex != cur_texture_) {
            d3d_device_->SetTexture(0, reinterpret_cast<hgeGAPITexture *>(tex));
            cur_texture_ = tex;
        }

        *max_prim = VERTEX_BUFFER_SIZE / prim_type;
        return vert_array_;
    }
    return nullptr;
}

void HGE_CALL HGE_Impl::Gfx_FinishBatch(const int nprim) {
    n_prim_ = nprim;
}

HTARGET HGE_CALL HGE_Impl::Target_Create(int width, int height,
                                         const bool zbuffer) {
    CRenderTargetList* pTarget;
    D3DSURFACE_DESC TDesc;

    pTarget = new CRenderTargetList;
    pTarget->pTex = nullptr;
    pTarget->pDepth = nullptr;

    if (FAILED(D3DXCreateTexture(d3d_device_, width, height, 1, D3DUSAGE_RENDERTARGET,
        d3dpp_->BackBufferFormat, D3DPOOL_DEFAULT, &pTarget->pTex))) {
        post_error("Can't create render target texture");
        delete pTarget;
        return 0;
    }

    pTarget->pTex->GetLevelDesc(0, &TDesc);
    pTarget->width = TDesc.Width;
    pTarget->height = TDesc.Height;

    if (zbuffer) {
#if HGE_DIRECTX_VER == 8
        if(FAILED(pD3DDevice->CreateDepthStencilSurface(pTarget->width, pTarget->height,
                  D3DFMT_D16, D3DMULTISAMPLE_NONE, &pTarget->pDepth)))
#endif
#if HGE_DIRECTX_VER == 9
        if (FAILED(d3d_device_->CreateDepthStencilSurface(width, height,
            D3DFMT_D16, D3DMULTISAMPLE_NONE, 0, false, &pTarget->pDepth, nullptr)))
#endif
        {
            pTarget->pTex->Release();
            post_error("Can't create render target depth buffer");
            delete pTarget;
            return 0;
        }
    }

    pTarget->next = targets_;
    targets_ = pTarget;

    return reinterpret_cast<HTARGET>(pTarget);
}

void HGE_CALL HGE_Impl::Target_Free(const HTARGET target) {
    auto p_target = targets_;
    CRenderTargetList* p_prev_target = nullptr;

    while (p_target) {
        if (reinterpret_cast<CRenderTargetList *>(target) == p_target) {
            if (p_prev_target) {
                p_prev_target->next = p_target->next;
            }
            else {
                targets_ = p_target->next;
            }

            if (p_target->pTex) {
                p_target->pTex->Release();
            }
            if (p_target->pDepth) {
                p_target->pDepth->Release();
            }

            delete p_target;
            return;
        }

        p_prev_target = p_target;
        p_target = p_target->next;
    }
}

HTEXTURE HGE_CALL HGE_Impl::Target_GetTexture(const HTARGET target) {
    auto targ = reinterpret_cast<CRenderTargetList *>(target);
    if (target) {
        return reinterpret_cast<HTEXTURE>(targ->pTex);
    }
    return 0;
}

HTEXTURE HGE_CALL HGE_Impl::Texture_Create(int width, int height) {
    hgeGAPITexture* p_tex;

    if (FAILED( D3DXCreateTexture( d3d_device_, width, height,
        1, // Mip levels
        0, // Usage
        D3DFMT_A8R8G8B8, // Format
        D3DPOOL_MANAGED, // Memory pool
        &p_tex ) )) {
        post_error("Can't create texture");
        return 0;
    }

    return reinterpret_cast<HTEXTURE>(p_tex);
}

HTEXTURE HGE_CALL HGE_Impl::Texture_Load(const char* filename,
                                         const DWORD size, bool bMipmap) {
    void* data;
    DWORD size1;
    D3DFORMAT fmt1, fmt2;
    hgeGAPITexture* p_tex;
    D3DXIMAGE_INFO info;

    if (size) {
        data = const_cast<void *>(reinterpret_cast<const void *>(filename));
        size1 = size;
    }
    else {
        data = pHGE->Resource_Load(filename, &size1);
        if (!data) {
            return 0;
        }
    }

    if (*static_cast<uint32_t*>(data) == 0x20534444) {
        // Compressed DDS format magic number
        fmt1 = D3DFMT_UNKNOWN;
        fmt2 = D3DFMT_A8R8G8B8;
    }
    else {
        fmt1 = D3DFMT_A8R8G8B8;
        fmt2 = D3DFMT_UNKNOWN;
    }

    //  if( FAILED( D3DXCreateTextureFromFileInMemory( pD3DDevice, data, _size, &pTex ) ) ) pTex=NULL;
    if (FAILED( D3DXCreateTextureFromFileInMemoryEx( d3d_device_, data, size1,
        D3DX_DEFAULT, D3DX_DEFAULT,
        bMipmap ? 0:1, // Mip levels
        0, // Usage
        fmt1, // Format
        D3DPOOL_MANAGED, // Memory pool
        D3DX_FILTER_NONE, // Filter
        D3DX_DEFAULT, // Mip filter
        0, // Color key
        &info, NULL,
        &p_tex ) ))

        if (FAILED( D3DXCreateTextureFromFileInMemoryEx( d3d_device_, data, size1,
            D3DX_DEFAULT, D3DX_DEFAULT,
            bMipmap ? 0:1, // Mip levels
            0, // Usage
            fmt2, // Format
            D3DPOOL_MANAGED, // Memory pool
            D3DX_FILTER_NONE, // Filter
            D3DX_DEFAULT, // Mip filter
            0, // Color key
            &info, NULL,
            &p_tex ) )) {
            post_error("Can't create texture");
            if (!size) {
                Resource_Free(data);
            }
            return NULL;
        }

    if (!size) {
        Resource_Free(data);
    }

    const auto tex_item = new CTextureList;
    tex_item->tex = reinterpret_cast<HTEXTURE>(p_tex);
    tex_item->width = info.Width;
    tex_item->height = info.Height;
    tex_item->next = textures_;
    textures_ = tex_item;

    return reinterpret_cast<HTEXTURE>(p_tex);
}

void HGE_CALL HGE_Impl::Texture_Free(const HTEXTURE tex) {
    auto p_tex = reinterpret_cast<hgeGAPITexture *>(tex);
    auto tex_item = textures_;
    CTextureList* tex_prev = nullptr;

    while (tex_item) {
        if (tex_item->tex == tex) {
            if (tex_prev) {
                tex_prev->next = tex_item->next;
            }
            else {
                textures_ = tex_item->next;
            }
            delete tex_item;
            break;
        }
        tex_prev = tex_item;
        tex_item = tex_item->next;
    }
    if (p_tex != nullptr) {
        p_tex->Release();
    }
}

int HGE_CALL HGE_Impl::Texture_GetWidth(const HTEXTURE tex,
                                        const bool b_original) {
    D3DSURFACE_DESC TDesc;
    auto p_tex = reinterpret_cast<hgeGAPITexture *>(tex);
    auto tex_item = textures_;

    if (b_original) {
        while (tex_item) {
            if (tex_item->tex == tex) {
                return tex_item->width;
            }
            tex_item = tex_item->next;
        }
        return 0;
    }
    if (FAILED(p_tex->GetLevelDesc(0, &TDesc))) {
        return 0;
    }
    return TDesc.Width;
}


int HGE_CALL HGE_Impl::Texture_GetHeight(const HTEXTURE tex,
                                         const bool bOriginal) {
    D3DSURFACE_DESC t_desc;
    auto p_tex = reinterpret_cast<hgeGAPITexture *>(tex);
    auto tex_item = textures_;

    if (bOriginal) {
        while (tex_item) {
            if (tex_item->tex == tex) {
                return tex_item->height;
            }
            tex_item = tex_item->next;
        }
        return 0;
    }
    if (FAILED(p_tex->GetLevelDesc(0, &t_desc))) {
        return 0;
    }
    return t_desc.Height;
}


DWORD* HGE_CALL HGE_Impl::Texture_Lock(const HTEXTURE tex,
                                          const bool b_read_only,
                                          const int left, const int top,
                                          const int width, const int height) {
    auto pTex = reinterpret_cast<hgeGAPITexture *>(tex);
    D3DSURFACE_DESC t_desc;
    D3DLOCKED_RECT t_rect;
    RECT region;
    RECT *prec;
    int flags;

    pTex->GetLevelDesc(0, &t_desc);
    if (t_desc.Format != D3DFMT_A8R8G8B8 && t_desc.Format != D3DFMT_X8R8G8B8) {
        return nullptr;
    }

    if (width && height) {
        region.left = left;
        region.top = top;
        region.right = left + width;
        region.bottom = top + height;
        prec = &region;
    }
    else {
        prec = nullptr;
    }

    if (b_read_only) {
        flags = D3DLOCK_READONLY;
    }
    else {
        flags = 0;
    }

    if (FAILED(pTex->LockRect(0, &t_rect, prec, flags))) {
        post_error("Can't lock texture");
        return nullptr;
    }

    return static_cast<DWORD *>(t_rect.pBits);
}


void HGE_CALL HGE_Impl::Texture_Unlock(const HTEXTURE tex) {
    auto p_tex = reinterpret_cast<hgeGAPITexture *>(tex);
    p_tex->UnlockRect(0);
}

//////// Implementation ////////

void HGE_Impl::render_batch(const bool b_end_scene) {
    if (vert_array_) {
        vertex_buf_->Unlock();

        if (n_prim_) {
            switch (cur_prim_type_) {
            case HGEPRIM_QUADS:
#if HGE_DIRECTX_VER == 8
                pD3DDevice->DrawIndexedPrimitive(
                    D3DPT_TRIANGLELIST, 0, nPrim<<2, 0, nPrim<<1);
#endif
#if HGE_DIRECTX_VER == 9
                d3d_device_->DrawIndexedPrimitive(
                    D3DPT_TRIANGLELIST, 0, 0, n_prim_ << 2, 0, n_prim_ << 1);
#endif
                break;

            case HGEPRIM_TRIPLES:
                d3d_device_->DrawPrimitive(D3DPT_TRIANGLELIST, 0, n_prim_);
                break;

            case HGEPRIM_LINES:
                d3d_device_->DrawPrimitive(D3DPT_LINELIST, 0, n_prim_);
                break;
            }

            n_prim_ = 0;
        }

        if (b_end_scene) {
            vert_array_ = nullptr;
        }
#if HGE_DIRECTX_VER == 8
        else {
            pVB->Lock( 0, 0, (uint8_t**)&VertArray, D3DLOCK_DISCARD );
        }
#endif
#if HGE_DIRECTX_VER == 9
        else {
            vertex_buf_->Lock(0, 0, reinterpret_cast<void **>(&vert_array_), 
                              D3DLOCK_DISCARD);
        }
#endif
    }
}

void HGE_Impl::set_blend_mode(const int blend) {
    auto d = -1;

    if ((blend & BLEND_ALPHABLEND) != (cur_blend_mode_ & BLEND_ALPHABLEND)) {
        if (blend & BLEND_ALPHABLEND) {
            d = D3DBLEND_INVSRCALPHA;
        }
        else {
            d = D3DBLEND_ONE;
        }
    }

    if ((blend & BLEND_DARKEN) != (cur_blend_mode_ & BLEND_DARKEN)) {
        if (blend & BLEND_DARKEN) {
            d3d_device_->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
            d = D3DBLEND_SRCCOLOR;
        }
        else {
            d3d_device_->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
            if (blend & BLEND_ALPHABLEND) {
                d = D3DBLEND_INVSRCALPHA;
            }
            else {
                d = D3DBLEND_ONE;
            }
        }
    }

    if (d != -1) {
        d3d_device_->SetRenderState(D3DRS_DESTBLEND, d);
    }

    if ((blend & BLEND_ZWRITE) != (cur_blend_mode_ & BLEND_ZWRITE)) {
        if (blend & BLEND_ZWRITE) {
            d3d_device_->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
        }
        else {
            d3d_device_->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        }
    }

    if ((blend & BLEND_COLORADD + BLEND_DARKEN) != 
        (cur_blend_mode_ & (BLEND_COLORADD + BLEND_DARKEN))) {
        if (blend & BLEND_COLORADD) {
            d3d_device_->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_ADD);
        }
        else if (blend & BLEND_DARKEN) {
            d3d_device_->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_BLENDCURRENTALPHA);
        }
        else {
            d3d_device_->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
        }
    }

    cur_blend_mode_ = blend;
}

void HGE_Impl::set_projection_matrix(const int width, const int height) {
    D3DXMATRIX tmp;
    D3DXMatrixScaling(&proj_matrix_, 1.0f, -1.0f, 1.0f);
    D3DXMatrixTranslation(&tmp, -0.5f, height + 0.5f, 0.0f);
    D3DXMatrixMultiply(&proj_matrix_, &proj_matrix_, &tmp);
    D3DXMatrixOrthoOffCenterLH(&tmp, 0, static_cast<float>(width),
                               0, static_cast<float>(height), 0.0f, 1.0f);
    D3DXMatrixMultiply(&proj_matrix_, &proj_matrix_, &tmp);
}

bool HGE_Impl::gfx_init() {
    static const char* szFormats[] = {
        "UNKNOWN", "R5G6B5", "X1R5G5B5", "A1R5G5B5", "X8R8G8B8", "A8R8G8B8"
    };
    hgeGAPIAdapterIdentifier ad_id;
    D3DDISPLAYMODE disp_mode;
    auto d3dfmt = D3DFMT_UNKNOWN;

    // Init D3D
#if HGE_DIRECTX_VER == 8
    pD3D=Direct3DCreate8(120); // D3D_SDK_VERSION
#endif
#if HGE_DIRECTX_VER == 9
    d3d_ = Direct3DCreate9(D3D_SDK_VERSION); // D3D_SDK_VERSION
#endif
    if (d3d_ == nullptr) {
        post_error("Can't create D3D interface");
        return false;
    }

    // Get adapter info

#if HGE_DIRECTX_VER == 8
    pD3D->GetAdapterIdentifier(D3DADAPTER_DEFAULT, D3DENUM_NO_WHQL_LEVEL, &AdID);
#endif
#if HGE_DIRECTX_VER == 9
    d3d_->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &ad_id);
#endif
    System_Log("D3D Driver: %s", ad_id.Driver);
    System_Log("Description: %s", ad_id.Description);
    System_Log("Version: %d.%d.%d.%d",
               HIWORD(ad_id.DriverVersion.HighPart),
               LOWORD(ad_id.DriverVersion.HighPart),
               HIWORD(ad_id.DriverVersion.LowPart),
               LOWORD(ad_id.DriverVersion.LowPart));

    // Set up Windowed presentation parameters

    if (FAILED(d3d_->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &disp_mode)) || disp_mode.Format ==
        D3DFMT_UNKNOWN) {
        post_error("Can't determine desktop video mode");
        if (windowed_) {
            return false;
        }
    }

    ZeroMemory(&d3dpp_windowed_, sizeof(d3dpp_windowed_));

    d3dpp_windowed_.BackBufferWidth = screen_width_;
    d3dpp_windowed_.BackBufferHeight = screen_height_;
    d3dpp_windowed_.BackBufferFormat = disp_mode.Format;
    d3dpp_windowed_.BackBufferCount = 1;
    d3dpp_windowed_.MultiSampleType = D3DMULTISAMPLE_NONE;
    d3dpp_windowed_.hDeviceWindow = hwnd_;
    d3dpp_windowed_.Windowed = TRUE;

#if HGE_DIRECTX_VER == 8
    if(nHGEFPS==HGEFPS_VSYNC) {
        d3dppW.SwapEffect = D3DSWAPEFFECT_COPY_VSYNC;
    } else {
        d3dppW.SwapEffect = D3DSWAPEFFECT_COPY;
    }
#endif
#if HGE_DIRECTX_VER == 9
    if (hgefps_ == HGEFPS_VSYNC) {
        d3dpp_windowed_.SwapEffect = D3DSWAPEFFECT_COPY;
        d3dpp_windowed_.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
    }
    else {
        d3dpp_windowed_.SwapEffect = D3DSWAPEFFECT_COPY;
        d3dpp_windowed_.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    }
#endif

    if (z_buffer_) {
        d3dpp_windowed_.EnableAutoDepthStencil = TRUE;
        d3dpp_windowed_.AutoDepthStencilFormat = D3DFMT_D16;
    }

    // Set up Full Screen presentation parameters

#if HGE_DIRECTX_VER == 8
    nModes=pD3D->GetAdapterModeCount(D3DADAPTER_DEFAULT);
#endif
#if HGE_DIRECTX_VER == 9
    const UINT n_modes = d3d_->GetAdapterModeCount(D3DADAPTER_DEFAULT, disp_mode.Format);
#endif

    for (UINT i = 0; i < n_modes; i++) {
#if HGE_DIRECTX_VER == 8
        pD3D->EnumAdapterModes(D3DADAPTER_DEFAULT, i, &Mode);
#endif
#if HGE_DIRECTX_VER == 9
        d3d_->EnumAdapterModes(D3DADAPTER_DEFAULT, disp_mode.Format, i, &disp_mode);
#endif

        if (disp_mode.Width != static_cast<UINT>(screen_width_)
            || disp_mode.Height != static_cast<UINT>(screen_height_)) {
            continue;
        }
        if (screen_bpp_ == 16 && (format_id(disp_mode.Format) > format_id(D3DFMT_A1R5G5B5))) {
            continue;
        }
        if (format_id(disp_mode.Format) > format_id(d3dfmt)) {
            d3dfmt = disp_mode.Format;
        }
    }

    if (d3dfmt == D3DFMT_UNKNOWN) {
        post_error("Can't find appropriate full screen video mode");
        if (!windowed_) {
            return false;
        }
    }

    ZeroMemory(&d3dpp_fullscreen_, sizeof(d3dpp_fullscreen_));

    d3dpp_fullscreen_.BackBufferWidth = screen_width_;
    d3dpp_fullscreen_.BackBufferHeight = screen_height_;
    d3dpp_fullscreen_.BackBufferFormat = d3dfmt;
    d3dpp_fullscreen_.BackBufferCount = 1;
    d3dpp_fullscreen_.MultiSampleType = D3DMULTISAMPLE_NONE;
    d3dpp_fullscreen_.hDeviceWindow = hwnd_;
    d3dpp_fullscreen_.Windowed = FALSE;

    d3dpp_fullscreen_.SwapEffect = D3DSWAPEFFECT_FLIP;
    d3dpp_fullscreen_.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;

#if HGE_DIRECTX_VER == 8
    if(nHGEFPS==HGEFPS_VSYNC) {
        d3dppFS.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    } else {
        d3dppFS.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    }
#endif
#if HGE_DIRECTX_VER == 9
    if (hgefps_ == HGEFPS_VSYNC) {
        d3dpp_fullscreen_.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    }
    else {
        d3dpp_fullscreen_.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    }
#endif
    if (z_buffer_) {
        d3dpp_fullscreen_.EnableAutoDepthStencil = TRUE;
        d3dpp_fullscreen_.AutoDepthStencilFormat = D3DFMT_D16;
    }

    d3dpp_ = windowed_ ? &d3dpp_windowed_ : &d3dpp_fullscreen_;

    if (format_id(d3dpp_->BackBufferFormat) < 4) {
        screen_bpp_ = 16;
    }
    else {
        screen_bpp_ = 32;
    }

    // Create D3D Device
    // #if HGE_DIRECTX_VER == 8
    //     if( FAILED( pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
    //                                   D3DCREATE_SOFTWARE_VERTEXPROCESSING,
    //                                   d3dpp, &pD3DDevice ) ) )
    //     {
    //         _PostError("Can't create D3D8 device");
    //         return false;
    //     }
    // #endif
    // #if HGE_DIRECTX_VER == 9
    hgeGAPICaps caps;
    d3d_->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);
    uint32_t vp;
    if ((caps.VertexShaderVersion < D3DVS_VERSION(1,1))
        || !(caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)) {
        System_Log("Software Vertex-processing device selected");
        vp = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    }
    else {
        System_Log("Hardware Vertex-processing device selected");
        vp = D3DCREATE_HARDWARE_VERTEXPROCESSING;
    }
    if (FAILED( d3d_->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd_, vp, d3dpp_,
        &d3d_device_ ) )) {
        post_error("Can't create D3D device");
        return false;
    }
    // #endif

    adjust_window();

    System_Log("Mode: %d x %d x %s\n", screen_width_, screen_height_, szFormats[format_id(d3dfmt)]);

    // Create vertex batch buffer

    vert_array_ = nullptr;
    textures_ = nullptr;

    // Init all stuff that can be lost

    set_projection_matrix(screen_width_, screen_height_);
    D3DXMatrixIdentity(&view_matrix_);

    if (!init_lost()) {
        return false;
    }

    Gfx_Clear(0);

    return true;
}

int HGE_Impl::format_id(const D3DFORMAT fmt) {
    switch (fmt) {
    case D3DFMT_R5G6B5:
        return 1;
    case D3DFMT_X1R5G5B5:
        return 2;
    case D3DFMT_A1R5G5B5:
        return 3;
    case D3DFMT_X8R8G8B8:
        return 4;
    case D3DFMT_A8R8G8B8:
        return 5;
    default:
        return 0;
    }
}

void HGE_Impl::adjust_window() {
    RECT* rc;
    LONG style;

    if (windowed_) {
        rc = &rect_windowed_;
        style = style_windowed_;
    }
    else {
        rc = &rect_fullscreen_;
        style = style_fullscreen_;
    }
    SetWindowLong(hwnd_, GWL_STYLE, style);

    style = GetWindowLong(hwnd_, GWL_EXSTYLE);
    if (windowed_) {
        SetWindowLong(hwnd_, GWL_EXSTYLE, style & (~WS_EX_TOPMOST));
        SetWindowPos(hwnd_, HWND_NOTOPMOST, rc->left, rc->top, rc->right - rc->left,
                     rc->bottom - rc->top,
                     SWP_FRAMECHANGED);
    }
    else {
        SetWindowLong(hwnd_, GWL_EXSTYLE, style | WS_EX_TOPMOST);
        SetWindowPos(hwnd_, HWND_TOPMOST, rc->left, rc->top, rc->right - rc->left,
                     rc->bottom - rc->top,
                     SWP_FRAMECHANGED);
    }
}

void HGE_Impl::resize(const int width, const int height, bool generateEvent) {
    d3dpp_windowed_.BackBufferWidth = width;
    d3dpp_windowed_.BackBufferHeight = height;
    screen_width_ = width;
    screen_height_ = height;

    set_projection_matrix(screen_width_, screen_height_);
    gfx_restore();

    if (generateEvent && proc_resized_func_) {
        proc_resized_func_();
    }
}

void HGE_Impl::gfx_done() {
    auto target = targets_;

    while (textures_) {
        Texture_Free(textures_->tex);
    }

    if (screen_surf_) {
        screen_surf_->Release();
        screen_surf_ = nullptr;
    }
    if (screen_depth_) {
        screen_depth_->Release();
        screen_depth_ = nullptr;
    }

    while (target) {
        if (target->pTex) {
            target->pTex->Release();
        }
        if (target->pDepth) {
            target->pDepth->Release();
        }
        const auto next_target = target->next;
        delete target;
        target = next_target;
    }
    targets_ = nullptr;

    if (index_buf_ && d3d_device_) {
#if HGE_DIRECTX_VER == 8
        pD3DDevice->SetIndices(NULL,0);
#endif
#if HGE_DIRECTX_VER == 9
        d3d_device_->SetIndices(nullptr);
#endif
        index_buf_->Release();
        index_buf_ = nullptr;
    }
    if (vertex_buf_ && d3d_device_) {
        if (vert_array_) {
            vertex_buf_->Unlock();
            vert_array_ = nullptr;
        }
#if HGE_DIRECTX_VER == 8
        pD3DDevice->SetStreamSource( 0, NULL, sizeof(hgeVertex) );
#endif
#if HGE_DIRECTX_VER == 9
        d3d_device_->SetStreamSource(0, nullptr, 0, sizeof(hgeVertex));
#endif
        vertex_buf_->Release();
        vertex_buf_ = nullptr;
    }
    if (d3d_device_) {
        d3d_device_->Release();
        d3d_device_ = nullptr;
    }
    if (d3d_) {
        d3d_->Release();
        d3d_ = nullptr;
    }
}


bool HGE_Impl::gfx_restore() {
    CRenderTargetList* target = targets_;

    //if(!pD3DDevice) return false;
    //if(pD3DDevice->TestCooperativeLevel() == D3DERR_DEVICELOST) return;

    if (screen_surf_) {
        screen_surf_->Release();
    }
    if (screen_depth_) {
        screen_depth_->Release();
    }

    while (target) {
        if (target->pTex) {
            target->pTex->Release();
        }
        if (target->pDepth) {
            target->pDepth->Release();
        }
        target = target->next;
    }

    if (index_buf_) {
#if HGE_DIRECTX_VER == 8
        pD3DDevice->SetIndices(NULL,0);
#endif
#if HGE_DIRECTX_VER == 9
        d3d_device_->SetIndices(nullptr);
#endif
        index_buf_->Release();
    }
    if (vertex_buf_) {
#if HGE_DIRECTX_VER == 8
        pD3DDevice->SetStreamSource( 0, NULL, sizeof(hgeVertex) );
#endif
#if HGE_DIRECTX_VER == 9
        d3d_device_->SetStreamSource(0, nullptr, 0, sizeof(hgeVertex));
#endif
        vertex_buf_->Release();
    }

    d3d_device_->Reset(d3dpp_);

    if (!init_lost()) {
        return false;
    }

    if (proc_gfx_restore_func_) {
        return proc_gfx_restore_func_();
    }

    return true;
}


bool HGE_Impl::init_lost() {
    CRenderTargetList* target = targets_;

    // Store render target

    screen_surf_ = nullptr;
    screen_depth_ = nullptr;

#if HGE_DIRECTX_VER == 8
    pD3DDevice->GetRenderTarget(&pScreenSurf);
#endif
#if HGE_DIRECTX_VER == 9
    d3d_device_->GetRenderTarget(0, &screen_surf_);
#endif
    d3d_device_->GetDepthStencilSurface(&screen_depth_);

    while (target) {
        if (target->pTex)
            D3DXCreateTexture(d3d_device_, target->width, target->height, 1, D3DUSAGE_RENDERTARGET,
                              d3dpp_->BackBufferFormat, D3DPOOL_DEFAULT, &target->pTex);
        if (target->pDepth)
#if HGE_DIRECTX_VER == 8
            pD3DDevice->CreateDepthStencilSurface(
                target->width, target->height, D3DFMT_D16, D3DMULTISAMPLE_NONE, 
                &target->pDepth);
#endif
#if HGE_DIRECTX_VER == 9
            d3d_device_->CreateDepthStencilSurface(
                target->width, target->height, D3DFMT_D16, D3DMULTISAMPLE_NONE,
                0, false, &target->pDepth, nullptr);
#endif
        target = target->next;
    }

    // Create Vertex buffer
#if HGE_DIRECTX_VER == 8
    if( FAILED (pD3DDevice->CreateVertexBuffer(VERTEX_BUFFER_SIZE*sizeof(hgeVertex),
                D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
                D3DFVF_HGEVERTEX,
                D3DPOOL_DEFAULT, &pVB )))
#endif
#if HGE_DIRECTX_VER == 9
    if (FAILED (d3d_device_->CreateVertexBuffer(VERTEX_BUFFER_SIZE*sizeof(hgeVertex),
        D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
        D3DFVF_HGEVERTEX,
        D3DPOOL_DEFAULT,
        &vertex_buf_,
        NULL)))
#endif
    {
        post_error("Can't create D3D vertex buffer");
        return false;
    }

#if HGE_DIRECTX_VER == 8
    pD3DDevice->SetVertexShader( D3DFVF_HGEVERTEX );
    pD3DDevice->SetStreamSource( 0, pVB, sizeof(hgeVertex) );
#endif
#if HGE_DIRECTX_VER == 9
    d3d_device_->SetVertexShader(nullptr);
    d3d_device_->SetFVF(D3DFVF_HGEVERTEX);
    d3d_device_->SetStreamSource(0, vertex_buf_, 0, sizeof(hgeVertex));
#endif

    // Create and setup Index buffer

#if HGE_DIRECTX_VER == 8
    if( FAILED( pD3DDevice->CreateIndexBuffer(VERTEX_BUFFER_SIZE*6/4*sizeof(uint16_t),
                D3DUSAGE_WRITEONLY,
                D3DFMT_INDEX16,
                D3DPOOL_DEFAULT, &pIB ) ) )
#endif
#if HGE_DIRECTX_VER == 9
    if (FAILED( d3d_device_->CreateIndexBuffer(VERTEX_BUFFER_SIZE*6/4*sizeof(uint16_t),
        D3DUSAGE_WRITEONLY,
        D3DFMT_INDEX16,
        D3DPOOL_DEFAULT,
        &index_buf_,
        NULL) ))
#endif
    {
        post_error("Can't create D3D index buffer");
        return false;
    }

    uint16_t *pIndices, n = 0;
#if HGE_DIRECTX_VER == 8
    if( FAILED( pIB->Lock( 0, 0, (uint8_t**)&pIndices, 0 ) ) )
#endif
#if HGE_DIRECTX_VER == 9
    if (FAILED( index_buf_->Lock( 0, 0, (VOID**)&pIndices, 0 ) ))
#endif
    {
        post_error("Can't lock D3D index buffer");
        return false;
    }

    for (int i = 0; i < VERTEX_BUFFER_SIZE / 4; i++) {
        *pIndices++ = n;
        *pIndices++ = n + 1;
        *pIndices++ = n + 2;
        *pIndices++ = n + 2;
        *pIndices++ = n + 3;
        *pIndices++ = n;
        n += 4;
    }

    index_buf_->Unlock();
#if HGE_DIRECTX_VER == 8
    pD3DDevice->SetIndices(pIB,0);
#endif
#if HGE_DIRECTX_VER == 9
    d3d_device_->SetIndices(index_buf_);
#endif

    // Set common render states

    //pD3DDevice->SetRenderState( D3DRS_LASTPIXEL, FALSE );
    d3d_device_->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    d3d_device_->SetRenderState(D3DRS_LIGHTING, FALSE);

    d3d_device_->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    d3d_device_->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    d3d_device_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

    d3d_device_->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
    d3d_device_->SetRenderState(D3DRS_ALPHAREF, 0x01);
    d3d_device_->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);

    d3d_device_->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    d3d_device_->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    d3d_device_->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

    d3d_device_->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    d3d_device_->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    d3d_device_->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

#if HGE_DIRECTX_VER == 8
    pD3DDevice->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTEXF_POINT);
    if(bTextureFilter) {
        pD3DDevice->SetTextureStageState(0,D3DTSS_MAGFILTER,D3DTEXF_LINEAR);
        pD3DDevice->SetTextureStageState(0,D3DTSS_MINFILTER,D3DTEXF_LINEAR);
    } else {
        pD3DDevice->SetTextureStageState(0,D3DTSS_MAGFILTER,D3DTEXF_POINT);
        pD3DDevice->SetTextureStageState(0,D3DTSS_MINFILTER,D3DTEXF_POINT);
    }
#endif
#if HGE_DIRECTX_VER == 9
    d3d_device_->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
    if (texture_filter_) {
        d3d_device_->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        d3d_device_->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    }
    else {
        d3d_device_->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
        d3d_device_->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    }
#endif
    n_prim_ = 0;
    cur_prim_type_ = HGEPRIM_QUADS;

    cur_blend_mode_ = BLEND_DEFAULT;
    // Reset default DirectX Zbuffer write to false
    if (false == z_buffer_) {
        d3d_device_->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    }

    cur_texture_ = NULL;
#if HGE_DIRECTX_VER >= 9
    cur_shader_ = NULL;
#endif

    d3d_device_->SetTransform(D3DTS_VIEW, &view_matrix_);
    d3d_device_->SetTransform(D3DTS_PROJECTION, &proj_matrix_);

    return true;
}

HSHADER HGE_CALL HGE_Impl::Shader_Create(const char* filename, DWORD size) {
    LPD3DXEFFECT ret = {};
    HRESULT res;

    if (size)
        res = D3DXCreateEffect(d3d_device_, filename, size, 0, 0, 0, 0, &ret, 0);
    else
        res = D3DXCreateEffectFromFileA(d3d_device_, filename, 0, 0, 0, 0, &ret, 0);

    if (FAILED(res)) {
        post_error("Can't create shader");
        return NULL;
    }
    return (HSHADER)ret;
}

HSHTECH HGE_CALL HGE_Impl::Shader_GetTechnique(HSHADER shad, const char *name) {
    if (shad) {
        return (HSHTECH) ((LPD3DXEFFECT) shad)->GetCurrentTechnique();
    }
    return 0;
}

void HGE_CALL HGE_Impl::Shader_SetTechnique(HSHADER shad, HSHTECH tech) {
    ((LPD3DXEFFECT)shad)->SetTechnique(reinterpret_cast<D3DXHANDLE>(tech));
}

int HGE_CALL HGE_Impl::Shader_Begin(HSHADER shad, int flags) {
    if (shad) {
        if (flags == 2) {
            flags = 4;
        } else if (flags != 1) {
            flags = 2 * (flags == 3);
        }
        UINT passes;
        if (!((LPD3DXEFFECT)shad)->Begin(&passes, flags)) {
            return passes;
        }
        post_error("Can't begin shader");
    }
    return 0;
}

void HGE_CALL HGE_Impl::Shader_End(HSHADER shad) {
    ((LPD3DXEFFECT)shad)->End();
}

void HGE_CALL HGE_Impl::Shader_BeginPass(HSHADER shad, int pass) {
    auto result = ((LPD3DXEFFECT)shad)->BeginPass(pass);
    if (result) {
        post_error("Can't begin shader pass");
    }
}

void HGE_CALL HGE_Impl::Shader_EndPass(HSHADER shad) {
    ((LPD3DXEFFECT)shad)->EndPass();
}

void HGE_CALL HGE_Impl::Shader_CommitChanges(HSHADER shad) {
    auto result = ((LPD3DXEFFECT)shad)->CommitChanges();
    if (result) {
        post_error("Can't commit changes");
    }
}

HSHPARAM HGE_CALL HGE_Impl::Shader_GetParam(HSHADER shad, const char *name) {
    return (HSHPARAM) ((LPD3DXEFFECT) shad)->GetParameterByName(NULL, name);
}

void HGE_CALL HGE_Impl::Shader_SetValue(HSHADER shad, HSHPARAM param, void *data, int length) {
    ((LPD3DXEFFECT)shad)->SetValue(reinterpret_cast<D3DXHANDLE>(param), data, length);
}

void HGE_CALL HGE_Impl::Shader_GetValue(HSHADER shad, HSHPARAM param, void *data, int length) {
    ((LPD3DXEFFECT)shad)->GetValue(reinterpret_cast<D3DXHANDLE>(param), data, length);
}

void HGE_CALL HGE_Impl::Shader_SetBool(HSHADER shad, HSHPARAM param, bool b) {
    ((LPD3DXEFFECT)shad)->SetBool(reinterpret_cast<D3DXHANDLE>(param), b);
}

bool HGE_CALL HGE_Impl::Shader_GetBool(HSHADER shad, HSHPARAM param) {
    BOOL ret;
    ((LPD3DXEFFECT)shad)->GetBool(reinterpret_cast<D3DXHANDLE>(param), &ret);
    return ret;
}

void HGE_CALL HGE_Impl::Shader_SetFloat(HSHADER shad, HSHPARAM param, float f) {
    ((LPD3DXEFFECT)shad)->SetFloat(reinterpret_cast<D3DXHANDLE>(param), f);
}

float HGE_CALL HGE_Impl::Shader_GetFloat(HSHADER shad, HSHPARAM param) {
    FLOAT ret;
    ((LPD3DXEFFECT)shad)->GetFloat(reinterpret_cast<D3DXHANDLE>(param), &ret);
    return ret;
}

void HGE_CALL HGE_Impl::Shader_SetInt(HSHADER shad, HSHPARAM param, int f) {
    ((LPD3DXEFFECT)shad)->SetInt(reinterpret_cast<D3DXHANDLE>(param), f);
}

int HGE_CALL HGE_Impl::Shader_GetInt(HSHADER shad, HSHPARAM param) {
    INT ret;
    ((LPD3DXEFFECT)shad)->GetInt(reinterpret_cast<D3DXHANDLE>(param), &ret);
    return ret;
}

void HGE_CALL HGE_Impl::Shader_SetTexture(HSHADER shad, HSHPARAM param, HTEXTURE tex) {
    ((LPD3DXEFFECT)shad)->SetTexture(reinterpret_cast<D3DXHANDLE>(param), reinterpret_cast<LPDIRECT3DBASETEXTURE9>(tex));
}

HTEXTURE    HGE_CALL HGE_Impl::Shader_GetTexture(HSHADER shad, HSHPARAM param) {
    HTEXTURE ret;
    ((LPD3DXEFFECT)shad)->GetTexture(reinterpret_cast<D3DXHANDLE>(param),
                                     reinterpret_cast<LPDIRECT3DBASETEXTURE9 *>(&ret));
    return ret;
}

void HGE_CALL HGE_Impl::Shader_SetVector(HSHADER shad, HSHPARAM param, float x, float y, float z, float w) {
    D3DXVECTOR4 v(x, y, z, w);
    ((LPD3DXEFFECT)shad)->SetVector(reinterpret_cast<D3DXHANDLE>(param), (const D3DXVECTOR4 *) &v);
}

void HGE_CALL HGE_Impl::Shader_GetVector(HSHADER shad, HSHPARAM param, float *x, float *y, float *z, float *w) {
    D3DXVECTOR4 v;
    ((LPD3DXEFFECT)shad)->GetVector(reinterpret_cast<D3DXHANDLE>(param), &v);
    *x = v.x;
    *y = v.y;
    *z = v.z;
    *w = v.w;
}
