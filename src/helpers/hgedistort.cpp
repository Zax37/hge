/*
** Haaf's Game Engine 1.7
** Copyright (C) 2003-2007, Relish Games
** hge.relishgames.com
**
** hgeDistortionMesh helper class implementation
*/

#include "../../include/hgedistort.h"


HGE* hgeDistortionMesh::hge_ = nullptr;


hgeDistortionMesh::hgeDistortionMesh(const int cols, const int rows) {

    hge_ = hgeCreate(HGE_VERSION);

    rows_ = rows;
    cols_ = cols;
    cellw_ = cellh_ = 0;
    quad_.tex = 0;
    quad_.blend = BLEND_COLORMUL | BLEND_ALPHABLEND | BLEND_ZWRITE;
    disp_array_ = new hgeVertex[rows * cols];

    for (int i = 0; i < rows * cols; i++) {
        disp_array_[i].x = 0.0f;
        disp_array_[i].y = 0.0f;
        disp_array_[i].tx = 0.0f;
        disp_array_[i].ty = 0.0f;

        disp_array_[i].z = 0.5f;
        disp_array_[i].col = 0xFFFFFFFF;
    }
}

hgeDistortionMesh::hgeDistortionMesh(const hgeDistortionMesh& dm) {
    hge_ = hgeCreate(HGE_VERSION);

    rows_ = dm.rows_;
    cols_ = dm.cols_;
    cellw_ = dm.cellw_;
    cellh_ = dm.cellh_;
    tx_ = dm.tx_;
    ty_ = dm.ty_;
    width_ = dm.width_;
    height_ = dm.height_;
    quad_ = dm.quad_;

    disp_array_ = new hgeVertex[rows_ * cols_];
    memcpy(disp_array_, dm.disp_array_, sizeof(hgeVertex) * rows_ * cols_);
}

hgeDistortionMesh::~hgeDistortionMesh() {
    delete[] disp_array_;
    hge_->Release();
}

hgeDistortionMesh& hgeDistortionMesh::operator=(const hgeDistortionMesh& dm) {
    if (this != &dm) {
        rows_ = dm.rows_;
        cols_ = dm.cols_;
        cellw_ = dm.cellw_;
        cellh_ = dm.cellh_;
        tx_ = dm.tx_;
        ty_ = dm.ty_;
        width_ = dm.width_;
        height_ = dm.height_;
        quad_ = dm.quad_;

        delete[] disp_array_;
        disp_array_ = new hgeVertex[rows_ * cols_];
        memcpy(disp_array_, dm.disp_array_, sizeof(hgeVertex) * rows_ * cols_);
    }

    return *this;

}

void hgeDistortionMesh::SetTexture(const HTEXTURE tex) {
    quad_.tex = tex;
}

void hgeDistortionMesh::SetTextureRect(const float x, const float y,
                                       const float w, const float h) {
    float tw, th;

    tx_ = x;
    ty_ = y;
    width_ = w;
    height_ = h;

    if (quad_.tex) {
        tw = static_cast<float>(hge_->Texture_GetWidth(quad_.tex));
        th = static_cast<float>(hge_->Texture_GetHeight(quad_.tex));
    }
    else {
        tw = w;
        th = h;
    }

    cellw_ = w / (cols_ - 1);
    cellh_ = h / (rows_ - 1);

    for (int j = 0; j < rows_; j++)
        for (int i = 0; i < cols_; i++) {
            disp_array_[j * cols_ + i].tx = (x + i * cellw_) / tw;
            disp_array_[j * cols_ + i].ty = (y + j * cellh_) / th;

            disp_array_[j * cols_ + i].x = i * cellw_;
            disp_array_[j * cols_ + i].y = j * cellh_;
        }
}

void hgeDistortionMesh::SetBlendMode(const int blend) {
    quad_.blend = blend;
}

void hgeDistortionMesh::Clear(const DWORD col, const float z) {

    for (auto j = 0; j < rows_; j++)
        for (auto i = 0; i < cols_; i++) {
            disp_array_[j * cols_ + i].x = i * cellw_;
            disp_array_[j * cols_ + i].y = j * cellh_;
            disp_array_[j * cols_ + i].col = col;
            disp_array_[j * cols_ + i].z = z;
        }
}

void hgeDistortionMesh::Render(const float x, const float y) {

    for (auto j = 0; j < rows_ - 1; j++)
        for (auto i = 0; i < cols_ - 1; i++) {
            const auto idx = j * cols_ + i;

            quad_.v[0].tx = disp_array_[idx].tx;
            quad_.v[0].ty = disp_array_[idx].ty;
            quad_.v[0].x = x + disp_array_[idx].x;
            quad_.v[0].y = y + disp_array_[idx].y;
            quad_.v[0].z = disp_array_[idx].z;
            quad_.v[0].col = disp_array_[idx].col;

            quad_.v[1].tx = disp_array_[idx + 1].tx;
            quad_.v[1].ty = disp_array_[idx + 1].ty;
            quad_.v[1].x = x + disp_array_[idx + 1].x;
            quad_.v[1].y = y + disp_array_[idx + 1].y;
            quad_.v[1].z = disp_array_[idx + 1].z;
            quad_.v[1].col = disp_array_[idx + 1].col;

            quad_.v[2].tx = disp_array_[idx + cols_ + 1].tx;
            quad_.v[2].ty = disp_array_[idx + cols_ + 1].ty;
            quad_.v[2].x = x + disp_array_[idx + cols_ + 1].x;
            quad_.v[2].y = y + disp_array_[idx + cols_ + 1].y;
            quad_.v[2].z = disp_array_[idx + cols_ + 1].z;
            quad_.v[2].col = disp_array_[idx + cols_ + 1].col;

            quad_.v[3].tx = disp_array_[idx + cols_].tx;
            quad_.v[3].ty = disp_array_[idx + cols_].ty;
            quad_.v[3].x = x + disp_array_[idx + cols_].x;
            quad_.v[3].y = y + disp_array_[idx + cols_].y;
            quad_.v[3].z = disp_array_[idx + cols_].z;
            quad_.v[3].col = disp_array_[idx + cols_].col;

            hge_->Gfx_RenderQuad(&quad_);
        }
}

void hgeDistortionMesh::SetZ(const int col, const int row, const float z) {
    if (row < rows_ && col < cols_) {
        disp_array_[row * cols_ + col].z = z;
    }
}

void hgeDistortionMesh::SetColor(const int col, const int row, 
                                 const DWORD color) {
    if (row < rows_ && col < cols_) {
        disp_array_[row * cols_ + col].col = color;
    }
}

void hgeDistortionMesh::SetDisplacement(const int col, const int row, 
                                        float dx, float dy, const int ref) {
    if (row < rows_ && col < cols_) {
        switch (ref) {
        case HGEDISP_NODE:
            dx += col * cellw_;
            dy += row * cellh_;
            break;
        case HGEDISP_CENTER:
            dx += cellw_ * (cols_ - 1) / 2;
            dy += cellh_ * (rows_ - 1) / 2;
            break;
        case HGEDISP_TOPLEFT:
            break;
        }

        disp_array_[row * cols_ + col].x = dx;
        disp_array_[row * cols_ + col].y = dy;
    }
}

float hgeDistortionMesh::GetZ(const int col, const int row) const {
    if (row < rows_ && col < cols_) {
        return disp_array_[row * cols_ + col].z;
    }
    return 0.0f;
}

DWORD hgeDistortionMesh::GetColor(const int col, const int row) const {
    if (row < rows_ && col < cols_) {
        return disp_array_[row * cols_ + col].col;
    }
    return 0;
}

void hgeDistortionMesh::GetDisplacement(const int col, const int row, 
                                        float* dx, float* dy, const int ref) const {
    if (row < rows_ && col < cols_) {
        switch (ref) {
        case HGEDISP_NODE:
            *dx = disp_array_[row * cols_ + col].x - col * cellw_;
            *dy = disp_array_[row * cols_ + col].y - row * cellh_;
            break;

        case HGEDISP_CENTER:
            *dx = disp_array_[row * cols_ + col].x - cellw_ * (cols_ - 1) / 2;
            *dy = disp_array_[row * cols_ + col].x - cellh_ * (rows_ - 1) / 2;
            break;

        case HGEDISP_TOPLEFT:
            *dx = disp_array_[row * cols_ + col].x;
            *dy = disp_array_[row * cols_ + col].y;
            break;
        }
    }
}
