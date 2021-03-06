/*
** Haaf's Game Engine 1.8
** Copyright (C) 2003-2007, Relish Games
** hge.relishgames.com
**
** Common core implementation header
*/


#pragma once

#include <hge.h>
#include <stdio.h>
#include <hge_gapi.h>


#define HGE_SPLASH_ENABLE

#define D3DFVF_HGEVERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)
#define VERTEX_BUFFER_SIZE 4000


typedef BOOL (WINAPI *GetSystemPowerStatusFunc)(LPSYSTEM_POWER_STATUS);


struct CRenderTargetList {
    int width;
    int height;
    hgeGAPITexture* pTex;
    hgeGAPISurface* pDepth;
    CRenderTargetList* next;
};

struct CTextureList {
    HTEXTURE tex;
    int width;
    int height;
    CTextureList* next;
};

struct CResourceList {
    char filename[_MAX_PATH];
    char password[64];
    CResourceList* next;
};

struct CStreamList {
    HSTREAM hstream;
    void* data;
    CStreamList* next;
};

struct CInputEventList {
    hgeInputEvent event;
    CInputEventList* next;
};


void prepare_demo();
void finish_demo();
bool demo_render_frame();

class FileDropListener : public IDropTarget
    {
public:
    explicit FileDropListener() : m_cRef(1) {}

    // IUnknown methods
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);
    STDMETHOD(QueryInterface)(REFIID riid, void FAR* FAR* ppvObj);

    // IDropTarget methods
    STDMETHOD(DragEnter)(LPDATAOBJECT pDataObj, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);
    STDMETHOD(DragOver)(DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);
    STDMETHOD(DragLeave)();
    STDMETHOD(Drop)(LPDATAOBJECT pDataObj, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);
private:
    LONG m_cRef;
};

/*
** HGE Interface implementation
*/
class HGE_Impl : public HGE {
public:
    void HGE_CALL Release() override;

    bool HGE_CALL System_Initiate() override;
    void HGE_CALL System_Shutdown() override;
    bool HGE_CALL System_Start() override;
    void HGE_CALL System_SetStateBool(hgeBoolState state, bool value) override;
    void HGE_CALL System_SetStateFunc(hgeFuncState state, hgeCallback value) override;
    void HGE_CALL System_SetStateHwnd(hgeHwndState state, HWND value) override;
    void HGE_CALL System_SetStateInt(hgeIntState state, int value) override;
    void HGE_CALL System_SetStateString(hgeStringState state, const char* value) override;
    bool HGE_CALL System_GetStateBool(hgeBoolState) override;
    hgeCallback HGE_CALL System_GetStateFunc(hgeFuncState) override;
    HWND HGE_CALL System_GetStateHwnd(hgeHwndState) override;
    int HGE_CALL System_GetStateInt(hgeIntState) override;
    const char* HGE_CALL System_GetStateString(hgeStringState) override;
    char* HGE_CALL System_GetErrorMessage() override;
    void HGE_CALL System_Log(const char* format, ...) override;
    bool HGE_CALL System_Launch(const char* url) override;
    void HGE_CALL System_Snapshot(const char* filename = nullptr) override;
    void HGE_CALL System_GetDroppedFilesPosition(int *x, int *y) override;
    void HGE_CALL System_DoManualMainLoop() override;

    void* HGE_CALL Resource_Load(const char* filename, DWORD* size = nullptr) override;
    void HGE_CALL Resource_Free(void* res) override;
    bool HGE_CALL Resource_AttachPack(const char* filename, const char* password = nullptr) override;
    void HGE_CALL Resource_RemovePack(const char* filename) override;
    void HGE_CALL Resource_RemoveAllPacks() override;
    char* HGE_CALL Resource_MakePath(const char* filename = nullptr) override;
    char* HGE_CALL Resource_EnumFiles(const char* wildcard = nullptr) override;
    char* HGE_CALL Resource_EnumFolders(const char* wildcard = nullptr) override;

    void HGE_CALL Ini_SetInt(const char* section, const char* name, int value) override;
    int HGE_CALL Ini_GetInt(const char* section, const char* name, int def_val) override;
    void HGE_CALL Ini_SetFloat(const char* section, const char* name, float value) override;
    float HGE_CALL Ini_GetFloat(const char* section, const char* name, float def_val) override;
    void HGE_CALL Ini_SetString(const char* section, const char* name,
                                        const char* value) override;
    char* HGE_CALL Ini_GetString(const char* section, const char* name,
                                         const char* def_val) override;

    void HGE_CALL Random_Seed(int seed = 0) override;
    int HGE_CALL Random_Int(int min, int max) override;
    float HGE_CALL Random_Float(float min, float max) override;

    float HGE_CALL Timer_GetTime() override;
    float HGE_CALL Timer_GetDelta() override;
    int HGE_CALL Timer_GetFPS() override;
    float HGE_CALL Timer_GetDeltaRealtime() override;

    HEFFECT HGE_CALL Effect_Load(const char* filename, DWORD size = 0) override;
    void HGE_CALL Effect_Free(HEFFECT eff) override;
    HCHANNEL HGE_CALL Effect_Play(HEFFECT eff) override;
    HCHANNEL HGE_CALL Effect_PlayEx(HEFFECT eff, int volume = 100, int pan = 0,
                                            float pitch = 1.0f, bool loop = false) override;

    HMUSIC HGE_CALL Music_Load(const char* filename, DWORD size = 0) override;
    void HGE_CALL Music_Free(HMUSIC mus) override;
    HCHANNEL HGE_CALL Music_Play(HMUSIC mus, bool loop, int volume = 100, int order = 0,
                                         int row = 0) override;
    void HGE_CALL Music_SetAmplification(HMUSIC music, int ampl) override;
    int HGE_CALL Music_GetAmplification(HMUSIC music) override;
    int HGE_CALL Music_GetLength(HMUSIC music) override;
    void HGE_CALL Music_SetPos(HMUSIC music, int order, int row) override;
    bool HGE_CALL Music_GetPos(HMUSIC music, int* order, int* row) override;
    void HGE_CALL Music_SetInstrVolume(HMUSIC music, int instr, int volume) override;
    int HGE_CALL Music_GetInstrVolume(HMUSIC music, int instr) override;
    void HGE_CALL Music_SetChannelVolume(HMUSIC music, int channel, int volume) override;
    int HGE_CALL Music_GetChannelVolume(HMUSIC music, int channel) override;

    HSTREAM HGE_CALL Stream_Load(const char* filename, DWORD size = 0) override;
    void HGE_CALL Stream_Free(HSTREAM stream) override;
    HCHANNEL HGE_CALL Stream_Play(HSTREAM stream, bool loop, int volume = 100) override;

    void HGE_CALL Channel_SetPanning(HCHANNEL chn, int pan) override;
    void HGE_CALL Channel_SetVolume(HCHANNEL chn, int volume) override;
    void HGE_CALL Channel_SetPitch(HCHANNEL chn, float pitch) override;
    void HGE_CALL Channel_Pause(HCHANNEL chn) override;
    void HGE_CALL Channel_Resume(HCHANNEL chn) override;
    void HGE_CALL Channel_Stop(HCHANNEL chn) override;
    void HGE_CALL Channel_PauseAll() override;
    void HGE_CALL Channel_ResumeAll() override;
    void HGE_CALL Channel_StopAll() override;
    bool HGE_CALL Channel_IsPlaying(HCHANNEL chn) override;
    float HGE_CALL Channel_GetLength(HCHANNEL chn) override;
    float HGE_CALL Channel_GetPos(HCHANNEL chn) override;
    void HGE_CALL Channel_SetPos(HCHANNEL chn, float fSeconds) override;
    void HGE_CALL Channel_SlideTo(HCHANNEL channel, float time, int volume,
                                          int pan = -101, float pitch = -1) override;
    bool HGE_CALL Channel_IsSliding(HCHANNEL channel) override;

    void HGE_CALL Input_GetMousePos(float* x, float* y) override;
    void HGE_CALL Input_SetMousePos(float x, float y) override;
    int HGE_CALL Input_GetMouseWheel() override;
    bool HGE_CALL Input_IsMouseOver() override;
    bool HGE_CALL Input_KeyDown(int key) override;
    bool HGE_CALL Input_KeyUp(int key) override;
    bool HGE_CALL Input_GetKeyState(int key) override;
    char* HGE_CALL Input_GetKeyName(int key) override;
    int HGE_CALL Input_GetKey() override;
    int HGE_CALL Input_GetChar() override;
    bool HGE_CALL Input_GetEvent(hgeInputEvent* event) override;

    bool HGE_CALL Gfx_BeginScene(HTARGET target = 0) override;
    void HGE_CALL Gfx_EndScene() override;
    void HGE_CALL Gfx_Clear(DWORD color) override;
    void HGE_CALL Gfx_RenderLine(float x1, float y1, float x2, float y2,
                                         DWORD color = 0xFFFFFFFF, float z = 0.5f) override;
    void HGE_CALL Gfx_RenderTriple(const hgeTriple* triple) override;
    void HGE_CALL Gfx_RenderQuad(const hgeQuad* quad, bool filled = true) override;
    void HGE_CALL Gfx_RenderBumpedQuad(const hgeBumpQuad* quad, int, int) override;
    hgeVertex* HGE_CALL Gfx_StartBatch(int prim_type, HTEXTURE tex, int blend,
                                               int* max_prim) override;
    void HGE_CALL Gfx_FinishBatch(int nprim) override;
    void HGE_CALL Gfx_GetClipping(int* x, int* y, int* w, int* h) override;
    void HGE_CALL Gfx_SetClipping(int x = 0, int y = 0, int w = 0, int h = 0) override;
    void HGE_CALL Gfx_SetTransform(float x = 0, float y = 0, float dx = 0, float dy = 0,
                                           float rot = 0, float hscale = 0, float vscale = 0) override;
    IDirect3DDevice9* HGE_CALL Gfx_GetDevice() override;
    void HGE_CALL Gfx_FlushBuffer() override;

    HSURFACE HGE_CALL Target_GetSurface(HTARGET) override;
    void HGE_CALL Surface_Free(HSURFACE) override;
    DWORD* HGE_CALL Surface_Lock(HSURFACE, bool, int, int, int, int) override;
    void HGE_CALL Surface_Unlock(HSURFACE) override;

    HSHADER     HGE_CALL    Shader_Create(const char * filename, DWORD size) override;
    HSHTECH     HGE_CALL    Shader_GetTechnique(HSHADER shad, const char * name) override;
    void        HGE_CALL    Shader_SetTechnique(HSHADER shad, HSHTECH tech) override;
    int         HGE_CALL    Shader_Begin(HSHADER shad, int flags) override;
    void        HGE_CALL    Shader_End(HSHADER shad) override;
    void        HGE_CALL    Shader_BeginPass(HSHADER shad, int pass) override;
    void        HGE_CALL    Shader_EndPass(HSHADER shad) override;
    void        HGE_CALL    Shader_CommitChanges(HSHADER shad) override;

    HSHPARAM    HGE_CALL    Shader_GetParam(HSHADER shad, const char * name) override;
    void        HGE_CALL    Shader_SetValue(HSHADER shad, HSHPARAM param, void * data, int length) override;
    void        HGE_CALL    Shader_GetValue(HSHADER shad, HSHPARAM param, void * data, int length) override;
    void        HGE_CALL    Shader_SetBool(HSHADER shad, HSHPARAM param, bool b) override;
    bool        HGE_CALL    Shader_GetBool(HSHADER shad, HSHPARAM param) override;
    void        HGE_CALL    Shader_SetFloat(HSHADER shad, HSHPARAM param, float f) override;
    float       HGE_CALL    Shader_GetFloat(HSHADER shad, HSHPARAM param) override;
    void        HGE_CALL    Shader_SetInt(HSHADER shad, HSHPARAM param, int f) override;
    int         HGE_CALL    Shader_GetInt(HSHADER shad, HSHPARAM param) override;
    void        HGE_CALL    Shader_SetTexture(HSHADER shad, HSHPARAM param, HTEXTURE tex) override;
    HTEXTURE    HGE_CALL    Shader_GetTexture(HSHADER shad, HSHPARAM param) override;
    void        HGE_CALL    Shader_SetVector(HSHADER shad, HSHPARAM param, float x, float y, float z, float w) override;
    void        HGE_CALL    Shader_GetVector(HSHADER shad, HSHPARAM param, float * x, float * y, float * z, float * w) override;

    HTARGET HGE_CALL Target_Create(int width, int height, bool zbuffer) override;
    void HGE_CALL Target_Free(HTARGET target) override;
    HTEXTURE HGE_CALL Target_GetTexture(HTARGET target) override;

    HTEXTURE HGE_CALL Texture_Create(int width, int height) override;
    HTEXTURE HGE_CALL Texture_Load(const char* filename, DWORD size = 0,
                                           bool bMipmap = false) override;
    void HGE_CALL Texture_Free(HTEXTURE tex) override;
    int HGE_CALL Texture_GetWidth(HTEXTURE tex, bool bOriginal = false) override;
    int HGE_CALL Texture_GetHeight(HTEXTURE tex, bool bOriginal = false) override;
    DWORD* HGE_CALL Texture_Lock(HTEXTURE tex, bool bReadOnly = true, int left = 0,
                                          int top = 0, int width = 0, int height = 0) override;
    void HGE_CALL Texture_Unlock(HTEXTURE tex) override;

    std::vector<std::string>& HGE_CALL System_GetDraggedFiles() override;

    //////// Implementation ////////

    static HGE_Impl* interface_get();
    void focus_change(bool bAct);
    void post_error(char* error);


    HINSTANCE h_instance_;
    HWND hwnd_;

    bool active_;
    char error_[256];
    char app_path_[_MAX_PATH];
    char ini_string_[256];


    // System States
    bool (*proc_frame_func_)();
    bool (*proc_render_func_)();
    bool (*proc_focus_lost_func_)();
    bool (*proc_focus_gain_func_)();
    bool (*proc_gfx_restore_func_)();
    bool (*proc_exit_func_)();
    bool (*proc_file_moved_in_func_)();
    bool (*proc_file_moved_out_func_)();
    bool (*proc_file_dropped_func_)();
    bool (*proc_resized_func_)();
    bool (*proc_fullscreen_toggle_func_)();

    const char* icon_;
    char win_title_[256];
    int screen_width_;
    int screen_height_;
    int screen_bpp_;
    bool windowed_;
    bool z_buffer_;
    bool texture_filter_;
    char ini_file_[_MAX_PATH];
    char log_file_[_MAX_PATH];
    bool use_sound_;
    int sample_rate_;
    int fx_volume_;
    int mus_volume_;
    int stream_volume_;
    int hgefps_;
    bool hide_mouse_;
    bool dont_suspend_;
    bool window_caption_;
    bool window_accept_files_;
    HWND hwnd_parent_;
    int border_width_;

#ifdef HGE_SPLASH_ENABLE
    bool splash_screen_enabled_;
#endif


    // Power
    int power_status_;
    HMODULE krnl32_;
    GetSystemPowerStatusFunc get_system_power_status_;

    void init_power_status();
    void update_power_status();
    void done_power_status() const;


    // Graphics
    D3DPRESENT_PARAMETERS* d3dpp_;

    D3DPRESENT_PARAMETERS d3dpp_windowed_;
    RECT rect_windowed_;
    LONG style_windowed_;

    D3DPRESENT_PARAMETERS d3dpp_fullscreen_;
    RECT rect_fullscreen_;
    LONG style_fullscreen_;

    hgeGAPI* d3d_;
    hgeGAPIDevice* d3d_device_;
    hgeGAPIVertexBuffer* vertex_buf_;
    hgeGAPIIndexBuffer* index_buf_;

    hgeGAPISurface* screen_surf_;
    hgeGAPISurface* screen_depth_;
    CRenderTargetList* targets_;
    CRenderTargetList* cur_target_;

    D3DXMATRIX view_matrix_;
    D3DXMATRIX proj_matrix_;

    CTextureList* textures_;
    hgeVertex* vert_array_;

    int n_prim_;
    int cur_prim_type_;
    int cur_blend_mode_;
    HTEXTURE cur_texture_;
#if HGE_DIRECTX_VER >= 9
    HSHADER cur_shader_;
#endif

    bool gfx_init();
    void gfx_done();
    bool gfx_restore();
    void adjust_window();
    void resize(int width, int height, bool generateEvent = true);
    bool init_lost();
    void render_batch(bool bEndScene = false);
    static int format_id(D3DFORMAT fmt);
    void set_blend_mode(int blend);
    void set_projection_matrix(int width, int height);


    // Audio
    HINSTANCE hBass;
    bool is_silent_;
    CStreamList* sound_streams_;

    bool sound_init();
    void sound_done();
    void set_mus_volume(int vol) const;
    void set_stream_volume(int vol) const;
    void set_fx_volume(int vol) const;


    // Input
    int v_key_;
    int input_char_;
    int zpos_;
    float xpos_;
    float ypos_;
    bool mouse_over_;
    bool is_captured_;
    char key_table_[256];
    CInputEventList* ev_queue_;

    void update_mouse();
    void input_init();
    void clear_queue();
    void build_event(int type, int key, int scan, int flags, int x, int y);


    // Resources
    char tmp_filename_[_MAX_PATH];
    CResourceList* res_list_;
    HANDLE h_search_;
    WIN32_FIND_DATA search_data_;


    // Timer
    float time_;
    float delta_time_;
    uint32_t fixed_delta_;
    int fps_;
    uint32_t t0_;
    uint32_t t0_fps_;
    uint32_t dt_;
    int cfps_;

    FileDropListener* listener = nullptr;
private:
    HGE_Impl();
};

extern HGE_Impl* pHGE;
