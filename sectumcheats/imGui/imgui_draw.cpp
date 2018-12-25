// dear imgui, v1.50 WIP
// (drawing and font code)

// Contains implementation for
// - ImDrawList
// - ImDrawData
// - ImFontAtlas
// - ImFont
// - Default font data

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_PLACEMENT_NEW
#include "imgui_internal.h"

#include <stdio.h>      // vsnprintf, sscanf, printf
#if !defined(alloca)
#ifdef _WIN32
#include <malloc.h>     // alloca
#elif (defined(__FreeBSD__) || defined(FreeBSD_kernel) || defined(__DragonFly__)) && !defined(__GLIBC__)
#include <stdlib.h>     // alloca. FreeBSD uses stdlib.h unless GLIBC
#else
#include <alloca.h>     // alloca
#endif
#endif

#ifdef _MSC_VER
#pragma warning (disable: 4505) // unreferenced local function has been removed (stb stuff)
#pragma warning (disable: 4996) // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#define snprintf _snprintf
#endif

#ifdef __clang__
#pragma clang diagnostic ignored "-Wold-style-cast"         // warning : use of old-style cast                              // yes, they are more terse.
#pragma clang diagnostic ignored "-Wfloat-equal"            // warning : comparing floating point with == or != is unsafe   // storing and comparing against same constants ok.
#pragma clang diagnostic ignored "-Wglobal-constructors"    // warning : declaration requires a global destructor           // similar to above, not sure what the exact difference it.
#pragma clang diagnostic ignored "-Wsign-conversion"        // warning : implicit conversion changes signedness             //
#if __has_warning("-Wreserved-id-macro")
#pragma clang diagnostic ignored "-Wreserved-id-macro"      // warning : macro name is a reserved identifier                //
#endif
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-function"          // warning: 'xxxx' defined but not used
#pragma GCC diagnostic ignored "-Wdouble-promotion"         // warning: implicit conversion from 'float' to 'double' when passing argument to function
#pragma GCC diagnostic ignored "-Wconversion"               // warning: conversion to 'xxxx' from 'xxxx' may alter its value
#pragma GCC diagnostic ignored "-Wcast-qual"                // warning: cast from type 'xxxx' to type 'xxxx' casts away qualifiers
#endif

//-------------------------------------------------------------------------
// STB libraries implementation
//-------------------------------------------------------------------------

//#define IMGUI_STB_NAMESPACE     ImGuiStb
//#define IMGUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION
//#define IMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION

#ifdef IMGUI_STB_NAMESPACE
namespace IMGUI_STB_NAMESPACE
{
#endif

#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable: 4456)                             // declaration of 'xx' hides previous local declaration
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"         // warning : use of old-style cast                              // yes, they are more terse.
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wmissing-prototypes"
#endif

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"              // warning: comparison is always true due to limited range of data type [-Wtype-limits]
#endif

#define STBRP_ASSERT(x)    IM_ASSERT(x)
#ifndef IMGUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION
#define STBRP_STATIC
#define STB_RECT_PACK_IMPLEMENTATION
#endif
#include "stb_rect_pack.h"

#define STBTT_malloc(x,u)  ((void)(u), ImGui::MemAlloc(x))
#define STBTT_free(x,u)    ((void)(u), ImGui::MemFree(x))
#define STBTT_assert(x)    IM_ASSERT(x)
#ifndef IMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#else
#define STBTT_DEF extern
#endif
#include "stb_truetype.h"

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#ifdef _MSC_VER
#pragma warning (pop)
#endif

#ifdef IMGUI_STB_NAMESPACE
} // namespace ImGuiStb
using namespace IMGUI_STB_NAMESPACE;
#endif

//-----------------------------------------------------------------------------
// ImDrawList
//-----------------------------------------------------------------------------

static const ImVec4 GNullClipRect(-8192.0f, -8192.0f, +8192.0f, +8192.0f); // Large values that are easy to encode in a few bits+shift

void ImDrawList::Clear()
{
    CmdBuffer.resize(0);
    IdxBuffer.resize(0);
    VtxBuffer.resize(0);
    _VtxCurrentIdx = 0;
    _VtxWritePtr = NULL;
    _IdxWritePtr = NULL;
    _ClipRectStack.resize(0);
    _TextureIdStack.resize(0);
    _Path.resize(0);
    _ChannelsCurrent = 0;
    _ChannelsCount = 1;
    // NB: Do not clear channels so our allocations are re-used after the first frame.
}

void ImDrawList::ClearFreeMemory()
{
    CmdBuffer.clear();
    IdxBuffer.clear();
    VtxBuffer.clear();
    _VtxCurrentIdx = 0;
    _VtxWritePtr = NULL;
    _IdxWritePtr = NULL;
    _ClipRectStack.clear();
    _TextureIdStack.clear();
    _Path.clear();
    _ChannelsCurrent = 0;
    _ChannelsCount = 1;
    for (int i = 0; i < _Channels.Size; i++)
    {
        if (i == 0) memset(&_Channels[0], 0, sizeof(_Channels[0]));  // channel 0 is a copy of CmdBuffer/IdxBuffer, don't destruct again
        _Channels[i].CmdBuffer.clear();
        _Channels[i].IdxBuffer.clear();
    }
    _Channels.clear();
}

// Use macros because C++ is a terrible language, we want guaranteed inline, no code in header, and no overhead in Debug mode
#define GetCurrentClipRect()    (_ClipRectStack.Size ? _ClipRectStack.Data[_ClipRectStack.Size-1]  : GNullClipRect)
#define GetCurrentTextureId()   (_TextureIdStack.Size ? _TextureIdStack.Data[_TextureIdStack.Size-1] : NULL)

void ImDrawList::AddDrawCmd()
{
    ImDrawCmd draw_cmd;
    draw_cmd.ClipRect = GetCurrentClipRect();
    draw_cmd.TextureId = GetCurrentTextureId();

    IM_ASSERT(draw_cmd.ClipRect.x <= draw_cmd.ClipRect.z && draw_cmd.ClipRect.y <= draw_cmd.ClipRect.w);
    CmdBuffer.push_back(draw_cmd);
}

void ImDrawList::AddCallback(ImDrawCallback callback, void* callback_data)
{
    ImDrawCmd* current_cmd = CmdBuffer.Size ? &CmdBuffer.back() : NULL;
    if (!current_cmd || current_cmd->ElemCount != 0 || current_cmd->UserCallback != NULL)
    {
        AddDrawCmd();
        current_cmd = &CmdBuffer.back();
    }
    current_cmd->UserCallback = callback;
    current_cmd->UserCallbackData = callback_data;

    AddDrawCmd(); // Force a new command after us (see comment below)
}

// Our scheme may appears a bit unusual, basically we want the most-common calls AddLine AddRect etc. to not have to perform any check so we always have a command ready in the stack.
// The cost of figuring out if a new command has to be added or if we can merge is paid in those Update** functions only.
void ImDrawList::UpdateClipRect()
{
    // If current command is used with different settings we need to add a new command
    const ImVec4 curr_clip_rect = GetCurrentClipRect();
    ImDrawCmd* curr_cmd = CmdBuffer.Size > 0 ? &CmdBuffer.Data[CmdBuffer.Size-1] : NULL;
    if (!curr_cmd || (curr_cmd->ElemCount != 0 && memcmp(&curr_cmd->ClipRect, &curr_clip_rect, sizeof(ImVec4)) != 0) || curr_cmd->UserCallback != NULL)
    {
        AddDrawCmd();
        return;
    }

    // Try to merge with previous command if it matches, else use current command
    ImDrawCmd* prev_cmd = CmdBuffer.Size > 1 ? curr_cmd - 1 : NULL;
    if (curr_cmd->ElemCount == 0 && prev_cmd && memcmp(&prev_cmd->ClipRect, &curr_clip_rect, sizeof(ImVec4)) == 0 && prev_cmd->TextureId == GetCurrentTextureId() && prev_cmd->UserCallback == NULL)
        CmdBuffer.pop_back();
    else
        curr_cmd->ClipRect = curr_clip_rect;
}

void ImDrawList::UpdateTextureID()
{
    // If current command is used with different settings we need to add a new command
    const ImTextureID curr_texture_id = GetCurrentTextureId();
    ImDrawCmd* curr_cmd = CmdBuffer.Size ? &CmdBuffer.back() : NULL;
    if (!curr_cmd || (curr_cmd->ElemCount != 0 && curr_cmd->TextureId != curr_texture_id) || curr_cmd->UserCallback != NULL)
    {
        AddDrawCmd();
        return;
    }

    // Try to merge with previous command if it matches, else use current command
    ImDrawCmd* prev_cmd = CmdBuffer.Size > 1 ? curr_cmd - 1 : NULL;
    if (prev_cmd && prev_cmd->TextureId == curr_texture_id && memcmp(&prev_cmd->ClipRect, &GetCurrentClipRect(), sizeof(ImVec4)) == 0 && prev_cmd->UserCallback == NULL)
        CmdBuffer.pop_back();
    else
        curr_cmd->TextureId = curr_texture_id;
}

#undef GetCurrentClipRect
#undef GetCurrentTextureId

// Render-level scissoring. This is passed down to your render function but not used for CPU-side coarse clipping. Prefer using higher-level ImGui::PushClipRect() to affect logic (hit-testing and widget culling)
void ImDrawList::PushClipRect(ImVec2 cr_min, ImVec2 cr_max, bool intersect_with_current_clip_rect)
{
    ImVec4 cr(cr_min.x, cr_min.y, cr_max.x, cr_max.y);
    if (intersect_with_current_clip_rect && _ClipRectStack.Size)
    {
        ImVec4 current = _ClipRectStack.Data[_ClipRectStack.Size-1];
        if (cr.x < current.x) cr.x = current.x;
        if (cr.y < current.y) cr.y = current.y;
        if (cr.z > current.z) cr.z = current.z;
        if (cr.w > current.w) cr.w = current.w;
    }
    cr.z = ImMax(cr.x, cr.z);
    cr.w = ImMax(cr.y, cr.w);

    _ClipRectStack.push_back(cr);
    UpdateClipRect();
}

void ImDrawList::PushClipRectFullScreen()
{
    PushClipRect(ImVec2(GNullClipRect.x, GNullClipRect.y), ImVec2(GNullClipRect.z, GNullClipRect.w));
    //PushClipRect(GetVisibleRect());   // FIXME-OPT: This would be more correct but we're not supposed to access ImGuiContext from here?
}

void ImDrawList::PopClipRect()
{
    IM_ASSERT(_ClipRectStack.Size > 0);
    _ClipRectStack.pop_back();
    UpdateClipRect();
}

void ImDrawList::PushTextureID(const ImTextureID& texture_id)
{
    _TextureIdStack.push_back(texture_id);
    UpdateTextureID();
}

void ImDrawList::PopTextureID()
{
    IM_ASSERT(_TextureIdStack.Size > 0);
    _TextureIdStack.pop_back();
    UpdateTextureID();
}

void ImDrawList::ChannelsSplit(int channels_count)
{
    IM_ASSERT(_ChannelsCurrent == 0 && _ChannelsCount == 1);
    int old_channels_count = _Channels.Size;
    if (old_channels_count < channels_count)
        _Channels.resize(channels_count);
    _ChannelsCount = channels_count;

    // _Channels[] (24 bytes each) hold storage that we'll swap with this->_CmdBuffer/_IdxBuffer
    // The content of _Channels[0] at this point doesn't matter. We clear it to make state tidy in a debugger but we don't strictly need to.
    // When we switch to the next channel, we'll copy _CmdBuffer/_IdxBuffer into _Channels[0] and then _Channels[1] into _CmdBuffer/_IdxBuffer
    memset(&_Channels[0], 0, sizeof(ImDrawChannel));
    for (int i = 1; i < channels_count; i++)
    {
        if (i >= old_channels_count)
        {
            IM_PLACEMENT_NEW(&_Channels[i]) ImDrawChannel();
        }
        else
        {
            _Channels[i].CmdBuffer.resize(0);
            _Channels[i].IdxBuffer.resize(0);
        }
        if (_Channels[i].CmdBuffer.Size == 0)
        {
            ImDrawCmd draw_cmd;
            draw_cmd.ClipRect = _ClipRectStack.back();
            draw_cmd.TextureId = _TextureIdStack.back();
            _Channels[i].CmdBuffer.push_back(draw_cmd);
        }
    }
}

void ImDrawList::ChannelsMerge()
{
    // Note that we never use or rely on channels.Size because it is merely a buffer that we never shrink back to 0 to keep all sub-buffers ready for use.
    if (_ChannelsCount <= 1)
        return;

    ChannelsSetCurrent(0);
    if (CmdBuffer.Size && CmdBuffer.back().ElemCount == 0)
        CmdBuffer.pop_back();

    int new_cmd_buffer_count = 0, new_idx_buffer_count = 0;
    for (int i = 1; i < _ChannelsCount; i++)
    {
        ImDrawChannel& ch = _Channels[i];
        if (ch.CmdBuffer.Size && ch.CmdBuffer.back().ElemCount == 0)
            ch.CmdBuffer.pop_back();
        new_cmd_buffer_count += ch.CmdBuffer.Size;
        new_idx_buffer_count += ch.IdxBuffer.Size;
    }
    CmdBuffer.resize(CmdBuffer.Size + new_cmd_buffer_count);
    IdxBuffer.resize(IdxBuffer.Size + new_idx_buffer_count);

    ImDrawCmd* cmd_write = CmdBuffer.Data + CmdBuffer.Size - new_cmd_buffer_count;
    _IdxWritePtr = IdxBuffer.Data + IdxBuffer.Size - new_idx_buffer_count;
    for (int i = 1; i < _ChannelsCount; i++)
    {
        ImDrawChannel& ch = _Channels[i];
        if (int sz = ch.CmdBuffer.Size) { memcpy(cmd_write, ch.CmdBuffer.Data, sz * sizeof(ImDrawCmd)); cmd_write += sz; }
        if (int sz = ch.IdxBuffer.Size) { memcpy(_IdxWritePtr, ch.IdxBuffer.Data, sz * sizeof(ImDrawIdx)); _IdxWritePtr += sz; }
    }
    AddDrawCmd();
    _ChannelsCount = 1;
}

void ImDrawList::ChannelsSetCurrent(int idx)
{
    IM_ASSERT(idx < _ChannelsCount);
    if (_ChannelsCurrent == idx) return;
    memcpy(&_Channels.Data[_ChannelsCurrent].CmdBuffer, &CmdBuffer, sizeof(CmdBuffer)); // copy 12 bytes, four times
    memcpy(&_Channels.Data[_ChannelsCurrent].IdxBuffer, &IdxBuffer, sizeof(IdxBuffer));
    _ChannelsCurrent = idx;
    memcpy(&CmdBuffer, &_Channels.Data[_ChannelsCurrent].CmdBuffer, sizeof(CmdBuffer));
    memcpy(&IdxBuffer, &_Channels.Data[_ChannelsCurrent].IdxBuffer, sizeof(IdxBuffer));
    _IdxWritePtr = IdxBuffer.Data + IdxBuffer.Size;
}

// NB: this can be called with negative count for removing primitives (as long as the result does not underflow)
void ImDrawList::PrimReserve(int idx_count, int vtx_count)
{
    ImDrawCmd& draw_cmd = CmdBuffer.Data[CmdBuffer.Size-1];
    draw_cmd.ElemCount += idx_count;

    int vtx_buffer_size = VtxBuffer.Size;
    VtxBuffer.resize(vtx_buffer_size + vtx_count);
    _VtxWritePtr = VtxBuffer.Data + vtx_buffer_size;

    int idx_buffer_size = IdxBuffer.Size;
    IdxBuffer.resize(idx_buffer_size + idx_count);
    _IdxWritePtr = IdxBuffer.Data + idx_buffer_size;
}

// Fully unrolled with inline call to keep our debug builds decently fast.
void ImDrawList::PrimRect(const ImVec2& a, const ImVec2& c, ImU32 col)
{
    ImVec2 b(c.x, a.y), d(a.x, c.y), uv(GImGui->FontTexUvWhitePixel);
    ImDrawIdx idx = (ImDrawIdx)_VtxCurrentIdx;
    _IdxWritePtr[0] = idx; _IdxWritePtr[1] = (ImDrawIdx)(idx+1); _IdxWritePtr[2] = (ImDrawIdx)(idx+2);
    _IdxWritePtr[3] = idx; _IdxWritePtr[4] = (ImDrawIdx)(idx+2); _IdxWritePtr[5] = (ImDrawIdx)(idx+3);
    _VtxWritePtr[0].pos = a; _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;
    _VtxWritePtr[1].pos = b; _VtxWritePtr[1].uv = uv; _VtxWritePtr[1].col = col;
    _VtxWritePtr[2].pos = c; _VtxWritePtr[2].uv = uv; _VtxWritePtr[2].col = col;
    _VtxWritePtr[3].pos = d; _VtxWritePtr[3].uv = uv; _VtxWritePtr[3].col = col;
    _VtxWritePtr += 4;
    _VtxCurrentIdx += 4;
    _IdxWritePtr += 6;
}

void ImDrawList::PrimRectUV(const ImVec2& a, const ImVec2& c, const ImVec2& uv_a, const ImVec2& uv_c, ImU32 col)
{
    ImVec2 b(c.x, a.y), d(a.x, c.y), uv_b(uv_c.x, uv_a.y), uv_d(uv_a.x, uv_c.y);
    ImDrawIdx idx = (ImDrawIdx)_VtxCurrentIdx;
    _IdxWritePtr[0] = idx; _IdxWritePtr[1] = (ImDrawIdx)(idx+1); _IdxWritePtr[2] = (ImDrawIdx)(idx+2);
    _IdxWritePtr[3] = idx; _IdxWritePtr[4] = (ImDrawIdx)(idx+2); _IdxWritePtr[5] = (ImDrawIdx)(idx+3);
    _VtxWritePtr[0].pos = a; _VtxWritePtr[0].uv = uv_a; _VtxWritePtr[0].col = col;
    _VtxWritePtr[1].pos = b; _VtxWritePtr[1].uv = uv_b; _VtxWritePtr[1].col = col;
    _VtxWritePtr[2].pos = c; _VtxWritePtr[2].uv = uv_c; _VtxWritePtr[2].col = col;
    _VtxWritePtr[3].pos = d; _VtxWritePtr[3].uv = uv_d; _VtxWritePtr[3].col = col;
    _VtxWritePtr += 4;
    _VtxCurrentIdx += 4;
    _IdxWritePtr += 6;
}

void ImDrawList::PrimQuadUV(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& d, const ImVec2& uv_a, const ImVec2& uv_b, const ImVec2& uv_c, const ImVec2& uv_d, ImU32 col)
{
    ImDrawIdx idx = (ImDrawIdx)_VtxCurrentIdx;
    _IdxWritePtr[0] = idx; _IdxWritePtr[1] = (ImDrawIdx)(idx+1); _IdxWritePtr[2] = (ImDrawIdx)(idx+2);
    _IdxWritePtr[3] = idx; _IdxWritePtr[4] = (ImDrawIdx)(idx+2); _IdxWritePtr[5] = (ImDrawIdx)(idx+3);
    _VtxWritePtr[0].pos = a; _VtxWritePtr[0].uv = uv_a; _VtxWritePtr[0].col = col;
    _VtxWritePtr[1].pos = b; _VtxWritePtr[1].uv = uv_b; _VtxWritePtr[1].col = col;
    _VtxWritePtr[2].pos = c; _VtxWritePtr[2].uv = uv_c; _VtxWritePtr[2].col = col;
    _VtxWritePtr[3].pos = d; _VtxWritePtr[3].uv = uv_d; _VtxWritePtr[3].col = col;
    _VtxWritePtr += 4;
    _VtxCurrentIdx += 4;
    _IdxWritePtr += 6;
}

// TODO: Thickness anti-aliased lines cap are missing their AA fringe.
void ImDrawList::AddPolyline(const ImVec2* points, const int points_count, ImU32 col, bool closed, float thickness, bool anti_aliased)
{
    if (points_count < 2)
        return;

    const ImVec2 uv = GImGui->FontTexUvWhitePixel;
    anti_aliased &= GImGui->Style.AntiAliasedLines;
    //if (ImGui::GetIO().KeyCtrl) anti_aliased = false; // Debug

    int count = points_count;
    if (!closed)
        count = points_count-1;

    const bool thick_line = thickness > 1.0f;
    if (anti_aliased)
    {
        // Anti-aliased stroke
        const float AA_SIZE = 1.0f;
        const ImU32 col_trans = col & IM_COL32(255,255,255,0);

        const int idx_count = thick_line ? count*18 : count*12;
        const int vtx_count = thick_line ? points_count*4 : points_count*3;
        PrimReserve(idx_count, vtx_count);

        // Temporary buffer
        ImVec2* temp_normals = (ImVec2*)alloca(points_count * (thick_line ? 5 : 3) * sizeof(ImVec2));
        ImVec2* temp_points = temp_normals + points_count;

        for (int i1 = 0; i1 < count; i1++)
        {
            const int i2 = (i1+1) == points_count ? 0 : i1+1;
            ImVec2 diff = points[i2] - points[i1];
            diff *= ImInvLength(diff, 1.0f);
            temp_normals[i1].x = diff.y;
            temp_normals[i1].y = -diff.x;
        }
        if (!closed)
            temp_normals[points_count-1] = temp_normals[points_count-2];

        if (!thick_line)
        {
            if (!closed)
            {
                temp_points[0] = points[0] + temp_normals[0] * AA_SIZE;
                temp_points[1] = points[0] - temp_normals[0] * AA_SIZE;
                temp_points[(points_count-1)*2+0] = points[points_count-1] + temp_normals[points_count-1] * AA_SIZE;
                temp_points[(points_count-1)*2+1] = points[points_count-1] - temp_normals[points_count-1] * AA_SIZE;
            }

            // FIXME-OPT: Merge the different loops, possibly remove the temporary buffer.
            unsigned int idx1 = _VtxCurrentIdx;
            for (int i1 = 0; i1 < count; i1++)
            {
                const int i2 = (i1+1) == points_count ? 0 : i1+1;
                unsigned int idx2 = (i1+1) == points_count ? _VtxCurrentIdx : idx1+3;

                // Average normals
                ImVec2 dm = (temp_normals[i1] + temp_normals[i2]) * 0.5f;
                float dmr2 = dm.x*dm.x + dm.y*dm.y;
                if (dmr2 > 0.000001f)
                {
                    float scale = 1.0f / dmr2;
                    if (scale > 100.0f) scale = 100.0f;
                    dm *= scale;
                }
                dm *= AA_SIZE;
                temp_points[i2*2+0] = points[i2] + dm;
                temp_points[i2*2+1] = points[i2] - dm;

                // Add indexes
                _IdxWritePtr[0] = (ImDrawIdx)(idx2+0); _IdxWritePtr[1] = (ImDrawIdx)(idx1+0); _IdxWritePtr[2] = (ImDrawIdx)(idx1+2);
                _IdxWritePtr[3] = (ImDrawIdx)(idx1+2); _IdxWritePtr[4] = (ImDrawIdx)(idx2+2); _IdxWritePtr[5] = (ImDrawIdx)(idx2+0);
                _IdxWritePtr[6] = (ImDrawIdx)(idx2+1); _IdxWritePtr[7] = (ImDrawIdx)(idx1+1); _IdxWritePtr[8] = (ImDrawIdx)(idx1+0);
                _IdxWritePtr[9] = (ImDrawIdx)(idx1+0); _IdxWritePtr[10]= (ImDrawIdx)(idx2+0); _IdxWritePtr[11]= (ImDrawIdx)(idx2+1);
                _IdxWritePtr += 12;

                idx1 = idx2;
            }

            // Add vertexes
            for (int i = 0; i < points_count; i++)
            {
                _VtxWritePtr[0].pos = points[i];          _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;
                _VtxWritePtr[1].pos = temp_points[i*2+0]; _VtxWritePtr[1].uv = uv; _VtxWritePtr[1].col = col_trans;
                _VtxWritePtr[2].pos = temp_points[i*2+1]; _VtxWritePtr[2].uv = uv; _VtxWritePtr[2].col = col_trans;
                _VtxWritePtr += 3;
            }
        }
        else
        {
            const float half_inner_thickness = (thickness - AA_SIZE) * 0.5f;
            if (!closed)
            {
                temp_points[0] = points[0] + temp_normals[0] * (half_inner_thickness + AA_SIZE);
                temp_points[1] = points[0] + temp_normals[0] * (half_inner_thickness);
                temp_points[2] = points[0] - temp_normals[0] * (half_inner_thickness);
                temp_points[3] = points[0] - temp_normals[0] * (half_inner_thickness + AA_SIZE);
                temp_points[(points_count-1)*4+0] = points[points_count-1] + temp_normals[points_count-1] * (half_inner_thickness + AA_SIZE);
                temp_points[(points_count-1)*4+1] = points[points_count-1] + temp_normals[points_count-1] * (half_inner_thickness);
                temp_points[(points_count-1)*4+2] = points[points_count-1] - temp_normals[points_count-1] * (half_inner_thickness);
                temp_points[(points_count-1)*4+3] = points[points_count-1] - temp_normals[points_count-1] * (half_inner_thickness + AA_SIZE);
            }

            // FIXME-OPT: Merge the different loops, possibly remove the temporary buffer.
            unsigned int idx1 = _VtxCurrentIdx;
            for (int i1 = 0; i1 < count; i1++)
            {
                const int i2 = (i1+1) == points_count ? 0 : i1+1;
                unsigned int idx2 = (i1+1) == points_count ? _VtxCurrentIdx : idx1+4;

                // Average normals
                ImVec2 dm = (temp_normals[i1] + temp_normals[i2]) * 0.5f;
                float dmr2 = dm.x*dm.x + dm.y*dm.y;
                if (dmr2 > 0.000001f)
                {
                    float scale = 1.0f / dmr2;
                    if (scale > 100.0f) scale = 100.0f;
                    dm *= scale;
                }
                ImVec2 dm_out = dm * (half_inner_thickness + AA_SIZE);
                ImVec2 dm_in = dm * half_inner_thickness;
                temp_points[i2*4+0] = points[i2] + dm_out;
                temp_points[i2*4+1] = points[i2] + dm_in;
                temp_points[i2*4+2] = points[i2] - dm_in;
                temp_points[i2*4+3] = points[i2] - dm_out;

                // Add indexes
                _IdxWritePtr[0]  = (ImDrawIdx)(idx2+1); _IdxWritePtr[1]  = (ImDrawIdx)(idx1+1); _IdxWritePtr[2]  = (ImDrawIdx)(idx1+2);
                _IdxWritePtr[3]  = (ImDrawIdx)(idx1+2); _IdxWritePtr[4]  = (ImDrawIdx)(idx2+2); _IdxWritePtr[5]  = (ImDrawIdx)(idx2+1);
                _IdxWritePtr[6]  = (ImDrawIdx)(idx2+1); _IdxWritePtr[7]  = (ImDrawIdx)(idx1+1); _IdxWritePtr[8]  = (ImDrawIdx)(idx1+0);
                _IdxWritePtr[9]  = (ImDrawIdx)(idx1+0); _IdxWritePtr[10] = (ImDrawIdx)(idx2+0); _IdxWritePtr[11] = (ImDrawIdx)(idx2+1);
                _IdxWritePtr[12] = (ImDrawIdx)(idx2+2); _IdxWritePtr[13] = (ImDrawIdx)(idx1+2); _IdxWritePtr[14] = (ImDrawIdx)(idx1+3);
                _IdxWritePtr[15] = (ImDrawIdx)(idx1+3); _IdxWritePtr[16] = (ImDrawIdx)(idx2+3); _IdxWritePtr[17] = (ImDrawIdx)(idx2+2);
                _IdxWritePtr += 18;

                idx1 = idx2;
            }

            // Add vertexes
            for (int i = 0; i < points_count; i++)
            {
                _VtxWritePtr[0].pos = temp_points[i*4+0]; _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col_trans;
                _VtxWritePtr[1].pos = temp_points[i*4+1]; _VtxWritePtr[1].uv = uv; _VtxWritePtr[1].col = col;
                _VtxWritePtr[2].pos = temp_points[i*4+2]; _VtxWritePtr[2].uv = uv; _VtxWritePtr[2].col = col;
                _VtxWritePtr[3].pos = temp_points[i*4+3]; _VtxWritePtr[3].uv = uv; _VtxWritePtr[3].col = col_trans;
                _VtxWritePtr += 4;
            }
        }
        _VtxCurrentIdx += (ImDrawIdx)vtx_count;
    }
    else
    {
        // Non Anti-aliased Stroke
        const int idx_count = count*6;
        const int vtx_count = count*4;      // FIXME-OPT: Not sharing edges
        PrimReserve(idx_count, vtx_count);

        for (int i1 = 0; i1 < count; i1++)
        {
            const int i2 = (i1+1) == points_count ? 0 : i1+1;
            const ImVec2& p1 = points[i1];
            const ImVec2& p2 = points[i2];
            ImVec2 diff = p2 - p1;
            diff *= ImInvLength(diff, 1.0f);

            const float dx = diff.x * (thickness * 0.5f);
            const float dy = diff.y * (thickness * 0.5f);
            _VtxWritePtr[0].pos.x = p1.x + dy; _VtxWritePtr[0].pos.y = p1.y - dx; _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;
            _VtxWritePtr[1].pos.x = p2.x + dy; _VtxWritePtr[1].pos.y = p2.y - dx; _VtxWritePtr[1].uv = uv; _VtxWritePtr[1].col = col;
            _VtxWritePtr[2].pos.x = p2.x - dy; _VtxWritePtr[2].pos.y = p2.y + dx; _VtxWritePtr[2].uv = uv; _VtxWritePtr[2].col = col;
            _VtxWritePtr[3].pos.x = p1.x - dy; _VtxWritePtr[3].pos.y = p1.y + dx; _VtxWritePtr[3].uv = uv; _VtxWritePtr[3].col = col;
            _VtxWritePtr += 4;

            _IdxWritePtr[0] = (ImDrawIdx)(_VtxCurrentIdx); _IdxWritePtr[1] = (ImDrawIdx)(_VtxCurrentIdx+1); _IdxWritePtr[2] = (ImDrawIdx)(_VtxCurrentIdx+2);
            _IdxWritePtr[3] = (ImDrawIdx)(_VtxCurrentIdx); _IdxWritePtr[4] = (ImDrawIdx)(_VtxCurrentIdx+2); _IdxWritePtr[5] = (ImDrawIdx)(_VtxCurrentIdx+3);
            _IdxWritePtr += 6;
            _VtxCurrentIdx += 4;
        }
    }
}

void ImDrawList::AddConvexPolyFilled(const ImVec2* points, const int points_count, ImU32 col, bool anti_aliased)
{
    const ImVec2 uv = GImGui->FontTexUvWhitePixel;
    anti_aliased &= GImGui->Style.AntiAliasedShapes;
    //if (ImGui::GetIO().KeyCtrl) anti_aliased = false; // Debug

    if (anti_aliased)
    {
        // Anti-aliased Fill
        const float AA_SIZE = 1.0f;
        const ImU32 col_trans = col & IM_COL32(255,255,255,0);
        const int idx_count = (points_count-2)*3 + points_count*6;
        const int vtx_count = (points_count*2);
        PrimReserve(idx_count, vtx_count);

        // Add indexes for fill
        unsigned int vtx_inner_idx = _VtxCurrentIdx;
        unsigned int vtx_outer_idx = _VtxCurrentIdx+1;
        for (int i = 2; i < points_count; i++)
        {
            _IdxWritePtr[0] = (ImDrawIdx)(vtx_inner_idx); _IdxWritePtr[1] = (ImDrawIdx)(vtx_inner_idx+((i-1)<<1)); _IdxWritePtr[2] = (ImDrawIdx)(vtx_inner_idx+(i<<1));
            _IdxWritePtr += 3;
        }

        // Compute normals
        ImVec2* temp_normals = (ImVec2*)alloca(points_count * sizeof(ImVec2));
        for (int i0 = points_count-1, i1 = 0; i1 < points_count; i0 = i1++)
        {
            const ImVec2& p0 = points[i0];
            const ImVec2& p1 = points[i1];
            ImVec2 diff = p1 - p0;
            diff *= ImInvLength(diff, 1.0f);
            temp_normals[i0].x = diff.y;
            temp_normals[i0].y = -diff.x;
        }

        for (int i0 = points_count-1, i1 = 0; i1 < points_count; i0 = i1++)
        {
            // Average normals
            const ImVec2& n0 = temp_normals[i0];
            const ImVec2& n1 = temp_normals[i1];
            ImVec2 dm = (n0 + n1) * 0.5f;
            float dmr2 = dm.x*dm.x + dm.y*dm.y;
            if (dmr2 > 0.000001f)
            {
                float scale = 1.0f / dmr2;
                if (scale > 100.0f) scale = 100.0f;
                dm *= scale;
            }
            dm *= AA_SIZE * 0.5f;

            // Add vertices
            _VtxWritePtr[0].pos = (points[i1] - dm); _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;        // Inner
            _VtxWritePtr[1].pos = (points[i1] + dm); _VtxWritePtr[1].uv = uv; _VtxWritePtr[1].col = col_trans;  // Outer
            _VtxWritePtr += 2;

            // Add indexes for fringes
            _IdxWritePtr[0] = (ImDrawIdx)(vtx_inner_idx+(i1<<1)); _IdxWritePtr[1] = (ImDrawIdx)(vtx_inner_idx+(i0<<1)); _IdxWritePtr[2] = (ImDrawIdx)(vtx_outer_idx+(i0<<1));
            _IdxWritePtr[3] = (ImDrawIdx)(vtx_outer_idx+(i0<<1)); _IdxWritePtr[4] = (ImDrawIdx)(vtx_outer_idx+(i1<<1)); _IdxWritePtr[5] = (ImDrawIdx)(vtx_inner_idx+(i1<<1));
            _IdxWritePtr += 6;
        }
        _VtxCurrentIdx += (ImDrawIdx)vtx_count;
    }
    else
    {
        // Non Anti-aliased Fill
        const int idx_count = (points_count-2)*3;
        const int vtx_count = points_count;
        PrimReserve(idx_count, vtx_count);
        for (int i = 0; i < vtx_count; i++)
        {
            _VtxWritePtr[0].pos = points[i]; _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;
            _VtxWritePtr++;
        }
        for (int i = 2; i < points_count; i++)
        {
            _IdxWritePtr[0] = (ImDrawIdx)(_VtxCurrentIdx); _IdxWritePtr[1] = (ImDrawIdx)(_VtxCurrentIdx+i-1); _IdxWritePtr[2] = (ImDrawIdx)(_VtxCurrentIdx+i);
            _IdxWritePtr += 3;
        }
        _VtxCurrentIdx += (ImDrawIdx)vtx_count;
    }
}

void ImDrawList::PathArcToFast(const ImVec2& centre, float radius, int amin, int amax)
{
    static ImVec2 circle_vtx[12];
    static bool circle_vtx_builds = false;
    const int circle_vtx_count = IM_ARRAYSIZE(circle_vtx);
    if (!circle_vtx_builds)
    {
        for (int i = 0; i < circle_vtx_count; i++)
        {
            const float a = ((float)i / (float)circle_vtx_count) * 2*IM_PI;
            circle_vtx[i].x = cosf(a);
            circle_vtx[i].y = sinf(a);
        }
        circle_vtx_builds = true;
    }

    if (amin > amax) return;
    if (radius == 0.0f)
    {
        _Path.push_back(centre);
    }
    else
    {
        _Path.reserve(_Path.Size + (amax - amin + 1));
        for (int a = amin; a <= amax; a++)
        {
            const ImVec2& c = circle_vtx[a % circle_vtx_count];
            _Path.push_back(ImVec2(centre.x + c.x * radius, centre.y + c.y * radius));
        }
    }
}

void ImDrawList::PathArcTo(const ImVec2& centre, float radius, float amin, float amax, int num_segments)
{
    if (radius == 0.0f)
        _Path.push_back(centre);
    _Path.reserve(_Path.Size + (num_segments + 1));
    for (int i = 0; i <= num_segments; i++)
    {
        const float a = amin + ((float)i / (float)num_segments) * (amax - amin);
        _Path.push_back(ImVec2(centre.x + cosf(a) * radius, centre.y + sinf(a) * radius));
    }
}

static void PathBezierToCasteljau(ImVector<ImVec2>* path, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, float tess_tol, int level)
{
    float dx = x4 - x1;
    float dy = y4 - y1;
    float d2 = ((x2 - x4) * dy - (y2 - y4) * dx);
    float d3 = ((x3 - x4) * dy - (y3 - y4) * dx);
    d2 = (d2 >= 0) ? d2 : -d2;
    d3 = (d3 >= 0) ? d3 : -d3;
    if ((d2+d3) * (d2+d3) < tess_tol * (dx*dx + dy*dy))
    {
        path->push_back(ImVec2(x4, y4));
    }
    else if (level < 10)
    {
        float x12 = (x1+x2)*0.5f,       y12 = (y1+y2)*0.5f;
        float x23 = (x2+x3)*0.5f,       y23 = (y2+y3)*0.5f;
        float x34 = (x3+x4)*0.5f,       y34 = (y3+y4)*0.5f;
        float x123 = (x12+x23)*0.5f,    y123 = (y12+y23)*0.5f;
        float x234 = (x23+x34)*0.5f,    y234 = (y23+y34)*0.5f;
        float x1234 = (x123+x234)*0.5f, y1234 = (y123+y234)*0.5f;

        PathBezierToCasteljau(path, x1,y1,        x12,y12,    x123,y123,  x1234,y1234, tess_tol, level+1);
        PathBezierToCasteljau(path, x1234,y1234,  x234,y234,  x34,y34,    x4,y4,       tess_tol, level+1);
    }
}

void ImDrawList::PathBezierCurveTo(const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, int num_segments)
{
    ImVec2 p1 = _Path.back();
    if (num_segments == 0)
    {
        // Auto-tessellated
        PathBezierToCasteljau(&_Path, p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y, GImGui->Style.CurveTessellationTol, 0);
    }
    else
    {
        float t_step = 1.0f / (float)num_segments;
        for (int i_step = 1; i_step <= num_segments; i_step++)
        {
            float t = t_step * i_step;
            float u = 1.0f - t;
            float w1 = u*u*u;
            float w2 = 3*u*u*t;
            float w3 = 3*u*t*t;
            float w4 = t*t*t;
            _Path.push_back(ImVec2(w1*p1.x + w2*p2.x + w3*p3.x + w4*p4.x, w1*p1.y + w2*p2.y + w3*p3.y + w4*p4.y));
        }
    }
}

void ImDrawList::PathRect(const ImVec2& a, const ImVec2& b, float rounding, int rounding_corners)
{
    float r = rounding;
    r = ImMin(r, fabsf(b.x-a.x) * ( ((rounding_corners&(1|2))==(1|2)) || ((rounding_corners&(4|8))==(4|8)) ? 0.5f : 1.0f ) - 1.0f);
    r = ImMin(r, fabsf(b.y-a.y) * ( ((rounding_corners&(1|8))==(1|8)) || ((rounding_corners&(2|4))==(2|4)) ? 0.5f : 1.0f ) - 1.0f);

    if (r <= 0.0f || rounding_corners == 0)
    {
        PathLineTo(a);
        PathLineTo(ImVec2(b.x,a.y));
        PathLineTo(b);
        PathLineTo(ImVec2(a.x,b.y));
    }
    else
    {
        const float r0 = (rounding_corners & 1) ? r : 0.0f;
        const float r1 = (rounding_corners & 2) ? r : 0.0f;
        const float r2 = (rounding_corners & 4) ? r : 0.0f;
        const float r3 = (rounding_corners & 8) ? r : 0.0f;
        PathArcToFast(ImVec2(a.x+r0,a.y+r0), r0, 6, 9);
        PathArcToFast(ImVec2(b.x-r1,a.y+r1), r1, 9, 12);
        PathArcToFast(ImVec2(b.x-r2,b.y-r2), r2, 0, 3);
        PathArcToFast(ImVec2(a.x+r3,b.y-r3), r3, 3, 6);
    }
}

void ImDrawList::AddLine(const ImVec2& a, const ImVec2& b, ImU32 col, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;
    PathLineTo(a + ImVec2(0.5f,0.5f));
    PathLineTo(b + ImVec2(0.5f,0.5f));
    PathStroke(col, false, thickness);
}

// a: upper-left, b: lower-right. we don't render 1 px sized rectangles properly.
void ImDrawList::AddRect(const ImVec2& a, const ImVec2& b, ImU32 col, float rounding, int rounding_corners_flags, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;
    PathRect(a + ImVec2(0.5f,0.5f), b - ImVec2(0.5f,0.5f), rounding, rounding_corners_flags);
    PathStroke(col, true, thickness);
}

void ImDrawList::AddRectFilled(const ImVec2& a, const ImVec2& b, ImU32 col, float rounding, int rounding_corners_flags)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;
    if (rounding > 0.0f)
    {
        PathRect(a, b, rounding, rounding_corners_flags);
        PathFillConvex(col);
    }
    else
    {
        PrimReserve(6, 4);
        PrimRect(a, b, col);
    }
}

void ImDrawList::AddRectFilledMultiColor(const ImVec2& a, const ImVec2& c, ImU32 col_upr_left, ImU32 col_upr_right, ImU32 col_bot_right, ImU32 col_bot_left)
{
    if (((col_upr_left | col_upr_right | col_bot_right | col_bot_left) & IM_COL32_A_MASK) == 0)
        return;

    const ImVec2 uv = GImGui->FontTexUvWhitePixel;
    PrimReserve(6, 4);
    PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx)); PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx+1)); PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx+2));
    PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx)); PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx+2)); PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx+3));
    PrimWriteVtx(a, uv, col_upr_left);
    PrimWriteVtx(ImVec2(c.x, a.y), uv, col_upr_right);
    PrimWriteVtx(c, uv, col_bot_right);
    PrimWriteVtx(ImVec2(a.x, c.y), uv, col_bot_left);
}

void ImDrawList::AddQuad(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& d, ImU32 col, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(a);
    PathLineTo(b);
    PathLineTo(c);
    PathLineTo(d);
    PathStroke(col, true, thickness);
}

void ImDrawList::AddQuadFilled(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& d, ImU32 col)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(a);
    PathLineTo(b);
    PathLineTo(c);
    PathLineTo(d);
    PathFillConvex(col);
}

void ImDrawList::AddTriangle(const ImVec2& a, const ImVec2& b, const ImVec2& c, ImU32 col, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(a);
    PathLineTo(b);
    PathLineTo(c);
    PathStroke(col, true, thickness);
}

void ImDrawList::AddTriangleFilled(const ImVec2& a, const ImVec2& b, const ImVec2& c, ImU32 col)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(a);
    PathLineTo(b);
    PathLineTo(c);
    PathFillConvex(col);
}

void ImDrawList::AddCircle(const ImVec2& centre, float radius, ImU32 col, int num_segments, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    const float a_max = IM_PI*2.0f * ((float)num_segments - 1.0f) / (float)num_segments;
    PathArcTo(centre, radius-0.5f, 0.0f, a_max, num_segments);
    PathStroke(col, true, thickness);
}

void ImDrawList::AddCircleFilled(const ImVec2& centre, float radius, ImU32 col, int num_segments)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    const float a_max = IM_PI*2.0f * ((float)num_segments - 1.0f) / (float)num_segments;
    PathArcTo(centre, radius, 0.0f, a_max, num_segments);
    PathFillConvex(col);
}

void ImDrawList::AddBezierCurve(const ImVec2& pos0, const ImVec2& cp0, const ImVec2& cp1, const ImVec2& pos1, ImU32 col, float thickness, int num_segments)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(pos0);
    PathBezierCurveTo(cp0, cp1, pos1, num_segments);
    PathStroke(col, false, thickness);
}

void ImDrawList::AddText(const ImFont* font, float font_size, const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end, float wrap_width, const ImVec4* cpu_fine_clip_rect)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    if (text_end == NULL)
        text_end = text_begin + strlen(text_begin);
    if (text_begin == text_end)
        return;

    // Note: This is one of the few instance of breaking the encapsulation of ImDrawList, as we pull this from ImGui state, but it is just SO useful.
    // Might just move Font/FontSize to ImDrawList?
    if (font == NULL)
        font = GImGui->Font;
    if (font_size == 0.0f)
        font_size = GImGui->FontSize;

    IM_ASSERT(font->ContainerAtlas->TexID == _TextureIdStack.back());  // Use high-level ImGui::PushFont() or low-level ImDrawList::PushTextureId() to change font.

    ImVec4 clip_rect = _ClipRectStack.back();
    if (cpu_fine_clip_rect)
    {
        clip_rect.x = ImMax(clip_rect.x, cpu_fine_clip_rect->x);
        clip_rect.y = ImMax(clip_rect.y, cpu_fine_clip_rect->y);
        clip_rect.z = ImMin(clip_rect.z, cpu_fine_clip_rect->z);
        clip_rect.w = ImMin(clip_rect.w, cpu_fine_clip_rect->w);
    }
    font->RenderText(this, font_size, pos, col, clip_rect, text_begin, text_end, wrap_width, cpu_fine_clip_rect != NULL);
}

void ImDrawList::AddText(const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end)
{
    AddText(GImGui->Font, GImGui->FontSize, pos, col, text_begin, text_end);
}

void ImDrawList::AddImage(ImTextureID user_texture_id, const ImVec2& a, const ImVec2& b, const ImVec2& uv_a, const ImVec2& uv_b, ImU32 col)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    // FIXME-OPT: This is wasting draw calls.
    const bool push_texture_id = _TextureIdStack.empty() || user_texture_id != _TextureIdStack.back();
    if (push_texture_id)
        PushTextureID(user_texture_id);

    PrimReserve(6, 4);
    PrimRectUV(a, b, uv_a, uv_b, col);

    if (push_texture_id)
        PopTextureID();
}

void ImDrawList::AddImageQuad(ImTextureID user_texture_id, const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& d, const ImVec2& uv_a, const ImVec2& uv_b, const ImVec2& uv_c, const ImVec2& uv_d, ImU32 col)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    const bool push_texture_id = _TextureIdStack.empty() || user_texture_id != _TextureIdStack.back();
    if (push_texture_id)
        PushTextureID(user_texture_id);

    PrimReserve(6, 4);
    PrimQuadUV(a, b, c, d, uv_a, uv_b, uv_c, uv_d, col);

    if (push_texture_id)
        PopTextureID();
}

//-----------------------------------------------------------------------------
// ImDrawData
//-----------------------------------------------------------------------------

// For backward compatibility: convert all buffers from indexed to de-indexed, in case you cannot render indexed. Note: this is slow and most likely a waste of resources. Always prefer indexed rendering!
void ImDrawData::DeIndexAllBuffers()
{
    ImVector<ImDrawVert> new_vtx_buffer;
    TotalVtxCount = TotalIdxCount = 0;
    for (int i = 0; i < CmdListsCount; i++)
    {
        ImDrawList* cmd_list = CmdLists[i];
        if (cmd_list->IdxBuffer.empty())
            continue;
        new_vtx_buffer.resize(cmd_list->IdxBuffer.Size);
        for (int j = 0; j < cmd_list->IdxBuffer.Size; j++)
            new_vtx_buffer[j] = cmd_list->VtxBuffer[cmd_list->IdxBuffer[j]];
        cmd_list->VtxBuffer.swap(new_vtx_buffer);
        cmd_list->IdxBuffer.resize(0);
        TotalVtxCount += cmd_list->VtxBuffer.Size;
    }
}

// Helper to scale the ClipRect field of each ImDrawCmd. Use if your final output buffer is at a different scale than ImGui expects, or if there is a difference between your window resolution and framebuffer resolution.
void ImDrawData::ScaleClipRects(const ImVec2& scale)
{
    for (int i = 0; i < CmdListsCount; i++)
    {
        ImDrawList* cmd_list = CmdLists[i];
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            ImDrawCmd* cmd = &cmd_list->CmdBuffer[cmd_i];
            cmd->ClipRect = ImVec4(cmd->ClipRect.x * scale.x, cmd->ClipRect.y * scale.y, cmd->ClipRect.z * scale.x, cmd->ClipRect.w * scale.y);
        }
    }
}

//-----------------------------------------------------------------------------
// ImFontAtlas
//-----------------------------------------------------------------------------

ImFontConfig::ImFontConfig()
{
    FontData = NULL;
    FontDataSize = 0;
    FontDataOwnedByAtlas = true;
    FontNo = 0;
    SizePixels = 0.0f;
    OversampleH = 3;
    OversampleV = 1;
    PixelSnapH = false;
    GlyphExtraSpacing = ImVec2(0.0f, 0.0f);
    GlyphRanges = NULL;
    MergeMode = false;
    MergeGlyphCenterV = false;
    DstFont = NULL;
    memset(Name, 0, sizeof(Name));
}

ImFontAtlas::ImFontAtlas()
{
    TexID = NULL;
    TexPixelsAlpha8 = NULL;
    TexPixelsRGBA32 = NULL;
    TexWidth = TexHeight = TexDesiredWidth = 0;
    TexUvWhitePixel = ImVec2(0, 0);
}

ImFontAtlas::~ImFontAtlas()
{
    Clear();
}

void    ImFontAtlas::ClearInputData()
{
    for (int i = 0; i < ConfigData.Size; i++)
        if (ConfigData[i].FontData && ConfigData[i].FontDataOwnedByAtlas)
        {
            ImGui::MemFree(ConfigData[i].FontData);
            ConfigData[i].FontData = NULL;
        }

    // When clearing this we lose access to the font name and other information used to build the font.
    for (int i = 0; i < Fonts.Size; i++)
        if (Fonts[i]->ConfigData >= ConfigData.Data && Fonts[i]->ConfigData < ConfigData.Data + ConfigData.Size)
        {
            Fonts[i]->ConfigData = NULL;
            Fonts[i]->ConfigDataCount = 0;
        }
    ConfigData.clear();
}

void    ImFontAtlas::ClearTexData()
{
    if (TexPixelsAlpha8)
        ImGui::MemFree(TexPixelsAlpha8);
    if (TexPixelsRGBA32)
        ImGui::MemFree(TexPixelsRGBA32);
    TexPixelsAlpha8 = NULL;
    TexPixelsRGBA32 = NULL;
}

void    ImFontAtlas::ClearFonts()
{
    for (int i = 0; i < Fonts.Size; i++)
    {
        Fonts[i]->~ImFont();
        ImGui::MemFree(Fonts[i]);
    }
    Fonts.clear();
}

void    ImFontAtlas::Clear()
{
    ClearInputData();
    ClearTexData();
    ClearFonts();
}

void    ImFontAtlas::GetTexDataAsAlpha8(unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel)
{
    // Build atlas on demand
    if (TexPixelsAlpha8 == NULL)
    {
        if (ConfigData.empty())
            AddFontDefault();
        Build();
    }

    *out_pixels = TexPixelsAlpha8;
    if (out_width) *out_width = TexWidth;
    if (out_height) *out_height = TexHeight;
    if (out_bytes_per_pixel) *out_bytes_per_pixel = 1;
}

void    ImFontAtlas::GetTexDataAsRGBA32(unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel)
{
    // Convert to RGBA32 format on demand
    // Although it is likely to be the most commonly used format, our font rendering is 1 channel / 8 bpp
    if (!TexPixelsRGBA32)
    {
        unsigned char* pixels;
        GetTexDataAsAlpha8(&pixels, NULL, NULL);
        TexPixelsRGBA32 = (unsigned int*)ImGui::MemAlloc((size_t)(TexWidth * TexHeight * 4));
        const unsigned char* src = pixels;
        unsigned int* dst = TexPixelsRGBA32;
        for (int n = TexWidth * TexHeight; n > 0; n--)
            *dst++ = IM_COL32(255, 255, 255, (unsigned int)(*src++));
    }

    *out_pixels = (unsigned char*)TexPixelsRGBA32;
    if (out_width) *out_width = TexWidth;
    if (out_height) *out_height = TexHeight;
    if (out_bytes_per_pixel) *out_bytes_per_pixel = 4;
}

ImFont* ImFontAtlas::AddFont(const ImFontConfig* font_cfg)
{
    IM_ASSERT(font_cfg->FontData != NULL && font_cfg->FontDataSize > 0);
    IM_ASSERT(font_cfg->SizePixels > 0.0f);

    // Create new font
    if (!font_cfg->MergeMode)
    {
        ImFont* font = (ImFont*)ImGui::MemAlloc(sizeof(ImFont));
        IM_PLACEMENT_NEW(font) ImFont();
        Fonts.push_back(font);
    }
    else
    {
        IM_ASSERT(!Fonts.empty()); // When using MergeMode make sure that a font has already been added before. You can use ImGui::AddFontDefault() to add the default imgui font.
    }

    ConfigData.push_back(*font_cfg);
    ImFontConfig& new_font_cfg = ConfigData.back();
	if (!new_font_cfg.DstFont)
	    new_font_cfg.DstFont = Fonts.back();
    if (!new_font_cfg.FontDataOwnedByAtlas)
    {
        new_font_cfg.FontData = ImGui::MemAlloc(new_font_cfg.FontDataSize);
        new_font_cfg.FontDataOwnedByAtlas = true;
        memcpy(new_font_cfg.FontData, font_cfg->FontData, (size_t)new_font_cfg.FontDataSize);
    }

    // Invalidate texture
    ClearTexData();
    return new_font_cfg.DstFont;
}

// Default font TTF is compressed with stb_compress then base85 encoded (see extra_fonts/binary_to_compressed_c.cpp for encoder)
static unsigned int stb_decompress_length(unsigned char *input);
static unsigned int stb_decompress(unsigned char *output, unsigned char *i, unsigned int length);
static const char*  GetDefaultCompressedFontDataTTFBase85();
static unsigned int Decode85Byte(char c)                                    { return c >= '\\' ? c-36 : c-35; }
static void         Decode85(const unsigned char* src, unsigned char* dst)
{
    while (*src)
    {
        unsigned int tmp = Decode85Byte(src[0]) + 85*(Decode85Byte(src[1]) + 85*(Decode85Byte(src[2]) + 85*(Decode85Byte(src[3]) + 85*Decode85Byte(src[4]))));
        dst[0] = ((tmp >> 0) & 0xFF); dst[1] = ((tmp >> 8) & 0xFF); dst[2] = ((tmp >> 16) & 0xFF); dst[3] = ((tmp >> 24) & 0xFF);   // We can't assume little-endianness.
        src += 5;
        dst += 4;
    }
}

// Load embedded ProggyClean.ttf at size 13, disable oversampling
ImFont* ImFontAtlas::AddFontDefault(const ImFontConfig* font_cfg_template)
{
    ImFontConfig font_cfg = font_cfg_template ? *font_cfg_template : ImFontConfig();
    if (!font_cfg_template)
    {
        font_cfg.OversampleH = font_cfg.OversampleV = 1;
        font_cfg.PixelSnapH = true;
    }
    if (font_cfg.Name[0] == '\0') strcpy(font_cfg.Name, "ProggyClean.ttf, 13px");

    const char* ttf_compressed_base85 = GetDefaultCompressedFontDataTTFBase85();
    ImFont* font = AddFontFromMemoryCompressedBase85TTF(ttf_compressed_base85, 13.0f, &font_cfg, GetGlyphRangesDefault());
    return font;
}

ImFont* ImFontAtlas::AddFontFromFileTTF(const char* filename, float size_pixels, const ImFontConfig* font_cfg_template, const ImWchar* glyph_ranges)
{
    int data_size = 0;
    void* data = ImFileLoadToMemory(filename, "rb", &data_size, 0);
    if (!data)
    {
        IM_ASSERT(0); // Could not load file.
        return NULL;
    }
    ImFontConfig font_cfg = font_cfg_template ? *font_cfg_template : ImFontConfig();
    if (font_cfg.Name[0] == '\0')
    {
        // Store a short copy of filename into into the font name for convenience
        const char* p;
        for (p = filename + strlen(filename); p > filename && p[-1] != '/' && p[-1] != '\\'; p--) {}
        snprintf(font_cfg.Name, IM_ARRAYSIZE(font_cfg.Name), "%s, %.0fpx", p, size_pixels);
    }
    return AddFontFromMemoryTTF(data, data_size, size_pixels, &font_cfg, glyph_ranges);
}

// NBM Transfer ownership of 'ttf_data' to ImFontAtlas, unless font_cfg_template->FontDataOwnedByAtlas == false. Owned TTF buffer will be deleted after Build().
ImFont* ImFontAtlas::AddFontFromMemoryTTF(void* ttf_data, int ttf_size, float size_pixels, const ImFontConfig* font_cfg_template, const ImWchar* glyph_ranges)
{
    ImFontConfig font_cfg = font_cfg_template ? *font_cfg_template : ImFontConfig();
    IM_ASSERT(font_cfg.FontData == NULL);
    font_cfg.FontData = ttf_data;
    font_cfg.FontDataSize = ttf_size;
    font_cfg.SizePixels = size_pixels;
    if (glyph_ranges)
        font_cfg.GlyphRanges = glyph_ranges;
    return AddFont(&font_cfg);
}

ImFont* ImFontAtlas::AddFontFromMemoryCompressedTTF(const void* compressed_ttf_data, int compressed_ttf_size, float size_pixels, const ImFontConfig* font_cfg_template, const ImWchar* glyph_ranges)
{
    const unsigned int buf_decompressed_size = stb_decompress_length((unsigned char*)compressed_ttf_data);
    unsigned char* buf_decompressed_data = (unsigned char *)ImGui::MemAlloc(buf_decompressed_size);
    stb_decompress(buf_decompressed_data, (unsigned char*)compressed_ttf_data, (unsigned int)compressed_ttf_size);

    ImFontConfig font_cfg = font_cfg_template ? *font_cfg_template : ImFontConfig();
    IM_ASSERT(font_cfg.FontData == NULL);
    font_cfg.FontDataOwnedByAtlas = true;
    return AddFontFromMemoryTTF(buf_decompressed_data, (int)buf_decompressed_size, size_pixels, &font_cfg, glyph_ranges);
}

ImFont* ImFontAtlas::AddFontFromMemoryCompressedBase85TTF(const char* compressed_ttf_data_base85, float size_pixels, const ImFontConfig* font_cfg, const ImWchar* glyph_ranges)
{
    int compressed_ttf_size = (((int)strlen(compressed_ttf_data_base85) + 4) / 5) * 4;
    void* compressed_ttf = ImGui::MemAlloc((size_t)compressed_ttf_size);
    Decode85((const unsigned char*)compressed_ttf_data_base85, (unsigned char*)compressed_ttf);
    ImFont* font = AddFontFromMemoryCompressedTTF(compressed_ttf, compressed_ttf_size, size_pixels, font_cfg, glyph_ranges);
    ImGui::MemFree(compressed_ttf);
    return font;
}

bool    ImFontAtlas::Build()
{
    IM_ASSERT(ConfigData.Size > 0);

    TexID = NULL;
    TexWidth = TexHeight = 0;
    TexUvWhitePixel = ImVec2(0, 0);
    ClearTexData();

    struct ImFontTempBuildData
    {
        stbtt_fontinfo      FontInfo;
        stbrp_rect*         Rects;
        stbtt_pack_range*   Ranges;
        int                 RangesCount;
    };
    ImFontTempBuildData* tmp_array = (ImFontTempBuildData*)ImGui::MemAlloc((size_t)ConfigData.Size * sizeof(ImFontTempBuildData));

    // Initialize font information early (so we can error without any cleanup) + count glyphs
    int total_glyph_count = 0;
    int total_glyph_range_count = 0;
    for (int input_i = 0; input_i < ConfigData.Size; input_i++)
    {
        ImFontConfig& cfg = ConfigData[input_i];
        ImFontTempBuildData& tmp = tmp_array[input_i];

        IM_ASSERT(cfg.DstFont && (!cfg.DstFont->IsLoaded() || cfg.DstFont->ContainerAtlas == this));
        const int font_offset = stbtt_GetFontOffsetForIndex((unsigned char*)cfg.FontData, cfg.FontNo);
        IM_ASSERT(font_offset >= 0);
        if (!stbtt_InitFont(&tmp.FontInfo, (unsigned char*)cfg.FontData, font_offset))
            return false;

        // Count glyphs
        if (!cfg.GlyphRanges)
            cfg.GlyphRanges = GetGlyphRangesDefault();
        for (const ImWchar* in_range = cfg.GlyphRanges; in_range[0] && in_range[1]; in_range += 2)
        {
            total_glyph_count += (in_range[1] - in_range[0]) + 1;
            total_glyph_range_count++;
        }
    }

    // Start packing. We need a known width for the skyline algorithm. Using a cheap heuristic here to decide of width. User can override TexDesiredWidth if they wish.
    // After packing is done, width shouldn't matter much, but some API/GPU have texture size limitations and increasing width can decrease height.
    TexWidth = (TexDesiredWidth > 0) ? TexDesiredWidth : (total_glyph_count > 4000) ? 4096 : (total_glyph_count > 2000) ? 2048 : (total_glyph_count > 1000) ? 1024 : 512;
    TexHeight = 0;
    const int max_tex_height = 1024*32;
    stbtt_pack_context spc;
    stbtt_PackBegin(&spc, NULL, TexWidth, max_tex_height, 0, 1, NULL);

    // Pack our extra data rectangles first, so it will be on the upper-left corner of our texture (UV will have small values).
    ImVector<stbrp_rect> extra_rects;
    RenderCustomTexData(0, &extra_rects);
    stbtt_PackSetOversampling(&spc, 1, 1);
    stbrp_pack_rects((stbrp_context*)spc.pack_info, &extra_rects[0], extra_rects.Size);
    for (int i = 0; i < extra_rects.Size; i++)
        if (extra_rects[i].was_packed)
            TexHeight = ImMax(TexHeight, extra_rects[i].y + extra_rects[i].h);

    // Allocate packing character data and flag packed characters buffer as non-packed (x0=y0=x1=y1=0)
    int buf_packedchars_n = 0, buf_rects_n = 0, buf_ranges_n = 0;
    stbtt_packedchar* buf_packedchars = (stbtt_packedchar*)ImGui::MemAlloc(total_glyph_count * sizeof(stbtt_packedchar));
    stbrp_rect* buf_rects = (stbrp_rect*)ImGui::MemAlloc(total_glyph_count * sizeof(stbrp_rect));
    stbtt_pack_range* buf_ranges = (stbtt_pack_range*)ImGui::MemAlloc(total_glyph_range_count * sizeof(stbtt_pack_range));
    memset(buf_packedchars, 0, total_glyph_count * sizeof(stbtt_packedchar));
    memset(buf_rects, 0, total_glyph_count * sizeof(stbrp_rect));              // Unnecessary but let's clear this for the sake of sanity.
    memset(buf_ranges, 0, total_glyph_range_count * sizeof(stbtt_pack_range));

    // First font pass: pack all glyphs (no rendering at this point, we are working with rectangles in an infinitely tall texture at this point)
    for (int input_i = 0; input_i < ConfigData.Size; input_i++)
    {
        ImFontConfig& cfg = ConfigData[input_i];
        ImFontTempBuildData& tmp = tmp_array[input_i];

        // Setup ranges
        int glyph_count = 0;
        int glyph_ranges_count = 0;
        for (const ImWchar* in_range = cfg.GlyphRanges; in_range[0] && in_range[1]; in_range += 2)
        {
            glyph_count += (in_range[1] - in_range[0]) + 1;
            glyph_ranges_count++;
        }
        tmp.Ranges = buf_ranges + buf_ranges_n;
        tmp.RangesCount = glyph_ranges_count;
        buf_ranges_n += glyph_ranges_count;
        for (int i = 0; i < glyph_ranges_count; i++)
        {
            const ImWchar* in_range = &cfg.GlyphRanges[i * 2];
            stbtt_pack_range& range = tmp.Ranges[i];
            range.font_size = cfg.SizePixels;
            range.first_unicode_codepoint_in_range = in_range[0];
            range.num_chars = (in_range[1] - in_range[0]) + 1;
            range.chardata_for_range = buf_packedchars + buf_packedchars_n;
            buf_packedchars_n += range.num_chars;
        }

        // Pack
        tmp.Rects = buf_rects + buf_rects_n;
        buf_rects_n += glyph_count;
        stbtt_PackSetOversampling(&spc, cfg.OversampleH, cfg.OversampleV);
        int n = stbtt_PackFontRangesGatherRects(&spc, &tmp.FontInfo, tmp.Ranges, tmp.RangesCount, tmp.Rects);
        stbrp_pack_rects((stbrp_context*)spc.pack_info, tmp.Rects, n);

        // Extend texture height
        for (int i = 0; i < n; i++)
            if (tmp.Rects[i].was_packed)
                TexHeight = ImMax(TexHeight, tmp.Rects[i].y + tmp.Rects[i].h);
    }
    IM_ASSERT(buf_rects_n == total_glyph_count);
    IM_ASSERT(buf_packedchars_n == total_glyph_count);
    IM_ASSERT(buf_ranges_n == total_glyph_range_count);

    // Create texture
    TexHeight = ImUpperPowerOfTwo(TexHeight);
    TexPixelsAlpha8 = (unsigned char*)ImGui::MemAlloc(TexWidth * TexHeight);
    memset(TexPixelsAlpha8, 0, TexWidth * TexHeight);
    spc.pixels = TexPixelsAlpha8;
    spc.height = TexHeight;

    // Second pass: render characters
    for (int input_i = 0; input_i < ConfigData.Size; input_i++)
    {
        ImFontConfig& cfg = ConfigData[input_i];
        ImFontTempBuildData& tmp = tmp_array[input_i];
        stbtt_PackSetOversampling(&spc, cfg.OversampleH, cfg.OversampleV);
        stbtt_PackFontRangesRenderIntoRects(&spc, &tmp.FontInfo, tmp.Ranges, tmp.RangesCount, tmp.Rects);
        tmp.Rects = NULL;
    }

    // End packing
    stbtt_PackEnd(&spc);
    ImGui::MemFree(buf_rects);
    buf_rects = NULL;

    // Third pass: setup ImFont and glyphs for runtime
    for (int input_i = 0; input_i < ConfigData.Size; input_i++)
    {
        ImFontConfig& cfg = ConfigData[input_i];
        ImFontTempBuildData& tmp = tmp_array[input_i];
        ImFont* dst_font = cfg.DstFont; // We can have multiple input fonts writing into a same destination font (when using MergeMode=true)

        float font_scale = stbtt_ScaleForPixelHeight(&tmp.FontInfo, cfg.SizePixels);
        int unscaled_ascent, unscaled_descent, unscaled_line_gap;
        stbtt_GetFontVMetrics(&tmp.FontInfo, &unscaled_ascent, &unscaled_descent, &unscaled_line_gap);

        float ascent = unscaled_ascent * font_scale;
        float descent = unscaled_descent * font_scale;
        if (!cfg.MergeMode)
        {
            dst_font->ContainerAtlas = this;
            dst_font->ConfigData = &cfg;
            dst_font->ConfigDataCount = 0;
            dst_font->FontSize = cfg.SizePixels;
            dst_font->Ascent = ascent;
            dst_font->Descent = descent;
            dst_font->Glyphs.resize(0);
            dst_font->MetricsTotalSurface = 0;
        }
        dst_font->ConfigDataCount++;
        float off_y = (cfg.MergeMode && cfg.MergeGlyphCenterV) ? (ascent - dst_font->Ascent) * 0.5f : 0.0f;

        dst_font->FallbackGlyph = NULL; // Always clear fallback so FindGlyph can return NULL. It will be set again in BuildLookupTable()
        for (int i = 0; i < tmp.RangesCount; i++)
        {
            stbtt_pack_range& range = tmp.Ranges[i];
            for (int char_idx = 0; char_idx < range.num_chars; char_idx += 1)
            {
                const stbtt_packedchar& pc = range.chardata_for_range[char_idx];
                if (!pc.x0 && !pc.x1 && !pc.y0 && !pc.y1)
                    continue;

                const int codepoint = range.first_unicode_codepoint_in_range + char_idx;
                if (cfg.MergeMode && dst_font->FindGlyph((unsigned short)codepoint))
                    continue;

                stbtt_aligned_quad q;
                float dummy_x = 0.0f, dummy_y = 0.0f;
                stbtt_GetPackedQuad(range.chardata_for_range, TexWidth, TexHeight, char_idx, &dummy_x, &dummy_y, &q, 0);

                dst_font->Glyphs.resize(dst_font->Glyphs.Size + 1);
                ImFont::Glyph& glyph = dst_font->Glyphs.back();
                glyph.Codepoint = (ImWchar)codepoint;
                glyph.X0 = q.x0; glyph.Y0 = q.y0; glyph.X1 = q.x1; glyph.Y1 = q.y1;
                glyph.U0 = q.s0; glyph.V0 = q.t0; glyph.U1 = q.s1; glyph.V1 = q.t1;
                glyph.Y0 += (float)(int)(dst_font->Ascent + off_y + 0.5f);
                glyph.Y1 += (float)(int)(dst_font->Ascent + off_y + 0.5f);
                glyph.XAdvance = (pc.xadvance + cfg.GlyphExtraSpacing.x);  // Bake spacing into XAdvance
                if (cfg.PixelSnapH)
                    glyph.XAdvance = (float)(int)(glyph.XAdvance + 0.5f);
                dst_font->MetricsTotalSurface += (int)((glyph.U1 - glyph.U0) * TexWidth + 1.99f) * (int)((glyph.V1 - glyph.V0) * TexHeight + 1.99f); // +1 to account for average padding, +0.99 to round
            }
        }
        cfg.DstFont->BuildLookupTable();
    }

    // Cleanup temporaries
    ImGui::MemFree(buf_packedchars);
    ImGui::MemFree(buf_ranges);
    ImGui::MemFree(tmp_array);

    // Render into our custom data block
    RenderCustomTexData(1, &extra_rects);

    return true;
}

void ImFontAtlas::RenderCustomTexData(int pass, void* p_rects)
{
    // A work of art lies ahead! (. = white layer, X = black layer, others are blank)
    // The white texels on the top left are the ones we'll use everywhere in ImGui to render filled shapes.
    const int TEX_DATA_W = 90;
    const int TEX_DATA_H = 27;
    const char texture_data[TEX_DATA_W*TEX_DATA_H+1] =
    {
        "..-         -XXXXXXX-    X    -           X           -XXXXXXX          -          XXXXXXX"
        "..-         -X.....X-   X.X   -          X.X          -X.....X          -          X.....X"
        "---         -XXX.XXX-  X...X  -         X...X         -X....X           -           X....X"
        "X           -  X.X  - X.....X -        X.....X        -X...X            -            X...X"
        "XX          -  X.X  -X.......X-       X.......X       -X..X.X           -           X.X..X"
        "X.X         -  X.X  -XXXX.XXXX-       XXXX.XXXX       -X.X X.X          -          X.X X.X"
        "X..X        -  X.X  -   X.X   -          X.X          -XX   X.X         -         X.X   XX"
        "X...X       -  X.X  -   X.X   -    XX    X.X    XX    -      X.X        -        X.X      "
        "X....X      -  X.X  -   X.X   -   X.X    X.X    X.X   -       X.X       -       X.X       "
        "X.....X     -  X.X  -   X.X   -  X..X    X.X    X..X  -        X.X      -      X.X        "
        "X......X    -  X.X  -   X.X   - X...XXXXXX.XXXXXX...X -         X.X   XX-XX   X.X         "
        "X.......X   -  X.X  -   X.X   -X.....................X-          X.X X.X-X.X X.X          "
        "X........X  -  X.X  -   X.X   - X...XXXXXX.XXXXXX...X -           X.X..X-X..X.X           "
        "X.........X -XXX.XXX-   X.X   -  X..X    X.X    X..X  -            X...X-X...X            "
        "X..........X-X.....X-   X.X   -   X.X    X.X    X.X   -           X....X-X....X           "
        "X......XXXXX-XXXXXXX-   X.X   -    XX    X.X    XX    -          X.....X-X.....X          "
        "X...X..X    ---------   X.X   -          X.X          -          XXXXXXX-XXXXXXX          "
        "X..X X..X   -       -XXXX.XXXX-       XXXX.XXXX       ------------------------------------"
        "X.X  X..X   -       -X.......X-       X.......X       -    XX           XX    -           "
        "XX    X..X  -       - X.....X -        X.....X        -   X.X           X.X   -           "
        "      X..X          -  X...X  -         X...X         -  X..X           X..X  -           "
        "       XX           -   X.X   -          X.X          - X...XXXXXXXXXXXXX...X -           "
        "------------        -    X    -           X           -X.....................X-           "
        "                    ----------------------------------- X...XXXXXXXXXXXXX...X -           "
        "                                                      -  X..X           X..X  -           "
        "                                                      -   X.X           X.X   -           "
        "                                                      -    XX           XX    -           "
    };

    ImVector<stbrp_rect>& rects = *(ImVector<stbrp_rect>*)p_rects;
    if (pass == 0)
    {
        // Request rectangles
        stbrp_rect r;
        memset(&r, 0, sizeof(r));
        r.w = (TEX_DATA_W*2)+1;
        r.h = TEX_DATA_H+1;
        rects.push_back(r);
    }
    else if (pass == 1)
    {
        // Render/copy pixels
        const stbrp_rect& r = rects[0];
        for (int y = 0, n = 0; y < TEX_DATA_H; y++)
            for (int x = 0; x < TEX_DATA_W; x++, n++)
            {
                const int offset0 = (int)(r.x + x) + (int)(r.y + y) * TexWidth;
                const int offset1 = offset0 + 1 + TEX_DATA_W;
                TexPixelsAlpha8[offset0] = texture_data[n] == '.' ? 0xFF : 0x00;
                TexPixelsAlpha8[offset1] = texture_data[n] == 'X' ? 0xFF : 0x00;
            }
        const ImVec2 tex_uv_scale(1.0f / TexWidth, 1.0f / TexHeight);
        TexUvWhitePixel = ImVec2((r.x + 0.5f) * tex_uv_scale.x, (r.y + 0.5f) * tex_uv_scale.y);

        // Setup mouse cursors
        const ImVec2 cursor_datas[ImGuiMouseCursor_Count_][3] =
        {
            // Pos ........ Size ......... Offset ......
            { ImVec2(0,3),  ImVec2(12,19), ImVec2( 0, 0) }, // ImGuiMouseCursor_Arrow
            { ImVec2(13,0), ImVec2(7,16),  ImVec2( 4, 8) }, // ImGuiMouseCursor_TextInput
            { ImVec2(31,0), ImVec2(23,23), ImVec2(11,11) }, // ImGuiMouseCursor_Move
            { ImVec2(21,0), ImVec2( 9,23), ImVec2( 5,11) }, // ImGuiMouseCursor_ResizeNS
            { ImVec2(55,18),ImVec2(23, 9), ImVec2(11, 5) }, // ImGuiMouseCursor_ResizeEW
            { ImVec2(73,0), ImVec2(17,17), ImVec2( 9, 9) }, // ImGuiMouseCursor_ResizeNESW
            { ImVec2(55,0), ImVec2(17,17), ImVec2( 9, 9) }, // ImGuiMouseCursor_ResizeNWSE
        };

        for (int type = 0; type < ImGuiMouseCursor_Count_; type++)
        {
            ImGuiMouseCursorData& cursor_data = GImGui->MouseCursorData[type];
            ImVec2 pos = cursor_datas[type][0] + ImVec2((float)r.x, (float)r.y);
            const ImVec2 size = cursor_datas[type][1];
            cursor_data.Type = type;
            cursor_data.Size = size;
            cursor_data.HotOffset = cursor_datas[type][2];
            cursor_data.TexUvMin[0] = (pos) * tex_uv_scale;
            cursor_data.TexUvMax[0] = (pos + size) * tex_uv_scale;
            pos.x += TEX_DATA_W+1;
            cursor_data.TexUvMin[1] = (pos) * tex_uv_scale;
            cursor_data.TexUvMax[1] = (pos + size) * tex_uv_scale;
        }
    }
}

// Retrieve list of range (2 int per range, values are inclusive)
const ImWchar*   ImFontAtlas::GetGlyphRangesDefault()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0,
    };
    return &ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesKorean()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x3131, 0x3163, // Korean alphabets
        0xAC00, 0xD79D, // Korean characters
        0,
    };
    return &ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesChinese()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x3000, 0x30FF, // Punctuations, Hiragana, Katakana
        0x31F0, 0x31FF, // Katakana Phonetic Extensions
        0xFF00, 0xFFEF, // Half-width characters
        0x4e00, 0x9FAF, // CJK Ideograms
        0,
    };
    return &ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesJapanese()
{
    // Store the 1946 ideograms code points as successive offsets from the initial unicode codepoint 0x4E00. Each offset has an implicit +1.
    // This encoding helps us reduce the source code size.
    static const short offsets_from_0x4E00[] =
    {
        -1,0,1,3,0,0,0,0,1,0,5,1,1,0,7,4,6,10,0,1,9,9,7,1,3,19,1,10,7,1,0,1,0,5,1,0,6,4,2,6,0,0,12,6,8,0,3,5,0,1,0,9,0,0,8,1,1,3,4,5,13,0,0,8,2,17,
        4,3,1,1,9,6,0,0,0,2,1,3,2,22,1,9,11,1,13,1,3,12,0,5,9,2,0,6,12,5,3,12,4,1,2,16,1,1,4,6,5,3,0,6,13,15,5,12,8,14,0,0,6,15,3,6,0,18,8,1,6,14,1,
        5,4,12,24,3,13,12,10,24,0,0,0,1,0,1,1,2,9,10,2,2,0,0,3,3,1,0,3,8,0,3,2,4,4,1,6,11,10,14,6,15,3,4,15,1,0,0,5,2,2,0,0,1,6,5,5,6,0,3,6,5,0,0,1,0,
        11,2,2,8,4,7,0,10,0,1,2,17,19,3,0,2,5,0,6,2,4,4,6,1,1,11,2,0,3,1,2,1,2,10,7,6,3,16,0,8,24,0,0,3,1,1,3,0,1,6,0,0,0,2,0,1,5,15,0,1,0,0,2,11,19,
        1,4,19,7,6,5,1,0,0,0,0,5,1,0,1,9,0,0,5,0,2,0,1,0,3,0,11,3,0,2,0,0,0,0,0,9,3,6,4,12,0,14,0,0,29,10,8,0,14,37,13,0,31,16,19,0,8,30,1,20,8,3,48,
        21,1,0,12,0,10,44,34,42,54,11,18,82,0,2,1,2,12,1,0,6,2,17,2,12,7,0,7,17,4,2,6,24,23,8,23,39,2,16,23,1,0,5,1,2,15,14,5,6,2,11,0,8,6,2,2,2,14,
        20,4,15,3,4,11,10,10,2,5,2,1,30,2,1,0,0,22,5,5,0,3,1,5,4,1,0,0,2,2,21,1,5,1,2,16,2,1,3,4,0,8,4,0,0,5,14,11,2,16,1,13,1,7,0,22,15,3,1,22,7,14,
        22,19,11,24,18,46,10,20,64,45,3,2,0,4,5,0,1,4,25,1,0,0,2,10,0,0,0,1,0,1,2,0,0,9,1,2,0,0,0,2,5,2,1,1,5,5,8,1,1,1,5,1,4,9,1,3,0,1,0,1,1,2,0,0,
        2,0,1,8,22,8,1,0,0,0,0,4,2,1,0,9,8,5,0,9,1,30,24,2,6,4,39,0,14,5,16,6,26,179,0,2,1,1,0,0,0,5,2,9,6,0,2,5,16,7,5,1,1,0,2,4,4,7,15,13,14,0,0,
        3,0,1,0,0,0,2,1,6,4,5,1,4,9,0,3,1,8,0,0,10,5,0,43,0,2,6,8,4,0,2,0,0,9,6,0,9,3,1,6,20,14,6,1,4,0,7,2,3,0,2,0,5,0,3,1,0,3,9,7,0,3,4,0,4,9,1,6,0,
        9,0,0,2,3,10,9,28,3,6,2,4,1,2,32,4,1,18,2,0,3,1,5,30,10,0,2,2,2,0,7,9,8,11,10,11,7,2,13,7,5,10,0,3,40,2,0,1,6,12,0,4,5,1,5,11,11,21,4,8,3,7,
        8,8,33,5,23,0,0,19,8,8,2,3,0,6,1,1,1,5,1,27,4,2,5,0,3,5,6,3,1,0,3,1,12,5,3,3,2,0,7,7,2,1,0,4,0,1,1,2,0,10,10,6,2,5,9,7,5,15,15,21,6,11,5,20,
        4,3,5,5,2,5,0,2,1,0,1,7,28,0,9,0,5,12,5,5,18,30,0,12,3,3,21,16,25,32,9,3,14,11,24,5,66,9,1,2,0,5,9,1,5,1,8,0,8,3,3,0,1,15,1,4,8,1,2,7,0,7,2,
        8,3,7,5,3,7,10,2,1,0,0,2,25,0,6,4,0,10,0,4,2,4,1,12,5,38,4,0,4,1,10,5,9,4,0,14,4,2,5,18,20,21,1,3,0,5,0,7,0,3,7,1,3,1,1,8,1,0,0,0,3,2,5,2,11,
        6,0,13,1,3,9,1,12,0,16,6,2,1,0,2,1,12,6,13,11,2,0,28,1,7,8,14,13,8,13,0,2,0,5,4,8,10,2,37,42,19,6,6,7,4,14,11,18,14,80,7,6,0,4,72,12,36,27,
        7,7,0,14,17,19,164,27,0,5,10,7,3,13,6,14,0,2,2,5,3,0,6,13,0,0,10,29,0,4,0,3,13,0,3,1,6,51,1,5,28,2,0,8,0,20,2,4,0,25,2,10,13,10,0,16,4,0,1,0,
        2,1,7,0,1,8,11,0,0,1,2,7,2,23,11,6,6,4,16,2,2,2,0,22,9,3,3,5,2,0,15,16,21,2,9,20,15,15,5,3,9,1,0,0,1,7,7,5,4,2,2,2,38,24,14,0,0,15,5,6,24,14,
        5,5,11,0,21,12,0,3,8,4,11,1,8,0,11,27,7,2,4,9,21,59,0,1,39,3,60,62,3,0,12,11,0,3,30,11,0,13,88,4,15,5,28,13,1,4,48,17,17,4,28,32,46,0,16,0,
        18,11,1,8,6,38,11,2,6,11,38,2,0,45,3,11,2,7,8,4,30,14,17,2,1,1,65,18,12,16,4,2,45,123,12,56,33,1,4,3,4,7,0,0,0,3,2,0,16,4,2,4,2,0,7,4,5,2,26,
        2,25,6,11,6,1,16,2,6,17,77,15,3,35,0,1,0,5,1,0,38,16,6,3,12,3,3,3,0,9,3,1,3,5,2,9,0,18,0,25,1,3,32,1,72,46,6,2,7,1,3,14,17,0,28,1,40,13,0,20,
        15,40,6,38,24,12,43,1,1,9,0,12,6,0,6,2,4,19,3,7,1,48,0,9,5,0,5,6,9,6,10,15,2,11,19,3,9,2,0,1,10,1,27,8,1,3,6,1,14,0,26,0,27,16,3,4,9,6,2,23,
        9,10,5,25,2,1,6,1,1,48,15,9,15,14,3,4,26,60,29,13,37,21,1,6,4,0,2,11,22,23,16,16,2,2,1,3,0,5,1,6,4,0,0,4,0,0,8,3,0,2,5,0,7,1,7,3,13,2,4,10,
        3,0,2,31,0,18,3,0,12,10,4,1,0,7,5,7,0,5,4,12,2,22,10,4,2,15,2,8,9,0,23,2,197,51,3,1,1,4,13,4,3,21,4,19,3,10,5,40,0,4,1,1,10,4,1,27,34,7,21,
        2,17,2,9,6,4,2,3,0,4,2,7,8,2,5,1,15,21,3,4,4,2,2,17,22,1,5,22,4,26,7,0,32,1,11,42,15,4,1,2,5,0,19,3,1,8,6,0,10,1,9,2,13,30,8,2,24,17,19,1,4,
        4,25,13,0,10,16,11,39,18,8,5,30,82,1,6,8,18,77,11,13,20,75,11,112,78,33,3,0,0,60,17,84,9,1,1,12,30,10,49,5,32,158,178,5,5,6,3,3,1,3,1,4,7,6,
        19,31,21,0,2,9,5,6,27,4,9,8,1,76,18,12,1,4,0,3,3,6,3,12,2,8,30,16,2,25,1,5,5,4,3,0,6,10,2,3,1,0,5,1,19,3,0,8,1,5,2,6,0,0,0,19,1,2,0,5,1,2,5,
        1,3,7,0,4,12,7,3,10,22,0,9,5,1,0,2,20,1,1,3,23,30,3,9,9,1,4,191,14,3,15,6,8,50,0,1,0,0,4,0,0,1,0,2,4,2,0,2,3,0,2,0,2,2,8,7,0,1,1,1,3,3,17,11,
        91,1,9,3,2,13,4,24,15,41,3,13,3,1,20,4,125,29,30,1,0,4,12,2,21,4,5,5,19,11,0,13,11,86,2,18,0,7,1,8,8,2,2,22,1,2,6,5,2,0,1,2,8,0,2,0,5,2,1,0,
        2,10,2,0,5,9,2,1,2,0,1,0,4,0,0,10,2,5,3,0,6,1,0,1,4,4,33,3,13,17,3,18,6,4,7,1,5,78,0,4,1,13,7,1,8,1,0,35,27,15,3,0,0,0,1,11,5,41,38,15,22,6,
        14,14,2,1,11,6,20,63,5,8,27,7,11,2,2,40,58,23,50,54,56,293,8,8,1,5,1,14,0,1,12,37,89,8,8,8,2,10,6,0,0,0,4,5,2,1,0,1,1,2,7,0,3,3,0,4,6,0,3,2,
        19,3,8,0,0,0,4,4,16,0,4,1,5,1,3,0,3,4,6,2,17,10,10,31,6,4,3,6,10,126,7,3,2,2,0,9,0,0,5,20,13,0,15,0,6,0,2,5,8,64,50,3,2,12,2,9,0,0,11,8,20,
        109,2,18,23,0,0,9,61,3,0,28,41,77,27,19,17,81,5,2,14,5,83,57,252,14,154,263,14,20,8,13,6,57,39,38,
    };
    static ImWchar base_ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x3000, 0x30FF, // Punctuations, Hiragana, Katakana
        0x31F0, 0x31FF, // Katakana Phonetic Extensions
        0xFF00, 0xFFEF, // Half-width characters
    };
    static bool full_ranges_unpacked = false;
    static ImWchar full_ranges[IM_ARRAYSIZE(base_ranges) + IM_ARRAYSIZE(offsets_from_0x4E00)*2 + 1];
    if (!full_ranges_unpacked)
    {
        // Unpack
        int codepoint = 0x4e00;
        memcpy(full_ranges, base_ranges, sizeof(base_ranges));
        ImWchar* dst = full_ranges + IM_ARRAYSIZE(base_ranges);;
        for (int n = 0; n < IM_ARRAYSIZE(offsets_from_0x4E00); n++, dst += 2)
            dst[0] = dst[1] = (ImWchar)(codepoint += (offsets_from_0x4E00[n] + 1));
        dst[0] = 0;
        full_ranges_unpacked = true;
    }
    return &full_ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesCyrillic()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
        0x2DE0, 0x2DFF, // Cyrillic Extended-A
        0xA640, 0xA69F, // Cyrillic Extended-B
        0,
    };
    return &ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesThai()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin
        0x0E00, 0x0E7F, // Thai
        0,
    };
    return &ranges[0];
}

//-----------------------------------------------------------------------------
// ImFont
//-----------------------------------------------------------------------------

ImFont::ImFont()
{
    Scale = 1.0f;
    FallbackChar = (ImWchar)'?';
    Clear();
}

ImFont::~ImFont()
{
    // Invalidate active font so that the user gets a clear crash instead of a dangling pointer.
    // If you want to delete fonts you need to do it between Render() and NewFrame().
    // FIXME-CLEANUP
    /*
    ImGuiContext& g = *GImGui;
    if (g.Font == this)
        g.Font = NULL;
    */
    Clear();
}

void    ImFont::Clear()
{
    FontSize = 0.0f;
    DisplayOffset = ImVec2(0.0f, 1.0f);
    Glyphs.clear();
    IndexXAdvance.clear();
    IndexLookup.clear();
    FallbackGlyph = NULL;
    FallbackXAdvance = 0.0f;
    ConfigDataCount = 0;
    ConfigData = NULL;
    ContainerAtlas = NULL;
    Ascent = Descent = 0.0f;
    MetricsTotalSurface = 0;
}

void ImFont::BuildLookupTable()
{
    int max_codepoint = 0;
    for (int i = 0; i != Glyphs.Size; i++)
        max_codepoint = ImMax(max_codepoint, (int)Glyphs[i].Codepoint);

    IM_ASSERT(Glyphs.Size < 0xFFFF); // -1 is reserved
    IndexXAdvance.clear();
    IndexLookup.clear();
    GrowIndex(max_codepoint + 1);
    for (int i = 0; i < Glyphs.Size; i++)
    {
        int codepoint = (int)Glyphs[i].Codepoint;
        IndexXAdvance[codepoint] = Glyphs[i].XAdvance;
        IndexLookup[codepoint] = (unsigned short)i;
    }

    // Create a glyph to handle TAB
    // FIXME: Needs proper TAB handling but it needs to be contextualized (or we could arbitrary say that each string starts at "column 0" ?)
    if (FindGlyph((unsigned short)' '))
    {
        if (Glyphs.back().Codepoint != '\t')   // So we can call this function multiple times
            Glyphs.resize(Glyphs.Size + 1);
        ImFont::Glyph& tab_glyph = Glyphs.back();
        tab_glyph = *FindGlyph((unsigned short)' ');
        tab_glyph.Codepoint = '\t';
        tab_glyph.XAdvance *= 4;
        IndexXAdvance[(int)tab_glyph.Codepoint] = (float)tab_glyph.XAdvance;
        IndexLookup[(int)tab_glyph.Codepoint] = (unsigned short)(Glyphs.Size-1);
    }

    FallbackGlyph = NULL;
    FallbackGlyph = FindGlyph(FallbackChar);
    FallbackXAdvance = FallbackGlyph ? FallbackGlyph->XAdvance : 0.0f;
    for (int i = 0; i < max_codepoint + 1; i++)
        if (IndexXAdvance[i] < 0.0f)
            IndexXAdvance[i] = FallbackXAdvance;
}

void ImFont::SetFallbackChar(ImWchar c)
{
    FallbackChar = c;
    BuildLookupTable();
}

void ImFont::GrowIndex(int new_size)
{
    IM_ASSERT(IndexXAdvance.Size == IndexLookup.Size);
    int old_size = IndexLookup.Size;
    if (new_size <= old_size)
        return;
    IndexXAdvance.resize(new_size);
    IndexLookup.resize(new_size);
    for (int i = old_size; i < new_size; i++)
    {
        IndexXAdvance[i] = -1.0f;
        IndexLookup[i] = (unsigned short)-1;
    }
}

void ImFont::AddRemapChar(ImWchar dst, ImWchar src, bool overwrite_dst)
{
    IM_ASSERT(IndexLookup.Size > 0);    // Currently this can only be called AFTER the font has been built, aka after calling ImFontAtlas::GetTexDataAs*() function.
    int index_size = IndexLookup.Size;

    if (dst < index_size && IndexLookup.Data[dst] == (unsigned short)-1 && !overwrite_dst) // 'dst' already exists
        return;
    if (src >= index_size && dst >= index_size) // both 'dst' and 'src' don't exist -> no-op
        return;

    GrowIndex(dst + 1);
    IndexLookup[dst] = (src < index_size) ? IndexLookup.Data[src] : (unsigned short)-1;
    IndexXAdvance[dst] = (src < index_size) ? IndexXAdvance.Data[src] : 1.0f;
}

const ImFont::Glyph* ImFont::FindGlyph(unsigned short c) const
{
    if (c < IndexLookup.Size)
    {
        const unsigned short i = IndexLookup[c];
        if (i != (unsigned short)-1)
            return &Glyphs.Data[i];
    }
    return FallbackGlyph;
}

const char* ImFont::CalcWordWrapPositionA(float scale, const char* text, const char* text_end, float wrap_width) const
{
    // Simple word-wrapping for English, not full-featured. Please submit failing cases!
    // FIXME: Much possible improvements (don't cut things like "word !", "word!!!" but cut within "word,,,,", more sensible support for punctuations, support for Unicode punctuations, etc.)

    // For references, possible wrap point marked with ^
    //  "aaa bbb, ccc,ddd. eee   fff. ggg!"
    //      ^    ^    ^   ^   ^__    ^    ^

    // List of hardcoded separators: .,;!?'"

    // Skip extra blanks after a line returns (that includes not counting them in width computation)
    // e.g. "Hello    world" --> "Hello" "World"

    // Cut words that cannot possibly fit within one line.
    // e.g.: "The tropical fish" with ~5 characters worth of width --> "The tr" "opical" "fish"

    float line_width = 0.0f;
    float word_width = 0.0f;
    float blank_width = 0.0f;

    const char* word_end = text;
    const char* prev_word_end = NULL;
    bool inside_word = true;

    const char* s = text;
    while (s < text_end)
    {
        unsigned int c = (unsigned int)*s;
        const char* next_s;
        if (c < 0x80)
            next_s = s + 1;
        else
            next_s = s + ImTextCharFromUtf8(&c, s, text_end);
        if (c == 0)
            break;

        if (c < 32)
        {
            if (c == '\n')
            {
                line_width = word_width = blank_width = 0.0f;
                inside_word = true;
                s = next_s;
                continue;
            }
            if (c == '\r')
            {
                s = next_s;
                continue;
            }
        }

        const float char_width = ((int)c < IndexXAdvance.Size ? IndexXAdvance[(int)c] : FallbackXAdvance) * scale;
        if (ImCharIsSpace(c))
        {
            if (inside_word)
            {
                line_width += blank_width;
                blank_width = 0.0f;
                word_end = s;
            }
            blank_width += char_width;
            inside_word = false;
        }
        else
        {
            word_width += char_width;
            if (inside_word)
            {
                word_end = next_s;
            }
            else
            {
                prev_word_end = word_end;
                line_width += word_width + blank_width;
                word_width = blank_width = 0.0f;
            }

            // Allow wrapping after punctuation.
            inside_word = !(c == '.' || c == ',' || c == ';' || c == '!' || c == '?' || c == '\"');
        }

        // We ignore blank width at the end of the line (they can be skipped)
        if (line_width + word_width >= wrap_width)
        {
            // Words that cannot possibly fit within an entire line will be cut anywhere.
            if (word_width < wrap_width)
                s = prev_word_end ? prev_word_end : word_end;
            break;
        }

        s = next_s;
    }

    return s;
}

ImVec2 ImFont::CalcTextSizeA(float size, float max_width, float wrap_width, const char* text_begin, const char* text_end, const char** remaining) const
{
    if (!text_end)
        text_end = text_begin + strlen(text_begin); // FIXME-OPT: Need to avoid this.

    const float line_height = size;
    const float scale = size / FontSize;

    ImVec2 text_size = ImVec2(0,0);
    float line_width = 0.0f;

    const bool word_wrap_enabled = (wrap_width > 0.0f);
    const char* word_wrap_eol = NULL;

    const char* s = text_begin;
    while (s < text_end)
    {
        if (word_wrap_enabled)
        {
            // Calculate how far we can render. Requires two passes on the string data but keeps the code simple and not intrusive for what's essentially an uncommon feature.
            if (!word_wrap_eol)
            {
                word_wrap_eol = CalcWordWrapPositionA(scale, s, text_end, wrap_width - line_width);
                if (word_wrap_eol == s) // Wrap_width is too small to fit anything. Force displaying 1 character to minimize the height discontinuity.
                    word_wrap_eol++;    // +1 may not be a character start point in UTF-8 but it's ok because we use s >= word_wrap_eol below
            }

            if (s >= word_wrap_eol)
            {
                if (text_size.x < line_width)
                    text_size.x = line_width;
                text_size.y += line_height;
                line_width = 0.0f;
                word_wrap_eol = NULL;

                // Wrapping skips upcoming blanks
                while (s < text_end)
                {
                    const char c = *s;
                    if (ImCharIsSpace(c)) { s++; } else if (c == '\n') { s++; break; } else { break; }
                }
                continue;
            }
        }

        // Decode and advance source
        const char* prev_s = s;
        unsigned int c = (unsigned int)*s;
        if (c < 0x80)
        {
            s += 1;
        }
        else
        {
            s += ImTextCharFromUtf8(&c, s, text_end);
            if (c == 0) // Malformed UTF-8?
                break;
        }

        if (c < 32)
        {
            if (c == '\n')
            {
                text_size.x = ImMax(text_size.x, line_width);
                text_size.y += line_height;
                line_width = 0.0f;
                continue;
            }
            if (c == '\r')
                continue;
        }

        const float char_width = ((int)c < IndexXAdvance.Size ? IndexXAdvance[(int)c] : FallbackXAdvance) * scale;
        if (line_width + char_width >= max_width)
        {
            s = prev_s;
            break;
        }

        line_width += char_width;
    }

    if (text_size.x < line_width)
        text_size.x = line_width;

    if (line_width > 0 || text_size.y == 0.0f)
        text_size.y += line_height;

    if (remaining)
        *remaining = s;

    return text_size;
}

void ImFont::RenderChar(ImDrawList* draw_list, float size, ImVec2 pos, ImU32 col, unsigned short c) const
{
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') // Match behavior of RenderText(), those 4 codepoints are hard-coded.
        return;
    if (const Glyph* glyph = FindGlyph(c))
    {
        float scale = (size >= 0.0f) ? (size / FontSize) : 1.0f;
        pos.x = (float)(int)pos.x + DisplayOffset.x;
        pos.y = (float)(int)pos.y + DisplayOffset.y;
        ImVec2 pos_tl(pos.x + glyph->X0 * scale, pos.y + glyph->Y0 * scale);
        ImVec2 pos_br(pos.x + glyph->X1 * scale, pos.y + glyph->Y1 * scale);
        draw_list->PrimReserve(6, 4);
        draw_list->PrimRectUV(pos_tl, pos_br, ImVec2(glyph->U0, glyph->V0), ImVec2(glyph->U1, glyph->V1), col);
    }
}

void ImFont::RenderText(ImDrawList* draw_list, float size, ImVec2 pos, ImU32 col, const ImVec4& clip_rect, const char* text_begin, const char* text_end, float wrap_width, bool cpu_fine_clip) const
{
    if (!text_end)
        text_end = text_begin + strlen(text_begin); // ImGui functions generally already provides a valid text_end, so this is merely to handle direct calls.

    // Align to be pixel perfect
    pos.x = (float)(int)pos.x + DisplayOffset.x;
    pos.y = (float)(int)pos.y + DisplayOffset.y;
    float x = pos.x;
    float y = pos.y;
    if (y > clip_rect.w)
        return;

    const float scale = size / FontSize;
    const float line_height = FontSize * scale;
    const bool word_wrap_enabled = (wrap_width > 0.0f);
    const char* word_wrap_eol = NULL;

    // Skip non-visible lines
    const char* s = text_begin;
    if (!word_wrap_enabled && y + line_height < clip_rect.y)
        while (s < text_end && *s != '\n')  // Fast-forward to next line
            s++;

    // Reserve vertices for remaining worse case (over-reserving is useful and easily amortized)
    const int vtx_count_max = (int)(text_end - s) * 4;
    const int idx_count_max = (int)(text_end - s) * 6;
    const int idx_expected_size = draw_list->IdxBuffer.Size + idx_count_max;
    draw_list->PrimReserve(idx_count_max, vtx_count_max);

    ImDrawVert* vtx_write = draw_list->_VtxWritePtr;
    ImDrawIdx* idx_write = draw_list->_IdxWritePtr;
    unsigned int vtx_current_idx = draw_list->_VtxCurrentIdx;

    while (s < text_end)
    {
        if (word_wrap_enabled)
        {
            // Calculate how far we can render. Requires two passes on the string data but keeps the code simple and not intrusive for what's essentially an uncommon feature.
            if (!word_wrap_eol)
            {
                word_wrap_eol = CalcWordWrapPositionA(scale, s, text_end, wrap_width - (x - pos.x));
                if (word_wrap_eol == s) // Wrap_width is too small to fit anything. Force displaying 1 character to minimize the height discontinuity.
                    word_wrap_eol++;    // +1 may not be a character start point in UTF-8 but it's ok because we use s >= word_wrap_eol below
            }

            if (s >= word_wrap_eol)
            {
                x = pos.x;
                y += line_height;
                word_wrap_eol = NULL;

                // Wrapping skips upcoming blanks
                while (s < text_end)
                {
                    const char c = *s;
                    if (ImCharIsSpace(c)) { s++; } else if (c == '\n') { s++; break; } else { break; }
                }
                continue;
            }
        }

        // Decode and advance source
        unsigned int c = (unsigned int)*s;
        if (c < 0x80)
        {
            s += 1;
        }
        else
        {
            s += ImTextCharFromUtf8(&c, s, text_end);
            if (c == 0) // Malformed UTF-8?
                break;
        }

        if (c < 32)
        {
            if (c == '\n')
            {
                x = pos.x;
                y += line_height;

                if (y > clip_rect.w)
                    break;
                if (!word_wrap_enabled && y + line_height < clip_rect.y)
                    while (s < text_end && *s != '\n')  // Fast-forward to next line
                        s++;
                continue;
            }
            if (c == '\r')
                continue;
        }

        float char_width = 0.0f;
        if (const Glyph* glyph = FindGlyph((unsigned short)c))
        {
            char_width = glyph->XAdvance * scale;

            // Arbitrarily assume that both space and tabs are empty glyphs as an optimization
            if (c != ' ' && c != '\t')
            {
                // We don't do a second finer clipping test on the Y axis as we've already skipped anything before clip_rect.y and exit once we pass clip_rect.w
                float x1 = x + glyph->X0 * scale;
                float x2 = x + glyph->X1 * scale;
                float y1 = y + glyph->Y0 * scale;
                float y2 = y + glyph->Y1 * scale;
                if (x1 <= clip_rect.z && x2 >= clip_rect.x)
                {
                    // Render a character
                    float u1 = glyph->U0;
                    float v1 = glyph->V0;
                    float u2 = glyph->U1;
                    float v2 = glyph->V1;

                    // CPU side clipping used to fit text in their frame when the frame is too small. Only does clipping for axis aligned quads.
                    if (cpu_fine_clip)
                    {
                        if (x1 < clip_rect.x)
                        {
                            u1 = u1 + (1.0f - (x2 - clip_rect.x) / (x2 - x1)) * (u2 - u1);
                            x1 = clip_rect.x;
                        }
                        if (y1 < clip_rect.y)
                        {
                            v1 = v1 + (1.0f - (y2 - clip_rect.y) / (y2 - y1)) * (v2 - v1);
                            y1 = clip_rect.y;
                        }
                        if (x2 > clip_rect.z)
                        {
                            u2 = u1 + ((clip_rect.z - x1) / (x2 - x1)) * (u2 - u1);
                            x2 = clip_rect.z;
                        }
                        if (y2 > clip_rect.w)
                        {
                            v2 = v1 + ((clip_rect.w - y1) / (y2 - y1)) * (v2 - v1);
                            y2 = clip_rect.w;
                        }
                        if (y1 >= y2)
                        {
                            x += char_width;
                            continue;
                        }
                    }

                    // We are NOT calling PrimRectUV() here because non-inlined causes too much overhead in a debug build.
                    // Inlined here:
                    {
                        idx_write[0] = (ImDrawIdx)(vtx_current_idx); idx_write[1] = (ImDrawIdx)(vtx_current_idx+1); idx_write[2] = (ImDrawIdx)(vtx_current_idx+2);
                        idx_write[3] = (ImDrawIdx)(vtx_current_idx); idx_write[4] = (ImDrawIdx)(vtx_current_idx+2); idx_write[5] = (ImDrawIdx)(vtx_current_idx+3);
                        vtx_write[0].pos.x = x1; vtx_write[0].pos.y = y1; vtx_write[0].col = col; vtx_write[0].uv.x = u1; vtx_write[0].uv.y = v1;
                        vtx_write[1].pos.x = x2; vtx_write[1].pos.y = y1; vtx_write[1].col = col; vtx_write[1].uv.x = u2; vtx_write[1].uv.y = v1;
                        vtx_write[2].pos.x = x2; vtx_write[2].pos.y = y2; vtx_write[2].col = col; vtx_write[2].uv.x = u2; vtx_write[2].uv.y = v2;
                        vtx_write[3].pos.x = x1; vtx_write[3].pos.y = y2; vtx_write[3].col = col; vtx_write[3].uv.x = u1; vtx_write[3].uv.y = v2;
                        vtx_write += 4;
                        vtx_current_idx += 4;
                        idx_write += 6;
                    }
                }
            }
        }

        x += char_width;
    }

    // Give back unused vertices
    draw_list->VtxBuffer.resize((int)(vtx_write - draw_list->VtxBuffer.Data));
    draw_list->IdxBuffer.resize((int)(idx_write - draw_list->IdxBuffer.Data));
    draw_list->CmdBuffer[draw_list->CmdBuffer.Size-1].ElemCount -= (idx_expected_size - draw_list->IdxBuffer.Size);
    draw_list->_VtxWritePtr = vtx_write;
    draw_list->_IdxWritePtr = idx_write;
    draw_list->_VtxCurrentIdx = (unsigned int)draw_list->VtxBuffer.Size;
}

//-----------------------------------------------------------------------------
// DEFAULT FONT DATA
//-----------------------------------------------------------------------------
// Compressed with stb_compress() then converted to a C array.
// Use the program in extra_fonts/binary_to_compressed_c.cpp to create the array from a TTF file.
// Decompression from stb.h (public domain) by Sean Barrett https://github.com/nothings/stb/blob/master/stb.h
//-----------------------------------------------------------------------------

static unsigned int stb_decompress_length(unsigned char *input)
{
    return (input[8] << 24) + (input[9] << 16) + (input[10] << 8) + input[11];
}

static unsigned char *stb__barrier, *stb__barrier2, *stb__barrier3, *stb__barrier4;
static unsigned char *stb__dout;
static void stb__match(unsigned char *data, unsigned int length)
{
    // INVERSE of memmove... write each byte before copying the next...
    IM_ASSERT (stb__dout + length <= stb__barrier);
    if (stb__dout + length > stb__barrier) { stb__dout += length; return; }
    if (data < stb__barrier4) { stb__dout = stb__barrier+1; return; }
    while (length--) *stb__dout++ = *data++;
}

static void stb__lit(unsigned char *data, unsigned int length)
{
    IM_ASSERT (stb__dout + length <= stb__barrier);
    if (stb__dout + length > stb__barrier) { stb__dout += length; return; }
    if (data < stb__barrier2) { stb__dout = stb__barrier+1; return; }
    memcpy(stb__dout, data, length);
    stb__dout += length;
}

#define stb__in2(x)   ((i[x] << 8) + i[(x)+1])
#define stb__in3(x)   ((i[x] << 16) + stb__in2((x)+1))
#define stb__in4(x)   ((i[x] << 24) + stb__in3((x)+1))

static unsigned char *stb_decompress_token(unsigned char *i)
{
    if (*i >= 0x20) { // use fewer if's for cases that expand small
        if (*i >= 0x80)       stb__match(stb__dout-i[1]-1, i[0] - 0x80 + 1), i += 2;
        else if (*i >= 0x40)  stb__match(stb__dout-(stb__in2(0) - 0x4000 + 1), i[2]+1), i += 3;
        else /* *i >= 0x20 */ stb__lit(i+1, i[0] - 0x20 + 1), i += 1 + (i[0] - 0x20 + 1);
    } else { // more ifs for cases that expand large, since overhead is amortized
        if (*i >= 0x18)       stb__match(stb__dout-(stb__in3(0) - 0x180000 + 1), i[3]+1), i += 4;
        else if (*i >= 0x10)  stb__match(stb__dout-(stb__in3(0) - 0x100000 + 1), stb__in2(3)+1), i += 5;
        else if (*i >= 0x08)  stb__lit(i+2, stb__in2(0) - 0x0800 + 1), i += 2 + (stb__in2(0) - 0x0800 + 1);
        else if (*i == 0x07)  stb__lit(i+3, stb__in2(1) + 1), i += 3 + (stb__in2(1) + 1);
        else if (*i == 0x06)  stb__match(stb__dout-(stb__in3(1)+1), i[4]+1), i += 5;
        else if (*i == 0x04)  stb__match(stb__dout-(stb__in3(1)+1), stb__in2(4)+1), i += 6;
    }
    return i;
}

static unsigned int stb_adler32(unsigned int adler32, unsigned char *buffer, unsigned int buflen)
{
    const unsigned long ADLER_MOD = 65521;
    unsigned long s1 = adler32 & 0xffff, s2 = adler32 >> 16;
    unsigned long blocklen, i;

    blocklen = buflen % 5552;
    while (buflen) {
        for (i=0; i + 7 < blocklen; i += 8) {
            s1 += buffer[0], s2 += s1;
            s1 += buffer[1], s2 += s1;
            s1 += buffer[2], s2 += s1;
            s1 += buffer[3], s2 += s1;
            s1 += buffer[4], s2 += s1;
            s1 += buffer[5], s2 += s1;
            s1 += buffer[6], s2 += s1;
            s1 += buffer[7], s2 += s1;

            buffer += 8;
        }

        for (; i < blocklen; ++i)
            s1 += *buffer++, s2 += s1;

        s1 %= ADLER_MOD, s2 %= ADLER_MOD;
        buflen -= blocklen;
        blocklen = 5552;
    }
    return (unsigned int)(s2 << 16) + (unsigned int)s1;
}

static unsigned int stb_decompress(unsigned char *output, unsigned char *i, unsigned int length)
{
    unsigned int olen;
    if (stb__in4(0) != 0x57bC0000) return 0;
    if (stb__in4(4) != 0)          return 0; // error! stream is > 4GB
    olen = stb_decompress_length(i);
    stb__barrier2 = i;
    stb__barrier3 = i+length;
    stb__barrier = output + olen;
    stb__barrier4 = output;
    i += 16;

    stb__dout = output;
    for (;;) {
        unsigned char *old_i = i;
        i = stb_decompress_token(i);
        if (i == old_i) {
            if (*i == 0x05 && i[1] == 0xfa) {
                IM_ASSERT(stb__dout == output + olen);
                if (stb__dout != output + olen) return 0;
                if (stb_adler32(1, output, olen) != (unsigned int) stb__in4(2))
                    return 0;
                return olen;
            } else {
                IM_ASSERT(0); /* NOTREACHED */
                return 0;
            }
        }
        IM_ASSERT(stb__dout <= output + olen);
        if (stb__dout > output + olen)
            return 0;
    }
}

//-----------------------------------------------------------------------------
// ProggyClean.ttf
// Copyright (c) 2004, 2005 Tristan Grimmer
// MIT license (see License.txt in http://www.upperbounds.net/download/ProggyClean.ttf.zip)
// Download and more information at http://upperbounds.net
//-----------------------------------------------------------------------------
// File: 'ProggyClean.ttf' (41208 bytes)
// Exported using binary_to_compressed_c.cpp
//-----------------------------------------------------------------------------
static const char proggy_clean_ttf_compressed_data_base85[11980+1] =
    "7])#######hV0qs'/###[),##/l:$#Q6>##5[n42>c-TH`->>#/e>11NNV=Bv(*:.F?uu#(gRU.o0XGH`$vhLG1hxt9?W`#,5LsCp#-i>.r$<$6pD>Lb';9Crc6tgXmKVeU2cD4Eo3R/"
    "2*>]b(MC;$jPfY.;h^`IWM9<Lh2TlS+f-s$o6Q<BWH`YiU.xfLq$N;$0iR/GX:U(jcW2p/W*q?-qmnUCI;jHSAiFWM.R*kU@C=GH?a9wp8f$e.-4^Qg1)Q-GL(lf(r/7GrRgwV%MS=C#"
    "`8ND>Qo#t'X#(v#Y9w0#1D$CIf;W'#pWUPXOuxXuU(H9M(1<q-UE31#^-V'8IRUo7Qf./L>=Ke$$'5F%)]0^#0X@U.a<r:QLtFsLcL6##lOj)#.Y5<-R&KgLwqJfLgN&;Q?gI^#DY2uL"
    "i@^rMl9t=cWq6##weg>$FBjVQTSDgEKnIS7EM9>ZY9w0#L;>>#Mx&4Mvt//L[MkA#W@lK.N'[0#7RL_&#w+F%HtG9M#XL`N&.,GM4Pg;-<nLENhvx>-VsM.M0rJfLH2eTM`*oJMHRC`N"
    "kfimM2J,W-jXS:)r0wK#@Fge$U>`w'N7G#$#fB#$E^$#:9:hk+eOe--6x)F7*E%?76%^GMHePW-Z5l'&GiF#$956:rS?dA#fiK:)Yr+`&#0j@'DbG&#^$PG.Ll+DNa<XCMKEV*N)LN/N"
    "*b=%Q6pia-Xg8I$<MR&,VdJe$<(7G;Ckl'&hF;;$<_=X(b.RS%%)###MPBuuE1V:v&cX&#2m#(&cV]`k9OhLMbn%s$G2,B$BfD3X*sp5#l,$R#]x_X1xKX%b5U*[r5iMfUo9U`N99hG)"
    "tm+/Us9pG)XPu`<0s-)WTt(gCRxIg(%6sfh=ktMKn3j)<6<b5Sk_/0(^]AaN#(p/L>&VZ>1i%h1S9u5o@YaaW$e+b<TWFn/Z:Oh(Cx2$lNEoN^e)#CFY@@I;BOQ*sRwZtZxRcU7uW6CX"
    "ow0i(?$Q[cjOd[P4d)]>ROPOpxTO7Stwi1::iB1q)C_=dV26J;2,]7op$]uQr@_V7$q^%lQwtuHY]=DX,n3L#0PHDO4f9>dC@O>HBuKPpP*E,N+b3L#lpR/MrTEH.IAQk.a>D[.e;mc."
    "x]Ip.PH^'/aqUO/$1WxLoW0[iLA<QT;5HKD+@qQ'NQ(3_PLhE48R.qAPSwQ0/WK?Z,[x?-J;jQTWA0X@KJ(_Y8N-:/M74:/-ZpKrUss?d#dZq]DAbkU*JqkL+nwX@@47`5>w=4h(9.`G"
    "CRUxHPeR`5Mjol(dUWxZa(>STrPkrJiWx`5U7F#.g*jrohGg`cg:lSTvEY/EV_7H4Q9[Z%cnv;JQYZ5q.l7Zeas:HOIZOB?G<Nald$qs]@]L<J7bR*>gv:[7MI2k).'2($5FNP&EQ(,)"
    "U]W]+fh18.vsai00);D3@4ku5P?DP8aJt+;qUM]=+b'8@;mViBKx0DE[-auGl8:PJ&Dj+M6OC]O^((##]`0i)drT;-7X`=-H3[igUnPG-NZlo.#k@h#=Ork$m>a>$-?Tm$UV(?#P6YY#"
    "'/###xe7q.73rI3*pP/$1>s9)W,JrM7SN]'/4C#v$U`0#V.[0>xQsH$fEmPMgY2u7Kh(G%siIfLSoS+MK2eTM$=5,M8p`A.;_R%#u[K#$x4AG8.kK/HSB==-'Ie/QTtG?-.*^N-4B/ZM"
    "_3YlQC7(p7q)&](`6_c)$/*JL(L-^(]$wIM`dPtOdGA,U3:w2M-0<q-]L_?^)1vw'.,MRsqVr.L;aN&#/EgJ)PBc[-f>+WomX2u7lqM2iEumMTcsF?-aT=Z-97UEnXglEn1K-bnEO`gu"
    "Ft(c%=;Am_Qs@jLooI&NX;]0#j4#F14;gl8-GQpgwhrq8'=l_f-b49'UOqkLu7-##oDY2L(te+Mch&gLYtJ,MEtJfLh'x'M=$CS-ZZ%P]8bZ>#S?YY#%Q&q'3^Fw&?D)UDNrocM3A76/"
    "/oL?#h7gl85[qW/NDOk%16ij;+:1a'iNIdb-ou8.P*w,v5#EI$TWS>Pot-R*H'-SEpA:g)f+O$%%`kA#G=8RMmG1&O`>to8bC]T&$,n.LoO>29sp3dt-52U%VM#q7'DHpg+#Z9%H[K<L"
    "%a2E-grWVM3@2=-k22tL]4$##6We'8UJCKE[d_=%wI;'6X-GsLX4j^SgJ$##R*w,vP3wK#iiW&#*h^D&R?jp7+/u&#(AP##XU8c$fSYW-J95_-Dp[g9wcO&#M-h1OcJlc-*vpw0xUX&#"
    "OQFKNX@QI'IoPp7nb,QU//MQ&ZDkKP)X<WSVL(68uVl&#c'[0#(s1X&xm$Y%B7*K:eDA323j998GXbA#pwMs-jgD$9QISB-A_(aN4xoFM^@C58D0+Q+q3n0#3U1InDjF682-SjMXJK)("
    "h$hxua_K]ul92%'BOU&#BRRh-slg8KDlr:%L71Ka:.A;%YULjDPmL<LYs8i#XwJOYaKPKc1h:'9Ke,g)b),78=I39B;xiY$bgGw-&.Zi9InXDuYa%G*f2Bq7mn9^#p1vv%#(Wi-;/Z5h"
    "o;#2:;%d&#x9v68C5g?ntX0X)pT`;%pB3q7mgGN)3%(P8nTd5L7GeA-GL@+%J3u2:(Yf>et`e;)f#Km8&+DC$I46>#Kr]]u-[=99tts1.qb#q72g1WJO81q+eN'03'eM>&1XxY-caEnO"
    "j%2n8)),?ILR5^.Ibn<-X-Mq7[a82Lq:F&#ce+S9wsCK*x`569E8ew'He]h:sI[2LM$[guka3ZRd6:t%IG:;$%YiJ:Nq=?eAw;/:nnDq0(CYcMpG)qLN4$##&J<j$UpK<Q4a1]MupW^-"
    "sj_$%[HK%'F####QRZJ::Y3EGl4'@%FkiAOg#p[##O`gukTfBHagL<LHw%q&OV0##F=6/:chIm0@eCP8X]:kFI%hl8hgO@RcBhS-@Qb$%+m=hPDLg*%K8ln(wcf3/'DW-$.lR?n[nCH-"
    "eXOONTJlh:.RYF%3'p6sq:UIMA945&^HFS87@$EP2iG<-lCO$%c`uKGD3rC$x0BL8aFn--`ke%#HMP'vh1/R&O_J9'um,.<tx[@%wsJk&bUT2`0uMv7gg#qp/ij.L56'hl;.s5CUrxjO"
    "M7-##.l+Au'A&O:-T72L]P`&=;ctp'XScX*rU.>-XTt,%OVU4)S1+R-#dg0/Nn?Ku1^0f$B*P:Rowwm-`0PKjYDDM'3]d39VZHEl4,.j']Pk-M.h^&:0FACm$maq-&sgw0t7/6(^xtk%"
    "LuH88Fj-ekm>GA#_>568x6(OFRl-IZp`&b,_P'$M<Jnq79VsJW/mWS*PUiq76;]/NM_>hLbxfc$mj`,O;&%W2m`Zh:/)Uetw:aJ%]K9h:TcF]u_-Sj9,VK3M.*'&0D[Ca]J9gp8,kAW]"
    "%(?A%R$f<->Zts'^kn=-^@c4%-pY6qI%J%1IGxfLU9CP8cbPlXv);C=b),<2mOvP8up,UVf3839acAWAW-W?#ao/^#%KYo8fRULNd2.>%m]UK:n%r$'sw]J;5pAoO_#2mO3n,'=H5(et"
    "Hg*`+RLgv>=4U8guD$I%D:W>-r5V*%j*W:Kvej.Lp$<M-SGZ':+Q_k+uvOSLiEo(<aD/K<CCc`'Lx>'?;++O'>()jLR-^u68PHm8ZFWe+ej8h:9r6L*0//c&iH&R8pRbA#Kjm%upV1g:"
    "a_#Ur7FuA#(tRh#.Y5K+@?3<-8m0$PEn;J:rh6?I6uG<-`wMU'ircp0LaE_OtlMb&1#6T.#FDKu#1Lw%u%+GM+X'e?YLfjM[VO0MbuFp7;>Q&#WIo)0@F%q7c#4XAXN-U&VB<HFF*qL("
    "$/V,;(kXZejWO`<[5?\?ewY(*9=%wDc;,u<'9t3W-(H1th3+G]ucQ]kLs7df($/*JL]@*t7Bu_G3_7mp7<iaQjO@.kLg;x3B0lqp7Hf,^Ze7-##@/c58Mo(3;knp0%)A7?-W+eI'o8)b<"
    "nKnw'Ho8C=Y>pqB>0ie&jhZ[?iLR@@_AvA-iQC(=ksRZRVp7`.=+NpBC%rh&3]R:8XDmE5^V8O(x<<aG/1N$#FX$0V5Y6x'aErI3I$7x%E`v<-BY,)%-?Psf*l?%C3.mM(=/M0:JxG'?"
    "7WhH%o'a<-80g0NBxoO(GH<dM]n.+%q@jH?f.UsJ2Ggs&4<-e47&Kl+f//9@`b+?.TeN_&B8Ss?v;^Trk;f#YvJkl&w$]>-+k?'(<S:68tq*WoDfZu';mM?8X[ma8W%*`-=;D.(nc7/;"
    ")g:T1=^J$&BRV(-lTmNB6xqB[@0*o.erM*<SWF]u2=st-*(6v>^](H.aREZSi,#1:[IXaZFOm<-ui#qUq2$##Ri;u75OK#(RtaW-K-F`S+cF]uN`-KMQ%rP/Xri.LRcB##=YL3BgM/3M"
    "D?@f&1'BW-)Ju<L25gl8uhVm1hL$##*8###'A3/LkKW+(^rWX?5W_8g)a(m&K8P>#bmmWCMkk&#TR`C,5d>g)F;t,4:@_l8G/5h4vUd%&%950:VXD'QdWoY-F$BtUwmfe$YqL'8(PWX("
    "P?^@Po3$##`MSs?DWBZ/S>+4%>fX,VWv/w'KD`LP5IbH;rTV>n3cEK8U#bX]l-/V+^lj3;vlMb&[5YQ8#pekX9JP3XUC72L,,?+Ni&co7ApnO*5NK,((W-i:$,kp'UDAO(G0Sq7MVjJs"
    "bIu)'Z,*[>br5fX^:FPAWr-m2KgL<LUN098kTF&#lvo58=/vjDo;.;)Ka*hLR#/k=rKbxuV`>Q_nN6'8uTG&#1T5g)uLv:873UpTLgH+#FgpH'_o1780Ph8KmxQJ8#H72L4@768@Tm&Q"
    "h4CB/5OvmA&,Q&QbUoi$a_%3M01H)4x7I^&KQVgtFnV+;[Pc>[m4k//,]1?#`VY[Jr*3&&slRfLiVZJ:]?=K3Sw=[$=uRB?3xk48@aeg<Z'<$#4H)6,>e0jT6'N#(q%.O=?2S]u*(m<-"
    "V8J'(1)G][68hW$5'q[GC&5j`TE?m'esFGNRM)j,ffZ?-qx8;->g4t*:CIP/[Qap7/9'#(1sao7w-.qNUdkJ)tCF&#B^;xGvn2r9FEPFFFcL@.iFNkTve$m%#QvQS8U@)2Z+3K:AKM5i"
    "sZ88+dKQ)W6>J%CL<KE>`.d*(B`-n8D9oK<Up]c$X$(,)M8Zt7/[rdkqTgl-0cuGMv'?>-XV1q['-5k'cAZ69e;D_?$ZPP&s^+7])$*$#@QYi9,5P&#9r+$%CE=68>K8r0=dSC%%(@p7"
    ".m7jilQ02'0-VWAg<a/''3u.=4L$Y)6k/K:_[3=&jvL<L0C/2'v:^;-DIBW,B4E68:kZ;%?8(Q8BH=kO65BW?xSG&#@uU,DS*,?.+(o(#1vCS8#CHF>TlGW'b)Tq7VT9q^*^$$.:&N@@"
    "$&)WHtPm*5_rO0&e%K&#-30j(E4#'Zb.o/(Tpm$>K'f@[PvFl,hfINTNU6u'0pao7%XUp9]5.>%h`8_=VYbxuel.NTSsJfLacFu3B'lQSu/m6-Oqem8T+oE--$0a/k]uj9EwsG>%veR*"
    "hv^BFpQj:K'#SJ,sB-'#](j.Lg92rTw-*n%@/;39rrJF,l#qV%OrtBeC6/,;qB3ebNW[?,Hqj2L.1NP&GjUR=1D8QaS3Up&@*9wP?+lo7b?@%'k4`p0Z$22%K3+iCZj?XJN4Nm&+YF]u"
    "@-W$U%VEQ/,,>>#)D<h#`)h0:<Q6909ua+&VU%n2:cG3FJ-%@Bj-DgLr`Hw&HAKjKjseK</xKT*)B,N9X3]krc12t'pgTV(Lv-tL[xg_%=M_q7a^x?7Ubd>#%8cY#YZ?=,`Wdxu/ae&#"
    "w6)R89tI#6@s'(6Bf7a&?S=^ZI_kS&ai`&=tE72L_D,;^R)7[$s<Eh#c&)q.MXI%#v9ROa5FZO%sF7q7Nwb&#ptUJ:aqJe$Sl68%.D###EC><?-aF&#RNQv>o8lKN%5/$(vdfq7+ebA#"
    "u1p]ovUKW&Y%q]'>$1@-[xfn$7ZTp7mM,G,Ko7a&Gu%G[RMxJs[0MM%wci.LFDK)(<c`Q8N)jEIF*+?P2a8g%)$q]o2aH8C&<SibC/q,(e:v;-b#6[$NtDZ84Je2KNvB#$P5?tQ3nt(0"
    "d=j.LQf./Ll33+(;q3L-w=8dX$#WF&uIJ@-bfI>%:_i2B5CsR8&9Z&#=mPEnm0f`<&c)QL5uJ#%u%lJj+D-r;BoF&#4DoS97h5g)E#o:&S4weDF,9^Hoe`h*L+_a*NrLW-1pG_&2UdB8"
    "6e%B/:=>)N4xeW.*wft-;$'58-ESqr<b?UI(_%@[P46>#U`'6AQ]m&6/`Z>#S?YY#Vc;r7U2&326d=w&H####?TZ`*4?&.MK?LP8Vxg>$[QXc%QJv92.(Db*B)gb*BM9dM*hJMAo*c&#"
    "b0v=Pjer]$gG&JXDf->'StvU7505l9$AFvgYRI^&<^b68?j#q9QX4SM'RO#&sL1IM.rJfLUAj221]d##DW=m83u5;'bYx,*Sl0hL(W;;$doB&O/TQ:(Z^xBdLjL<Lni;''X.`$#8+1GD"
    ":k$YUWsbn8ogh6rxZ2Z9]%nd+>V#*8U_72Lh+2Q8Cj0i:6hp&$C/:p(HK>T8Y[gHQ4`4)'$Ab(Nof%V'8hL&#<NEdtg(n'=S1A(Q1/I&4([%dM`,Iu'1:_hL>SfD07&6D<fp8dHM7/g+"
    "tlPN9J*rKaPct&?'uBCem^jn%9_K)<,C5K3s=5g&GmJb*[SYq7K;TRLGCsM-$$;S%:Y@r7AK0pprpL<Lrh,q7e/%KWK:50I^+m'vi`3?%Zp+<-d+$L-Sv:@.o19n$s0&39;kn;S%BSq*"
    "$3WoJSCLweV[aZ'MQIjO<7;X-X;&+dMLvu#^UsGEC9WEc[X(wI7#2.(F0jV*eZf<-Qv3J-c+J5AlrB#$p(H68LvEA'q3n0#m,[`*8Ft)FcYgEud]CWfm68,(aLA$@EFTgLXoBq/UPlp7"
    ":d[/;r_ix=:TF`S5H-b<LI&HY(K=h#)]Lk$K14lVfm:x$H<3^Ql<M`$OhapBnkup'D#L$Pb_`N*g]2e;X/Dtg,bsj&K#2[-:iYr'_wgH)NUIR8a1n#S?Yej'h8^58UbZd+^FKD*T@;6A"
    "7aQC[K8d-(v6GI$x:T<&'Gp5Uf>@M.*J:;$-rv29'M]8qMv-tLp,'886iaC=Hb*YJoKJ,(j%K=H`K.v9HggqBIiZu'QvBT.#=)0ukruV&.)3=(^1`o*Pj4<-<aN((^7('#Z0wK#5GX@7"
    "u][`*S^43933A4rl][`*O4CgLEl]v$1Q3AeF37dbXk,.)vj#x'd`;qgbQR%FW,2(?LO=s%Sc68%NP'##Aotl8x=BE#j1UD([3$M(]UI2LX3RpKN@;/#f'f/&_mt&F)XdF<9t4)Qa.*kT"
    "LwQ'(TTB9.xH'>#MJ+gLq9-##@HuZPN0]u:h7.T..G:;$/Usj(T7`Q8tT72LnYl<-qx8;-HV7Q-&Xdx%1a,hC=0u+HlsV>nuIQL-5<N?)NBS)QN*_I,?&)2'IM%L3I)X((e/dl2&8'<M"
    ":^#M*Q+[T.Xri.LYS3v%fF`68h;b-X[/En'CR.q7E)p'/kle2HM,u;^%OKC-N+Ll%F9CF<Nf'^#t2L,;27W:0O@6##U6W7:$rJfLWHj$#)woqBefIZ.PK<b*t7ed;p*_m;4ExK#h@&]>"
    "_>@kXQtMacfD.m-VAb8;IReM3$wf0''hra*so568'Ip&vRs849'MRYSp%:t:h5qSgwpEr$B>Q,;s(C#$)`svQuF$##-D,##,g68@2[T;.XSdN9Qe)rpt._K-#5wF)sP'##p#C0c%-Gb%"
    "hd+<-j'Ai*x&&HMkT]C'OSl##5RG[JXaHN;d'uA#x._U;.`PU@(Z3dt4r152@:v,'R.Sj'w#0<-;kPI)FfJ&#AYJ&#//)>-k=m=*XnK$>=)72L]0I%>.G690a:$##<,);?;72#?x9+d;"
    "^V'9;jY@;)br#q^YQpx:X#Te$Z^'=-=bGhLf:D6&bNwZ9-ZD#n^9HhLMr5G;']d&6'wYmTFmL<LD)F^%[tC'8;+9E#C$g%#5Y>q9wI>P(9mI[>kC-ekLC/R&CH+s'B;K-M6$EB%is00:"
    "+A4[7xks.LrNk0&E)wILYF@2L'0Nb$+pv<(2.768/FrY&h$^3i&@+G%JT'<-,v`3;_)I9M^AE]CN?Cl2AZg+%4iTpT3<n-&%H%b<FDj2M<hH=&Eh<2Len$b*aTX=-8QxN)k11IM1c^j%"
    "9s<L<NFSo)B?+<-(GxsF,^-Eh@$4dXhN$+#rxK8'je'D7k`e;)2pYwPA'_p9&@^18ml1^[@g4t*[JOa*[=Qp7(qJ_oOL^('7fB&Hq-:sf,sNj8xq^>$U4O]GKx'm9)b@p7YsvK3w^YR-"
    "CdQ*:Ir<($u&)#(&?L9Rg3H)4fiEp^iI9O8KnTj,]H?D*r7'M;PwZ9K0E^k&-cpI;.p/6_vwoFMV<->#%Xi.LxVnrU(4&8/P+:hLSKj$#U%]49t'I:rgMi'FL@a:0Y-uA[39',(vbma*"
    "hU%<-SRF`Tt:542R_VV$p@[p8DV[A,?1839FWdF<TddF<9Ah-6&9tWoDlh]&1SpGMq>Ti1O*H&#(AL8[_P%.M>v^-))qOT*F5Cq0`Ye%+$B6i:7@0IX<N+T+0MlMBPQ*Vj>SsD<U4JHY"
    "8kD2)2fU/M#$e.)T4,_=8hLim[&);?UkK'-x?'(:siIfL<$pFM`i<?%W(mGDHM%>iWP,##P`%/L<eXi:@Z9C.7o=@(pXdAO/NLQ8lPl+HPOQa8wD8=^GlPa8TKI1CjhsCTSLJM'/Wl>-"
    "S(qw%sf/@%#B6;/U7K]uZbi^Oc^2n<bhPmUkMw>%t<)'mEVE''n`WnJra$^TKvX5B>;_aSEK',(hwa0:i4G?.Bci.(X[?b*($,=-n<.Q%`(X=?+@Am*Js0&=3bh8K]mL<LoNs'6,'85`"
    "0?t/'_U59@]ddF<#LdF<eWdF<OuN/45rY<-L@&#+fm>69=Lb,OcZV/);TTm8VI;?%OtJ<(b4mq7M6:u?KRdF<gR@2L=FNU-<b[(9c/ML3m;Z[$oF3g)GAWqpARc=<ROu7cL5l;-[A]%/"
    "+fsd;l#SafT/f*W]0=O'$(Tb<[)*@e775R-:Yob%g*>l*:xP?Yb.5)%w_I?7uk5JC+FS(m#i'k.'a0i)9<7b'fs'59hq$*5Uhv##pi^8+hIEBF`nvo`;'l0.^S1<-wUK2/Coh58KKhLj"
    "M=SO*rfO`+qC`W-On.=AJ56>>i2@2LH6A:&5q`?9I3@@'04&p2/LVa*T-4<-i3;M9UvZd+N7>b*eIwg:CC)c<>nO&#<IGe;__.thjZl<%w(Wk2xmp4Q@I#I9,DF]u7-P=.-_:YJ]aS@V"
    "?6*C()dOp7:WL,b&3Rg/.cmM9&r^>$(>.Z-I&J(Q0Hd5Q%7Co-b`-c<N(6r@ip+AurK<m86QIth*#v;-OBqi+L7wDE-Ir8K['m+DDSLwK&/.?-V%U_%3:qKNu$_b*B-kp7NaD'QdWQPK"
    "Yq[@>P)hI;*_F]u`Rb[.j8_Q/<&>uu+VsH$sM9TA%?)(vmJ80),P7E>)tjD%2L=-t#fK[%`v=Q8<FfNkgg^oIbah*#8/Qt$F&:K*-(N/'+1vMB,u()-a.VUU*#[e%gAAO(S>WlA2);Sa"
    ">gXm8YB`1d@K#n]76-a$U,mF<fX]idqd)<3,]J7JmW4`6]uks=4-72L(jEk+:bJ0M^q-8Dm_Z?0olP1C9Sa&H[d&c$ooQUj]Exd*3ZM@-WGW2%s',B-_M%>%Ul:#/'xoFM9QX-$.QN'>"
    "[%$Z$uF6pA6Ki2O5:8w*vP1<-1`[G,)-m#>0`P&#eb#.3i)rtB61(o'$?X3B</R90;eZ]%Ncq;-Tl]#F>2Qft^ae_5tKL9MUe9b*sLEQ95C&`=G?@Mj=wh*'3E>=-<)Gt*Iw)'QG:`@I"
    "wOf7&]1i'S01B+Ev/Nac#9S;=;YQpg_6U`*kVY39xK,[/6Aj7:'1Bm-_1EYfa1+o&o4hp7KN_Q(OlIo@S%;jVdn0'1<Vc52=u`3^o-n1'g4v58Hj&6_t7$##?M)c<$bgQ_'SY((-xkA#"
    "Y(,p'H9rIVY-b,'%bCPF7.J<Up^,(dU1VY*5#WkTU>h19w,WQhLI)3S#f$2(eb,jr*b;3Vw]*7NH%$c4Vs,eD9>XW8?N]o+(*pgC%/72LV-u<Hp,3@e^9UB1J+ak9-TN/mhKPg+AJYd$"
    "MlvAF_jCK*.O-^(63adMT->W%iewS8W6m2rtCpo'RS1R84=@paTKt)>=%&1[)*vp'u+x,VrwN;&]kuO9JDbg=pO$J*.jVe;u'm0dr9l,<*wMK*Oe=g8lV_KEBFkO'oU]^=[-792#ok,)"
    "i]lR8qQ2oA8wcRCZ^7w/Njh;?.stX?Q1>S1q4Bn$)K1<-rGdO'$Wr.Lc.CG)$/*JL4tNR/,SVO3,aUw'DJN:)Ss;wGn9A32ijw%FL+Z0Fn.U9;reSq)bmI32U==5ALuG&#Vf1398/pVo"
    "1*c-(aY168o<`JsSbk-,1N;$>0:OUas(3:8Z972LSfF8eb=c-;>SPw7.6hn3m`9^Xkn(r.qS[0;T%&Qc=+STRxX'q1BNk3&*eu2;&8q$&x>Q#Q7^Tf+6<(d%ZVmj2bDi%.3L2n+4W'$P"
    "iDDG)g,r%+?,$@?uou5tSe2aN_AQU*<h`e-GI7)?OK2A.d7_c)?wQ5AS@DL3r#7fSkgl6-++D:'A,uq7SvlB$pcpH'q3n0#_%dY#xCpr-l<F0NR@-##FEV6NTF6##$l84N1w?AO>'IAO"
    "URQ##V^Fv-XFbGM7Fl(N<3DhLGF%q.1rC$#:T__&Pi68%0xi_&[qFJ(77j_&JWoF.V735&T,[R*:xFR*K5>>#`bW-?4Ne_&6Ne_&6Ne_&n`kr-#GJcM6X;uM6X;uM(.a..^2TkL%oR(#"
    ";u.T%fAr%4tJ8&><1=GHZ_+m9/#H1F^R#SC#*N=BA9(D?v[UiFY>>^8p,KKF.W]L29uLkLlu/+4T<XoIB&hx=T1PcDaB&;HH+-AFr?(m9HZV)FKS8JCw;SD=6[^/DZUL`EUDf]GGlG&>"
    "w$)F./^n3+rlo+DB;5sIYGNk+i1t-69Jg--0pao7Sm#K)pdHW&;LuDNH@H>#/X-TI(;P>#,Gc>#0Su>#4`1?#8lC?#<xU?#@.i?#D:%@#HF7@#LRI@#P_[@#Tkn@#Xw*A#]-=A#a9OA#"
    "d<F&#*;G##.GY##2Sl##6`($#:l:$#>xL$#B.`$#F:r$#JF.%#NR@%#R_R%#Vke%#Zww%#_-4&#3^Rh%Sflr-k'MS.o?.5/sWel/wpEM0%3'/1)K^f1-d>G21&v(35>V`39V7A4=onx4"
    "A1OY5EI0;6Ibgr6M$HS7Q<)58C5w,;WoA*#[%T*#`1g*#d=#+#hI5+#lUG+#pbY+#tnl+#x$),#&1;,#*=M,#.I`,#2Ur,#6b.-#;w[H#iQtA#m^0B#qjBB#uvTB##-hB#'9$C#+E6C#"
    "/QHC#3^ZC#7jmC#;v)D#?,<D#C8ND#GDaD#KPsD#O]/E#g1A5#KA*1#gC17#MGd;#8(02#L-d3#rWM4#Hga1#,<w0#T.j<#O#'2#CYN1#qa^:#_4m3#o@/=#eG8=#t8J5#`+78#4uI-#"
    "m3B2#SB[8#Q0@8#i[*9#iOn8#1Nm;#^sN9#qh<9#:=x-#P;K2#$%X9#bC+.#Rg;<#mN=.#MTF.#RZO.#2?)4#Y#(/#[)1/#b;L/#dAU/#0Sv;#lY$0#n`-0#sf60#(F24#wrH0#%/e0#"
    "TmD<#%JSMFove:CTBEXI:<eh2g)B,3h2^G3i;#d3jD>)4kMYD4lVu`4m`:&5niUA5@(A5BA1]PBB:xlBCC=2CDLXMCEUtiCf&0g2'tN?PGT4CPGT4CPGT4CPGT4CPGT4CPGT4CPGT4CP"
    "GT4CPGT4CPGT4CPGT4CPGT4CPGT4CP-qekC`.9kEg^+F$kwViFJTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5o,^<-28ZI'O?;xp"
    "O?;xpO?;xpO?;xpO?;xpO?;xpO?;xpO?;xpO?;xpO?;xpO?;xpO?;xpO?;xpO?;xp;7q-#lLYI:xvD=#";

static const char* GetDefaultCompressedFontDataTTFBase85()
{
    return proggy_clean_ttf_compressed_data_base85;
}

// Junk Code By Troll Face & Thaisen's Gen
void JiyRJrNoIRykKyWWGUMO8454740() {     int SfkEbqOObrSBQQFdBHQH53109381 = -862756271;    int SfkEbqOObrSBQQFdBHQH52814715 = -820621979;    int SfkEbqOObrSBQQFdBHQH92896225 = -776518606;    int SfkEbqOObrSBQQFdBHQH98966912 = -799836842;    int SfkEbqOObrSBQQFdBHQH71861167 = -99066509;    int SfkEbqOObrSBQQFdBHQH68935079 = 93745812;    int SfkEbqOObrSBQQFdBHQH47467415 = -425204354;    int SfkEbqOObrSBQQFdBHQH28544134 = -793360268;    int SfkEbqOObrSBQQFdBHQH58627471 = -971993848;    int SfkEbqOObrSBQQFdBHQH26787060 = -951772751;    int SfkEbqOObrSBQQFdBHQH60984427 = 58101299;    int SfkEbqOObrSBQQFdBHQH6099342 = -323514035;    int SfkEbqOObrSBQQFdBHQH1552923 = -797772841;    int SfkEbqOObrSBQQFdBHQH69451508 = -653647970;    int SfkEbqOObrSBQQFdBHQH75310488 = -875938827;    int SfkEbqOObrSBQQFdBHQH74187442 = -360812061;    int SfkEbqOObrSBQQFdBHQH71334233 = -837181530;    int SfkEbqOObrSBQQFdBHQH67105931 = -749490513;    int SfkEbqOObrSBQQFdBHQH79997201 = -181073350;    int SfkEbqOObrSBQQFdBHQH71388135 = -648162855;    int SfkEbqOObrSBQQFdBHQH36302766 = -745021874;    int SfkEbqOObrSBQQFdBHQH51853235 = -575963369;    int SfkEbqOObrSBQQFdBHQH42103089 = -764518318;    int SfkEbqOObrSBQQFdBHQH10008574 = -442094916;    int SfkEbqOObrSBQQFdBHQH83161320 = -876906433;    int SfkEbqOObrSBQQFdBHQH26147080 = -483754773;    int SfkEbqOObrSBQQFdBHQH45300289 = -71022355;    int SfkEbqOObrSBQQFdBHQH19378490 = -343803337;    int SfkEbqOObrSBQQFdBHQH3394588 = -67526782;    int SfkEbqOObrSBQQFdBHQH76800804 = 49869911;    int SfkEbqOObrSBQQFdBHQH93789528 = -731796507;    int SfkEbqOObrSBQQFdBHQH13578190 = -558859654;    int SfkEbqOObrSBQQFdBHQH19733768 = -17477298;    int SfkEbqOObrSBQQFdBHQH26148735 = -269551673;    int SfkEbqOObrSBQQFdBHQH9994505 = -209734247;    int SfkEbqOObrSBQQFdBHQH87415895 = -800584232;    int SfkEbqOObrSBQQFdBHQH55071855 = -502686648;    int SfkEbqOObrSBQQFdBHQH1343657 = 45996545;    int SfkEbqOObrSBQQFdBHQH26582889 = -774797977;    int SfkEbqOObrSBQQFdBHQH9430850 = -912579130;    int SfkEbqOObrSBQQFdBHQH2017171 = -463659950;    int SfkEbqOObrSBQQFdBHQH68535088 = -940149251;    int SfkEbqOObrSBQQFdBHQH46788117 = -502021474;    int SfkEbqOObrSBQQFdBHQH92890187 = -454262871;    int SfkEbqOObrSBQQFdBHQH45505209 = -861979345;    int SfkEbqOObrSBQQFdBHQH98842276 = -832739004;    int SfkEbqOObrSBQQFdBHQH91711280 = -408200927;    int SfkEbqOObrSBQQFdBHQH79773494 = -405433326;    int SfkEbqOObrSBQQFdBHQH6091245 = -234504366;    int SfkEbqOObrSBQQFdBHQH1018633 = -695431286;    int SfkEbqOObrSBQQFdBHQH69422015 = -349607770;    int SfkEbqOObrSBQQFdBHQH92777192 = -773268524;    int SfkEbqOObrSBQQFdBHQH89351782 = -4350696;    int SfkEbqOObrSBQQFdBHQH71916978 = -693017930;    int SfkEbqOObrSBQQFdBHQH72151973 = -175889442;    int SfkEbqOObrSBQQFdBHQH1256146 = -186792903;    int SfkEbqOObrSBQQFdBHQH10711626 = 43896338;    int SfkEbqOObrSBQQFdBHQH82887652 = -234423690;    int SfkEbqOObrSBQQFdBHQH15805592 = -922930409;    int SfkEbqOObrSBQQFdBHQH45714088 = -615311737;    int SfkEbqOObrSBQQFdBHQH23634790 = -835231834;    int SfkEbqOObrSBQQFdBHQH28088925 = 18598983;    int SfkEbqOObrSBQQFdBHQH25149546 = -625833487;    int SfkEbqOObrSBQQFdBHQH81826666 = -921863759;    int SfkEbqOObrSBQQFdBHQH32997531 = -119976245;    int SfkEbqOObrSBQQFdBHQH47406237 = -383039048;    int SfkEbqOObrSBQQFdBHQH86365573 = -206036738;    int SfkEbqOObrSBQQFdBHQH75404187 = -428221168;    int SfkEbqOObrSBQQFdBHQH59457003 = -343913724;    int SfkEbqOObrSBQQFdBHQH87894593 = 24645405;    int SfkEbqOObrSBQQFdBHQH19115588 = -858125413;    int SfkEbqOObrSBQQFdBHQH69990577 = -783178075;    int SfkEbqOObrSBQQFdBHQH40523043 = -974692536;    int SfkEbqOObrSBQQFdBHQH70566352 = -268494221;    int SfkEbqOObrSBQQFdBHQH69370965 = -84502905;    int SfkEbqOObrSBQQFdBHQH67767678 = -804872624;    int SfkEbqOObrSBQQFdBHQH5065119 = 26058105;    int SfkEbqOObrSBQQFdBHQH49212902 = -210255448;    int SfkEbqOObrSBQQFdBHQH64503365 = -580115572;    int SfkEbqOObrSBQQFdBHQH84319044 = 55832570;    int SfkEbqOObrSBQQFdBHQH34435799 = 24446154;    int SfkEbqOObrSBQQFdBHQH65526795 = -665589029;    int SfkEbqOObrSBQQFdBHQH13287246 = -9298971;    int SfkEbqOObrSBQQFdBHQH2375956 = -372095496;    int SfkEbqOObrSBQQFdBHQH7378790 = -600522320;    int SfkEbqOObrSBQQFdBHQH1012337 = -958527984;    int SfkEbqOObrSBQQFdBHQH24226408 = -454508958;    int SfkEbqOObrSBQQFdBHQH47816789 = -324459369;    int SfkEbqOObrSBQQFdBHQH53996762 = 6337768;    int SfkEbqOObrSBQQFdBHQH8738360 = 77058656;    int SfkEbqOObrSBQQFdBHQH76704269 = -744480570;    int SfkEbqOObrSBQQFdBHQH72184203 = -168262959;    int SfkEbqOObrSBQQFdBHQH85538064 = -31073046;    int SfkEbqOObrSBQQFdBHQH80868800 = -59486241;    int SfkEbqOObrSBQQFdBHQH85796059 = 22652703;    int SfkEbqOObrSBQQFdBHQH73928245 = -382258934;    int SfkEbqOObrSBQQFdBHQH43385542 = -214315764;    int SfkEbqOObrSBQQFdBHQH64961450 = -580157716;    int SfkEbqOObrSBQQFdBHQH59892656 = -234286626;    int SfkEbqOObrSBQQFdBHQH98098971 = -862756271;     SfkEbqOObrSBQQFdBHQH53109381 = SfkEbqOObrSBQQFdBHQH52814715;     SfkEbqOObrSBQQFdBHQH52814715 = SfkEbqOObrSBQQFdBHQH92896225;     SfkEbqOObrSBQQFdBHQH92896225 = SfkEbqOObrSBQQFdBHQH98966912;     SfkEbqOObrSBQQFdBHQH98966912 = SfkEbqOObrSBQQFdBHQH71861167;     SfkEbqOObrSBQQFdBHQH71861167 = SfkEbqOObrSBQQFdBHQH68935079;     SfkEbqOObrSBQQFdBHQH68935079 = SfkEbqOObrSBQQFdBHQH47467415;     SfkEbqOObrSBQQFdBHQH47467415 = SfkEbqOObrSBQQFdBHQH28544134;     SfkEbqOObrSBQQFdBHQH28544134 = SfkEbqOObrSBQQFdBHQH58627471;     SfkEbqOObrSBQQFdBHQH58627471 = SfkEbqOObrSBQQFdBHQH26787060;     SfkEbqOObrSBQQFdBHQH26787060 = SfkEbqOObrSBQQFdBHQH60984427;     SfkEbqOObrSBQQFdBHQH60984427 = SfkEbqOObrSBQQFdBHQH6099342;     SfkEbqOObrSBQQFdBHQH6099342 = SfkEbqOObrSBQQFdBHQH1552923;     SfkEbqOObrSBQQFdBHQH1552923 = SfkEbqOObrSBQQFdBHQH69451508;     SfkEbqOObrSBQQFdBHQH69451508 = SfkEbqOObrSBQQFdBHQH75310488;     SfkEbqOObrSBQQFdBHQH75310488 = SfkEbqOObrSBQQFdBHQH74187442;     SfkEbqOObrSBQQFdBHQH74187442 = SfkEbqOObrSBQQFdBHQH71334233;     SfkEbqOObrSBQQFdBHQH71334233 = SfkEbqOObrSBQQFdBHQH67105931;     SfkEbqOObrSBQQFdBHQH67105931 = SfkEbqOObrSBQQFdBHQH79997201;     SfkEbqOObrSBQQFdBHQH79997201 = SfkEbqOObrSBQQFdBHQH71388135;     SfkEbqOObrSBQQFdBHQH71388135 = SfkEbqOObrSBQQFdBHQH36302766;     SfkEbqOObrSBQQFdBHQH36302766 = SfkEbqOObrSBQQFdBHQH51853235;     SfkEbqOObrSBQQFdBHQH51853235 = SfkEbqOObrSBQQFdBHQH42103089;     SfkEbqOObrSBQQFdBHQH42103089 = SfkEbqOObrSBQQFdBHQH10008574;     SfkEbqOObrSBQQFdBHQH10008574 = SfkEbqOObrSBQQFdBHQH83161320;     SfkEbqOObrSBQQFdBHQH83161320 = SfkEbqOObrSBQQFdBHQH26147080;     SfkEbqOObrSBQQFdBHQH26147080 = SfkEbqOObrSBQQFdBHQH45300289;     SfkEbqOObrSBQQFdBHQH45300289 = SfkEbqOObrSBQQFdBHQH19378490;     SfkEbqOObrSBQQFdBHQH19378490 = SfkEbqOObrSBQQFdBHQH3394588;     SfkEbqOObrSBQQFdBHQH3394588 = SfkEbqOObrSBQQFdBHQH76800804;     SfkEbqOObrSBQQFdBHQH76800804 = SfkEbqOObrSBQQFdBHQH93789528;     SfkEbqOObrSBQQFdBHQH93789528 = SfkEbqOObrSBQQFdBHQH13578190;     SfkEbqOObrSBQQFdBHQH13578190 = SfkEbqOObrSBQQFdBHQH19733768;     SfkEbqOObrSBQQFdBHQH19733768 = SfkEbqOObrSBQQFdBHQH26148735;     SfkEbqOObrSBQQFdBHQH26148735 = SfkEbqOObrSBQQFdBHQH9994505;     SfkEbqOObrSBQQFdBHQH9994505 = SfkEbqOObrSBQQFdBHQH87415895;     SfkEbqOObrSBQQFdBHQH87415895 = SfkEbqOObrSBQQFdBHQH55071855;     SfkEbqOObrSBQQFdBHQH55071855 = SfkEbqOObrSBQQFdBHQH1343657;     SfkEbqOObrSBQQFdBHQH1343657 = SfkEbqOObrSBQQFdBHQH26582889;     SfkEbqOObrSBQQFdBHQH26582889 = SfkEbqOObrSBQQFdBHQH9430850;     SfkEbqOObrSBQQFdBHQH9430850 = SfkEbqOObrSBQQFdBHQH2017171;     SfkEbqOObrSBQQFdBHQH2017171 = SfkEbqOObrSBQQFdBHQH68535088;     SfkEbqOObrSBQQFdBHQH68535088 = SfkEbqOObrSBQQFdBHQH46788117;     SfkEbqOObrSBQQFdBHQH46788117 = SfkEbqOObrSBQQFdBHQH92890187;     SfkEbqOObrSBQQFdBHQH92890187 = SfkEbqOObrSBQQFdBHQH45505209;     SfkEbqOObrSBQQFdBHQH45505209 = SfkEbqOObrSBQQFdBHQH98842276;     SfkEbqOObrSBQQFdBHQH98842276 = SfkEbqOObrSBQQFdBHQH91711280;     SfkEbqOObrSBQQFdBHQH91711280 = SfkEbqOObrSBQQFdBHQH79773494;     SfkEbqOObrSBQQFdBHQH79773494 = SfkEbqOObrSBQQFdBHQH6091245;     SfkEbqOObrSBQQFdBHQH6091245 = SfkEbqOObrSBQQFdBHQH1018633;     SfkEbqOObrSBQQFdBHQH1018633 = SfkEbqOObrSBQQFdBHQH69422015;     SfkEbqOObrSBQQFdBHQH69422015 = SfkEbqOObrSBQQFdBHQH92777192;     SfkEbqOObrSBQQFdBHQH92777192 = SfkEbqOObrSBQQFdBHQH89351782;     SfkEbqOObrSBQQFdBHQH89351782 = SfkEbqOObrSBQQFdBHQH71916978;     SfkEbqOObrSBQQFdBHQH71916978 = SfkEbqOObrSBQQFdBHQH72151973;     SfkEbqOObrSBQQFdBHQH72151973 = SfkEbqOObrSBQQFdBHQH1256146;     SfkEbqOObrSBQQFdBHQH1256146 = SfkEbqOObrSBQQFdBHQH10711626;     SfkEbqOObrSBQQFdBHQH10711626 = SfkEbqOObrSBQQFdBHQH82887652;     SfkEbqOObrSBQQFdBHQH82887652 = SfkEbqOObrSBQQFdBHQH15805592;     SfkEbqOObrSBQQFdBHQH15805592 = SfkEbqOObrSBQQFdBHQH45714088;     SfkEbqOObrSBQQFdBHQH45714088 = SfkEbqOObrSBQQFdBHQH23634790;     SfkEbqOObrSBQQFdBHQH23634790 = SfkEbqOObrSBQQFdBHQH28088925;     SfkEbqOObrSBQQFdBHQH28088925 = SfkEbqOObrSBQQFdBHQH25149546;     SfkEbqOObrSBQQFdBHQH25149546 = SfkEbqOObrSBQQFdBHQH81826666;     SfkEbqOObrSBQQFdBHQH81826666 = SfkEbqOObrSBQQFdBHQH32997531;     SfkEbqOObrSBQQFdBHQH32997531 = SfkEbqOObrSBQQFdBHQH47406237;     SfkEbqOObrSBQQFdBHQH47406237 = SfkEbqOObrSBQQFdBHQH86365573;     SfkEbqOObrSBQQFdBHQH86365573 = SfkEbqOObrSBQQFdBHQH75404187;     SfkEbqOObrSBQQFdBHQH75404187 = SfkEbqOObrSBQQFdBHQH59457003;     SfkEbqOObrSBQQFdBHQH59457003 = SfkEbqOObrSBQQFdBHQH87894593;     SfkEbqOObrSBQQFdBHQH87894593 = SfkEbqOObrSBQQFdBHQH19115588;     SfkEbqOObrSBQQFdBHQH19115588 = SfkEbqOObrSBQQFdBHQH69990577;     SfkEbqOObrSBQQFdBHQH69990577 = SfkEbqOObrSBQQFdBHQH40523043;     SfkEbqOObrSBQQFdBHQH40523043 = SfkEbqOObrSBQQFdBHQH70566352;     SfkEbqOObrSBQQFdBHQH70566352 = SfkEbqOObrSBQQFdBHQH69370965;     SfkEbqOObrSBQQFdBHQH69370965 = SfkEbqOObrSBQQFdBHQH67767678;     SfkEbqOObrSBQQFdBHQH67767678 = SfkEbqOObrSBQQFdBHQH5065119;     SfkEbqOObrSBQQFdBHQH5065119 = SfkEbqOObrSBQQFdBHQH49212902;     SfkEbqOObrSBQQFdBHQH49212902 = SfkEbqOObrSBQQFdBHQH64503365;     SfkEbqOObrSBQQFdBHQH64503365 = SfkEbqOObrSBQQFdBHQH84319044;     SfkEbqOObrSBQQFdBHQH84319044 = SfkEbqOObrSBQQFdBHQH34435799;     SfkEbqOObrSBQQFdBHQH34435799 = SfkEbqOObrSBQQFdBHQH65526795;     SfkEbqOObrSBQQFdBHQH65526795 = SfkEbqOObrSBQQFdBHQH13287246;     SfkEbqOObrSBQQFdBHQH13287246 = SfkEbqOObrSBQQFdBHQH2375956;     SfkEbqOObrSBQQFdBHQH2375956 = SfkEbqOObrSBQQFdBHQH7378790;     SfkEbqOObrSBQQFdBHQH7378790 = SfkEbqOObrSBQQFdBHQH1012337;     SfkEbqOObrSBQQFdBHQH1012337 = SfkEbqOObrSBQQFdBHQH24226408;     SfkEbqOObrSBQQFdBHQH24226408 = SfkEbqOObrSBQQFdBHQH47816789;     SfkEbqOObrSBQQFdBHQH47816789 = SfkEbqOObrSBQQFdBHQH53996762;     SfkEbqOObrSBQQFdBHQH53996762 = SfkEbqOObrSBQQFdBHQH8738360;     SfkEbqOObrSBQQFdBHQH8738360 = SfkEbqOObrSBQQFdBHQH76704269;     SfkEbqOObrSBQQFdBHQH76704269 = SfkEbqOObrSBQQFdBHQH72184203;     SfkEbqOObrSBQQFdBHQH72184203 = SfkEbqOObrSBQQFdBHQH85538064;     SfkEbqOObrSBQQFdBHQH85538064 = SfkEbqOObrSBQQFdBHQH80868800;     SfkEbqOObrSBQQFdBHQH80868800 = SfkEbqOObrSBQQFdBHQH85796059;     SfkEbqOObrSBQQFdBHQH85796059 = SfkEbqOObrSBQQFdBHQH73928245;     SfkEbqOObrSBQQFdBHQH73928245 = SfkEbqOObrSBQQFdBHQH43385542;     SfkEbqOObrSBQQFdBHQH43385542 = SfkEbqOObrSBQQFdBHQH64961450;     SfkEbqOObrSBQQFdBHQH64961450 = SfkEbqOObrSBQQFdBHQH59892656;     SfkEbqOObrSBQQFdBHQH59892656 = SfkEbqOObrSBQQFdBHQH98098971;     SfkEbqOObrSBQQFdBHQH98098971 = SfkEbqOObrSBQQFdBHQH53109381;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void MCtgEFuuRPzmnnrPiIOc14863879() {     int tEGgDUqPDQybTMXMhdSo87198171 = -92154674;    int tEGgDUqPDQybTMXMhdSo8167495 = -409444767;    int tEGgDUqPDQybTMXMhdSo64141802 = -239758912;    int tEGgDUqPDQybTMXMhdSo59092317 = -83414128;    int tEGgDUqPDQybTMXMhdSo25585965 = -875840566;    int tEGgDUqPDQybTMXMhdSo1760638 = -849625757;    int tEGgDUqPDQybTMXMhdSo54729252 = -43770097;    int tEGgDUqPDQybTMXMhdSo68856291 = -654402357;    int tEGgDUqPDQybTMXMhdSo70735281 = -825624247;    int tEGgDUqPDQybTMXMhdSo87332036 = 4092251;    int tEGgDUqPDQybTMXMhdSo50518250 = -555171217;    int tEGgDUqPDQybTMXMhdSo14468924 = -438116232;    int tEGgDUqPDQybTMXMhdSo47695532 = 92703565;    int tEGgDUqPDQybTMXMhdSo33484229 = -842436646;    int tEGgDUqPDQybTMXMhdSo36192593 = -170140819;    int tEGgDUqPDQybTMXMhdSo32346220 = -185199435;    int tEGgDUqPDQybTMXMhdSo73191502 = -58023624;    int tEGgDUqPDQybTMXMhdSo8734876 = -216898607;    int tEGgDUqPDQybTMXMhdSo6152759 = -175835868;    int tEGgDUqPDQybTMXMhdSo53229292 = -993091494;    int tEGgDUqPDQybTMXMhdSo72216316 = -535743620;    int tEGgDUqPDQybTMXMhdSo97815931 = -112614550;    int tEGgDUqPDQybTMXMhdSo38364094 = -818791436;    int tEGgDUqPDQybTMXMhdSo97020401 = -792201145;    int tEGgDUqPDQybTMXMhdSo2277840 = -500726206;    int tEGgDUqPDQybTMXMhdSo82295899 = 72778694;    int tEGgDUqPDQybTMXMhdSo64148541 = -95152241;    int tEGgDUqPDQybTMXMhdSo4154942 = -595274536;    int tEGgDUqPDQybTMXMhdSo20163777 = -248886534;    int tEGgDUqPDQybTMXMhdSo79399805 = -484129150;    int tEGgDUqPDQybTMXMhdSo62376499 = -128699958;    int tEGgDUqPDQybTMXMhdSo4071472 = 5466692;    int tEGgDUqPDQybTMXMhdSo37213013 = -735586842;    int tEGgDUqPDQybTMXMhdSo86934794 = -65046168;    int tEGgDUqPDQybTMXMhdSo1987832 = -340449436;    int tEGgDUqPDQybTMXMhdSo80303733 = -844223696;    int tEGgDUqPDQybTMXMhdSo971143 = 38691407;    int tEGgDUqPDQybTMXMhdSo23873546 = -102244837;    int tEGgDUqPDQybTMXMhdSo57615858 = -271643170;    int tEGgDUqPDQybTMXMhdSo37740907 = -969848145;    int tEGgDUqPDQybTMXMhdSo28453126 = -588627505;    int tEGgDUqPDQybTMXMhdSo20849868 = -198939473;    int tEGgDUqPDQybTMXMhdSo48872672 = -111946186;    int tEGgDUqPDQybTMXMhdSo19641036 = -416932699;    int tEGgDUqPDQybTMXMhdSo29235688 = -206167275;    int tEGgDUqPDQybTMXMhdSo59787304 = -346380113;    int tEGgDUqPDQybTMXMhdSo14447578 = -620356411;    int tEGgDUqPDQybTMXMhdSo609223 = -419913599;    int tEGgDUqPDQybTMXMhdSo39214848 = -572275459;    int tEGgDUqPDQybTMXMhdSo42662299 = -713383655;    int tEGgDUqPDQybTMXMhdSo62988342 = 14690598;    int tEGgDUqPDQybTMXMhdSo38049008 = -989370504;    int tEGgDUqPDQybTMXMhdSo99829820 = -91254970;    int tEGgDUqPDQybTMXMhdSo40628507 = 34631977;    int tEGgDUqPDQybTMXMhdSo42647562 = -553578367;    int tEGgDUqPDQybTMXMhdSo89382240 = -979540124;    int tEGgDUqPDQybTMXMhdSo69803401 = -590653332;    int tEGgDUqPDQybTMXMhdSo67121401 = -447557767;    int tEGgDUqPDQybTMXMhdSo56814477 = -582687922;    int tEGgDUqPDQybTMXMhdSo43290065 = -848619261;    int tEGgDUqPDQybTMXMhdSo37612097 = -654473517;    int tEGgDUqPDQybTMXMhdSo50574310 = -448495562;    int tEGgDUqPDQybTMXMhdSo48692514 = -305515823;    int tEGgDUqPDQybTMXMhdSo91335475 = -241495098;    int tEGgDUqPDQybTMXMhdSo24955537 = -867207791;    int tEGgDUqPDQybTMXMhdSo46446778 = -460637910;    int tEGgDUqPDQybTMXMhdSo77255910 = -702529391;    int tEGgDUqPDQybTMXMhdSo60760738 = -842250268;    int tEGgDUqPDQybTMXMhdSo31496397 = -401987210;    int tEGgDUqPDQybTMXMhdSo55888859 = -325917123;    int tEGgDUqPDQybTMXMhdSo31375077 = -123890842;    int tEGgDUqPDQybTMXMhdSo49317956 = -955778788;    int tEGgDUqPDQybTMXMhdSo51119017 = -945255437;    int tEGgDUqPDQybTMXMhdSo68411852 = -205987723;    int tEGgDUqPDQybTMXMhdSo24776166 = -304463990;    int tEGgDUqPDQybTMXMhdSo51366448 = -236804147;    int tEGgDUqPDQybTMXMhdSo48943260 = 99331636;    int tEGgDUqPDQybTMXMhdSo18723059 = -301858738;    int tEGgDUqPDQybTMXMhdSo67784713 = -486033870;    int tEGgDUqPDQybTMXMhdSo42490536 = -54346094;    int tEGgDUqPDQybTMXMhdSo67848321 = -306864895;    int tEGgDUqPDQybTMXMhdSo63539318 = -675238643;    int tEGgDUqPDQybTMXMhdSo64940094 = 77000922;    int tEGgDUqPDQybTMXMhdSo77501478 = -535502879;    int tEGgDUqPDQybTMXMhdSo16411464 = -398819748;    int tEGgDUqPDQybTMXMhdSo24327491 = -139329455;    int tEGgDUqPDQybTMXMhdSo4241652 = -903278339;    int tEGgDUqPDQybTMXMhdSo96584506 = -670218819;    int tEGgDUqPDQybTMXMhdSo44287232 = -511467802;    int tEGgDUqPDQybTMXMhdSo12605592 = -360909312;    int tEGgDUqPDQybTMXMhdSo10500333 = -153570365;    int tEGgDUqPDQybTMXMhdSo33849741 = -513750827;    int tEGgDUqPDQybTMXMhdSo67059069 = -519556915;    int tEGgDUqPDQybTMXMhdSo14325793 = -423023910;    int tEGgDUqPDQybTMXMhdSo128811 = -215374629;    int tEGgDUqPDQybTMXMhdSo77878816 = -40131943;    int tEGgDUqPDQybTMXMhdSo72157353 = -893423651;    int tEGgDUqPDQybTMXMhdSo57537197 = -870451088;    int tEGgDUqPDQybTMXMhdSo94685498 = -549724908;    int tEGgDUqPDQybTMXMhdSo82788910 = -92154674;     tEGgDUqPDQybTMXMhdSo87198171 = tEGgDUqPDQybTMXMhdSo8167495;     tEGgDUqPDQybTMXMhdSo8167495 = tEGgDUqPDQybTMXMhdSo64141802;     tEGgDUqPDQybTMXMhdSo64141802 = tEGgDUqPDQybTMXMhdSo59092317;     tEGgDUqPDQybTMXMhdSo59092317 = tEGgDUqPDQybTMXMhdSo25585965;     tEGgDUqPDQybTMXMhdSo25585965 = tEGgDUqPDQybTMXMhdSo1760638;     tEGgDUqPDQybTMXMhdSo1760638 = tEGgDUqPDQybTMXMhdSo54729252;     tEGgDUqPDQybTMXMhdSo54729252 = tEGgDUqPDQybTMXMhdSo68856291;     tEGgDUqPDQybTMXMhdSo68856291 = tEGgDUqPDQybTMXMhdSo70735281;     tEGgDUqPDQybTMXMhdSo70735281 = tEGgDUqPDQybTMXMhdSo87332036;     tEGgDUqPDQybTMXMhdSo87332036 = tEGgDUqPDQybTMXMhdSo50518250;     tEGgDUqPDQybTMXMhdSo50518250 = tEGgDUqPDQybTMXMhdSo14468924;     tEGgDUqPDQybTMXMhdSo14468924 = tEGgDUqPDQybTMXMhdSo47695532;     tEGgDUqPDQybTMXMhdSo47695532 = tEGgDUqPDQybTMXMhdSo33484229;     tEGgDUqPDQybTMXMhdSo33484229 = tEGgDUqPDQybTMXMhdSo36192593;     tEGgDUqPDQybTMXMhdSo36192593 = tEGgDUqPDQybTMXMhdSo32346220;     tEGgDUqPDQybTMXMhdSo32346220 = tEGgDUqPDQybTMXMhdSo73191502;     tEGgDUqPDQybTMXMhdSo73191502 = tEGgDUqPDQybTMXMhdSo8734876;     tEGgDUqPDQybTMXMhdSo8734876 = tEGgDUqPDQybTMXMhdSo6152759;     tEGgDUqPDQybTMXMhdSo6152759 = tEGgDUqPDQybTMXMhdSo53229292;     tEGgDUqPDQybTMXMhdSo53229292 = tEGgDUqPDQybTMXMhdSo72216316;     tEGgDUqPDQybTMXMhdSo72216316 = tEGgDUqPDQybTMXMhdSo97815931;     tEGgDUqPDQybTMXMhdSo97815931 = tEGgDUqPDQybTMXMhdSo38364094;     tEGgDUqPDQybTMXMhdSo38364094 = tEGgDUqPDQybTMXMhdSo97020401;     tEGgDUqPDQybTMXMhdSo97020401 = tEGgDUqPDQybTMXMhdSo2277840;     tEGgDUqPDQybTMXMhdSo2277840 = tEGgDUqPDQybTMXMhdSo82295899;     tEGgDUqPDQybTMXMhdSo82295899 = tEGgDUqPDQybTMXMhdSo64148541;     tEGgDUqPDQybTMXMhdSo64148541 = tEGgDUqPDQybTMXMhdSo4154942;     tEGgDUqPDQybTMXMhdSo4154942 = tEGgDUqPDQybTMXMhdSo20163777;     tEGgDUqPDQybTMXMhdSo20163777 = tEGgDUqPDQybTMXMhdSo79399805;     tEGgDUqPDQybTMXMhdSo79399805 = tEGgDUqPDQybTMXMhdSo62376499;     tEGgDUqPDQybTMXMhdSo62376499 = tEGgDUqPDQybTMXMhdSo4071472;     tEGgDUqPDQybTMXMhdSo4071472 = tEGgDUqPDQybTMXMhdSo37213013;     tEGgDUqPDQybTMXMhdSo37213013 = tEGgDUqPDQybTMXMhdSo86934794;     tEGgDUqPDQybTMXMhdSo86934794 = tEGgDUqPDQybTMXMhdSo1987832;     tEGgDUqPDQybTMXMhdSo1987832 = tEGgDUqPDQybTMXMhdSo80303733;     tEGgDUqPDQybTMXMhdSo80303733 = tEGgDUqPDQybTMXMhdSo971143;     tEGgDUqPDQybTMXMhdSo971143 = tEGgDUqPDQybTMXMhdSo23873546;     tEGgDUqPDQybTMXMhdSo23873546 = tEGgDUqPDQybTMXMhdSo57615858;     tEGgDUqPDQybTMXMhdSo57615858 = tEGgDUqPDQybTMXMhdSo37740907;     tEGgDUqPDQybTMXMhdSo37740907 = tEGgDUqPDQybTMXMhdSo28453126;     tEGgDUqPDQybTMXMhdSo28453126 = tEGgDUqPDQybTMXMhdSo20849868;     tEGgDUqPDQybTMXMhdSo20849868 = tEGgDUqPDQybTMXMhdSo48872672;     tEGgDUqPDQybTMXMhdSo48872672 = tEGgDUqPDQybTMXMhdSo19641036;     tEGgDUqPDQybTMXMhdSo19641036 = tEGgDUqPDQybTMXMhdSo29235688;     tEGgDUqPDQybTMXMhdSo29235688 = tEGgDUqPDQybTMXMhdSo59787304;     tEGgDUqPDQybTMXMhdSo59787304 = tEGgDUqPDQybTMXMhdSo14447578;     tEGgDUqPDQybTMXMhdSo14447578 = tEGgDUqPDQybTMXMhdSo609223;     tEGgDUqPDQybTMXMhdSo609223 = tEGgDUqPDQybTMXMhdSo39214848;     tEGgDUqPDQybTMXMhdSo39214848 = tEGgDUqPDQybTMXMhdSo42662299;     tEGgDUqPDQybTMXMhdSo42662299 = tEGgDUqPDQybTMXMhdSo62988342;     tEGgDUqPDQybTMXMhdSo62988342 = tEGgDUqPDQybTMXMhdSo38049008;     tEGgDUqPDQybTMXMhdSo38049008 = tEGgDUqPDQybTMXMhdSo99829820;     tEGgDUqPDQybTMXMhdSo99829820 = tEGgDUqPDQybTMXMhdSo40628507;     tEGgDUqPDQybTMXMhdSo40628507 = tEGgDUqPDQybTMXMhdSo42647562;     tEGgDUqPDQybTMXMhdSo42647562 = tEGgDUqPDQybTMXMhdSo89382240;     tEGgDUqPDQybTMXMhdSo89382240 = tEGgDUqPDQybTMXMhdSo69803401;     tEGgDUqPDQybTMXMhdSo69803401 = tEGgDUqPDQybTMXMhdSo67121401;     tEGgDUqPDQybTMXMhdSo67121401 = tEGgDUqPDQybTMXMhdSo56814477;     tEGgDUqPDQybTMXMhdSo56814477 = tEGgDUqPDQybTMXMhdSo43290065;     tEGgDUqPDQybTMXMhdSo43290065 = tEGgDUqPDQybTMXMhdSo37612097;     tEGgDUqPDQybTMXMhdSo37612097 = tEGgDUqPDQybTMXMhdSo50574310;     tEGgDUqPDQybTMXMhdSo50574310 = tEGgDUqPDQybTMXMhdSo48692514;     tEGgDUqPDQybTMXMhdSo48692514 = tEGgDUqPDQybTMXMhdSo91335475;     tEGgDUqPDQybTMXMhdSo91335475 = tEGgDUqPDQybTMXMhdSo24955537;     tEGgDUqPDQybTMXMhdSo24955537 = tEGgDUqPDQybTMXMhdSo46446778;     tEGgDUqPDQybTMXMhdSo46446778 = tEGgDUqPDQybTMXMhdSo77255910;     tEGgDUqPDQybTMXMhdSo77255910 = tEGgDUqPDQybTMXMhdSo60760738;     tEGgDUqPDQybTMXMhdSo60760738 = tEGgDUqPDQybTMXMhdSo31496397;     tEGgDUqPDQybTMXMhdSo31496397 = tEGgDUqPDQybTMXMhdSo55888859;     tEGgDUqPDQybTMXMhdSo55888859 = tEGgDUqPDQybTMXMhdSo31375077;     tEGgDUqPDQybTMXMhdSo31375077 = tEGgDUqPDQybTMXMhdSo49317956;     tEGgDUqPDQybTMXMhdSo49317956 = tEGgDUqPDQybTMXMhdSo51119017;     tEGgDUqPDQybTMXMhdSo51119017 = tEGgDUqPDQybTMXMhdSo68411852;     tEGgDUqPDQybTMXMhdSo68411852 = tEGgDUqPDQybTMXMhdSo24776166;     tEGgDUqPDQybTMXMhdSo24776166 = tEGgDUqPDQybTMXMhdSo51366448;     tEGgDUqPDQybTMXMhdSo51366448 = tEGgDUqPDQybTMXMhdSo48943260;     tEGgDUqPDQybTMXMhdSo48943260 = tEGgDUqPDQybTMXMhdSo18723059;     tEGgDUqPDQybTMXMhdSo18723059 = tEGgDUqPDQybTMXMhdSo67784713;     tEGgDUqPDQybTMXMhdSo67784713 = tEGgDUqPDQybTMXMhdSo42490536;     tEGgDUqPDQybTMXMhdSo42490536 = tEGgDUqPDQybTMXMhdSo67848321;     tEGgDUqPDQybTMXMhdSo67848321 = tEGgDUqPDQybTMXMhdSo63539318;     tEGgDUqPDQybTMXMhdSo63539318 = tEGgDUqPDQybTMXMhdSo64940094;     tEGgDUqPDQybTMXMhdSo64940094 = tEGgDUqPDQybTMXMhdSo77501478;     tEGgDUqPDQybTMXMhdSo77501478 = tEGgDUqPDQybTMXMhdSo16411464;     tEGgDUqPDQybTMXMhdSo16411464 = tEGgDUqPDQybTMXMhdSo24327491;     tEGgDUqPDQybTMXMhdSo24327491 = tEGgDUqPDQybTMXMhdSo4241652;     tEGgDUqPDQybTMXMhdSo4241652 = tEGgDUqPDQybTMXMhdSo96584506;     tEGgDUqPDQybTMXMhdSo96584506 = tEGgDUqPDQybTMXMhdSo44287232;     tEGgDUqPDQybTMXMhdSo44287232 = tEGgDUqPDQybTMXMhdSo12605592;     tEGgDUqPDQybTMXMhdSo12605592 = tEGgDUqPDQybTMXMhdSo10500333;     tEGgDUqPDQybTMXMhdSo10500333 = tEGgDUqPDQybTMXMhdSo33849741;     tEGgDUqPDQybTMXMhdSo33849741 = tEGgDUqPDQybTMXMhdSo67059069;     tEGgDUqPDQybTMXMhdSo67059069 = tEGgDUqPDQybTMXMhdSo14325793;     tEGgDUqPDQybTMXMhdSo14325793 = tEGgDUqPDQybTMXMhdSo128811;     tEGgDUqPDQybTMXMhdSo128811 = tEGgDUqPDQybTMXMhdSo77878816;     tEGgDUqPDQybTMXMhdSo77878816 = tEGgDUqPDQybTMXMhdSo72157353;     tEGgDUqPDQybTMXMhdSo72157353 = tEGgDUqPDQybTMXMhdSo57537197;     tEGgDUqPDQybTMXMhdSo57537197 = tEGgDUqPDQybTMXMhdSo94685498;     tEGgDUqPDQybTMXMhdSo94685498 = tEGgDUqPDQybTMXMhdSo82788910;     tEGgDUqPDQybTMXMhdSo82788910 = tEGgDUqPDQybTMXMhdSo87198171;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void bUovNVCMZSKPAtyxeMfs12552498() {     int RlDgAaWaJjXIYJvbkVyl69824253 = -747178771;    int RlDgAaWaJjXIYJvbkVyl67144615 = -868232589;    int RlDgAaWaJjXIYJvbkVyl80077842 = 62449090;    int RlDgAaWaJjXIYJvbkVyl63577881 = -282345194;    int RlDgAaWaJjXIYJvbkVyl84866943 = -804399991;    int RlDgAaWaJjXIYJvbkVyl57343391 = -557651927;    int RlDgAaWaJjXIYJvbkVyl22602827 = -925263536;    int RlDgAaWaJjXIYJvbkVyl88583662 = -729138765;    int RlDgAaWaJjXIYJvbkVyl5975006 = -958075352;    int RlDgAaWaJjXIYJvbkVyl89279338 = -89868538;    int RlDgAaWaJjXIYJvbkVyl58553077 = -816362719;    int RlDgAaWaJjXIYJvbkVyl53780178 = -294736317;    int RlDgAaWaJjXIYJvbkVyl71446338 = -580469245;    int RlDgAaWaJjXIYJvbkVyl7339703 = -111090331;    int RlDgAaWaJjXIYJvbkVyl31556343 = -881002276;    int RlDgAaWaJjXIYJvbkVyl38767139 = -766746830;    int RlDgAaWaJjXIYJvbkVyl81044530 = -235920435;    int RlDgAaWaJjXIYJvbkVyl20907254 = -82348525;    int RlDgAaWaJjXIYJvbkVyl71699076 = -50640836;    int RlDgAaWaJjXIYJvbkVyl72154840 = -226483359;    int RlDgAaWaJjXIYJvbkVyl53751062 = 60938940;    int RlDgAaWaJjXIYJvbkVyl8510194 = 92219878;    int RlDgAaWaJjXIYJvbkVyl35504139 = -613290489;    int RlDgAaWaJjXIYJvbkVyl1468225 = -619043202;    int RlDgAaWaJjXIYJvbkVyl73447325 = -124263398;    int RlDgAaWaJjXIYJvbkVyl54963990 = -595741300;    int RlDgAaWaJjXIYJvbkVyl86526938 = -773464203;    int RlDgAaWaJjXIYJvbkVyl29073268 = -788868641;    int RlDgAaWaJjXIYJvbkVyl10698388 = -588108457;    int RlDgAaWaJjXIYJvbkVyl7818579 = -391415311;    int RlDgAaWaJjXIYJvbkVyl90426449 = -161154997;    int RlDgAaWaJjXIYJvbkVyl27945743 = -132256675;    int RlDgAaWaJjXIYJvbkVyl12637188 = 82399371;    int RlDgAaWaJjXIYJvbkVyl47546182 = -889119707;    int RlDgAaWaJjXIYJvbkVyl67010663 = -50294780;    int RlDgAaWaJjXIYJvbkVyl91792200 = -921832649;    int RlDgAaWaJjXIYJvbkVyl40312983 = -416721289;    int RlDgAaWaJjXIYJvbkVyl25290462 = -352909229;    int RlDgAaWaJjXIYJvbkVyl65808224 = -156513469;    int RlDgAaWaJjXIYJvbkVyl2535422 = -211986538;    int RlDgAaWaJjXIYJvbkVyl5179259 = -760871323;    int RlDgAaWaJjXIYJvbkVyl77688849 = -895119504;    int RlDgAaWaJjXIYJvbkVyl63890432 = -581571632;    int RlDgAaWaJjXIYJvbkVyl67978543 = -933939255;    int RlDgAaWaJjXIYJvbkVyl60997766 = -20974649;    int RlDgAaWaJjXIYJvbkVyl64126739 = 56405280;    int RlDgAaWaJjXIYJvbkVyl35879012 = -740793742;    int RlDgAaWaJjXIYJvbkVyl36915320 = -402150711;    int RlDgAaWaJjXIYJvbkVyl5005489 = -724039752;    int RlDgAaWaJjXIYJvbkVyl16235975 = -884344511;    int RlDgAaWaJjXIYJvbkVyl95967529 = -535932864;    int RlDgAaWaJjXIYJvbkVyl61893254 = -819812146;    int RlDgAaWaJjXIYJvbkVyl93787403 = -126285113;    int RlDgAaWaJjXIYJvbkVyl70786913 = 86865544;    int RlDgAaWaJjXIYJvbkVyl58801436 = -626644190;    int RlDgAaWaJjXIYJvbkVyl61314060 = -739398649;    int RlDgAaWaJjXIYJvbkVyl31640477 = -154942101;    int RlDgAaWaJjXIYJvbkVyl78609618 = -318507708;    int RlDgAaWaJjXIYJvbkVyl90130556 = -58081797;    int RlDgAaWaJjXIYJvbkVyl29902954 = -108658692;    int RlDgAaWaJjXIYJvbkVyl70816452 = -784187725;    int RlDgAaWaJjXIYJvbkVyl93529558 = -36394896;    int RlDgAaWaJjXIYJvbkVyl77885274 = -41030308;    int RlDgAaWaJjXIYJvbkVyl98156427 = -466660042;    int RlDgAaWaJjXIYJvbkVyl98852889 = -928713541;    int RlDgAaWaJjXIYJvbkVyl30607334 = -584106044;    int RlDgAaWaJjXIYJvbkVyl41142991 = -277135688;    int RlDgAaWaJjXIYJvbkVyl23900157 = -691349538;    int RlDgAaWaJjXIYJvbkVyl40329039 = 39204448;    int RlDgAaWaJjXIYJvbkVyl39764143 = -959169627;    int RlDgAaWaJjXIYJvbkVyl98454155 = -250025542;    int RlDgAaWaJjXIYJvbkVyl55754069 = -883011206;    int RlDgAaWaJjXIYJvbkVyl55099030 = -925835057;    int RlDgAaWaJjXIYJvbkVyl69163655 = -838654299;    int RlDgAaWaJjXIYJvbkVyl66975581 = -465612037;    int RlDgAaWaJjXIYJvbkVyl76062212 = -43941557;    int RlDgAaWaJjXIYJvbkVyl44619762 = -326208491;    int RlDgAaWaJjXIYJvbkVyl67525596 = -679351235;    int RlDgAaWaJjXIYJvbkVyl40470459 = -498068554;    int RlDgAaWaJjXIYJvbkVyl9320587 = -80668678;    int RlDgAaWaJjXIYJvbkVyl19084979 = -854947559;    int RlDgAaWaJjXIYJvbkVyl49611618 = -271313492;    int RlDgAaWaJjXIYJvbkVyl24067780 = 35171110;    int RlDgAaWaJjXIYJvbkVyl94462412 = -703763947;    int RlDgAaWaJjXIYJvbkVyl11851049 = -855482447;    int RlDgAaWaJjXIYJvbkVyl28533195 = -341342851;    int RlDgAaWaJjXIYJvbkVyl34158339 = 94028437;    int RlDgAaWaJjXIYJvbkVyl41850274 = 95533826;    int RlDgAaWaJjXIYJvbkVyl88744746 = -162475517;    int RlDgAaWaJjXIYJvbkVyl5696603 = -310896131;    int RlDgAaWaJjXIYJvbkVyl60151724 = -666890549;    int RlDgAaWaJjXIYJvbkVyl61703365 = 1786419;    int RlDgAaWaJjXIYJvbkVyl35159905 = -194827433;    int RlDgAaWaJjXIYJvbkVyl35905270 = 52145222;    int RlDgAaWaJjXIYJvbkVyl31718969 = -427798813;    int RlDgAaWaJjXIYJvbkVyl11649701 = -624476428;    int RlDgAaWaJjXIYJvbkVyl99803575 = -754089197;    int RlDgAaWaJjXIYJvbkVyl65734005 = -14911591;    int RlDgAaWaJjXIYJvbkVyl69125653 = 94774286;    int RlDgAaWaJjXIYJvbkVyl30390432 = -747178771;     RlDgAaWaJjXIYJvbkVyl69824253 = RlDgAaWaJjXIYJvbkVyl67144615;     RlDgAaWaJjXIYJvbkVyl67144615 = RlDgAaWaJjXIYJvbkVyl80077842;     RlDgAaWaJjXIYJvbkVyl80077842 = RlDgAaWaJjXIYJvbkVyl63577881;     RlDgAaWaJjXIYJvbkVyl63577881 = RlDgAaWaJjXIYJvbkVyl84866943;     RlDgAaWaJjXIYJvbkVyl84866943 = RlDgAaWaJjXIYJvbkVyl57343391;     RlDgAaWaJjXIYJvbkVyl57343391 = RlDgAaWaJjXIYJvbkVyl22602827;     RlDgAaWaJjXIYJvbkVyl22602827 = RlDgAaWaJjXIYJvbkVyl88583662;     RlDgAaWaJjXIYJvbkVyl88583662 = RlDgAaWaJjXIYJvbkVyl5975006;     RlDgAaWaJjXIYJvbkVyl5975006 = RlDgAaWaJjXIYJvbkVyl89279338;     RlDgAaWaJjXIYJvbkVyl89279338 = RlDgAaWaJjXIYJvbkVyl58553077;     RlDgAaWaJjXIYJvbkVyl58553077 = RlDgAaWaJjXIYJvbkVyl53780178;     RlDgAaWaJjXIYJvbkVyl53780178 = RlDgAaWaJjXIYJvbkVyl71446338;     RlDgAaWaJjXIYJvbkVyl71446338 = RlDgAaWaJjXIYJvbkVyl7339703;     RlDgAaWaJjXIYJvbkVyl7339703 = RlDgAaWaJjXIYJvbkVyl31556343;     RlDgAaWaJjXIYJvbkVyl31556343 = RlDgAaWaJjXIYJvbkVyl38767139;     RlDgAaWaJjXIYJvbkVyl38767139 = RlDgAaWaJjXIYJvbkVyl81044530;     RlDgAaWaJjXIYJvbkVyl81044530 = RlDgAaWaJjXIYJvbkVyl20907254;     RlDgAaWaJjXIYJvbkVyl20907254 = RlDgAaWaJjXIYJvbkVyl71699076;     RlDgAaWaJjXIYJvbkVyl71699076 = RlDgAaWaJjXIYJvbkVyl72154840;     RlDgAaWaJjXIYJvbkVyl72154840 = RlDgAaWaJjXIYJvbkVyl53751062;     RlDgAaWaJjXIYJvbkVyl53751062 = RlDgAaWaJjXIYJvbkVyl8510194;     RlDgAaWaJjXIYJvbkVyl8510194 = RlDgAaWaJjXIYJvbkVyl35504139;     RlDgAaWaJjXIYJvbkVyl35504139 = RlDgAaWaJjXIYJvbkVyl1468225;     RlDgAaWaJjXIYJvbkVyl1468225 = RlDgAaWaJjXIYJvbkVyl73447325;     RlDgAaWaJjXIYJvbkVyl73447325 = RlDgAaWaJjXIYJvbkVyl54963990;     RlDgAaWaJjXIYJvbkVyl54963990 = RlDgAaWaJjXIYJvbkVyl86526938;     RlDgAaWaJjXIYJvbkVyl86526938 = RlDgAaWaJjXIYJvbkVyl29073268;     RlDgAaWaJjXIYJvbkVyl29073268 = RlDgAaWaJjXIYJvbkVyl10698388;     RlDgAaWaJjXIYJvbkVyl10698388 = RlDgAaWaJjXIYJvbkVyl7818579;     RlDgAaWaJjXIYJvbkVyl7818579 = RlDgAaWaJjXIYJvbkVyl90426449;     RlDgAaWaJjXIYJvbkVyl90426449 = RlDgAaWaJjXIYJvbkVyl27945743;     RlDgAaWaJjXIYJvbkVyl27945743 = RlDgAaWaJjXIYJvbkVyl12637188;     RlDgAaWaJjXIYJvbkVyl12637188 = RlDgAaWaJjXIYJvbkVyl47546182;     RlDgAaWaJjXIYJvbkVyl47546182 = RlDgAaWaJjXIYJvbkVyl67010663;     RlDgAaWaJjXIYJvbkVyl67010663 = RlDgAaWaJjXIYJvbkVyl91792200;     RlDgAaWaJjXIYJvbkVyl91792200 = RlDgAaWaJjXIYJvbkVyl40312983;     RlDgAaWaJjXIYJvbkVyl40312983 = RlDgAaWaJjXIYJvbkVyl25290462;     RlDgAaWaJjXIYJvbkVyl25290462 = RlDgAaWaJjXIYJvbkVyl65808224;     RlDgAaWaJjXIYJvbkVyl65808224 = RlDgAaWaJjXIYJvbkVyl2535422;     RlDgAaWaJjXIYJvbkVyl2535422 = RlDgAaWaJjXIYJvbkVyl5179259;     RlDgAaWaJjXIYJvbkVyl5179259 = RlDgAaWaJjXIYJvbkVyl77688849;     RlDgAaWaJjXIYJvbkVyl77688849 = RlDgAaWaJjXIYJvbkVyl63890432;     RlDgAaWaJjXIYJvbkVyl63890432 = RlDgAaWaJjXIYJvbkVyl67978543;     RlDgAaWaJjXIYJvbkVyl67978543 = RlDgAaWaJjXIYJvbkVyl60997766;     RlDgAaWaJjXIYJvbkVyl60997766 = RlDgAaWaJjXIYJvbkVyl64126739;     RlDgAaWaJjXIYJvbkVyl64126739 = RlDgAaWaJjXIYJvbkVyl35879012;     RlDgAaWaJjXIYJvbkVyl35879012 = RlDgAaWaJjXIYJvbkVyl36915320;     RlDgAaWaJjXIYJvbkVyl36915320 = RlDgAaWaJjXIYJvbkVyl5005489;     RlDgAaWaJjXIYJvbkVyl5005489 = RlDgAaWaJjXIYJvbkVyl16235975;     RlDgAaWaJjXIYJvbkVyl16235975 = RlDgAaWaJjXIYJvbkVyl95967529;     RlDgAaWaJjXIYJvbkVyl95967529 = RlDgAaWaJjXIYJvbkVyl61893254;     RlDgAaWaJjXIYJvbkVyl61893254 = RlDgAaWaJjXIYJvbkVyl93787403;     RlDgAaWaJjXIYJvbkVyl93787403 = RlDgAaWaJjXIYJvbkVyl70786913;     RlDgAaWaJjXIYJvbkVyl70786913 = RlDgAaWaJjXIYJvbkVyl58801436;     RlDgAaWaJjXIYJvbkVyl58801436 = RlDgAaWaJjXIYJvbkVyl61314060;     RlDgAaWaJjXIYJvbkVyl61314060 = RlDgAaWaJjXIYJvbkVyl31640477;     RlDgAaWaJjXIYJvbkVyl31640477 = RlDgAaWaJjXIYJvbkVyl78609618;     RlDgAaWaJjXIYJvbkVyl78609618 = RlDgAaWaJjXIYJvbkVyl90130556;     RlDgAaWaJjXIYJvbkVyl90130556 = RlDgAaWaJjXIYJvbkVyl29902954;     RlDgAaWaJjXIYJvbkVyl29902954 = RlDgAaWaJjXIYJvbkVyl70816452;     RlDgAaWaJjXIYJvbkVyl70816452 = RlDgAaWaJjXIYJvbkVyl93529558;     RlDgAaWaJjXIYJvbkVyl93529558 = RlDgAaWaJjXIYJvbkVyl77885274;     RlDgAaWaJjXIYJvbkVyl77885274 = RlDgAaWaJjXIYJvbkVyl98156427;     RlDgAaWaJjXIYJvbkVyl98156427 = RlDgAaWaJjXIYJvbkVyl98852889;     RlDgAaWaJjXIYJvbkVyl98852889 = RlDgAaWaJjXIYJvbkVyl30607334;     RlDgAaWaJjXIYJvbkVyl30607334 = RlDgAaWaJjXIYJvbkVyl41142991;     RlDgAaWaJjXIYJvbkVyl41142991 = RlDgAaWaJjXIYJvbkVyl23900157;     RlDgAaWaJjXIYJvbkVyl23900157 = RlDgAaWaJjXIYJvbkVyl40329039;     RlDgAaWaJjXIYJvbkVyl40329039 = RlDgAaWaJjXIYJvbkVyl39764143;     RlDgAaWaJjXIYJvbkVyl39764143 = RlDgAaWaJjXIYJvbkVyl98454155;     RlDgAaWaJjXIYJvbkVyl98454155 = RlDgAaWaJjXIYJvbkVyl55754069;     RlDgAaWaJjXIYJvbkVyl55754069 = RlDgAaWaJjXIYJvbkVyl55099030;     RlDgAaWaJjXIYJvbkVyl55099030 = RlDgAaWaJjXIYJvbkVyl69163655;     RlDgAaWaJjXIYJvbkVyl69163655 = RlDgAaWaJjXIYJvbkVyl66975581;     RlDgAaWaJjXIYJvbkVyl66975581 = RlDgAaWaJjXIYJvbkVyl76062212;     RlDgAaWaJjXIYJvbkVyl76062212 = RlDgAaWaJjXIYJvbkVyl44619762;     RlDgAaWaJjXIYJvbkVyl44619762 = RlDgAaWaJjXIYJvbkVyl67525596;     RlDgAaWaJjXIYJvbkVyl67525596 = RlDgAaWaJjXIYJvbkVyl40470459;     RlDgAaWaJjXIYJvbkVyl40470459 = RlDgAaWaJjXIYJvbkVyl9320587;     RlDgAaWaJjXIYJvbkVyl9320587 = RlDgAaWaJjXIYJvbkVyl19084979;     RlDgAaWaJjXIYJvbkVyl19084979 = RlDgAaWaJjXIYJvbkVyl49611618;     RlDgAaWaJjXIYJvbkVyl49611618 = RlDgAaWaJjXIYJvbkVyl24067780;     RlDgAaWaJjXIYJvbkVyl24067780 = RlDgAaWaJjXIYJvbkVyl94462412;     RlDgAaWaJjXIYJvbkVyl94462412 = RlDgAaWaJjXIYJvbkVyl11851049;     RlDgAaWaJjXIYJvbkVyl11851049 = RlDgAaWaJjXIYJvbkVyl28533195;     RlDgAaWaJjXIYJvbkVyl28533195 = RlDgAaWaJjXIYJvbkVyl34158339;     RlDgAaWaJjXIYJvbkVyl34158339 = RlDgAaWaJjXIYJvbkVyl41850274;     RlDgAaWaJjXIYJvbkVyl41850274 = RlDgAaWaJjXIYJvbkVyl88744746;     RlDgAaWaJjXIYJvbkVyl88744746 = RlDgAaWaJjXIYJvbkVyl5696603;     RlDgAaWaJjXIYJvbkVyl5696603 = RlDgAaWaJjXIYJvbkVyl60151724;     RlDgAaWaJjXIYJvbkVyl60151724 = RlDgAaWaJjXIYJvbkVyl61703365;     RlDgAaWaJjXIYJvbkVyl61703365 = RlDgAaWaJjXIYJvbkVyl35159905;     RlDgAaWaJjXIYJvbkVyl35159905 = RlDgAaWaJjXIYJvbkVyl35905270;     RlDgAaWaJjXIYJvbkVyl35905270 = RlDgAaWaJjXIYJvbkVyl31718969;     RlDgAaWaJjXIYJvbkVyl31718969 = RlDgAaWaJjXIYJvbkVyl11649701;     RlDgAaWaJjXIYJvbkVyl11649701 = RlDgAaWaJjXIYJvbkVyl99803575;     RlDgAaWaJjXIYJvbkVyl99803575 = RlDgAaWaJjXIYJvbkVyl65734005;     RlDgAaWaJjXIYJvbkVyl65734005 = RlDgAaWaJjXIYJvbkVyl69125653;     RlDgAaWaJjXIYJvbkVyl69125653 = RlDgAaWaJjXIYJvbkVyl30390432;     RlDgAaWaJjXIYJvbkVyl30390432 = RlDgAaWaJjXIYJvbkVyl69824253;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void dStDMINqSAxHEKJJYcxr71204168() {     int aUDvZfKDrBUaDfkWrdRf33254906 = -330952184;    int aUDvZfKDrBUaDfkWrdRf92396194 = -628007203;    int aUDvZfKDrBUaDfkWrdRf72667712 = -15625370;    int aUDvZfKDrBUaDfkWrdRf68851859 = -296324921;    int aUDvZfKDrBUaDfkWrdRf25981522 = -66123022;    int aUDvZfKDrBUaDfkWrdRf34974619 = -398875157;    int aUDvZfKDrBUaDfkWrdRf80033104 = -249590037;    int aUDvZfKDrBUaDfkWrdRf41267490 = 52458920;    int aUDvZfKDrBUaDfkWrdRf54957926 = -432004493;    int aUDvZfKDrBUaDfkWrdRf23497368 = -669715055;    int aUDvZfKDrBUaDfkWrdRf26158344 = -254205377;    int aUDvZfKDrBUaDfkWrdRf30894866 = -929053083;    int aUDvZfKDrBUaDfkWrdRf69614475 = -902114256;    int aUDvZfKDrBUaDfkWrdRf41369609 = -844003849;    int aUDvZfKDrBUaDfkWrdRf54890536 = 11832883;    int aUDvZfKDrBUaDfkWrdRf38961823 = -253946268;    int aUDvZfKDrBUaDfkWrdRf44901731 = -467191969;    int aUDvZfKDrBUaDfkWrdRf65243237 = 42337955;    int aUDvZfKDrBUaDfkWrdRf36575122 = 43802322;    int aUDvZfKDrBUaDfkWrdRf67283050 = -641070144;    int aUDvZfKDrBUaDfkWrdRf14198282 = -631570039;    int aUDvZfKDrBUaDfkWrdRf71373391 = -723137881;    int aUDvZfKDrBUaDfkWrdRf66699732 = -558678305;    int aUDvZfKDrBUaDfkWrdRf98280623 = -157669047;    int aUDvZfKDrBUaDfkWrdRf56882145 = -498872336;    int aUDvZfKDrBUaDfkWrdRf97089306 = -949129316;    int aUDvZfKDrBUaDfkWrdRf67335986 = -703216027;    int aUDvZfKDrBUaDfkWrdRf41471020 = -44183060;    int aUDvZfKDrBUaDfkWrdRf8003536 = -21180308;    int aUDvZfKDrBUaDfkWrdRf47335354 = -42450674;    int aUDvZfKDrBUaDfkWrdRf85354101 = -196717996;    int aUDvZfKDrBUaDfkWrdRf58651422 = -731454928;    int aUDvZfKDrBUaDfkWrdRf32623495 = -890692780;    int aUDvZfKDrBUaDfkWrdRf43250104 = -997766250;    int aUDvZfKDrBUaDfkWrdRf63337791 = -593377114;    int aUDvZfKDrBUaDfkWrdRf67514969 = -966011312;    int aUDvZfKDrBUaDfkWrdRf54362152 = -886403404;    int aUDvZfKDrBUaDfkWrdRf64945542 = -712300182;    int aUDvZfKDrBUaDfkWrdRf53621502 = -410311442;    int aUDvZfKDrBUaDfkWrdRf7615073 = -361078877;    int aUDvZfKDrBUaDfkWrdRf59397598 = -589763897;    int aUDvZfKDrBUaDfkWrdRf87218299 = -578312739;    int aUDvZfKDrBUaDfkWrdRf77291387 = -763872546;    int aUDvZfKDrBUaDfkWrdRf47452989 = -381598555;    int aUDvZfKDrBUaDfkWrdRf32792239 = -629775586;    int aUDvZfKDrBUaDfkWrdRf59093900 = -610943345;    int aUDvZfKDrBUaDfkWrdRf60181898 = -218160049;    int aUDvZfKDrBUaDfkWrdRf67520420 = -573262045;    int aUDvZfKDrBUaDfkWrdRf51346029 = -639810736;    int aUDvZfKDrBUaDfkWrdRf69497578 = -206312888;    int aUDvZfKDrBUaDfkWrdRf34603902 = -203617382;    int aUDvZfKDrBUaDfkWrdRf79840823 = -348840152;    int aUDvZfKDrBUaDfkWrdRf24638133 = -177445353;    int aUDvZfKDrBUaDfkWrdRf59521408 = -156522904;    int aUDvZfKDrBUaDfkWrdRf57005887 = -650291479;    int aUDvZfKDrBUaDfkWrdRf61881515 = -607814303;    int aUDvZfKDrBUaDfkWrdRf25696463 = 30671101;    int aUDvZfKDrBUaDfkWrdRf74387089 = -857956323;    int aUDvZfKDrBUaDfkWrdRf11969714 = -797452586;    int aUDvZfKDrBUaDfkWrdRf28892216 = -116993706;    int aUDvZfKDrBUaDfkWrdRf67638632 = -695659131;    int aUDvZfKDrBUaDfkWrdRf38562085 = -105406977;    int aUDvZfKDrBUaDfkWrdRf33263954 = -926360773;    int aUDvZfKDrBUaDfkWrdRf7622572 = -289553819;    int aUDvZfKDrBUaDfkWrdRf38143266 = -372997059;    int aUDvZfKDrBUaDfkWrdRf67506922 = -522750449;    int aUDvZfKDrBUaDfkWrdRf98271371 = 61639697;    int aUDvZfKDrBUaDfkWrdRf26364372 = -904348006;    int aUDvZfKDrBUaDfkWrdRf78031817 = -150626735;    int aUDvZfKDrBUaDfkWrdRf87375567 = -22155806;    int aUDvZfKDrBUaDfkWrdRf84599671 = -367542865;    int aUDvZfKDrBUaDfkWrdRf79956189 = -754891787;    int aUDvZfKDrBUaDfkWrdRf11621736 = -547350604;    int aUDvZfKDrBUaDfkWrdRf28960049 = -595118801;    int aUDvZfKDrBUaDfkWrdRf7885453 = 48693753;    int aUDvZfKDrBUaDfkWrdRf26979983 = 46742700;    int aUDvZfKDrBUaDfkWrdRf94082003 = -959265336;    int aUDvZfKDrBUaDfkWrdRf19246743 = -77079750;    int aUDvZfKDrBUaDfkWrdRf65488385 = -527893462;    int aUDvZfKDrBUaDfkWrdRf97788245 = -887928991;    int aUDvZfKDrBUaDfkWrdRf36907408 = -630969268;    int aUDvZfKDrBUaDfkWrdRf99815566 = -29953983;    int aUDvZfKDrBUaDfkWrdRf90124990 = -404372325;    int aUDvZfKDrBUaDfkWrdRf38505958 = -814867420;    int aUDvZfKDrBUaDfkWrdRf12731452 = -838833292;    int aUDvZfKDrBUaDfkWrdRf5513279 = -847877844;    int aUDvZfKDrBUaDfkWrdRf34013289 = -454009576;    int aUDvZfKDrBUaDfkWrdRf73102087 = -634169877;    int aUDvZfKDrBUaDfkWrdRf86244217 = -247474772;    int aUDvZfKDrBUaDfkWrdRf1456277 = -985562811;    int aUDvZfKDrBUaDfkWrdRf41818507 = -896682413;    int aUDvZfKDrBUaDfkWrdRf79975063 = 71552918;    int aUDvZfKDrBUaDfkWrdRf52975828 = -914847597;    int aUDvZfKDrBUaDfkWrdRf24729286 = -193317736;    int aUDvZfKDrBUaDfkWrdRf39976441 = -665419746;    int aUDvZfKDrBUaDfkWrdRf20835514 = -384356921;    int aUDvZfKDrBUaDfkWrdRf53954346 = -651951966;    int aUDvZfKDrBUaDfkWrdRf69668815 = -374318727;    int aUDvZfKDrBUaDfkWrdRf9309723 = 91398504;    int aUDvZfKDrBUaDfkWrdRf65285316 = -330952184;     aUDvZfKDrBUaDfkWrdRf33254906 = aUDvZfKDrBUaDfkWrdRf92396194;     aUDvZfKDrBUaDfkWrdRf92396194 = aUDvZfKDrBUaDfkWrdRf72667712;     aUDvZfKDrBUaDfkWrdRf72667712 = aUDvZfKDrBUaDfkWrdRf68851859;     aUDvZfKDrBUaDfkWrdRf68851859 = aUDvZfKDrBUaDfkWrdRf25981522;     aUDvZfKDrBUaDfkWrdRf25981522 = aUDvZfKDrBUaDfkWrdRf34974619;     aUDvZfKDrBUaDfkWrdRf34974619 = aUDvZfKDrBUaDfkWrdRf80033104;     aUDvZfKDrBUaDfkWrdRf80033104 = aUDvZfKDrBUaDfkWrdRf41267490;     aUDvZfKDrBUaDfkWrdRf41267490 = aUDvZfKDrBUaDfkWrdRf54957926;     aUDvZfKDrBUaDfkWrdRf54957926 = aUDvZfKDrBUaDfkWrdRf23497368;     aUDvZfKDrBUaDfkWrdRf23497368 = aUDvZfKDrBUaDfkWrdRf26158344;     aUDvZfKDrBUaDfkWrdRf26158344 = aUDvZfKDrBUaDfkWrdRf30894866;     aUDvZfKDrBUaDfkWrdRf30894866 = aUDvZfKDrBUaDfkWrdRf69614475;     aUDvZfKDrBUaDfkWrdRf69614475 = aUDvZfKDrBUaDfkWrdRf41369609;     aUDvZfKDrBUaDfkWrdRf41369609 = aUDvZfKDrBUaDfkWrdRf54890536;     aUDvZfKDrBUaDfkWrdRf54890536 = aUDvZfKDrBUaDfkWrdRf38961823;     aUDvZfKDrBUaDfkWrdRf38961823 = aUDvZfKDrBUaDfkWrdRf44901731;     aUDvZfKDrBUaDfkWrdRf44901731 = aUDvZfKDrBUaDfkWrdRf65243237;     aUDvZfKDrBUaDfkWrdRf65243237 = aUDvZfKDrBUaDfkWrdRf36575122;     aUDvZfKDrBUaDfkWrdRf36575122 = aUDvZfKDrBUaDfkWrdRf67283050;     aUDvZfKDrBUaDfkWrdRf67283050 = aUDvZfKDrBUaDfkWrdRf14198282;     aUDvZfKDrBUaDfkWrdRf14198282 = aUDvZfKDrBUaDfkWrdRf71373391;     aUDvZfKDrBUaDfkWrdRf71373391 = aUDvZfKDrBUaDfkWrdRf66699732;     aUDvZfKDrBUaDfkWrdRf66699732 = aUDvZfKDrBUaDfkWrdRf98280623;     aUDvZfKDrBUaDfkWrdRf98280623 = aUDvZfKDrBUaDfkWrdRf56882145;     aUDvZfKDrBUaDfkWrdRf56882145 = aUDvZfKDrBUaDfkWrdRf97089306;     aUDvZfKDrBUaDfkWrdRf97089306 = aUDvZfKDrBUaDfkWrdRf67335986;     aUDvZfKDrBUaDfkWrdRf67335986 = aUDvZfKDrBUaDfkWrdRf41471020;     aUDvZfKDrBUaDfkWrdRf41471020 = aUDvZfKDrBUaDfkWrdRf8003536;     aUDvZfKDrBUaDfkWrdRf8003536 = aUDvZfKDrBUaDfkWrdRf47335354;     aUDvZfKDrBUaDfkWrdRf47335354 = aUDvZfKDrBUaDfkWrdRf85354101;     aUDvZfKDrBUaDfkWrdRf85354101 = aUDvZfKDrBUaDfkWrdRf58651422;     aUDvZfKDrBUaDfkWrdRf58651422 = aUDvZfKDrBUaDfkWrdRf32623495;     aUDvZfKDrBUaDfkWrdRf32623495 = aUDvZfKDrBUaDfkWrdRf43250104;     aUDvZfKDrBUaDfkWrdRf43250104 = aUDvZfKDrBUaDfkWrdRf63337791;     aUDvZfKDrBUaDfkWrdRf63337791 = aUDvZfKDrBUaDfkWrdRf67514969;     aUDvZfKDrBUaDfkWrdRf67514969 = aUDvZfKDrBUaDfkWrdRf54362152;     aUDvZfKDrBUaDfkWrdRf54362152 = aUDvZfKDrBUaDfkWrdRf64945542;     aUDvZfKDrBUaDfkWrdRf64945542 = aUDvZfKDrBUaDfkWrdRf53621502;     aUDvZfKDrBUaDfkWrdRf53621502 = aUDvZfKDrBUaDfkWrdRf7615073;     aUDvZfKDrBUaDfkWrdRf7615073 = aUDvZfKDrBUaDfkWrdRf59397598;     aUDvZfKDrBUaDfkWrdRf59397598 = aUDvZfKDrBUaDfkWrdRf87218299;     aUDvZfKDrBUaDfkWrdRf87218299 = aUDvZfKDrBUaDfkWrdRf77291387;     aUDvZfKDrBUaDfkWrdRf77291387 = aUDvZfKDrBUaDfkWrdRf47452989;     aUDvZfKDrBUaDfkWrdRf47452989 = aUDvZfKDrBUaDfkWrdRf32792239;     aUDvZfKDrBUaDfkWrdRf32792239 = aUDvZfKDrBUaDfkWrdRf59093900;     aUDvZfKDrBUaDfkWrdRf59093900 = aUDvZfKDrBUaDfkWrdRf60181898;     aUDvZfKDrBUaDfkWrdRf60181898 = aUDvZfKDrBUaDfkWrdRf67520420;     aUDvZfKDrBUaDfkWrdRf67520420 = aUDvZfKDrBUaDfkWrdRf51346029;     aUDvZfKDrBUaDfkWrdRf51346029 = aUDvZfKDrBUaDfkWrdRf69497578;     aUDvZfKDrBUaDfkWrdRf69497578 = aUDvZfKDrBUaDfkWrdRf34603902;     aUDvZfKDrBUaDfkWrdRf34603902 = aUDvZfKDrBUaDfkWrdRf79840823;     aUDvZfKDrBUaDfkWrdRf79840823 = aUDvZfKDrBUaDfkWrdRf24638133;     aUDvZfKDrBUaDfkWrdRf24638133 = aUDvZfKDrBUaDfkWrdRf59521408;     aUDvZfKDrBUaDfkWrdRf59521408 = aUDvZfKDrBUaDfkWrdRf57005887;     aUDvZfKDrBUaDfkWrdRf57005887 = aUDvZfKDrBUaDfkWrdRf61881515;     aUDvZfKDrBUaDfkWrdRf61881515 = aUDvZfKDrBUaDfkWrdRf25696463;     aUDvZfKDrBUaDfkWrdRf25696463 = aUDvZfKDrBUaDfkWrdRf74387089;     aUDvZfKDrBUaDfkWrdRf74387089 = aUDvZfKDrBUaDfkWrdRf11969714;     aUDvZfKDrBUaDfkWrdRf11969714 = aUDvZfKDrBUaDfkWrdRf28892216;     aUDvZfKDrBUaDfkWrdRf28892216 = aUDvZfKDrBUaDfkWrdRf67638632;     aUDvZfKDrBUaDfkWrdRf67638632 = aUDvZfKDrBUaDfkWrdRf38562085;     aUDvZfKDrBUaDfkWrdRf38562085 = aUDvZfKDrBUaDfkWrdRf33263954;     aUDvZfKDrBUaDfkWrdRf33263954 = aUDvZfKDrBUaDfkWrdRf7622572;     aUDvZfKDrBUaDfkWrdRf7622572 = aUDvZfKDrBUaDfkWrdRf38143266;     aUDvZfKDrBUaDfkWrdRf38143266 = aUDvZfKDrBUaDfkWrdRf67506922;     aUDvZfKDrBUaDfkWrdRf67506922 = aUDvZfKDrBUaDfkWrdRf98271371;     aUDvZfKDrBUaDfkWrdRf98271371 = aUDvZfKDrBUaDfkWrdRf26364372;     aUDvZfKDrBUaDfkWrdRf26364372 = aUDvZfKDrBUaDfkWrdRf78031817;     aUDvZfKDrBUaDfkWrdRf78031817 = aUDvZfKDrBUaDfkWrdRf87375567;     aUDvZfKDrBUaDfkWrdRf87375567 = aUDvZfKDrBUaDfkWrdRf84599671;     aUDvZfKDrBUaDfkWrdRf84599671 = aUDvZfKDrBUaDfkWrdRf79956189;     aUDvZfKDrBUaDfkWrdRf79956189 = aUDvZfKDrBUaDfkWrdRf11621736;     aUDvZfKDrBUaDfkWrdRf11621736 = aUDvZfKDrBUaDfkWrdRf28960049;     aUDvZfKDrBUaDfkWrdRf28960049 = aUDvZfKDrBUaDfkWrdRf7885453;     aUDvZfKDrBUaDfkWrdRf7885453 = aUDvZfKDrBUaDfkWrdRf26979983;     aUDvZfKDrBUaDfkWrdRf26979983 = aUDvZfKDrBUaDfkWrdRf94082003;     aUDvZfKDrBUaDfkWrdRf94082003 = aUDvZfKDrBUaDfkWrdRf19246743;     aUDvZfKDrBUaDfkWrdRf19246743 = aUDvZfKDrBUaDfkWrdRf65488385;     aUDvZfKDrBUaDfkWrdRf65488385 = aUDvZfKDrBUaDfkWrdRf97788245;     aUDvZfKDrBUaDfkWrdRf97788245 = aUDvZfKDrBUaDfkWrdRf36907408;     aUDvZfKDrBUaDfkWrdRf36907408 = aUDvZfKDrBUaDfkWrdRf99815566;     aUDvZfKDrBUaDfkWrdRf99815566 = aUDvZfKDrBUaDfkWrdRf90124990;     aUDvZfKDrBUaDfkWrdRf90124990 = aUDvZfKDrBUaDfkWrdRf38505958;     aUDvZfKDrBUaDfkWrdRf38505958 = aUDvZfKDrBUaDfkWrdRf12731452;     aUDvZfKDrBUaDfkWrdRf12731452 = aUDvZfKDrBUaDfkWrdRf5513279;     aUDvZfKDrBUaDfkWrdRf5513279 = aUDvZfKDrBUaDfkWrdRf34013289;     aUDvZfKDrBUaDfkWrdRf34013289 = aUDvZfKDrBUaDfkWrdRf73102087;     aUDvZfKDrBUaDfkWrdRf73102087 = aUDvZfKDrBUaDfkWrdRf86244217;     aUDvZfKDrBUaDfkWrdRf86244217 = aUDvZfKDrBUaDfkWrdRf1456277;     aUDvZfKDrBUaDfkWrdRf1456277 = aUDvZfKDrBUaDfkWrdRf41818507;     aUDvZfKDrBUaDfkWrdRf41818507 = aUDvZfKDrBUaDfkWrdRf79975063;     aUDvZfKDrBUaDfkWrdRf79975063 = aUDvZfKDrBUaDfkWrdRf52975828;     aUDvZfKDrBUaDfkWrdRf52975828 = aUDvZfKDrBUaDfkWrdRf24729286;     aUDvZfKDrBUaDfkWrdRf24729286 = aUDvZfKDrBUaDfkWrdRf39976441;     aUDvZfKDrBUaDfkWrdRf39976441 = aUDvZfKDrBUaDfkWrdRf20835514;     aUDvZfKDrBUaDfkWrdRf20835514 = aUDvZfKDrBUaDfkWrdRf53954346;     aUDvZfKDrBUaDfkWrdRf53954346 = aUDvZfKDrBUaDfkWrdRf69668815;     aUDvZfKDrBUaDfkWrdRf69668815 = aUDvZfKDrBUaDfkWrdRf9309723;     aUDvZfKDrBUaDfkWrdRf9309723 = aUDvZfKDrBUaDfkWrdRf65285316;     aUDvZfKDrBUaDfkWrdRf65285316 = aUDvZfKDrBUaDfkWrdRf33254906;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void AYVwEjrazUEQSVqATtLV29855840() {     int PcczXfoiKuvwfqIdilZy96685558 = 85274403;    int PcczXfoiKuvwfqIdilZy17647774 = -387781817;    int PcczXfoiKuvwfqIdilZy65257583 = -93699830;    int PcczXfoiKuvwfqIdilZy74125838 = -310304647;    int PcczXfoiKuvwfqIdilZy67096100 = -427846053;    int PcczXfoiKuvwfqIdilZy12605848 = -240098387;    int PcczXfoiKuvwfqIdilZy37463382 = -673916537;    int PcczXfoiKuvwfqIdilZy93951317 = -265943396;    int PcczXfoiKuvwfqIdilZy3940846 = 94066366;    int PcczXfoiKuvwfqIdilZy57715397 = -149561573;    int PcczXfoiKuvwfqIdilZy93763611 = -792048036;    int PcczXfoiKuvwfqIdilZy8009555 = -463369850;    int PcczXfoiKuvwfqIdilZy67782612 = -123759266;    int PcczXfoiKuvwfqIdilZy75399515 = -476917366;    int PcczXfoiKuvwfqIdilZy78224730 = -195331958;    int PcczXfoiKuvwfqIdilZy39156508 = -841145707;    int PcczXfoiKuvwfqIdilZy8758933 = -698463503;    int PcczXfoiKuvwfqIdilZy9579220 = -932975565;    int PcczXfoiKuvwfqIdilZy1451168 = -961754519;    int PcczXfoiKuvwfqIdilZy62411261 = 44343071;    int PcczXfoiKuvwfqIdilZy74645502 = -224079017;    int PcczXfoiKuvwfqIdilZy34236589 = -438495640;    int PcczXfoiKuvwfqIdilZy97895324 = -504066120;    int PcczXfoiKuvwfqIdilZy95093023 = -796294892;    int PcczXfoiKuvwfqIdilZy40316966 = -873481274;    int PcczXfoiKuvwfqIdilZy39214622 = -202517333;    int PcczXfoiKuvwfqIdilZy48145034 = -632967851;    int PcczXfoiKuvwfqIdilZy53868771 = -399497480;    int PcczXfoiKuvwfqIdilZy5308685 = -554252158;    int PcczXfoiKuvwfqIdilZy86852129 = -793486037;    int PcczXfoiKuvwfqIdilZy80281754 = -232280996;    int PcczXfoiKuvwfqIdilZy89357100 = -230653182;    int PcczXfoiKuvwfqIdilZy52609803 = -763784931;    int PcczXfoiKuvwfqIdilZy38954026 = -6412794;    int PcczXfoiKuvwfqIdilZy59664919 = -36459449;    int PcczXfoiKuvwfqIdilZy43237738 = 89810026;    int PcczXfoiKuvwfqIdilZy68411321 = -256085520;    int PcczXfoiKuvwfqIdilZy4600623 = 28308865;    int PcczXfoiKuvwfqIdilZy41434780 = -664109415;    int PcczXfoiKuvwfqIdilZy12694725 = -510171216;    int PcczXfoiKuvwfqIdilZy13615938 = -418656471;    int PcczXfoiKuvwfqIdilZy96747749 = -261505974;    int PcczXfoiKuvwfqIdilZy90692342 = -946173459;    int PcczXfoiKuvwfqIdilZy26927435 = -929257855;    int PcczXfoiKuvwfqIdilZy4586712 = -138576523;    int PcczXfoiKuvwfqIdilZy54061061 = -178291970;    int PcczXfoiKuvwfqIdilZy84484785 = -795526356;    int PcczXfoiKuvwfqIdilZy98125520 = -744373378;    int PcczXfoiKuvwfqIdilZy97686569 = -555581720;    int PcczXfoiKuvwfqIdilZy22759181 = -628281265;    int PcczXfoiKuvwfqIdilZy73240274 = -971301900;    int PcczXfoiKuvwfqIdilZy97788391 = -977868159;    int PcczXfoiKuvwfqIdilZy55488861 = -228605593;    int PcczXfoiKuvwfqIdilZy48255903 = -399911352;    int PcczXfoiKuvwfqIdilZy55210337 = -673938768;    int PcczXfoiKuvwfqIdilZy62448970 = -476229958;    int PcczXfoiKuvwfqIdilZy19752449 = -883715697;    int PcczXfoiKuvwfqIdilZy70164560 = -297404938;    int PcczXfoiKuvwfqIdilZy33808872 = -436823374;    int PcczXfoiKuvwfqIdilZy27881478 = -125328721;    int PcczXfoiKuvwfqIdilZy64460813 = -607130537;    int PcczXfoiKuvwfqIdilZy83594610 = -174419058;    int PcczXfoiKuvwfqIdilZy88642633 = -711691239;    int PcczXfoiKuvwfqIdilZy17088717 = -112447597;    int PcczXfoiKuvwfqIdilZy77433642 = -917280577;    int PcczXfoiKuvwfqIdilZy4406511 = -461394855;    int PcczXfoiKuvwfqIdilZy55399752 = -699584919;    int PcczXfoiKuvwfqIdilZy28828587 = -17346473;    int PcczXfoiKuvwfqIdilZy15734596 = -340457917;    int PcczXfoiKuvwfqIdilZy34986992 = -185141984;    int PcczXfoiKuvwfqIdilZy70745186 = -485060187;    int PcczXfoiKuvwfqIdilZy4158310 = -626772369;    int PcczXfoiKuvwfqIdilZy68144440 = -168866151;    int PcczXfoiKuvwfqIdilZy88756443 = -351583304;    int PcczXfoiKuvwfqIdilZy48795324 = -537000458;    int PcczXfoiKuvwfqIdilZy77897753 = -962573044;    int PcczXfoiKuvwfqIdilZy43544246 = -492322181;    int PcczXfoiKuvwfqIdilZy70967890 = -574808266;    int PcczXfoiKuvwfqIdilZy90506311 = -557718370;    int PcczXfoiKuvwfqIdilZy86255905 = -595189304;    int PcczXfoiKuvwfqIdilZy54729837 = -406990977;    int PcczXfoiKuvwfqIdilZy50019514 = -888594473;    int PcczXfoiKuvwfqIdilZy56182202 = -843915760;    int PcczXfoiKuvwfqIdilZy82549503 = -925970894;    int PcczXfoiKuvwfqIdilZy13611855 = -822184138;    int PcczXfoiKuvwfqIdilZy82493363 = -254412838;    int PcczXfoiKuvwfqIdilZy33868239 = 97952411;    int PcczXfoiKuvwfqIdilZy4353901 = -263873580;    int PcczXfoiKuvwfqIdilZy83743688 = -332474027;    int PcczXfoiKuvwfqIdilZy97215949 = -560229492;    int PcczXfoiKuvwfqIdilZy23485289 = -26474277;    int PcczXfoiKuvwfqIdilZy98246761 = -958680582;    int PcczXfoiKuvwfqIdilZy70791750 = -534867761;    int PcczXfoiKuvwfqIdilZy13553302 = -438780694;    int PcczXfoiKuvwfqIdilZy48233912 = -903040680;    int PcczXfoiKuvwfqIdilZy30021327 = -144237414;    int PcczXfoiKuvwfqIdilZy8105117 = -549814736;    int PcczXfoiKuvwfqIdilZy73603626 = -733725863;    int PcczXfoiKuvwfqIdilZy49493793 = 88022721;    int PcczXfoiKuvwfqIdilZy180202 = 85274403;     PcczXfoiKuvwfqIdilZy96685558 = PcczXfoiKuvwfqIdilZy17647774;     PcczXfoiKuvwfqIdilZy17647774 = PcczXfoiKuvwfqIdilZy65257583;     PcczXfoiKuvwfqIdilZy65257583 = PcczXfoiKuvwfqIdilZy74125838;     PcczXfoiKuvwfqIdilZy74125838 = PcczXfoiKuvwfqIdilZy67096100;     PcczXfoiKuvwfqIdilZy67096100 = PcczXfoiKuvwfqIdilZy12605848;     PcczXfoiKuvwfqIdilZy12605848 = PcczXfoiKuvwfqIdilZy37463382;     PcczXfoiKuvwfqIdilZy37463382 = PcczXfoiKuvwfqIdilZy93951317;     PcczXfoiKuvwfqIdilZy93951317 = PcczXfoiKuvwfqIdilZy3940846;     PcczXfoiKuvwfqIdilZy3940846 = PcczXfoiKuvwfqIdilZy57715397;     PcczXfoiKuvwfqIdilZy57715397 = PcczXfoiKuvwfqIdilZy93763611;     PcczXfoiKuvwfqIdilZy93763611 = PcczXfoiKuvwfqIdilZy8009555;     PcczXfoiKuvwfqIdilZy8009555 = PcczXfoiKuvwfqIdilZy67782612;     PcczXfoiKuvwfqIdilZy67782612 = PcczXfoiKuvwfqIdilZy75399515;     PcczXfoiKuvwfqIdilZy75399515 = PcczXfoiKuvwfqIdilZy78224730;     PcczXfoiKuvwfqIdilZy78224730 = PcczXfoiKuvwfqIdilZy39156508;     PcczXfoiKuvwfqIdilZy39156508 = PcczXfoiKuvwfqIdilZy8758933;     PcczXfoiKuvwfqIdilZy8758933 = PcczXfoiKuvwfqIdilZy9579220;     PcczXfoiKuvwfqIdilZy9579220 = PcczXfoiKuvwfqIdilZy1451168;     PcczXfoiKuvwfqIdilZy1451168 = PcczXfoiKuvwfqIdilZy62411261;     PcczXfoiKuvwfqIdilZy62411261 = PcczXfoiKuvwfqIdilZy74645502;     PcczXfoiKuvwfqIdilZy74645502 = PcczXfoiKuvwfqIdilZy34236589;     PcczXfoiKuvwfqIdilZy34236589 = PcczXfoiKuvwfqIdilZy97895324;     PcczXfoiKuvwfqIdilZy97895324 = PcczXfoiKuvwfqIdilZy95093023;     PcczXfoiKuvwfqIdilZy95093023 = PcczXfoiKuvwfqIdilZy40316966;     PcczXfoiKuvwfqIdilZy40316966 = PcczXfoiKuvwfqIdilZy39214622;     PcczXfoiKuvwfqIdilZy39214622 = PcczXfoiKuvwfqIdilZy48145034;     PcczXfoiKuvwfqIdilZy48145034 = PcczXfoiKuvwfqIdilZy53868771;     PcczXfoiKuvwfqIdilZy53868771 = PcczXfoiKuvwfqIdilZy5308685;     PcczXfoiKuvwfqIdilZy5308685 = PcczXfoiKuvwfqIdilZy86852129;     PcczXfoiKuvwfqIdilZy86852129 = PcczXfoiKuvwfqIdilZy80281754;     PcczXfoiKuvwfqIdilZy80281754 = PcczXfoiKuvwfqIdilZy89357100;     PcczXfoiKuvwfqIdilZy89357100 = PcczXfoiKuvwfqIdilZy52609803;     PcczXfoiKuvwfqIdilZy52609803 = PcczXfoiKuvwfqIdilZy38954026;     PcczXfoiKuvwfqIdilZy38954026 = PcczXfoiKuvwfqIdilZy59664919;     PcczXfoiKuvwfqIdilZy59664919 = PcczXfoiKuvwfqIdilZy43237738;     PcczXfoiKuvwfqIdilZy43237738 = PcczXfoiKuvwfqIdilZy68411321;     PcczXfoiKuvwfqIdilZy68411321 = PcczXfoiKuvwfqIdilZy4600623;     PcczXfoiKuvwfqIdilZy4600623 = PcczXfoiKuvwfqIdilZy41434780;     PcczXfoiKuvwfqIdilZy41434780 = PcczXfoiKuvwfqIdilZy12694725;     PcczXfoiKuvwfqIdilZy12694725 = PcczXfoiKuvwfqIdilZy13615938;     PcczXfoiKuvwfqIdilZy13615938 = PcczXfoiKuvwfqIdilZy96747749;     PcczXfoiKuvwfqIdilZy96747749 = PcczXfoiKuvwfqIdilZy90692342;     PcczXfoiKuvwfqIdilZy90692342 = PcczXfoiKuvwfqIdilZy26927435;     PcczXfoiKuvwfqIdilZy26927435 = PcczXfoiKuvwfqIdilZy4586712;     PcczXfoiKuvwfqIdilZy4586712 = PcczXfoiKuvwfqIdilZy54061061;     PcczXfoiKuvwfqIdilZy54061061 = PcczXfoiKuvwfqIdilZy84484785;     PcczXfoiKuvwfqIdilZy84484785 = PcczXfoiKuvwfqIdilZy98125520;     PcczXfoiKuvwfqIdilZy98125520 = PcczXfoiKuvwfqIdilZy97686569;     PcczXfoiKuvwfqIdilZy97686569 = PcczXfoiKuvwfqIdilZy22759181;     PcczXfoiKuvwfqIdilZy22759181 = PcczXfoiKuvwfqIdilZy73240274;     PcczXfoiKuvwfqIdilZy73240274 = PcczXfoiKuvwfqIdilZy97788391;     PcczXfoiKuvwfqIdilZy97788391 = PcczXfoiKuvwfqIdilZy55488861;     PcczXfoiKuvwfqIdilZy55488861 = PcczXfoiKuvwfqIdilZy48255903;     PcczXfoiKuvwfqIdilZy48255903 = PcczXfoiKuvwfqIdilZy55210337;     PcczXfoiKuvwfqIdilZy55210337 = PcczXfoiKuvwfqIdilZy62448970;     PcczXfoiKuvwfqIdilZy62448970 = PcczXfoiKuvwfqIdilZy19752449;     PcczXfoiKuvwfqIdilZy19752449 = PcczXfoiKuvwfqIdilZy70164560;     PcczXfoiKuvwfqIdilZy70164560 = PcczXfoiKuvwfqIdilZy33808872;     PcczXfoiKuvwfqIdilZy33808872 = PcczXfoiKuvwfqIdilZy27881478;     PcczXfoiKuvwfqIdilZy27881478 = PcczXfoiKuvwfqIdilZy64460813;     PcczXfoiKuvwfqIdilZy64460813 = PcczXfoiKuvwfqIdilZy83594610;     PcczXfoiKuvwfqIdilZy83594610 = PcczXfoiKuvwfqIdilZy88642633;     PcczXfoiKuvwfqIdilZy88642633 = PcczXfoiKuvwfqIdilZy17088717;     PcczXfoiKuvwfqIdilZy17088717 = PcczXfoiKuvwfqIdilZy77433642;     PcczXfoiKuvwfqIdilZy77433642 = PcczXfoiKuvwfqIdilZy4406511;     PcczXfoiKuvwfqIdilZy4406511 = PcczXfoiKuvwfqIdilZy55399752;     PcczXfoiKuvwfqIdilZy55399752 = PcczXfoiKuvwfqIdilZy28828587;     PcczXfoiKuvwfqIdilZy28828587 = PcczXfoiKuvwfqIdilZy15734596;     PcczXfoiKuvwfqIdilZy15734596 = PcczXfoiKuvwfqIdilZy34986992;     PcczXfoiKuvwfqIdilZy34986992 = PcczXfoiKuvwfqIdilZy70745186;     PcczXfoiKuvwfqIdilZy70745186 = PcczXfoiKuvwfqIdilZy4158310;     PcczXfoiKuvwfqIdilZy4158310 = PcczXfoiKuvwfqIdilZy68144440;     PcczXfoiKuvwfqIdilZy68144440 = PcczXfoiKuvwfqIdilZy88756443;     PcczXfoiKuvwfqIdilZy88756443 = PcczXfoiKuvwfqIdilZy48795324;     PcczXfoiKuvwfqIdilZy48795324 = PcczXfoiKuvwfqIdilZy77897753;     PcczXfoiKuvwfqIdilZy77897753 = PcczXfoiKuvwfqIdilZy43544246;     PcczXfoiKuvwfqIdilZy43544246 = PcczXfoiKuvwfqIdilZy70967890;     PcczXfoiKuvwfqIdilZy70967890 = PcczXfoiKuvwfqIdilZy90506311;     PcczXfoiKuvwfqIdilZy90506311 = PcczXfoiKuvwfqIdilZy86255905;     PcczXfoiKuvwfqIdilZy86255905 = PcczXfoiKuvwfqIdilZy54729837;     PcczXfoiKuvwfqIdilZy54729837 = PcczXfoiKuvwfqIdilZy50019514;     PcczXfoiKuvwfqIdilZy50019514 = PcczXfoiKuvwfqIdilZy56182202;     PcczXfoiKuvwfqIdilZy56182202 = PcczXfoiKuvwfqIdilZy82549503;     PcczXfoiKuvwfqIdilZy82549503 = PcczXfoiKuvwfqIdilZy13611855;     PcczXfoiKuvwfqIdilZy13611855 = PcczXfoiKuvwfqIdilZy82493363;     PcczXfoiKuvwfqIdilZy82493363 = PcczXfoiKuvwfqIdilZy33868239;     PcczXfoiKuvwfqIdilZy33868239 = PcczXfoiKuvwfqIdilZy4353901;     PcczXfoiKuvwfqIdilZy4353901 = PcczXfoiKuvwfqIdilZy83743688;     PcczXfoiKuvwfqIdilZy83743688 = PcczXfoiKuvwfqIdilZy97215949;     PcczXfoiKuvwfqIdilZy97215949 = PcczXfoiKuvwfqIdilZy23485289;     PcczXfoiKuvwfqIdilZy23485289 = PcczXfoiKuvwfqIdilZy98246761;     PcczXfoiKuvwfqIdilZy98246761 = PcczXfoiKuvwfqIdilZy70791750;     PcczXfoiKuvwfqIdilZy70791750 = PcczXfoiKuvwfqIdilZy13553302;     PcczXfoiKuvwfqIdilZy13553302 = PcczXfoiKuvwfqIdilZy48233912;     PcczXfoiKuvwfqIdilZy48233912 = PcczXfoiKuvwfqIdilZy30021327;     PcczXfoiKuvwfqIdilZy30021327 = PcczXfoiKuvwfqIdilZy8105117;     PcczXfoiKuvwfqIdilZy8105117 = PcczXfoiKuvwfqIdilZy73603626;     PcczXfoiKuvwfqIdilZy73603626 = PcczXfoiKuvwfqIdilZy49493793;     PcczXfoiKuvwfqIdilZy49493793 = PcczXfoiKuvwfqIdilZy180202;     PcczXfoiKuvwfqIdilZy180202 = PcczXfoiKuvwfqIdilZy96685558;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void zMgwHzWIeqCakHbEMzSG36264979() {     int kxIRiJrVfbquQHSzoZih30774350 = -244123999;    int kxIRiJrVfbquQHSzoZih73000554 = 23395395;    int kxIRiJrVfbquQHSzoZih36503160 = -656940136;    int kxIRiJrVfbquQHSzoZih34251242 = -693881933;    int kxIRiJrVfbquQHSzoZih20820898 = -104620110;    int kxIRiJrVfbquQHSzoZih45431406 = -83469956;    int kxIRiJrVfbquQHSzoZih44725219 = -292482280;    int kxIRiJrVfbquQHSzoZih34263476 = -126985485;    int kxIRiJrVfbquQHSzoZih16048656 = -859564033;    int kxIRiJrVfbquQHSzoZih18260374 = -293696570;    int kxIRiJrVfbquQHSzoZih83297433 = -305320552;    int kxIRiJrVfbquQHSzoZih16379138 = -577972047;    int kxIRiJrVfbquQHSzoZih13925223 = -333282860;    int kxIRiJrVfbquQHSzoZih39432235 = -665706041;    int kxIRiJrVfbquQHSzoZih39106834 = -589533950;    int kxIRiJrVfbquQHSzoZih97315284 = -665533081;    int kxIRiJrVfbquQHSzoZih10616201 = 80694402;    int kxIRiJrVfbquQHSzoZih51208164 = -400383659;    int kxIRiJrVfbquQHSzoZih27606726 = -956517037;    int kxIRiJrVfbquQHSzoZih44252418 = -300585567;    int kxIRiJrVfbquQHSzoZih10559053 = -14800762;    int kxIRiJrVfbquQHSzoZih80199285 = 24853179;    int kxIRiJrVfbquQHSzoZih94156330 = -558339239;    int kxIRiJrVfbquQHSzoZih82104851 = -46401121;    int kxIRiJrVfbquQHSzoZih59433485 = -497301046;    int kxIRiJrVfbquQHSzoZih95363442 = -745983866;    int kxIRiJrVfbquQHSzoZih66993285 = -657097737;    int kxIRiJrVfbquQHSzoZih38645223 = -650968678;    int kxIRiJrVfbquQHSzoZih22077874 = -735611910;    int kxIRiJrVfbquQHSzoZih89451130 = -227485098;    int kxIRiJrVfbquQHSzoZih48868725 = -729184447;    int kxIRiJrVfbquQHSzoZih79850383 = -766326836;    int kxIRiJrVfbquQHSzoZih70089049 = -381894475;    int kxIRiJrVfbquQHSzoZih99740085 = -901907289;    int kxIRiJrVfbquQHSzoZih51658246 = -167174638;    int kxIRiJrVfbquQHSzoZih36125577 = 46170562;    int kxIRiJrVfbquQHSzoZih14310609 = -814707464;    int kxIRiJrVfbquQHSzoZih27130512 = -119932516;    int kxIRiJrVfbquQHSzoZih72467749 = -160954609;    int kxIRiJrVfbquQHSzoZih41004783 = -567440232;    int kxIRiJrVfbquQHSzoZih40051893 = -543624025;    int kxIRiJrVfbquQHSzoZih49062530 = -620296197;    int kxIRiJrVfbquQHSzoZih92776897 = -556098171;    int kxIRiJrVfbquQHSzoZih53678283 = -891927684;    int kxIRiJrVfbquQHSzoZih88317191 = -582764453;    int kxIRiJrVfbquQHSzoZih15006089 = -791933079;    int kxIRiJrVfbquQHSzoZih7221083 = 92318159;    int kxIRiJrVfbquQHSzoZih18961250 = -758853651;    int kxIRiJrVfbquQHSzoZih30810173 = -893352813;    int kxIRiJrVfbquQHSzoZih64402848 = -646233634;    int kxIRiJrVfbquQHSzoZih66806601 = -607003532;    int kxIRiJrVfbquQHSzoZih43060207 = -93970139;    int kxIRiJrVfbquQHSzoZih65966899 = -315509867;    int kxIRiJrVfbquQHSzoZih16967431 = -772261446;    int kxIRiJrVfbquQHSzoZih25705926 = 48372307;    int kxIRiJrVfbquQHSzoZih50575065 = -168977179;    int kxIRiJrVfbquQHSzoZih78844224 = -418265367;    int kxIRiJrVfbquQHSzoZih54398309 = -510539015;    int kxIRiJrVfbquQHSzoZih74817756 = -96580887;    int kxIRiJrVfbquQHSzoZih25457456 = -358636245;    int kxIRiJrVfbquQHSzoZih78438120 = -426372220;    int kxIRiJrVfbquQHSzoZih6079996 = -641513603;    int kxIRiJrVfbquQHSzoZih12185603 = -391373575;    int kxIRiJrVfbquQHSzoZih26597525 = -532078936;    int kxIRiJrVfbquQHSzoZih69391648 = -564512123;    int kxIRiJrVfbquQHSzoZih3447051 = -538993716;    int kxIRiJrVfbquQHSzoZih46290089 = -96077572;    int kxIRiJrVfbquQHSzoZih14185138 = -431375572;    int kxIRiJrVfbquQHSzoZih87773988 = -398531403;    int kxIRiJrVfbquQHSzoZih2981258 = -535704512;    int kxIRiJrVfbquQHSzoZih83004676 = -850825617;    int kxIRiJrVfbquQHSzoZih83485688 = -799373082;    int kxIRiJrVfbquQHSzoZih78740414 = -139429051;    int kxIRiJrVfbquQHSzoZih86601943 = -289076805;    int kxIRiJrVfbquQHSzoZih4200525 = -756961543;    int kxIRiJrVfbquQHSzoZih61496523 = -394504566;    int kxIRiJrVfbquQHSzoZih87422387 = -419048650;    int kxIRiJrVfbquQHSzoZih40478047 = -666411556;    int kxIRiJrVfbquQHSzoZih93787659 = -463636669;    int kxIRiJrVfbquQHSzoZih44427397 = -705367968;    int kxIRiJrVfbquQHSzoZih88142359 = -738302025;    int kxIRiJrVfbquQHSzoZih48032036 = -898244087;    int kxIRiJrVfbquQHSzoZih7835051 = -757615866;    int kxIRiJrVfbquQHSzoZih57675026 = 10621723;    int kxIRiJrVfbquQHSzoZih22644529 = -620481566;    int kxIRiJrVfbquQHSzoZih5808518 = -535214309;    int kxIRiJrVfbquQHSzoZih13883484 = -350816969;    int kxIRiJrVfbquQHSzoZih53121618 = -609633030;    int kxIRiJrVfbquQHSzoZih74034159 = -850279596;    int kxIRiJrVfbquQHSzoZih1083182 = -998197460;    int kxIRiJrVfbquQHSzoZih57281352 = -535564072;    int kxIRiJrVfbquQHSzoZih59912300 = -204168450;    int kxIRiJrVfbquQHSzoZih52312755 = 76648370;    int kxIRiJrVfbquQHSzoZih47010294 = -802318364;    int kxIRiJrVfbquQHSzoZih62566662 = -41068012;    int kxIRiJrVfbquQHSzoZih33971898 = -902110423;    int kxIRiJrVfbquQHSzoZih36876928 = -128922622;    int kxIRiJrVfbquQHSzoZih66179372 = 75980765;    int kxIRiJrVfbquQHSzoZih84286635 = -227415561;    int kxIRiJrVfbquQHSzoZih84870140 = -244123999;     kxIRiJrVfbquQHSzoZih30774350 = kxIRiJrVfbquQHSzoZih73000554;     kxIRiJrVfbquQHSzoZih73000554 = kxIRiJrVfbquQHSzoZih36503160;     kxIRiJrVfbquQHSzoZih36503160 = kxIRiJrVfbquQHSzoZih34251242;     kxIRiJrVfbquQHSzoZih34251242 = kxIRiJrVfbquQHSzoZih20820898;     kxIRiJrVfbquQHSzoZih20820898 = kxIRiJrVfbquQHSzoZih45431406;     kxIRiJrVfbquQHSzoZih45431406 = kxIRiJrVfbquQHSzoZih44725219;     kxIRiJrVfbquQHSzoZih44725219 = kxIRiJrVfbquQHSzoZih34263476;     kxIRiJrVfbquQHSzoZih34263476 = kxIRiJrVfbquQHSzoZih16048656;     kxIRiJrVfbquQHSzoZih16048656 = kxIRiJrVfbquQHSzoZih18260374;     kxIRiJrVfbquQHSzoZih18260374 = kxIRiJrVfbquQHSzoZih83297433;     kxIRiJrVfbquQHSzoZih83297433 = kxIRiJrVfbquQHSzoZih16379138;     kxIRiJrVfbquQHSzoZih16379138 = kxIRiJrVfbquQHSzoZih13925223;     kxIRiJrVfbquQHSzoZih13925223 = kxIRiJrVfbquQHSzoZih39432235;     kxIRiJrVfbquQHSzoZih39432235 = kxIRiJrVfbquQHSzoZih39106834;     kxIRiJrVfbquQHSzoZih39106834 = kxIRiJrVfbquQHSzoZih97315284;     kxIRiJrVfbquQHSzoZih97315284 = kxIRiJrVfbquQHSzoZih10616201;     kxIRiJrVfbquQHSzoZih10616201 = kxIRiJrVfbquQHSzoZih51208164;     kxIRiJrVfbquQHSzoZih51208164 = kxIRiJrVfbquQHSzoZih27606726;     kxIRiJrVfbquQHSzoZih27606726 = kxIRiJrVfbquQHSzoZih44252418;     kxIRiJrVfbquQHSzoZih44252418 = kxIRiJrVfbquQHSzoZih10559053;     kxIRiJrVfbquQHSzoZih10559053 = kxIRiJrVfbquQHSzoZih80199285;     kxIRiJrVfbquQHSzoZih80199285 = kxIRiJrVfbquQHSzoZih94156330;     kxIRiJrVfbquQHSzoZih94156330 = kxIRiJrVfbquQHSzoZih82104851;     kxIRiJrVfbquQHSzoZih82104851 = kxIRiJrVfbquQHSzoZih59433485;     kxIRiJrVfbquQHSzoZih59433485 = kxIRiJrVfbquQHSzoZih95363442;     kxIRiJrVfbquQHSzoZih95363442 = kxIRiJrVfbquQHSzoZih66993285;     kxIRiJrVfbquQHSzoZih66993285 = kxIRiJrVfbquQHSzoZih38645223;     kxIRiJrVfbquQHSzoZih38645223 = kxIRiJrVfbquQHSzoZih22077874;     kxIRiJrVfbquQHSzoZih22077874 = kxIRiJrVfbquQHSzoZih89451130;     kxIRiJrVfbquQHSzoZih89451130 = kxIRiJrVfbquQHSzoZih48868725;     kxIRiJrVfbquQHSzoZih48868725 = kxIRiJrVfbquQHSzoZih79850383;     kxIRiJrVfbquQHSzoZih79850383 = kxIRiJrVfbquQHSzoZih70089049;     kxIRiJrVfbquQHSzoZih70089049 = kxIRiJrVfbquQHSzoZih99740085;     kxIRiJrVfbquQHSzoZih99740085 = kxIRiJrVfbquQHSzoZih51658246;     kxIRiJrVfbquQHSzoZih51658246 = kxIRiJrVfbquQHSzoZih36125577;     kxIRiJrVfbquQHSzoZih36125577 = kxIRiJrVfbquQHSzoZih14310609;     kxIRiJrVfbquQHSzoZih14310609 = kxIRiJrVfbquQHSzoZih27130512;     kxIRiJrVfbquQHSzoZih27130512 = kxIRiJrVfbquQHSzoZih72467749;     kxIRiJrVfbquQHSzoZih72467749 = kxIRiJrVfbquQHSzoZih41004783;     kxIRiJrVfbquQHSzoZih41004783 = kxIRiJrVfbquQHSzoZih40051893;     kxIRiJrVfbquQHSzoZih40051893 = kxIRiJrVfbquQHSzoZih49062530;     kxIRiJrVfbquQHSzoZih49062530 = kxIRiJrVfbquQHSzoZih92776897;     kxIRiJrVfbquQHSzoZih92776897 = kxIRiJrVfbquQHSzoZih53678283;     kxIRiJrVfbquQHSzoZih53678283 = kxIRiJrVfbquQHSzoZih88317191;     kxIRiJrVfbquQHSzoZih88317191 = kxIRiJrVfbquQHSzoZih15006089;     kxIRiJrVfbquQHSzoZih15006089 = kxIRiJrVfbquQHSzoZih7221083;     kxIRiJrVfbquQHSzoZih7221083 = kxIRiJrVfbquQHSzoZih18961250;     kxIRiJrVfbquQHSzoZih18961250 = kxIRiJrVfbquQHSzoZih30810173;     kxIRiJrVfbquQHSzoZih30810173 = kxIRiJrVfbquQHSzoZih64402848;     kxIRiJrVfbquQHSzoZih64402848 = kxIRiJrVfbquQHSzoZih66806601;     kxIRiJrVfbquQHSzoZih66806601 = kxIRiJrVfbquQHSzoZih43060207;     kxIRiJrVfbquQHSzoZih43060207 = kxIRiJrVfbquQHSzoZih65966899;     kxIRiJrVfbquQHSzoZih65966899 = kxIRiJrVfbquQHSzoZih16967431;     kxIRiJrVfbquQHSzoZih16967431 = kxIRiJrVfbquQHSzoZih25705926;     kxIRiJrVfbquQHSzoZih25705926 = kxIRiJrVfbquQHSzoZih50575065;     kxIRiJrVfbquQHSzoZih50575065 = kxIRiJrVfbquQHSzoZih78844224;     kxIRiJrVfbquQHSzoZih78844224 = kxIRiJrVfbquQHSzoZih54398309;     kxIRiJrVfbquQHSzoZih54398309 = kxIRiJrVfbquQHSzoZih74817756;     kxIRiJrVfbquQHSzoZih74817756 = kxIRiJrVfbquQHSzoZih25457456;     kxIRiJrVfbquQHSzoZih25457456 = kxIRiJrVfbquQHSzoZih78438120;     kxIRiJrVfbquQHSzoZih78438120 = kxIRiJrVfbquQHSzoZih6079996;     kxIRiJrVfbquQHSzoZih6079996 = kxIRiJrVfbquQHSzoZih12185603;     kxIRiJrVfbquQHSzoZih12185603 = kxIRiJrVfbquQHSzoZih26597525;     kxIRiJrVfbquQHSzoZih26597525 = kxIRiJrVfbquQHSzoZih69391648;     kxIRiJrVfbquQHSzoZih69391648 = kxIRiJrVfbquQHSzoZih3447051;     kxIRiJrVfbquQHSzoZih3447051 = kxIRiJrVfbquQHSzoZih46290089;     kxIRiJrVfbquQHSzoZih46290089 = kxIRiJrVfbquQHSzoZih14185138;     kxIRiJrVfbquQHSzoZih14185138 = kxIRiJrVfbquQHSzoZih87773988;     kxIRiJrVfbquQHSzoZih87773988 = kxIRiJrVfbquQHSzoZih2981258;     kxIRiJrVfbquQHSzoZih2981258 = kxIRiJrVfbquQHSzoZih83004676;     kxIRiJrVfbquQHSzoZih83004676 = kxIRiJrVfbquQHSzoZih83485688;     kxIRiJrVfbquQHSzoZih83485688 = kxIRiJrVfbquQHSzoZih78740414;     kxIRiJrVfbquQHSzoZih78740414 = kxIRiJrVfbquQHSzoZih86601943;     kxIRiJrVfbquQHSzoZih86601943 = kxIRiJrVfbquQHSzoZih4200525;     kxIRiJrVfbquQHSzoZih4200525 = kxIRiJrVfbquQHSzoZih61496523;     kxIRiJrVfbquQHSzoZih61496523 = kxIRiJrVfbquQHSzoZih87422387;     kxIRiJrVfbquQHSzoZih87422387 = kxIRiJrVfbquQHSzoZih40478047;     kxIRiJrVfbquQHSzoZih40478047 = kxIRiJrVfbquQHSzoZih93787659;     kxIRiJrVfbquQHSzoZih93787659 = kxIRiJrVfbquQHSzoZih44427397;     kxIRiJrVfbquQHSzoZih44427397 = kxIRiJrVfbquQHSzoZih88142359;     kxIRiJrVfbquQHSzoZih88142359 = kxIRiJrVfbquQHSzoZih48032036;     kxIRiJrVfbquQHSzoZih48032036 = kxIRiJrVfbquQHSzoZih7835051;     kxIRiJrVfbquQHSzoZih7835051 = kxIRiJrVfbquQHSzoZih57675026;     kxIRiJrVfbquQHSzoZih57675026 = kxIRiJrVfbquQHSzoZih22644529;     kxIRiJrVfbquQHSzoZih22644529 = kxIRiJrVfbquQHSzoZih5808518;     kxIRiJrVfbquQHSzoZih5808518 = kxIRiJrVfbquQHSzoZih13883484;     kxIRiJrVfbquQHSzoZih13883484 = kxIRiJrVfbquQHSzoZih53121618;     kxIRiJrVfbquQHSzoZih53121618 = kxIRiJrVfbquQHSzoZih74034159;     kxIRiJrVfbquQHSzoZih74034159 = kxIRiJrVfbquQHSzoZih1083182;     kxIRiJrVfbquQHSzoZih1083182 = kxIRiJrVfbquQHSzoZih57281352;     kxIRiJrVfbquQHSzoZih57281352 = kxIRiJrVfbquQHSzoZih59912300;     kxIRiJrVfbquQHSzoZih59912300 = kxIRiJrVfbquQHSzoZih52312755;     kxIRiJrVfbquQHSzoZih52312755 = kxIRiJrVfbquQHSzoZih47010294;     kxIRiJrVfbquQHSzoZih47010294 = kxIRiJrVfbquQHSzoZih62566662;     kxIRiJrVfbquQHSzoZih62566662 = kxIRiJrVfbquQHSzoZih33971898;     kxIRiJrVfbquQHSzoZih33971898 = kxIRiJrVfbquQHSzoZih36876928;     kxIRiJrVfbquQHSzoZih36876928 = kxIRiJrVfbquQHSzoZih66179372;     kxIRiJrVfbquQHSzoZih66179372 = kxIRiJrVfbquQHSzoZih84286635;     kxIRiJrVfbquQHSzoZih84286635 = kxIRiJrVfbquQHSzoZih84870140;     kxIRiJrVfbquQHSzoZih84870140 = kxIRiJrVfbquQHSzoZih30774350;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void aFmVAtPkmjvgdUihiZSB94916650() {     int pFDklSnYUrrHrqWTKbSF94205002 = -927897413;    int pFDklSnYUrrHrqWTKbSF98252133 = -836379219;    int pFDklSnYUrrHrqWTKbSF29093030 = -735014595;    int pFDklSnYUrrHrqWTKbSF39525220 = -707861659;    int pFDklSnYUrrHrqWTKbSF61935475 = -466343142;    int pFDklSnYUrrHrqWTKbSF23062634 = 75306814;    int pFDklSnYUrrHrqWTKbSF2155496 = -716808781;    int pFDklSnYUrrHrqWTKbSF86947303 = -445387801;    int pFDklSnYUrrHrqWTKbSF65031575 = -333493174;    int pFDklSnYUrrHrqWTKbSF52478402 = -873543087;    int pFDklSnYUrrHrqWTKbSF50902701 = -843163210;    int pFDklSnYUrrHrqWTKbSF93493826 = -112288814;    int pFDklSnYUrrHrqWTKbSF12093359 = -654927871;    int pFDklSnYUrrHrqWTKbSF73462141 = -298619558;    int pFDklSnYUrrHrqWTKbSF62441027 = -796698791;    int pFDklSnYUrrHrqWTKbSF97509969 = -152732519;    int pFDklSnYUrrHrqWTKbSF74473401 = -150577132;    int pFDklSnYUrrHrqWTKbSF95544147 = -275697179;    int pFDklSnYUrrHrqWTKbSF92482771 = -862073878;    int pFDklSnYUrrHrqWTKbSF39380629 = -715172352;    int pFDklSnYUrrHrqWTKbSF71006273 = -707309741;    int pFDklSnYUrrHrqWTKbSF43062483 = -790504580;    int pFDklSnYUrrHrqWTKbSF25351923 = -503727054;    int pFDklSnYUrrHrqWTKbSF78917250 = -685026966;    int pFDklSnYUrrHrqWTKbSF42868306 = -871909984;    int pFDklSnYUrrHrqWTKbSF37488758 = 628118;    int pFDklSnYUrrHrqWTKbSF47802333 = -586849561;    int pFDklSnYUrrHrqWTKbSF51042974 = 93716903;    int pFDklSnYUrrHrqWTKbSF19383022 = -168683760;    int pFDklSnYUrrHrqWTKbSF28967905 = -978520461;    int pFDklSnYUrrHrqWTKbSF43796378 = -764747447;    int pFDklSnYUrrHrqWTKbSF10556062 = -265525089;    int pFDklSnYUrrHrqWTKbSF90075357 = -254986626;    int pFDklSnYUrrHrqWTKbSF95444006 = 89446168;    int pFDklSnYUrrHrqWTKbSF47985374 = -710256973;    int pFDklSnYUrrHrqWTKbSF11848346 = 1991899;    int pFDklSnYUrrHrqWTKbSF28359778 = -184389580;    int pFDklSnYUrrHrqWTKbSF66785592 = -479323469;    int pFDklSnYUrrHrqWTKbSF60281028 = -414752582;    int pFDklSnYUrrHrqWTKbSF46084434 = -716532571;    int pFDklSnYUrrHrqWTKbSF94270232 = -372516600;    int pFDklSnYUrrHrqWTKbSF58591980 = -303489432;    int pFDklSnYUrrHrqWTKbSF6177853 = -738399085;    int pFDklSnYUrrHrqWTKbSF33152730 = -339586984;    int pFDklSnYUrrHrqWTKbSF60111664 = -91565390;    int pFDklSnYUrrHrqWTKbSF9973250 = -359281704;    int pFDklSnYUrrHrqWTKbSF31523969 = -485048148;    int pFDklSnYUrrHrqWTKbSF49566349 = -929964984;    int pFDklSnYUrrHrqWTKbSF77150713 = -809123797;    int pFDklSnYUrrHrqWTKbSF17664451 = 31797989;    int pFDklSnYUrrHrqWTKbSF5442974 = -274688050;    int pFDklSnYUrrHrqWTKbSF61007776 = -722998145;    int pFDklSnYUrrHrqWTKbSF96817628 = -366670108;    int pFDklSnYUrrHrqWTKbSF5701926 = 84350106;    int pFDklSnYUrrHrqWTKbSF23910377 = 24725019;    int pFDklSnYUrrHrqWTKbSF51142520 = -37392833;    int pFDklSnYUrrHrqWTKbSF72900210 = -232652165;    int pFDklSnYUrrHrqWTKbSF50175780 = 50012370;    int pFDklSnYUrrHrqWTKbSF96656914 = -835951676;    int pFDklSnYUrrHrqWTKbSF24446718 = -366971260;    int pFDklSnYUrrHrqWTKbSF75260300 = -337843626;    int pFDklSnYUrrHrqWTKbSF51112522 = -710525684;    int pFDklSnYUrrHrqWTKbSF67564282 = -176704041;    int pFDklSnYUrrHrqWTKbSF36063670 = -354972714;    int pFDklSnYUrrHrqWTKbSF8682025 = -8795641;    int pFDklSnYUrrHrqWTKbSF40346639 = -477638121;    int pFDklSnYUrrHrqWTKbSF3418470 = -857302188;    int pFDklSnYUrrHrqWTKbSF16649353 = -644374039;    int pFDklSnYUrrHrqWTKbSF25476767 = -588362586;    int pFDklSnYUrrHrqWTKbSF50592682 = -698690691;    int pFDklSnYUrrHrqWTKbSF69150191 = -968342940;    int pFDklSnYUrrHrqWTKbSF7687809 = -671253663;    int pFDklSnYUrrHrqWTKbSF35263120 = -860944598;    int pFDklSnYUrrHrqWTKbSF46398337 = -45541308;    int pFDklSnYUrrHrqWTKbSF45110396 = -242655753;    int pFDklSnYUrrHrqWTKbSF12414294 = -303820310;    int pFDklSnYUrrHrqWTKbSF36884630 = 47894505;    int pFDklSnYUrrHrqWTKbSF92199193 = -64140071;    int pFDklSnYUrrHrqWTKbSF18805586 = -493461577;    int pFDklSnYUrrHrqWTKbSF32895056 = -412628281;    int pFDklSnYUrrHrqWTKbSF5964790 = -514323735;    int pFDklSnYUrrHrqWTKbSF98235984 = -656884577;    int pFDklSnYUrrHrqWTKbSF73892261 = -97159301;    int pFDklSnYUrrHrqWTKbSF1718571 = -100481750;    int pFDklSnYUrrHrqWTKbSF23524932 = -603832411;    int pFDklSnYUrrHrqWTKbSF82788602 = 58250698;    int pFDklSnYUrrHrqWTKbSF13738434 = -898854982;    int pFDklSnYUrrHrqWTKbSF84373431 = -239336733;    int pFDklSnYUrrHrqWTKbSF71533630 = -935278851;    int pFDklSnYUrrHrqWTKbSF96842854 = -572864140;    int pFDklSnYUrrHrqWTKbSF38948135 = -765355936;    int pFDklSnYUrrHrqWTKbSF78183998 = -134401950;    int pFDklSnYUrrHrqWTKbSF70128678 = -643371794;    int pFDklSnYUrrHrqWTKbSF35834310 = 52218678;    int pFDklSnYUrrHrqWTKbSF70824134 = -278688946;    int pFDklSnYUrrHrqWTKbSF43157711 = -661990916;    int pFDklSnYUrrHrqWTKbSF91027698 = -26785391;    int pFDklSnYUrrHrqWTKbSF70114183 = -283426372;    int pFDklSnYUrrHrqWTKbSF24470705 = -230791344;    int pFDklSnYUrrHrqWTKbSF19765026 = -927897413;     pFDklSnYUrrHrqWTKbSF94205002 = pFDklSnYUrrHrqWTKbSF98252133;     pFDklSnYUrrHrqWTKbSF98252133 = pFDklSnYUrrHrqWTKbSF29093030;     pFDklSnYUrrHrqWTKbSF29093030 = pFDklSnYUrrHrqWTKbSF39525220;     pFDklSnYUrrHrqWTKbSF39525220 = pFDklSnYUrrHrqWTKbSF61935475;     pFDklSnYUrrHrqWTKbSF61935475 = pFDklSnYUrrHrqWTKbSF23062634;     pFDklSnYUrrHrqWTKbSF23062634 = pFDklSnYUrrHrqWTKbSF2155496;     pFDklSnYUrrHrqWTKbSF2155496 = pFDklSnYUrrHrqWTKbSF86947303;     pFDklSnYUrrHrqWTKbSF86947303 = pFDklSnYUrrHrqWTKbSF65031575;     pFDklSnYUrrHrqWTKbSF65031575 = pFDklSnYUrrHrqWTKbSF52478402;     pFDklSnYUrrHrqWTKbSF52478402 = pFDklSnYUrrHrqWTKbSF50902701;     pFDklSnYUrrHrqWTKbSF50902701 = pFDklSnYUrrHrqWTKbSF93493826;     pFDklSnYUrrHrqWTKbSF93493826 = pFDklSnYUrrHrqWTKbSF12093359;     pFDklSnYUrrHrqWTKbSF12093359 = pFDklSnYUrrHrqWTKbSF73462141;     pFDklSnYUrrHrqWTKbSF73462141 = pFDklSnYUrrHrqWTKbSF62441027;     pFDklSnYUrrHrqWTKbSF62441027 = pFDklSnYUrrHrqWTKbSF97509969;     pFDklSnYUrrHrqWTKbSF97509969 = pFDklSnYUrrHrqWTKbSF74473401;     pFDklSnYUrrHrqWTKbSF74473401 = pFDklSnYUrrHrqWTKbSF95544147;     pFDklSnYUrrHrqWTKbSF95544147 = pFDklSnYUrrHrqWTKbSF92482771;     pFDklSnYUrrHrqWTKbSF92482771 = pFDklSnYUrrHrqWTKbSF39380629;     pFDklSnYUrrHrqWTKbSF39380629 = pFDklSnYUrrHrqWTKbSF71006273;     pFDklSnYUrrHrqWTKbSF71006273 = pFDklSnYUrrHrqWTKbSF43062483;     pFDklSnYUrrHrqWTKbSF43062483 = pFDklSnYUrrHrqWTKbSF25351923;     pFDklSnYUrrHrqWTKbSF25351923 = pFDklSnYUrrHrqWTKbSF78917250;     pFDklSnYUrrHrqWTKbSF78917250 = pFDklSnYUrrHrqWTKbSF42868306;     pFDklSnYUrrHrqWTKbSF42868306 = pFDklSnYUrrHrqWTKbSF37488758;     pFDklSnYUrrHrqWTKbSF37488758 = pFDklSnYUrrHrqWTKbSF47802333;     pFDklSnYUrrHrqWTKbSF47802333 = pFDklSnYUrrHrqWTKbSF51042974;     pFDklSnYUrrHrqWTKbSF51042974 = pFDklSnYUrrHrqWTKbSF19383022;     pFDklSnYUrrHrqWTKbSF19383022 = pFDklSnYUrrHrqWTKbSF28967905;     pFDklSnYUrrHrqWTKbSF28967905 = pFDklSnYUrrHrqWTKbSF43796378;     pFDklSnYUrrHrqWTKbSF43796378 = pFDklSnYUrrHrqWTKbSF10556062;     pFDklSnYUrrHrqWTKbSF10556062 = pFDklSnYUrrHrqWTKbSF90075357;     pFDklSnYUrrHrqWTKbSF90075357 = pFDklSnYUrrHrqWTKbSF95444006;     pFDklSnYUrrHrqWTKbSF95444006 = pFDklSnYUrrHrqWTKbSF47985374;     pFDklSnYUrrHrqWTKbSF47985374 = pFDklSnYUrrHrqWTKbSF11848346;     pFDklSnYUrrHrqWTKbSF11848346 = pFDklSnYUrrHrqWTKbSF28359778;     pFDklSnYUrrHrqWTKbSF28359778 = pFDklSnYUrrHrqWTKbSF66785592;     pFDklSnYUrrHrqWTKbSF66785592 = pFDklSnYUrrHrqWTKbSF60281028;     pFDklSnYUrrHrqWTKbSF60281028 = pFDklSnYUrrHrqWTKbSF46084434;     pFDklSnYUrrHrqWTKbSF46084434 = pFDklSnYUrrHrqWTKbSF94270232;     pFDklSnYUrrHrqWTKbSF94270232 = pFDklSnYUrrHrqWTKbSF58591980;     pFDklSnYUrrHrqWTKbSF58591980 = pFDklSnYUrrHrqWTKbSF6177853;     pFDklSnYUrrHrqWTKbSF6177853 = pFDklSnYUrrHrqWTKbSF33152730;     pFDklSnYUrrHrqWTKbSF33152730 = pFDklSnYUrrHrqWTKbSF60111664;     pFDklSnYUrrHrqWTKbSF60111664 = pFDklSnYUrrHrqWTKbSF9973250;     pFDklSnYUrrHrqWTKbSF9973250 = pFDklSnYUrrHrqWTKbSF31523969;     pFDklSnYUrrHrqWTKbSF31523969 = pFDklSnYUrrHrqWTKbSF49566349;     pFDklSnYUrrHrqWTKbSF49566349 = pFDklSnYUrrHrqWTKbSF77150713;     pFDklSnYUrrHrqWTKbSF77150713 = pFDklSnYUrrHrqWTKbSF17664451;     pFDklSnYUrrHrqWTKbSF17664451 = pFDklSnYUrrHrqWTKbSF5442974;     pFDklSnYUrrHrqWTKbSF5442974 = pFDklSnYUrrHrqWTKbSF61007776;     pFDklSnYUrrHrqWTKbSF61007776 = pFDklSnYUrrHrqWTKbSF96817628;     pFDklSnYUrrHrqWTKbSF96817628 = pFDklSnYUrrHrqWTKbSF5701926;     pFDklSnYUrrHrqWTKbSF5701926 = pFDklSnYUrrHrqWTKbSF23910377;     pFDklSnYUrrHrqWTKbSF23910377 = pFDklSnYUrrHrqWTKbSF51142520;     pFDklSnYUrrHrqWTKbSF51142520 = pFDklSnYUrrHrqWTKbSF72900210;     pFDklSnYUrrHrqWTKbSF72900210 = pFDklSnYUrrHrqWTKbSF50175780;     pFDklSnYUrrHrqWTKbSF50175780 = pFDklSnYUrrHrqWTKbSF96656914;     pFDklSnYUrrHrqWTKbSF96656914 = pFDklSnYUrrHrqWTKbSF24446718;     pFDklSnYUrrHrqWTKbSF24446718 = pFDklSnYUrrHrqWTKbSF75260300;     pFDklSnYUrrHrqWTKbSF75260300 = pFDklSnYUrrHrqWTKbSF51112522;     pFDklSnYUrrHrqWTKbSF51112522 = pFDklSnYUrrHrqWTKbSF67564282;     pFDklSnYUrrHrqWTKbSF67564282 = pFDklSnYUrrHrqWTKbSF36063670;     pFDklSnYUrrHrqWTKbSF36063670 = pFDklSnYUrrHrqWTKbSF8682025;     pFDklSnYUrrHrqWTKbSF8682025 = pFDklSnYUrrHrqWTKbSF40346639;     pFDklSnYUrrHrqWTKbSF40346639 = pFDklSnYUrrHrqWTKbSF3418470;     pFDklSnYUrrHrqWTKbSF3418470 = pFDklSnYUrrHrqWTKbSF16649353;     pFDklSnYUrrHrqWTKbSF16649353 = pFDklSnYUrrHrqWTKbSF25476767;     pFDklSnYUrrHrqWTKbSF25476767 = pFDklSnYUrrHrqWTKbSF50592682;     pFDklSnYUrrHrqWTKbSF50592682 = pFDklSnYUrrHrqWTKbSF69150191;     pFDklSnYUrrHrqWTKbSF69150191 = pFDklSnYUrrHrqWTKbSF7687809;     pFDklSnYUrrHrqWTKbSF7687809 = pFDklSnYUrrHrqWTKbSF35263120;     pFDklSnYUrrHrqWTKbSF35263120 = pFDklSnYUrrHrqWTKbSF46398337;     pFDklSnYUrrHrqWTKbSF46398337 = pFDklSnYUrrHrqWTKbSF45110396;     pFDklSnYUrrHrqWTKbSF45110396 = pFDklSnYUrrHrqWTKbSF12414294;     pFDklSnYUrrHrqWTKbSF12414294 = pFDklSnYUrrHrqWTKbSF36884630;     pFDklSnYUrrHrqWTKbSF36884630 = pFDklSnYUrrHrqWTKbSF92199193;     pFDklSnYUrrHrqWTKbSF92199193 = pFDklSnYUrrHrqWTKbSF18805586;     pFDklSnYUrrHrqWTKbSF18805586 = pFDklSnYUrrHrqWTKbSF32895056;     pFDklSnYUrrHrqWTKbSF32895056 = pFDklSnYUrrHrqWTKbSF5964790;     pFDklSnYUrrHrqWTKbSF5964790 = pFDklSnYUrrHrqWTKbSF98235984;     pFDklSnYUrrHrqWTKbSF98235984 = pFDklSnYUrrHrqWTKbSF73892261;     pFDklSnYUrrHrqWTKbSF73892261 = pFDklSnYUrrHrqWTKbSF1718571;     pFDklSnYUrrHrqWTKbSF1718571 = pFDklSnYUrrHrqWTKbSF23524932;     pFDklSnYUrrHrqWTKbSF23524932 = pFDklSnYUrrHrqWTKbSF82788602;     pFDklSnYUrrHrqWTKbSF82788602 = pFDklSnYUrrHrqWTKbSF13738434;     pFDklSnYUrrHrqWTKbSF13738434 = pFDklSnYUrrHrqWTKbSF84373431;     pFDklSnYUrrHrqWTKbSF84373431 = pFDklSnYUrrHrqWTKbSF71533630;     pFDklSnYUrrHrqWTKbSF71533630 = pFDklSnYUrrHrqWTKbSF96842854;     pFDklSnYUrrHrqWTKbSF96842854 = pFDklSnYUrrHrqWTKbSF38948135;     pFDklSnYUrrHrqWTKbSF38948135 = pFDklSnYUrrHrqWTKbSF78183998;     pFDklSnYUrrHrqWTKbSF78183998 = pFDklSnYUrrHrqWTKbSF70128678;     pFDklSnYUrrHrqWTKbSF70128678 = pFDklSnYUrrHrqWTKbSF35834310;     pFDklSnYUrrHrqWTKbSF35834310 = pFDklSnYUrrHrqWTKbSF70824134;     pFDklSnYUrrHrqWTKbSF70824134 = pFDklSnYUrrHrqWTKbSF43157711;     pFDklSnYUrrHrqWTKbSF43157711 = pFDklSnYUrrHrqWTKbSF91027698;     pFDklSnYUrrHrqWTKbSF91027698 = pFDklSnYUrrHrqWTKbSF70114183;     pFDklSnYUrrHrqWTKbSF70114183 = pFDklSnYUrrHrqWTKbSF24470705;     pFDklSnYUrrHrqWTKbSF24470705 = pFDklSnYUrrHrqWTKbSF19765026;     pFDklSnYUrrHrqWTKbSF19765026 = pFDklSnYUrrHrqWTKbSF94205002;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void gRPtixYwBKpMjhGwkYeW1325790() {     int rPkulXwciigsOFBJMOxM28293793 = -157295815;    int rPkulXwciigsOFBJMOxM53604914 = -425202008;    int rPkulXwciigsOFBJMOxM338608 = -198254901;    int rPkulXwciigsOFBJMOxM99650624 = 8561055;    int rPkulXwciigsOFBJMOxM15660273 = -143117199;    int rPkulXwciigsOFBJMOxM55888192 = -868064755;    int rPkulXwciigsOFBJMOxM9417333 = -335374524;    int rPkulXwciigsOFBJMOxM27259461 = -306429889;    int rPkulXwciigsOFBJMOxM77139385 = -187123574;    int rPkulXwciigsOFBJMOxM13023379 = 82321916;    int rPkulXwciigsOFBJMOxM40436523 = -356435726;    int rPkulXwciigsOFBJMOxM1863410 = -226891011;    int rPkulXwciigsOFBJMOxM58235969 = -864451465;    int rPkulXwciigsOFBJMOxM37494861 = -487408234;    int rPkulXwciigsOFBJMOxM23323131 = -90900783;    int rPkulXwciigsOFBJMOxM55668746 = 22880107;    int rPkulXwciigsOFBJMOxM76330669 = -471419227;    int rPkulXwciigsOFBJMOxM37173092 = -843105273;    int rPkulXwciigsOFBJMOxM18638330 = -856836396;    int rPkulXwciigsOFBJMOxM21221786 = 39899009;    int rPkulXwciigsOFBJMOxM6919824 = -498031486;    int rPkulXwciigsOFBJMOxM89025179 = -327155761;    int rPkulXwciigsOFBJMOxM21612928 = -558000173;    int rPkulXwciigsOFBJMOxM65929078 = 64866806;    int rPkulXwciigsOFBJMOxM61984825 = -495729757;    int rPkulXwciigsOFBJMOxM93637577 = -542838415;    int rPkulXwciigsOFBJMOxM66650585 = -610979447;    int rPkulXwciigsOFBJMOxM35819426 = -157754296;    int rPkulXwciigsOFBJMOxM36152211 = -350043512;    int rPkulXwciigsOFBJMOxM31566906 = -412519521;    int rPkulXwciigsOFBJMOxM12383349 = -161650898;    int rPkulXwciigsOFBJMOxM1049345 = -801198744;    int rPkulXwciigsOFBJMOxM7554603 = -973096170;    int rPkulXwciigsOFBJMOxM56230066 = -806048327;    int rPkulXwciigsOFBJMOxM39978701 = -840972162;    int rPkulXwciigsOFBJMOxM4736184 = -41647565;    int rPkulXwciigsOFBJMOxM74259065 = -743011525;    int rPkulXwciigsOFBJMOxM89315482 = -627564851;    int rPkulXwciigsOFBJMOxM91313997 = 88402225;    int rPkulXwciigsOFBJMOxM74394492 = -773801586;    int rPkulXwciigsOFBJMOxM20706189 = -497484154;    int rPkulXwciigsOFBJMOxM10906760 = -662279654;    int rPkulXwciigsOFBJMOxM8262408 = -348323797;    int rPkulXwciigsOFBJMOxM59903578 = -302256812;    int rPkulXwciigsOFBJMOxM43842144 = -535753320;    int rPkulXwciigsOFBJMOxM70918278 = -972922813;    int rPkulXwciigsOFBJMOxM54260266 = -697203632;    int rPkulXwciigsOFBJMOxM70402078 = -944445257;    int rPkulXwciigsOFBJMOxM10274316 = -46894889;    int rPkulXwciigsOFBJMOxM59308117 = 13845620;    int rPkulXwciigsOFBJMOxM99009300 = 89610318;    int rPkulXwciigsOFBJMOxM6279592 = -939100125;    int rPkulXwciigsOFBJMOxM7295667 = -453574382;    int rPkulXwciigsOFBJMOxM74413454 = -287999987;    int rPkulXwciigsOFBJMOxM94405965 = -352963906;    int rPkulXwciigsOFBJMOxM39268614 = -830140054;    int rPkulXwciigsOFBJMOxM31991986 = -867201835;    int rPkulXwciigsOFBJMOxM34409529 = -163121708;    int rPkulXwciigsOFBJMOxM37665799 = -495709188;    int rPkulXwciigsOFBJMOxM22022695 = -600278784;    int rPkulXwciigsOFBJMOxM89237607 = -157085309;    int rPkulXwciigsOFBJMOxM73597907 = -77620229;    int rPkulXwciigsOFBJMOxM91107250 = -956386377;    int rPkulXwciigsOFBJMOxM45572479 = -774604053;    int rPkulXwciigsOFBJMOxM640031 = -756027187;    int rPkulXwciigsOFBJMOxM39387179 = -555236983;    int rPkulXwciigsOFBJMOxM94308806 = -253794841;    int rPkulXwciigsOFBJMOxM2005903 = 41596861;    int rPkulXwciigsOFBJMOxM97516160 = -646436072;    int rPkulXwciigsOFBJMOxM18586948 = 50746782;    int rPkulXwciigsOFBJMOxM81409681 = -234108369;    int rPkulXwciigsOFBJMOxM87015187 = -843854376;    int rPkulXwciigsOFBJMOxM45859094 = -831507498;    int rPkulXwciigsOFBJMOxM44243838 = 16965190;    int rPkulXwciigsOFBJMOxM515598 = -462616838;    int rPkulXwciigsOFBJMOxM96013064 = -835751833;    int rPkulXwciigsOFBJMOxM80762771 = -978831965;    int rPkulXwciigsOFBJMOxM61709350 = -155743361;    int rPkulXwciigsOFBJMOxM22086935 = -399379875;    int rPkulXwciigsOFBJMOxM91066547 = -522806944;    int rPkulXwciigsOFBJMOxM39377312 = -845634783;    int rPkulXwciigsOFBJMOxM96248506 = -666534190;    int rPkulXwciigsOFBJMOxM25545110 = -10859407;    int rPkulXwciigsOFBJMOxM76844093 = -263889133;    int rPkulXwciigsOFBJMOxM32557606 = -402129840;    int rPkulXwciigsOFBJMOxM6103757 = -222550773;    int rPkulXwciigsOFBJMOxM93753678 = -247624363;    int rPkulXwciigsOFBJMOxM33141148 = -585096183;    int rPkulXwciigsOFBJMOxM61824100 = -353084421;    int rPkulXwciigsOFBJMOxM710087 = 89167891;    int rPkulXwciigsOFBJMOxM72744198 = -174445730;    int rPkulXwciigsOFBJMOxM39849537 = -479889818;    int rPkulXwciigsOFBJMOxM51649683 = -31855663;    int rPkulXwciigsOFBJMOxM69291302 = -311318992;    int rPkulXwciigsOFBJMOxM85156884 = -516716278;    int rPkulXwciigsOFBJMOxM47108281 = -319863925;    int rPkulXwciigsOFBJMOxM19799510 = -705893277;    int rPkulXwciigsOFBJMOxM62689929 = -573719744;    int rPkulXwciigsOFBJMOxM59263547 = -546229626;    int rPkulXwciigsOFBJMOxM4454965 = -157295815;     rPkulXwciigsOFBJMOxM28293793 = rPkulXwciigsOFBJMOxM53604914;     rPkulXwciigsOFBJMOxM53604914 = rPkulXwciigsOFBJMOxM338608;     rPkulXwciigsOFBJMOxM338608 = rPkulXwciigsOFBJMOxM99650624;     rPkulXwciigsOFBJMOxM99650624 = rPkulXwciigsOFBJMOxM15660273;     rPkulXwciigsOFBJMOxM15660273 = rPkulXwciigsOFBJMOxM55888192;     rPkulXwciigsOFBJMOxM55888192 = rPkulXwciigsOFBJMOxM9417333;     rPkulXwciigsOFBJMOxM9417333 = rPkulXwciigsOFBJMOxM27259461;     rPkulXwciigsOFBJMOxM27259461 = rPkulXwciigsOFBJMOxM77139385;     rPkulXwciigsOFBJMOxM77139385 = rPkulXwciigsOFBJMOxM13023379;     rPkulXwciigsOFBJMOxM13023379 = rPkulXwciigsOFBJMOxM40436523;     rPkulXwciigsOFBJMOxM40436523 = rPkulXwciigsOFBJMOxM1863410;     rPkulXwciigsOFBJMOxM1863410 = rPkulXwciigsOFBJMOxM58235969;     rPkulXwciigsOFBJMOxM58235969 = rPkulXwciigsOFBJMOxM37494861;     rPkulXwciigsOFBJMOxM37494861 = rPkulXwciigsOFBJMOxM23323131;     rPkulXwciigsOFBJMOxM23323131 = rPkulXwciigsOFBJMOxM55668746;     rPkulXwciigsOFBJMOxM55668746 = rPkulXwciigsOFBJMOxM76330669;     rPkulXwciigsOFBJMOxM76330669 = rPkulXwciigsOFBJMOxM37173092;     rPkulXwciigsOFBJMOxM37173092 = rPkulXwciigsOFBJMOxM18638330;     rPkulXwciigsOFBJMOxM18638330 = rPkulXwciigsOFBJMOxM21221786;     rPkulXwciigsOFBJMOxM21221786 = rPkulXwciigsOFBJMOxM6919824;     rPkulXwciigsOFBJMOxM6919824 = rPkulXwciigsOFBJMOxM89025179;     rPkulXwciigsOFBJMOxM89025179 = rPkulXwciigsOFBJMOxM21612928;     rPkulXwciigsOFBJMOxM21612928 = rPkulXwciigsOFBJMOxM65929078;     rPkulXwciigsOFBJMOxM65929078 = rPkulXwciigsOFBJMOxM61984825;     rPkulXwciigsOFBJMOxM61984825 = rPkulXwciigsOFBJMOxM93637577;     rPkulXwciigsOFBJMOxM93637577 = rPkulXwciigsOFBJMOxM66650585;     rPkulXwciigsOFBJMOxM66650585 = rPkulXwciigsOFBJMOxM35819426;     rPkulXwciigsOFBJMOxM35819426 = rPkulXwciigsOFBJMOxM36152211;     rPkulXwciigsOFBJMOxM36152211 = rPkulXwciigsOFBJMOxM31566906;     rPkulXwciigsOFBJMOxM31566906 = rPkulXwciigsOFBJMOxM12383349;     rPkulXwciigsOFBJMOxM12383349 = rPkulXwciigsOFBJMOxM1049345;     rPkulXwciigsOFBJMOxM1049345 = rPkulXwciigsOFBJMOxM7554603;     rPkulXwciigsOFBJMOxM7554603 = rPkulXwciigsOFBJMOxM56230066;     rPkulXwciigsOFBJMOxM56230066 = rPkulXwciigsOFBJMOxM39978701;     rPkulXwciigsOFBJMOxM39978701 = rPkulXwciigsOFBJMOxM4736184;     rPkulXwciigsOFBJMOxM4736184 = rPkulXwciigsOFBJMOxM74259065;     rPkulXwciigsOFBJMOxM74259065 = rPkulXwciigsOFBJMOxM89315482;     rPkulXwciigsOFBJMOxM89315482 = rPkulXwciigsOFBJMOxM91313997;     rPkulXwciigsOFBJMOxM91313997 = rPkulXwciigsOFBJMOxM74394492;     rPkulXwciigsOFBJMOxM74394492 = rPkulXwciigsOFBJMOxM20706189;     rPkulXwciigsOFBJMOxM20706189 = rPkulXwciigsOFBJMOxM10906760;     rPkulXwciigsOFBJMOxM10906760 = rPkulXwciigsOFBJMOxM8262408;     rPkulXwciigsOFBJMOxM8262408 = rPkulXwciigsOFBJMOxM59903578;     rPkulXwciigsOFBJMOxM59903578 = rPkulXwciigsOFBJMOxM43842144;     rPkulXwciigsOFBJMOxM43842144 = rPkulXwciigsOFBJMOxM70918278;     rPkulXwciigsOFBJMOxM70918278 = rPkulXwciigsOFBJMOxM54260266;     rPkulXwciigsOFBJMOxM54260266 = rPkulXwciigsOFBJMOxM70402078;     rPkulXwciigsOFBJMOxM70402078 = rPkulXwciigsOFBJMOxM10274316;     rPkulXwciigsOFBJMOxM10274316 = rPkulXwciigsOFBJMOxM59308117;     rPkulXwciigsOFBJMOxM59308117 = rPkulXwciigsOFBJMOxM99009300;     rPkulXwciigsOFBJMOxM99009300 = rPkulXwciigsOFBJMOxM6279592;     rPkulXwciigsOFBJMOxM6279592 = rPkulXwciigsOFBJMOxM7295667;     rPkulXwciigsOFBJMOxM7295667 = rPkulXwciigsOFBJMOxM74413454;     rPkulXwciigsOFBJMOxM74413454 = rPkulXwciigsOFBJMOxM94405965;     rPkulXwciigsOFBJMOxM94405965 = rPkulXwciigsOFBJMOxM39268614;     rPkulXwciigsOFBJMOxM39268614 = rPkulXwciigsOFBJMOxM31991986;     rPkulXwciigsOFBJMOxM31991986 = rPkulXwciigsOFBJMOxM34409529;     rPkulXwciigsOFBJMOxM34409529 = rPkulXwciigsOFBJMOxM37665799;     rPkulXwciigsOFBJMOxM37665799 = rPkulXwciigsOFBJMOxM22022695;     rPkulXwciigsOFBJMOxM22022695 = rPkulXwciigsOFBJMOxM89237607;     rPkulXwciigsOFBJMOxM89237607 = rPkulXwciigsOFBJMOxM73597907;     rPkulXwciigsOFBJMOxM73597907 = rPkulXwciigsOFBJMOxM91107250;     rPkulXwciigsOFBJMOxM91107250 = rPkulXwciigsOFBJMOxM45572479;     rPkulXwciigsOFBJMOxM45572479 = rPkulXwciigsOFBJMOxM640031;     rPkulXwciigsOFBJMOxM640031 = rPkulXwciigsOFBJMOxM39387179;     rPkulXwciigsOFBJMOxM39387179 = rPkulXwciigsOFBJMOxM94308806;     rPkulXwciigsOFBJMOxM94308806 = rPkulXwciigsOFBJMOxM2005903;     rPkulXwciigsOFBJMOxM2005903 = rPkulXwciigsOFBJMOxM97516160;     rPkulXwciigsOFBJMOxM97516160 = rPkulXwciigsOFBJMOxM18586948;     rPkulXwciigsOFBJMOxM18586948 = rPkulXwciigsOFBJMOxM81409681;     rPkulXwciigsOFBJMOxM81409681 = rPkulXwciigsOFBJMOxM87015187;     rPkulXwciigsOFBJMOxM87015187 = rPkulXwciigsOFBJMOxM45859094;     rPkulXwciigsOFBJMOxM45859094 = rPkulXwciigsOFBJMOxM44243838;     rPkulXwciigsOFBJMOxM44243838 = rPkulXwciigsOFBJMOxM515598;     rPkulXwciigsOFBJMOxM515598 = rPkulXwciigsOFBJMOxM96013064;     rPkulXwciigsOFBJMOxM96013064 = rPkulXwciigsOFBJMOxM80762771;     rPkulXwciigsOFBJMOxM80762771 = rPkulXwciigsOFBJMOxM61709350;     rPkulXwciigsOFBJMOxM61709350 = rPkulXwciigsOFBJMOxM22086935;     rPkulXwciigsOFBJMOxM22086935 = rPkulXwciigsOFBJMOxM91066547;     rPkulXwciigsOFBJMOxM91066547 = rPkulXwciigsOFBJMOxM39377312;     rPkulXwciigsOFBJMOxM39377312 = rPkulXwciigsOFBJMOxM96248506;     rPkulXwciigsOFBJMOxM96248506 = rPkulXwciigsOFBJMOxM25545110;     rPkulXwciigsOFBJMOxM25545110 = rPkulXwciigsOFBJMOxM76844093;     rPkulXwciigsOFBJMOxM76844093 = rPkulXwciigsOFBJMOxM32557606;     rPkulXwciigsOFBJMOxM32557606 = rPkulXwciigsOFBJMOxM6103757;     rPkulXwciigsOFBJMOxM6103757 = rPkulXwciigsOFBJMOxM93753678;     rPkulXwciigsOFBJMOxM93753678 = rPkulXwciigsOFBJMOxM33141148;     rPkulXwciigsOFBJMOxM33141148 = rPkulXwciigsOFBJMOxM61824100;     rPkulXwciigsOFBJMOxM61824100 = rPkulXwciigsOFBJMOxM710087;     rPkulXwciigsOFBJMOxM710087 = rPkulXwciigsOFBJMOxM72744198;     rPkulXwciigsOFBJMOxM72744198 = rPkulXwciigsOFBJMOxM39849537;     rPkulXwciigsOFBJMOxM39849537 = rPkulXwciigsOFBJMOxM51649683;     rPkulXwciigsOFBJMOxM51649683 = rPkulXwciigsOFBJMOxM69291302;     rPkulXwciigsOFBJMOxM69291302 = rPkulXwciigsOFBJMOxM85156884;     rPkulXwciigsOFBJMOxM85156884 = rPkulXwciigsOFBJMOxM47108281;     rPkulXwciigsOFBJMOxM47108281 = rPkulXwciigsOFBJMOxM19799510;     rPkulXwciigsOFBJMOxM19799510 = rPkulXwciigsOFBJMOxM62689929;     rPkulXwciigsOFBJMOxM62689929 = rPkulXwciigsOFBJMOxM59263547;     rPkulXwciigsOFBJMOxM59263547 = rPkulXwciigsOFBJMOxM4454965;     rPkulXwciigsOFBJMOxM4454965 = rPkulXwciigsOFBJMOxM28293793;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void METAkFmGfSTVBrDjwfIq59977460() {     int ZIyvEGdNxUorsMCiEioZ91724445 = -841069228;    int ZIyvEGdNxUorsMCiEioZ78856493 = -184976622;    int ZIyvEGdNxUorsMCiEioZ92928477 = -276329361;    int ZIyvEGdNxUorsMCiEioZ4924603 = -5418671;    int ZIyvEGdNxUorsMCiEioZ56774851 = -504840230;    int ZIyvEGdNxUorsMCiEioZ33519421 = -709287985;    int ZIyvEGdNxUorsMCiEioZ66847610 = -759701024;    int ZIyvEGdNxUorsMCiEioZ79943289 = -624832205;    int ZIyvEGdNxUorsMCiEioZ26122305 = -761052715;    int ZIyvEGdNxUorsMCiEioZ47241408 = -497524602;    int ZIyvEGdNxUorsMCiEioZ8041791 = -894278384;    int ZIyvEGdNxUorsMCiEioZ78978097 = -861207777;    int ZIyvEGdNxUorsMCiEioZ56404106 = -86096475;    int ZIyvEGdNxUorsMCiEioZ71524767 = -120321751;    int ZIyvEGdNxUorsMCiEioZ46657325 = -298065624;    int ZIyvEGdNxUorsMCiEioZ55863431 = -564319331;    int ZIyvEGdNxUorsMCiEioZ40187871 = -702690761;    int ZIyvEGdNxUorsMCiEioZ81509074 = -718418793;    int ZIyvEGdNxUorsMCiEioZ83514375 = -762393237;    int ZIyvEGdNxUorsMCiEioZ16349997 = -374687776;    int ZIyvEGdNxUorsMCiEioZ67367044 = -90540465;    int ZIyvEGdNxUorsMCiEioZ51888376 = -42513520;    int ZIyvEGdNxUorsMCiEioZ52808521 = -503387989;    int ZIyvEGdNxUorsMCiEioZ62741478 = -573759039;    int ZIyvEGdNxUorsMCiEioZ45419646 = -870338695;    int ZIyvEGdNxUorsMCiEioZ35762894 = -896226431;    int ZIyvEGdNxUorsMCiEioZ47459633 = -540731271;    int ZIyvEGdNxUorsMCiEioZ48217177 = -513068715;    int ZIyvEGdNxUorsMCiEioZ33457359 = -883115363;    int ZIyvEGdNxUorsMCiEioZ71083681 = -63554885;    int ZIyvEGdNxUorsMCiEioZ7311002 = -197213898;    int ZIyvEGdNxUorsMCiEioZ31755023 = -300396997;    int ZIyvEGdNxUorsMCiEioZ27540911 = -846188321;    int ZIyvEGdNxUorsMCiEioZ51933988 = -914694870;    int ZIyvEGdNxUorsMCiEioZ36305829 = -284054497;    int ZIyvEGdNxUorsMCiEioZ80458952 = -85826227;    int ZIyvEGdNxUorsMCiEioZ88308234 = -112693640;    int ZIyvEGdNxUorsMCiEioZ28970563 = -986955804;    int ZIyvEGdNxUorsMCiEioZ79127275 = -165395748;    int ZIyvEGdNxUorsMCiEioZ79474143 = -922893926;    int ZIyvEGdNxUorsMCiEioZ74924528 = -326376728;    int ZIyvEGdNxUorsMCiEioZ20436210 = -345472889;    int ZIyvEGdNxUorsMCiEioZ21663364 = -530624710;    int ZIyvEGdNxUorsMCiEioZ39378024 = -849916113;    int ZIyvEGdNxUorsMCiEioZ15636617 = -44554257;    int ZIyvEGdNxUorsMCiEioZ65885439 = -540271438;    int ZIyvEGdNxUorsMCiEioZ78563152 = -174569940;    int ZIyvEGdNxUorsMCiEioZ1007179 = -15556590;    int ZIyvEGdNxUorsMCiEioZ56614856 = 37334127;    int ZIyvEGdNxUorsMCiEioZ12569721 = -408122757;    int ZIyvEGdNxUorsMCiEioZ37645673 = -678074200;    int ZIyvEGdNxUorsMCiEioZ24227161 = -468128131;    int ZIyvEGdNxUorsMCiEioZ38146395 = -504734622;    int ZIyvEGdNxUorsMCiEioZ63147949 = -531388435;    int ZIyvEGdNxUorsMCiEioZ92610416 = -376611195;    int ZIyvEGdNxUorsMCiEioZ39836069 = -698555709;    int ZIyvEGdNxUorsMCiEioZ26047972 = -681588634;    int ZIyvEGdNxUorsMCiEioZ30187000 = -702570322;    int ZIyvEGdNxUorsMCiEioZ59504957 = -135079977;    int ZIyvEGdNxUorsMCiEioZ21011957 = -608613799;    int ZIyvEGdNxUorsMCiEioZ86059788 = -68556715;    int ZIyvEGdNxUorsMCiEioZ18630434 = -146632310;    int ZIyvEGdNxUorsMCiEioZ46485930 = -741716843;    int ZIyvEGdNxUorsMCiEioZ55038623 = -597497831;    int ZIyvEGdNxUorsMCiEioZ39930407 = -200310705;    int ZIyvEGdNxUorsMCiEioZ76286767 = -493881388;    int ZIyvEGdNxUorsMCiEioZ51437187 = 84980543;    int ZIyvEGdNxUorsMCiEioZ4470118 = -171401606;    int ZIyvEGdNxUorsMCiEioZ35218938 = -836267254;    int ZIyvEGdNxUorsMCiEioZ66198372 = -112239397;    int ZIyvEGdNxUorsMCiEioZ67555196 = -351625692;    int ZIyvEGdNxUorsMCiEioZ11217309 = -715734958;    int ZIyvEGdNxUorsMCiEioZ2381800 = -453023045;    int ZIyvEGdNxUorsMCiEioZ4040232 = -839499312;    int ZIyvEGdNxUorsMCiEioZ41425469 = 51688952;    int ZIyvEGdNxUorsMCiEioZ46930835 = -745067576;    int ZIyvEGdNxUorsMCiEioZ30225013 = -511888810;    int ZIyvEGdNxUorsMCiEioZ13430498 = -653471877;    int ZIyvEGdNxUorsMCiEioZ47104861 = -429204783;    int ZIyvEGdNxUorsMCiEioZ79534207 = -230067257;    int ZIyvEGdNxUorsMCiEioZ57199741 = -621656492;    int ZIyvEGdNxUorsMCiEioZ46452455 = -425174681;    int ZIyvEGdNxUorsMCiEioZ91602320 = -450402842;    int ZIyvEGdNxUorsMCiEioZ20887639 = -374992606;    int ZIyvEGdNxUorsMCiEioZ33438008 = -385480685;    int ZIyvEGdNxUorsMCiEioZ83083840 = -729085767;    int ZIyvEGdNxUorsMCiEioZ93608627 = -795662376;    int ZIyvEGdNxUorsMCiEioZ64392961 = -214799886;    int ZIyvEGdNxUorsMCiEioZ59323571 = -438083675;    int ZIyvEGdNxUorsMCiEioZ96469760 = -585498789;    int ZIyvEGdNxUorsMCiEioZ54410980 = -404237594;    int ZIyvEGdNxUorsMCiEioZ58121235 = -410123318;    int ZIyvEGdNxUorsMCiEioZ69465605 = -751875827;    int ZIyvEGdNxUorsMCiEioZ58115318 = -556781950;    int ZIyvEGdNxUorsMCiEioZ93414355 = -754337211;    int ZIyvEGdNxUorsMCiEioZ56294095 = -79744419;    int ZIyvEGdNxUorsMCiEioZ73950279 = -603756047;    int ZIyvEGdNxUorsMCiEioZ66624740 = -933126880;    int ZIyvEGdNxUorsMCiEioZ99447617 = -549605408;    int ZIyvEGdNxUorsMCiEioZ39349849 = -841069228;     ZIyvEGdNxUorsMCiEioZ91724445 = ZIyvEGdNxUorsMCiEioZ78856493;     ZIyvEGdNxUorsMCiEioZ78856493 = ZIyvEGdNxUorsMCiEioZ92928477;     ZIyvEGdNxUorsMCiEioZ92928477 = ZIyvEGdNxUorsMCiEioZ4924603;     ZIyvEGdNxUorsMCiEioZ4924603 = ZIyvEGdNxUorsMCiEioZ56774851;     ZIyvEGdNxUorsMCiEioZ56774851 = ZIyvEGdNxUorsMCiEioZ33519421;     ZIyvEGdNxUorsMCiEioZ33519421 = ZIyvEGdNxUorsMCiEioZ66847610;     ZIyvEGdNxUorsMCiEioZ66847610 = ZIyvEGdNxUorsMCiEioZ79943289;     ZIyvEGdNxUorsMCiEioZ79943289 = ZIyvEGdNxUorsMCiEioZ26122305;     ZIyvEGdNxUorsMCiEioZ26122305 = ZIyvEGdNxUorsMCiEioZ47241408;     ZIyvEGdNxUorsMCiEioZ47241408 = ZIyvEGdNxUorsMCiEioZ8041791;     ZIyvEGdNxUorsMCiEioZ8041791 = ZIyvEGdNxUorsMCiEioZ78978097;     ZIyvEGdNxUorsMCiEioZ78978097 = ZIyvEGdNxUorsMCiEioZ56404106;     ZIyvEGdNxUorsMCiEioZ56404106 = ZIyvEGdNxUorsMCiEioZ71524767;     ZIyvEGdNxUorsMCiEioZ71524767 = ZIyvEGdNxUorsMCiEioZ46657325;     ZIyvEGdNxUorsMCiEioZ46657325 = ZIyvEGdNxUorsMCiEioZ55863431;     ZIyvEGdNxUorsMCiEioZ55863431 = ZIyvEGdNxUorsMCiEioZ40187871;     ZIyvEGdNxUorsMCiEioZ40187871 = ZIyvEGdNxUorsMCiEioZ81509074;     ZIyvEGdNxUorsMCiEioZ81509074 = ZIyvEGdNxUorsMCiEioZ83514375;     ZIyvEGdNxUorsMCiEioZ83514375 = ZIyvEGdNxUorsMCiEioZ16349997;     ZIyvEGdNxUorsMCiEioZ16349997 = ZIyvEGdNxUorsMCiEioZ67367044;     ZIyvEGdNxUorsMCiEioZ67367044 = ZIyvEGdNxUorsMCiEioZ51888376;     ZIyvEGdNxUorsMCiEioZ51888376 = ZIyvEGdNxUorsMCiEioZ52808521;     ZIyvEGdNxUorsMCiEioZ52808521 = ZIyvEGdNxUorsMCiEioZ62741478;     ZIyvEGdNxUorsMCiEioZ62741478 = ZIyvEGdNxUorsMCiEioZ45419646;     ZIyvEGdNxUorsMCiEioZ45419646 = ZIyvEGdNxUorsMCiEioZ35762894;     ZIyvEGdNxUorsMCiEioZ35762894 = ZIyvEGdNxUorsMCiEioZ47459633;     ZIyvEGdNxUorsMCiEioZ47459633 = ZIyvEGdNxUorsMCiEioZ48217177;     ZIyvEGdNxUorsMCiEioZ48217177 = ZIyvEGdNxUorsMCiEioZ33457359;     ZIyvEGdNxUorsMCiEioZ33457359 = ZIyvEGdNxUorsMCiEioZ71083681;     ZIyvEGdNxUorsMCiEioZ71083681 = ZIyvEGdNxUorsMCiEioZ7311002;     ZIyvEGdNxUorsMCiEioZ7311002 = ZIyvEGdNxUorsMCiEioZ31755023;     ZIyvEGdNxUorsMCiEioZ31755023 = ZIyvEGdNxUorsMCiEioZ27540911;     ZIyvEGdNxUorsMCiEioZ27540911 = ZIyvEGdNxUorsMCiEioZ51933988;     ZIyvEGdNxUorsMCiEioZ51933988 = ZIyvEGdNxUorsMCiEioZ36305829;     ZIyvEGdNxUorsMCiEioZ36305829 = ZIyvEGdNxUorsMCiEioZ80458952;     ZIyvEGdNxUorsMCiEioZ80458952 = ZIyvEGdNxUorsMCiEioZ88308234;     ZIyvEGdNxUorsMCiEioZ88308234 = ZIyvEGdNxUorsMCiEioZ28970563;     ZIyvEGdNxUorsMCiEioZ28970563 = ZIyvEGdNxUorsMCiEioZ79127275;     ZIyvEGdNxUorsMCiEioZ79127275 = ZIyvEGdNxUorsMCiEioZ79474143;     ZIyvEGdNxUorsMCiEioZ79474143 = ZIyvEGdNxUorsMCiEioZ74924528;     ZIyvEGdNxUorsMCiEioZ74924528 = ZIyvEGdNxUorsMCiEioZ20436210;     ZIyvEGdNxUorsMCiEioZ20436210 = ZIyvEGdNxUorsMCiEioZ21663364;     ZIyvEGdNxUorsMCiEioZ21663364 = ZIyvEGdNxUorsMCiEioZ39378024;     ZIyvEGdNxUorsMCiEioZ39378024 = ZIyvEGdNxUorsMCiEioZ15636617;     ZIyvEGdNxUorsMCiEioZ15636617 = ZIyvEGdNxUorsMCiEioZ65885439;     ZIyvEGdNxUorsMCiEioZ65885439 = ZIyvEGdNxUorsMCiEioZ78563152;     ZIyvEGdNxUorsMCiEioZ78563152 = ZIyvEGdNxUorsMCiEioZ1007179;     ZIyvEGdNxUorsMCiEioZ1007179 = ZIyvEGdNxUorsMCiEioZ56614856;     ZIyvEGdNxUorsMCiEioZ56614856 = ZIyvEGdNxUorsMCiEioZ12569721;     ZIyvEGdNxUorsMCiEioZ12569721 = ZIyvEGdNxUorsMCiEioZ37645673;     ZIyvEGdNxUorsMCiEioZ37645673 = ZIyvEGdNxUorsMCiEioZ24227161;     ZIyvEGdNxUorsMCiEioZ24227161 = ZIyvEGdNxUorsMCiEioZ38146395;     ZIyvEGdNxUorsMCiEioZ38146395 = ZIyvEGdNxUorsMCiEioZ63147949;     ZIyvEGdNxUorsMCiEioZ63147949 = ZIyvEGdNxUorsMCiEioZ92610416;     ZIyvEGdNxUorsMCiEioZ92610416 = ZIyvEGdNxUorsMCiEioZ39836069;     ZIyvEGdNxUorsMCiEioZ39836069 = ZIyvEGdNxUorsMCiEioZ26047972;     ZIyvEGdNxUorsMCiEioZ26047972 = ZIyvEGdNxUorsMCiEioZ30187000;     ZIyvEGdNxUorsMCiEioZ30187000 = ZIyvEGdNxUorsMCiEioZ59504957;     ZIyvEGdNxUorsMCiEioZ59504957 = ZIyvEGdNxUorsMCiEioZ21011957;     ZIyvEGdNxUorsMCiEioZ21011957 = ZIyvEGdNxUorsMCiEioZ86059788;     ZIyvEGdNxUorsMCiEioZ86059788 = ZIyvEGdNxUorsMCiEioZ18630434;     ZIyvEGdNxUorsMCiEioZ18630434 = ZIyvEGdNxUorsMCiEioZ46485930;     ZIyvEGdNxUorsMCiEioZ46485930 = ZIyvEGdNxUorsMCiEioZ55038623;     ZIyvEGdNxUorsMCiEioZ55038623 = ZIyvEGdNxUorsMCiEioZ39930407;     ZIyvEGdNxUorsMCiEioZ39930407 = ZIyvEGdNxUorsMCiEioZ76286767;     ZIyvEGdNxUorsMCiEioZ76286767 = ZIyvEGdNxUorsMCiEioZ51437187;     ZIyvEGdNxUorsMCiEioZ51437187 = ZIyvEGdNxUorsMCiEioZ4470118;     ZIyvEGdNxUorsMCiEioZ4470118 = ZIyvEGdNxUorsMCiEioZ35218938;     ZIyvEGdNxUorsMCiEioZ35218938 = ZIyvEGdNxUorsMCiEioZ66198372;     ZIyvEGdNxUorsMCiEioZ66198372 = ZIyvEGdNxUorsMCiEioZ67555196;     ZIyvEGdNxUorsMCiEioZ67555196 = ZIyvEGdNxUorsMCiEioZ11217309;     ZIyvEGdNxUorsMCiEioZ11217309 = ZIyvEGdNxUorsMCiEioZ2381800;     ZIyvEGdNxUorsMCiEioZ2381800 = ZIyvEGdNxUorsMCiEioZ4040232;     ZIyvEGdNxUorsMCiEioZ4040232 = ZIyvEGdNxUorsMCiEioZ41425469;     ZIyvEGdNxUorsMCiEioZ41425469 = ZIyvEGdNxUorsMCiEioZ46930835;     ZIyvEGdNxUorsMCiEioZ46930835 = ZIyvEGdNxUorsMCiEioZ30225013;     ZIyvEGdNxUorsMCiEioZ30225013 = ZIyvEGdNxUorsMCiEioZ13430498;     ZIyvEGdNxUorsMCiEioZ13430498 = ZIyvEGdNxUorsMCiEioZ47104861;     ZIyvEGdNxUorsMCiEioZ47104861 = ZIyvEGdNxUorsMCiEioZ79534207;     ZIyvEGdNxUorsMCiEioZ79534207 = ZIyvEGdNxUorsMCiEioZ57199741;     ZIyvEGdNxUorsMCiEioZ57199741 = ZIyvEGdNxUorsMCiEioZ46452455;     ZIyvEGdNxUorsMCiEioZ46452455 = ZIyvEGdNxUorsMCiEioZ91602320;     ZIyvEGdNxUorsMCiEioZ91602320 = ZIyvEGdNxUorsMCiEioZ20887639;     ZIyvEGdNxUorsMCiEioZ20887639 = ZIyvEGdNxUorsMCiEioZ33438008;     ZIyvEGdNxUorsMCiEioZ33438008 = ZIyvEGdNxUorsMCiEioZ83083840;     ZIyvEGdNxUorsMCiEioZ83083840 = ZIyvEGdNxUorsMCiEioZ93608627;     ZIyvEGdNxUorsMCiEioZ93608627 = ZIyvEGdNxUorsMCiEioZ64392961;     ZIyvEGdNxUorsMCiEioZ64392961 = ZIyvEGdNxUorsMCiEioZ59323571;     ZIyvEGdNxUorsMCiEioZ59323571 = ZIyvEGdNxUorsMCiEioZ96469760;     ZIyvEGdNxUorsMCiEioZ96469760 = ZIyvEGdNxUorsMCiEioZ54410980;     ZIyvEGdNxUorsMCiEioZ54410980 = ZIyvEGdNxUorsMCiEioZ58121235;     ZIyvEGdNxUorsMCiEioZ58121235 = ZIyvEGdNxUorsMCiEioZ69465605;     ZIyvEGdNxUorsMCiEioZ69465605 = ZIyvEGdNxUorsMCiEioZ58115318;     ZIyvEGdNxUorsMCiEioZ58115318 = ZIyvEGdNxUorsMCiEioZ93414355;     ZIyvEGdNxUorsMCiEioZ93414355 = ZIyvEGdNxUorsMCiEioZ56294095;     ZIyvEGdNxUorsMCiEioZ56294095 = ZIyvEGdNxUorsMCiEioZ73950279;     ZIyvEGdNxUorsMCiEioZ73950279 = ZIyvEGdNxUorsMCiEioZ66624740;     ZIyvEGdNxUorsMCiEioZ66624740 = ZIyvEGdNxUorsMCiEioZ99447617;     ZIyvEGdNxUorsMCiEioZ99447617 = ZIyvEGdNxUorsMCiEioZ39349849;     ZIyvEGdNxUorsMCiEioZ39349849 = ZIyvEGdNxUorsMCiEioZ91724445;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void bIyFYHLFmuQTMWfCbPEp18629132() {     int kPuBmNsyGCgDwQsBphxo55155098 = -424842641;    int kPuBmNsyGCgDwQsBphxo4108073 = 55248764;    int kPuBmNsyGCgDwQsBphxo85518347 = -354403821;    int kPuBmNsyGCgDwQsBphxo10198581 = -19398398;    int kPuBmNsyGCgDwQsBphxo97889428 = -866563261;    int kPuBmNsyGCgDwQsBphxo11150649 = -550511214;    int kPuBmNsyGCgDwQsBphxo24277888 = -84027525;    int kPuBmNsyGCgDwQsBphxo32627117 = -943234521;    int kPuBmNsyGCgDwQsBphxo75105224 = -234981856;    int kPuBmNsyGCgDwQsBphxo81459437 = 22628881;    int kPuBmNsyGCgDwQsBphxo75647057 = -332121043;    int kPuBmNsyGCgDwQsBphxo56092786 = -395524544;    int kPuBmNsyGCgDwQsBphxo54572243 = -407741486;    int kPuBmNsyGCgDwQsBphxo5554674 = -853235268;    int kPuBmNsyGCgDwQsBphxo69991518 = -505230464;    int kPuBmNsyGCgDwQsBphxo56058115 = -51518770;    int kPuBmNsyGCgDwQsBphxo4045072 = -933962295;    int kPuBmNsyGCgDwQsBphxo25845058 = -593732313;    int kPuBmNsyGCgDwQsBphxo48390422 = -667950079;    int kPuBmNsyGCgDwQsBphxo11478208 = -789274561;    int kPuBmNsyGCgDwQsBphxo27814265 = -783049443;    int kPuBmNsyGCgDwQsBphxo14751574 = -857871278;    int kPuBmNsyGCgDwQsBphxo84004114 = -448775804;    int kPuBmNsyGCgDwQsBphxo59553877 = -112384884;    int kPuBmNsyGCgDwQsBphxo28854467 = -144947633;    int kPuBmNsyGCgDwQsBphxo77888209 = -149614448;    int kPuBmNsyGCgDwQsBphxo28268681 = -470483095;    int kPuBmNsyGCgDwQsBphxo60614928 = -868383134;    int kPuBmNsyGCgDwQsBphxo30762507 = -316187213;    int kPuBmNsyGCgDwQsBphxo10600457 = -814590248;    int kPuBmNsyGCgDwQsBphxo2238655 = -232776897;    int kPuBmNsyGCgDwQsBphxo62460702 = -899595250;    int kPuBmNsyGCgDwQsBphxo47527219 = -719280472;    int kPuBmNsyGCgDwQsBphxo47637909 = 76658587;    int kPuBmNsyGCgDwQsBphxo32632958 = -827136832;    int kPuBmNsyGCgDwQsBphxo56181721 = -130004890;    int kPuBmNsyGCgDwQsBphxo2357405 = -582375756;    int kPuBmNsyGCgDwQsBphxo68625643 = -246346756;    int kPuBmNsyGCgDwQsBphxo66940553 = -419193721;    int kPuBmNsyGCgDwQsBphxo84553795 = 28013735;    int kPuBmNsyGCgDwQsBphxo29142867 = -155269302;    int kPuBmNsyGCgDwQsBphxo29965660 = -28666124;    int kPuBmNsyGCgDwQsBphxo35064319 = -712925624;    int kPuBmNsyGCgDwQsBphxo18852470 = -297575413;    int kPuBmNsyGCgDwQsBphxo87431089 = -653355194;    int kPuBmNsyGCgDwQsBphxo60852600 = -107620063;    int kPuBmNsyGCgDwQsBphxo2866040 = -751936247;    int kPuBmNsyGCgDwQsBphxo31612278 = -186667924;    int kPuBmNsyGCgDwQsBphxo2955397 = -978436858;    int kPuBmNsyGCgDwQsBphxo65831324 = -830091134;    int kPuBmNsyGCgDwQsBphxo76282045 = -345758718;    int kPuBmNsyGCgDwQsBphxo42174729 = 2843863;    int kPuBmNsyGCgDwQsBphxo68997124 = -555894862;    int kPuBmNsyGCgDwQsBphxo51882444 = -774776883;    int kPuBmNsyGCgDwQsBphxo90814866 = -400258484;    int kPuBmNsyGCgDwQsBphxo40403524 = -566971363;    int kPuBmNsyGCgDwQsBphxo20103959 = -495975432;    int kPuBmNsyGCgDwQsBphxo25964471 = -142018937;    int kPuBmNsyGCgDwQsBphxo81344114 = -874450765;    int kPuBmNsyGCgDwQsBphxo20001220 = -616948813;    int kPuBmNsyGCgDwQsBphxo82881968 = 19971880;    int kPuBmNsyGCgDwQsBphxo63662959 = -215644391;    int kPuBmNsyGCgDwQsBphxo1864610 = -527047308;    int kPuBmNsyGCgDwQsBphxo64504767 = -420391608;    int kPuBmNsyGCgDwQsBphxo79220783 = -744594222;    int kPuBmNsyGCgDwQsBphxo13186356 = -432525793;    int kPuBmNsyGCgDwQsBphxo8565568 = -676244073;    int kPuBmNsyGCgDwQsBphxo6934334 = -384400073;    int kPuBmNsyGCgDwQsBphxo72921716 = 73901563;    int kPuBmNsyGCgDwQsBphxo13809797 = -275225575;    int kPuBmNsyGCgDwQsBphxo53700711 = -469143014;    int kPuBmNsyGCgDwQsBphxo35419429 = -587615539;    int kPuBmNsyGCgDwQsBphxo58904504 = -74538592;    int kPuBmNsyGCgDwQsBphxo63836626 = -595963815;    int kPuBmNsyGCgDwQsBphxo82335340 = -534005259;    int kPuBmNsyGCgDwQsBphxo97848604 = -654383319;    int kPuBmNsyGCgDwQsBphxo79687255 = -44945655;    int kPuBmNsyGCgDwQsBphxo65151644 = -51200392;    int kPuBmNsyGCgDwQsBphxo72122787 = -459029691;    int kPuBmNsyGCgDwQsBphxo68001866 = 62672429;    int kPuBmNsyGCgDwQsBphxo75022170 = -397678201;    int kPuBmNsyGCgDwQsBphxo96656402 = -183815171;    int kPuBmNsyGCgDwQsBphxo57659531 = -889946277;    int kPuBmNsyGCgDwQsBphxo64931183 = -486096079;    int kPuBmNsyGCgDwQsBphxo34318411 = -368831531;    int kPuBmNsyGCgDwQsBphxo60063925 = -135620761;    int kPuBmNsyGCgDwQsBphxo93463577 = -243700389;    int kPuBmNsyGCgDwQsBphxo95644774 = -944503589;    int kPuBmNsyGCgDwQsBphxo56823043 = -523082930;    int kPuBmNsyGCgDwQsBphxo92229433 = -160165469;    int kPuBmNsyGCgDwQsBphxo36077763 = -634029458;    int kPuBmNsyGCgDwQsBphxo76392933 = -340356819;    int kPuBmNsyGCgDwQsBphxo87281528 = -371895992;    int kPuBmNsyGCgDwQsBphxo46939334 = -802244908;    int kPuBmNsyGCgDwQsBphxo1671828 = -991958145;    int kPuBmNsyGCgDwQsBphxo65479908 = -939624912;    int kPuBmNsyGCgDwQsBphxo28101050 = -501618817;    int kPuBmNsyGCgDwQsBphxo70559551 = -192534016;    int kPuBmNsyGCgDwQsBphxo39631687 = -552981191;    int kPuBmNsyGCgDwQsBphxo74244734 = -424842641;     kPuBmNsyGCgDwQsBphxo55155098 = kPuBmNsyGCgDwQsBphxo4108073;     kPuBmNsyGCgDwQsBphxo4108073 = kPuBmNsyGCgDwQsBphxo85518347;     kPuBmNsyGCgDwQsBphxo85518347 = kPuBmNsyGCgDwQsBphxo10198581;     kPuBmNsyGCgDwQsBphxo10198581 = kPuBmNsyGCgDwQsBphxo97889428;     kPuBmNsyGCgDwQsBphxo97889428 = kPuBmNsyGCgDwQsBphxo11150649;     kPuBmNsyGCgDwQsBphxo11150649 = kPuBmNsyGCgDwQsBphxo24277888;     kPuBmNsyGCgDwQsBphxo24277888 = kPuBmNsyGCgDwQsBphxo32627117;     kPuBmNsyGCgDwQsBphxo32627117 = kPuBmNsyGCgDwQsBphxo75105224;     kPuBmNsyGCgDwQsBphxo75105224 = kPuBmNsyGCgDwQsBphxo81459437;     kPuBmNsyGCgDwQsBphxo81459437 = kPuBmNsyGCgDwQsBphxo75647057;     kPuBmNsyGCgDwQsBphxo75647057 = kPuBmNsyGCgDwQsBphxo56092786;     kPuBmNsyGCgDwQsBphxo56092786 = kPuBmNsyGCgDwQsBphxo54572243;     kPuBmNsyGCgDwQsBphxo54572243 = kPuBmNsyGCgDwQsBphxo5554674;     kPuBmNsyGCgDwQsBphxo5554674 = kPuBmNsyGCgDwQsBphxo69991518;     kPuBmNsyGCgDwQsBphxo69991518 = kPuBmNsyGCgDwQsBphxo56058115;     kPuBmNsyGCgDwQsBphxo56058115 = kPuBmNsyGCgDwQsBphxo4045072;     kPuBmNsyGCgDwQsBphxo4045072 = kPuBmNsyGCgDwQsBphxo25845058;     kPuBmNsyGCgDwQsBphxo25845058 = kPuBmNsyGCgDwQsBphxo48390422;     kPuBmNsyGCgDwQsBphxo48390422 = kPuBmNsyGCgDwQsBphxo11478208;     kPuBmNsyGCgDwQsBphxo11478208 = kPuBmNsyGCgDwQsBphxo27814265;     kPuBmNsyGCgDwQsBphxo27814265 = kPuBmNsyGCgDwQsBphxo14751574;     kPuBmNsyGCgDwQsBphxo14751574 = kPuBmNsyGCgDwQsBphxo84004114;     kPuBmNsyGCgDwQsBphxo84004114 = kPuBmNsyGCgDwQsBphxo59553877;     kPuBmNsyGCgDwQsBphxo59553877 = kPuBmNsyGCgDwQsBphxo28854467;     kPuBmNsyGCgDwQsBphxo28854467 = kPuBmNsyGCgDwQsBphxo77888209;     kPuBmNsyGCgDwQsBphxo77888209 = kPuBmNsyGCgDwQsBphxo28268681;     kPuBmNsyGCgDwQsBphxo28268681 = kPuBmNsyGCgDwQsBphxo60614928;     kPuBmNsyGCgDwQsBphxo60614928 = kPuBmNsyGCgDwQsBphxo30762507;     kPuBmNsyGCgDwQsBphxo30762507 = kPuBmNsyGCgDwQsBphxo10600457;     kPuBmNsyGCgDwQsBphxo10600457 = kPuBmNsyGCgDwQsBphxo2238655;     kPuBmNsyGCgDwQsBphxo2238655 = kPuBmNsyGCgDwQsBphxo62460702;     kPuBmNsyGCgDwQsBphxo62460702 = kPuBmNsyGCgDwQsBphxo47527219;     kPuBmNsyGCgDwQsBphxo47527219 = kPuBmNsyGCgDwQsBphxo47637909;     kPuBmNsyGCgDwQsBphxo47637909 = kPuBmNsyGCgDwQsBphxo32632958;     kPuBmNsyGCgDwQsBphxo32632958 = kPuBmNsyGCgDwQsBphxo56181721;     kPuBmNsyGCgDwQsBphxo56181721 = kPuBmNsyGCgDwQsBphxo2357405;     kPuBmNsyGCgDwQsBphxo2357405 = kPuBmNsyGCgDwQsBphxo68625643;     kPuBmNsyGCgDwQsBphxo68625643 = kPuBmNsyGCgDwQsBphxo66940553;     kPuBmNsyGCgDwQsBphxo66940553 = kPuBmNsyGCgDwQsBphxo84553795;     kPuBmNsyGCgDwQsBphxo84553795 = kPuBmNsyGCgDwQsBphxo29142867;     kPuBmNsyGCgDwQsBphxo29142867 = kPuBmNsyGCgDwQsBphxo29965660;     kPuBmNsyGCgDwQsBphxo29965660 = kPuBmNsyGCgDwQsBphxo35064319;     kPuBmNsyGCgDwQsBphxo35064319 = kPuBmNsyGCgDwQsBphxo18852470;     kPuBmNsyGCgDwQsBphxo18852470 = kPuBmNsyGCgDwQsBphxo87431089;     kPuBmNsyGCgDwQsBphxo87431089 = kPuBmNsyGCgDwQsBphxo60852600;     kPuBmNsyGCgDwQsBphxo60852600 = kPuBmNsyGCgDwQsBphxo2866040;     kPuBmNsyGCgDwQsBphxo2866040 = kPuBmNsyGCgDwQsBphxo31612278;     kPuBmNsyGCgDwQsBphxo31612278 = kPuBmNsyGCgDwQsBphxo2955397;     kPuBmNsyGCgDwQsBphxo2955397 = kPuBmNsyGCgDwQsBphxo65831324;     kPuBmNsyGCgDwQsBphxo65831324 = kPuBmNsyGCgDwQsBphxo76282045;     kPuBmNsyGCgDwQsBphxo76282045 = kPuBmNsyGCgDwQsBphxo42174729;     kPuBmNsyGCgDwQsBphxo42174729 = kPuBmNsyGCgDwQsBphxo68997124;     kPuBmNsyGCgDwQsBphxo68997124 = kPuBmNsyGCgDwQsBphxo51882444;     kPuBmNsyGCgDwQsBphxo51882444 = kPuBmNsyGCgDwQsBphxo90814866;     kPuBmNsyGCgDwQsBphxo90814866 = kPuBmNsyGCgDwQsBphxo40403524;     kPuBmNsyGCgDwQsBphxo40403524 = kPuBmNsyGCgDwQsBphxo20103959;     kPuBmNsyGCgDwQsBphxo20103959 = kPuBmNsyGCgDwQsBphxo25964471;     kPuBmNsyGCgDwQsBphxo25964471 = kPuBmNsyGCgDwQsBphxo81344114;     kPuBmNsyGCgDwQsBphxo81344114 = kPuBmNsyGCgDwQsBphxo20001220;     kPuBmNsyGCgDwQsBphxo20001220 = kPuBmNsyGCgDwQsBphxo82881968;     kPuBmNsyGCgDwQsBphxo82881968 = kPuBmNsyGCgDwQsBphxo63662959;     kPuBmNsyGCgDwQsBphxo63662959 = kPuBmNsyGCgDwQsBphxo1864610;     kPuBmNsyGCgDwQsBphxo1864610 = kPuBmNsyGCgDwQsBphxo64504767;     kPuBmNsyGCgDwQsBphxo64504767 = kPuBmNsyGCgDwQsBphxo79220783;     kPuBmNsyGCgDwQsBphxo79220783 = kPuBmNsyGCgDwQsBphxo13186356;     kPuBmNsyGCgDwQsBphxo13186356 = kPuBmNsyGCgDwQsBphxo8565568;     kPuBmNsyGCgDwQsBphxo8565568 = kPuBmNsyGCgDwQsBphxo6934334;     kPuBmNsyGCgDwQsBphxo6934334 = kPuBmNsyGCgDwQsBphxo72921716;     kPuBmNsyGCgDwQsBphxo72921716 = kPuBmNsyGCgDwQsBphxo13809797;     kPuBmNsyGCgDwQsBphxo13809797 = kPuBmNsyGCgDwQsBphxo53700711;     kPuBmNsyGCgDwQsBphxo53700711 = kPuBmNsyGCgDwQsBphxo35419429;     kPuBmNsyGCgDwQsBphxo35419429 = kPuBmNsyGCgDwQsBphxo58904504;     kPuBmNsyGCgDwQsBphxo58904504 = kPuBmNsyGCgDwQsBphxo63836626;     kPuBmNsyGCgDwQsBphxo63836626 = kPuBmNsyGCgDwQsBphxo82335340;     kPuBmNsyGCgDwQsBphxo82335340 = kPuBmNsyGCgDwQsBphxo97848604;     kPuBmNsyGCgDwQsBphxo97848604 = kPuBmNsyGCgDwQsBphxo79687255;     kPuBmNsyGCgDwQsBphxo79687255 = kPuBmNsyGCgDwQsBphxo65151644;     kPuBmNsyGCgDwQsBphxo65151644 = kPuBmNsyGCgDwQsBphxo72122787;     kPuBmNsyGCgDwQsBphxo72122787 = kPuBmNsyGCgDwQsBphxo68001866;     kPuBmNsyGCgDwQsBphxo68001866 = kPuBmNsyGCgDwQsBphxo75022170;     kPuBmNsyGCgDwQsBphxo75022170 = kPuBmNsyGCgDwQsBphxo96656402;     kPuBmNsyGCgDwQsBphxo96656402 = kPuBmNsyGCgDwQsBphxo57659531;     kPuBmNsyGCgDwQsBphxo57659531 = kPuBmNsyGCgDwQsBphxo64931183;     kPuBmNsyGCgDwQsBphxo64931183 = kPuBmNsyGCgDwQsBphxo34318411;     kPuBmNsyGCgDwQsBphxo34318411 = kPuBmNsyGCgDwQsBphxo60063925;     kPuBmNsyGCgDwQsBphxo60063925 = kPuBmNsyGCgDwQsBphxo93463577;     kPuBmNsyGCgDwQsBphxo93463577 = kPuBmNsyGCgDwQsBphxo95644774;     kPuBmNsyGCgDwQsBphxo95644774 = kPuBmNsyGCgDwQsBphxo56823043;     kPuBmNsyGCgDwQsBphxo56823043 = kPuBmNsyGCgDwQsBphxo92229433;     kPuBmNsyGCgDwQsBphxo92229433 = kPuBmNsyGCgDwQsBphxo36077763;     kPuBmNsyGCgDwQsBphxo36077763 = kPuBmNsyGCgDwQsBphxo76392933;     kPuBmNsyGCgDwQsBphxo76392933 = kPuBmNsyGCgDwQsBphxo87281528;     kPuBmNsyGCgDwQsBphxo87281528 = kPuBmNsyGCgDwQsBphxo46939334;     kPuBmNsyGCgDwQsBphxo46939334 = kPuBmNsyGCgDwQsBphxo1671828;     kPuBmNsyGCgDwQsBphxo1671828 = kPuBmNsyGCgDwQsBphxo65479908;     kPuBmNsyGCgDwQsBphxo65479908 = kPuBmNsyGCgDwQsBphxo28101050;     kPuBmNsyGCgDwQsBphxo28101050 = kPuBmNsyGCgDwQsBphxo70559551;     kPuBmNsyGCgDwQsBphxo70559551 = kPuBmNsyGCgDwQsBphxo39631687;     kPuBmNsyGCgDwQsBphxo39631687 = kPuBmNsyGCgDwQsBphxo74244734;     kPuBmNsyGCgDwQsBphxo74244734 = kPuBmNsyGCgDwQsBphxo55155098;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void zFvlfkawuIlWfAwqCdAP25038271() {     int FZpavHZNjJgUNmTKZArc89243889 = -754241043;    int FZpavHZNjJgUNmTKZArc59460853 = -633574024;    int FZpavHZNjJgUNmTKZArc56763925 = -917644127;    int FZpavHZNjJgUNmTKZArc70323985 = -402975684;    int FZpavHZNjJgUNmTKZArc51614226 = -543337318;    int FZpavHZNjJgUNmTKZArc43976208 = -393882784;    int FZpavHZNjJgUNmTKZArc31539725 = -802593268;    int FZpavHZNjJgUNmTKZArc72939274 = -804276609;    int FZpavHZNjJgUNmTKZArc87213034 = -88612255;    int FZpavHZNjJgUNmTKZArc42004414 = -121506116;    int FZpavHZNjJgUNmTKZArc65180880 = -945393559;    int FZpavHZNjJgUNmTKZArc64462369 = -510126741;    int FZpavHZNjJgUNmTKZArc714853 = -617265080;    int FZpavHZNjJgUNmTKZArc69587394 = 57976057;    int FZpavHZNjJgUNmTKZArc30873622 = -899432457;    int FZpavHZNjJgUNmTKZArc14216893 = -975906144;    int FZpavHZNjJgUNmTKZArc5902340 = -154804390;    int FZpavHZNjJgUNmTKZArc67474002 = -61140407;    int FZpavHZNjJgUNmTKZArc74545979 = -662712596;    int FZpavHZNjJgUNmTKZArc93319364 = -34203199;    int FZpavHZNjJgUNmTKZArc63727815 = -573771188;    int FZpavHZNjJgUNmTKZArc60714270 = -394522460;    int FZpavHZNjJgUNmTKZArc80265119 = -503048923;    int FZpavHZNjJgUNmTKZArc46565705 = -462491113;    int FZpavHZNjJgUNmTKZArc47970986 = -868767406;    int FZpavHZNjJgUNmTKZArc34037029 = -693080981;    int FZpavHZNjJgUNmTKZArc47116932 = -494612981;    int FZpavHZNjJgUNmTKZArc45391380 = -19854333;    int FZpavHZNjJgUNmTKZArc47531696 = -497546965;    int FZpavHZNjJgUNmTKZArc13199458 = -248589308;    int FZpavHZNjJgUNmTKZArc70825624 = -729680348;    int FZpavHZNjJgUNmTKZArc52953984 = -335268905;    int FZpavHZNjJgUNmTKZArc65006464 = -337390015;    int FZpavHZNjJgUNmTKZArc8423969 = -818835908;    int FZpavHZNjJgUNmTKZArc24626284 = -957852021;    int FZpavHZNjJgUNmTKZArc49069560 = -173644354;    int FZpavHZNjJgUNmTKZArc48256692 = -40997700;    int FZpavHZNjJgUNmTKZArc91155532 = -394588138;    int FZpavHZNjJgUNmTKZArc97973523 = 83961085;    int FZpavHZNjJgUNmTKZArc12863853 = -29255280;    int FZpavHZNjJgUNmTKZArc55578823 = -280236856;    int FZpavHZNjJgUNmTKZArc82280440 = -387456347;    int FZpavHZNjJgUNmTKZArc37148874 = -322850336;    int FZpavHZNjJgUNmTKZArc45603318 = -260245241;    int FZpavHZNjJgUNmTKZArc71161569 = 2456876;    int FZpavHZNjJgUNmTKZArc21797628 = -721261172;    int FZpavHZNjJgUNmTKZArc25602336 = -964091731;    int FZpavHZNjJgUNmTKZArc52448007 = -201148197;    int FZpavHZNjJgUNmTKZArc36079000 = -216207950;    int FZpavHZNjJgUNmTKZArc7474991 = -848043503;    int FZpavHZNjJgUNmTKZArc69848372 = 18539650;    int FZpavHZNjJgUNmTKZArc87446545 = -213258117;    int FZpavHZNjJgUNmTKZArc79475162 = -642799136;    int FZpavHZNjJgUNmTKZArc20593973 = -47126977;    int FZpavHZNjJgUNmTKZArc61310456 = -777947409;    int FZpavHZNjJgUNmTKZArc28529619 = -259718584;    int FZpavHZNjJgUNmTKZArc79195733 = -30525102;    int FZpavHZNjJgUNmTKZArc10198220 = -355153015;    int FZpavHZNjJgUNmTKZArc22353000 = -534208278;    int FZpavHZNjJgUNmTKZArc17577197 = -850256338;    int FZpavHZNjJgUNmTKZArc96859275 = -899269803;    int FZpavHZNjJgUNmTKZArc86148344 = -682738936;    int FZpavHZNjJgUNmTKZArc25407579 = -206729645;    int FZpavHZNjJgUNmTKZArc74013576 = -840022948;    int FZpavHZNjJgUNmTKZArc71178789 = -391825769;    int FZpavHZNjJgUNmTKZArc12226896 = -510124655;    int FZpavHZNjJgUNmTKZArc99455904 = -72736726;    int FZpavHZNjJgUNmTKZArc92290883 = -798429173;    int FZpavHZNjJgUNmTKZArc44961110 = 15828077;    int FZpavHZNjJgUNmTKZArc81804062 = -625788103;    int FZpavHZNjJgUNmTKZArc65960201 = -834908444;    int FZpavHZNjJgUNmTKZArc14746808 = -760216252;    int FZpavHZNjJgUNmTKZArc69500478 = -45101492;    int FZpavHZNjJgUNmTKZArc61682126 = -533457317;    int FZpavHZNjJgUNmTKZArc37740541 = -753966343;    int FZpavHZNjJgUNmTKZArc81447375 = -86314842;    int FZpavHZNjJgUNmTKZArc23565397 = 28327876;    int FZpavHZNjJgUNmTKZArc34661801 = -142803682;    int FZpavHZNjJgUNmTKZArc75404136 = -364947990;    int FZpavHZNjJgUNmTKZArc26173358 = -47506234;    int FZpavHZNjJgUNmTKZArc8434693 = -728989250;    int FZpavHZNjJgUNmTKZArc94668924 = -193464785;    int FZpavHZNjJgUNmTKZArc9312380 = -803646383;    int FZpavHZNjJgUNmTKZArc40056706 = -649503462;    int FZpavHZNjJgUNmTKZArc43351085 = -167128959;    int FZpavHZNjJgUNmTKZArc83379079 = -416422231;    int FZpavHZNjJgUNmTKZArc73478822 = -692469769;    int FZpavHZNjJgUNmTKZArc44412492 = -190263039;    int FZpavHZNjJgUNmTKZArc47113513 = 59111500;    int FZpavHZNjJgUNmTKZArc96096665 = -598133437;    int FZpavHZNjJgUNmTKZArc69873826 = -43119253;    int FZpavHZNjJgUNmTKZArc38058472 = -685844686;    int FZpavHZNjJgUNmTKZArc68802533 = -860379860;    int FZpavHZNjJgUNmTKZArc80396326 = -65782578;    int FZpavHZNjJgUNmTKZArc16004578 = -129985477;    int FZpavHZNjJgUNmTKZArc69430478 = -597497921;    int FZpavHZNjJgUNmTKZArc56872861 = -80726703;    int FZpavHZNjJgUNmTKZArc63135297 = -482827389;    int FZpavHZNjJgUNmTKZArc74424529 = -868419473;    int FZpavHZNjJgUNmTKZArc58934673 = -754241043;     FZpavHZNjJgUNmTKZArc89243889 = FZpavHZNjJgUNmTKZArc59460853;     FZpavHZNjJgUNmTKZArc59460853 = FZpavHZNjJgUNmTKZArc56763925;     FZpavHZNjJgUNmTKZArc56763925 = FZpavHZNjJgUNmTKZArc70323985;     FZpavHZNjJgUNmTKZArc70323985 = FZpavHZNjJgUNmTKZArc51614226;     FZpavHZNjJgUNmTKZArc51614226 = FZpavHZNjJgUNmTKZArc43976208;     FZpavHZNjJgUNmTKZArc43976208 = FZpavHZNjJgUNmTKZArc31539725;     FZpavHZNjJgUNmTKZArc31539725 = FZpavHZNjJgUNmTKZArc72939274;     FZpavHZNjJgUNmTKZArc72939274 = FZpavHZNjJgUNmTKZArc87213034;     FZpavHZNjJgUNmTKZArc87213034 = FZpavHZNjJgUNmTKZArc42004414;     FZpavHZNjJgUNmTKZArc42004414 = FZpavHZNjJgUNmTKZArc65180880;     FZpavHZNjJgUNmTKZArc65180880 = FZpavHZNjJgUNmTKZArc64462369;     FZpavHZNjJgUNmTKZArc64462369 = FZpavHZNjJgUNmTKZArc714853;     FZpavHZNjJgUNmTKZArc714853 = FZpavHZNjJgUNmTKZArc69587394;     FZpavHZNjJgUNmTKZArc69587394 = FZpavHZNjJgUNmTKZArc30873622;     FZpavHZNjJgUNmTKZArc30873622 = FZpavHZNjJgUNmTKZArc14216893;     FZpavHZNjJgUNmTKZArc14216893 = FZpavHZNjJgUNmTKZArc5902340;     FZpavHZNjJgUNmTKZArc5902340 = FZpavHZNjJgUNmTKZArc67474002;     FZpavHZNjJgUNmTKZArc67474002 = FZpavHZNjJgUNmTKZArc74545979;     FZpavHZNjJgUNmTKZArc74545979 = FZpavHZNjJgUNmTKZArc93319364;     FZpavHZNjJgUNmTKZArc93319364 = FZpavHZNjJgUNmTKZArc63727815;     FZpavHZNjJgUNmTKZArc63727815 = FZpavHZNjJgUNmTKZArc60714270;     FZpavHZNjJgUNmTKZArc60714270 = FZpavHZNjJgUNmTKZArc80265119;     FZpavHZNjJgUNmTKZArc80265119 = FZpavHZNjJgUNmTKZArc46565705;     FZpavHZNjJgUNmTKZArc46565705 = FZpavHZNjJgUNmTKZArc47970986;     FZpavHZNjJgUNmTKZArc47970986 = FZpavHZNjJgUNmTKZArc34037029;     FZpavHZNjJgUNmTKZArc34037029 = FZpavHZNjJgUNmTKZArc47116932;     FZpavHZNjJgUNmTKZArc47116932 = FZpavHZNjJgUNmTKZArc45391380;     FZpavHZNjJgUNmTKZArc45391380 = FZpavHZNjJgUNmTKZArc47531696;     FZpavHZNjJgUNmTKZArc47531696 = FZpavHZNjJgUNmTKZArc13199458;     FZpavHZNjJgUNmTKZArc13199458 = FZpavHZNjJgUNmTKZArc70825624;     FZpavHZNjJgUNmTKZArc70825624 = FZpavHZNjJgUNmTKZArc52953984;     FZpavHZNjJgUNmTKZArc52953984 = FZpavHZNjJgUNmTKZArc65006464;     FZpavHZNjJgUNmTKZArc65006464 = FZpavHZNjJgUNmTKZArc8423969;     FZpavHZNjJgUNmTKZArc8423969 = FZpavHZNjJgUNmTKZArc24626284;     FZpavHZNjJgUNmTKZArc24626284 = FZpavHZNjJgUNmTKZArc49069560;     FZpavHZNjJgUNmTKZArc49069560 = FZpavHZNjJgUNmTKZArc48256692;     FZpavHZNjJgUNmTKZArc48256692 = FZpavHZNjJgUNmTKZArc91155532;     FZpavHZNjJgUNmTKZArc91155532 = FZpavHZNjJgUNmTKZArc97973523;     FZpavHZNjJgUNmTKZArc97973523 = FZpavHZNjJgUNmTKZArc12863853;     FZpavHZNjJgUNmTKZArc12863853 = FZpavHZNjJgUNmTKZArc55578823;     FZpavHZNjJgUNmTKZArc55578823 = FZpavHZNjJgUNmTKZArc82280440;     FZpavHZNjJgUNmTKZArc82280440 = FZpavHZNjJgUNmTKZArc37148874;     FZpavHZNjJgUNmTKZArc37148874 = FZpavHZNjJgUNmTKZArc45603318;     FZpavHZNjJgUNmTKZArc45603318 = FZpavHZNjJgUNmTKZArc71161569;     FZpavHZNjJgUNmTKZArc71161569 = FZpavHZNjJgUNmTKZArc21797628;     FZpavHZNjJgUNmTKZArc21797628 = FZpavHZNjJgUNmTKZArc25602336;     FZpavHZNjJgUNmTKZArc25602336 = FZpavHZNjJgUNmTKZArc52448007;     FZpavHZNjJgUNmTKZArc52448007 = FZpavHZNjJgUNmTKZArc36079000;     FZpavHZNjJgUNmTKZArc36079000 = FZpavHZNjJgUNmTKZArc7474991;     FZpavHZNjJgUNmTKZArc7474991 = FZpavHZNjJgUNmTKZArc69848372;     FZpavHZNjJgUNmTKZArc69848372 = FZpavHZNjJgUNmTKZArc87446545;     FZpavHZNjJgUNmTKZArc87446545 = FZpavHZNjJgUNmTKZArc79475162;     FZpavHZNjJgUNmTKZArc79475162 = FZpavHZNjJgUNmTKZArc20593973;     FZpavHZNjJgUNmTKZArc20593973 = FZpavHZNjJgUNmTKZArc61310456;     FZpavHZNjJgUNmTKZArc61310456 = FZpavHZNjJgUNmTKZArc28529619;     FZpavHZNjJgUNmTKZArc28529619 = FZpavHZNjJgUNmTKZArc79195733;     FZpavHZNjJgUNmTKZArc79195733 = FZpavHZNjJgUNmTKZArc10198220;     FZpavHZNjJgUNmTKZArc10198220 = FZpavHZNjJgUNmTKZArc22353000;     FZpavHZNjJgUNmTKZArc22353000 = FZpavHZNjJgUNmTKZArc17577197;     FZpavHZNjJgUNmTKZArc17577197 = FZpavHZNjJgUNmTKZArc96859275;     FZpavHZNjJgUNmTKZArc96859275 = FZpavHZNjJgUNmTKZArc86148344;     FZpavHZNjJgUNmTKZArc86148344 = FZpavHZNjJgUNmTKZArc25407579;     FZpavHZNjJgUNmTKZArc25407579 = FZpavHZNjJgUNmTKZArc74013576;     FZpavHZNjJgUNmTKZArc74013576 = FZpavHZNjJgUNmTKZArc71178789;     FZpavHZNjJgUNmTKZArc71178789 = FZpavHZNjJgUNmTKZArc12226896;     FZpavHZNjJgUNmTKZArc12226896 = FZpavHZNjJgUNmTKZArc99455904;     FZpavHZNjJgUNmTKZArc99455904 = FZpavHZNjJgUNmTKZArc92290883;     FZpavHZNjJgUNmTKZArc92290883 = FZpavHZNjJgUNmTKZArc44961110;     FZpavHZNjJgUNmTKZArc44961110 = FZpavHZNjJgUNmTKZArc81804062;     FZpavHZNjJgUNmTKZArc81804062 = FZpavHZNjJgUNmTKZArc65960201;     FZpavHZNjJgUNmTKZArc65960201 = FZpavHZNjJgUNmTKZArc14746808;     FZpavHZNjJgUNmTKZArc14746808 = FZpavHZNjJgUNmTKZArc69500478;     FZpavHZNjJgUNmTKZArc69500478 = FZpavHZNjJgUNmTKZArc61682126;     FZpavHZNjJgUNmTKZArc61682126 = FZpavHZNjJgUNmTKZArc37740541;     FZpavHZNjJgUNmTKZArc37740541 = FZpavHZNjJgUNmTKZArc81447375;     FZpavHZNjJgUNmTKZArc81447375 = FZpavHZNjJgUNmTKZArc23565397;     FZpavHZNjJgUNmTKZArc23565397 = FZpavHZNjJgUNmTKZArc34661801;     FZpavHZNjJgUNmTKZArc34661801 = FZpavHZNjJgUNmTKZArc75404136;     FZpavHZNjJgUNmTKZArc75404136 = FZpavHZNjJgUNmTKZArc26173358;     FZpavHZNjJgUNmTKZArc26173358 = FZpavHZNjJgUNmTKZArc8434693;     FZpavHZNjJgUNmTKZArc8434693 = FZpavHZNjJgUNmTKZArc94668924;     FZpavHZNjJgUNmTKZArc94668924 = FZpavHZNjJgUNmTKZArc9312380;     FZpavHZNjJgUNmTKZArc9312380 = FZpavHZNjJgUNmTKZArc40056706;     FZpavHZNjJgUNmTKZArc40056706 = FZpavHZNjJgUNmTKZArc43351085;     FZpavHZNjJgUNmTKZArc43351085 = FZpavHZNjJgUNmTKZArc83379079;     FZpavHZNjJgUNmTKZArc83379079 = FZpavHZNjJgUNmTKZArc73478822;     FZpavHZNjJgUNmTKZArc73478822 = FZpavHZNjJgUNmTKZArc44412492;     FZpavHZNjJgUNmTKZArc44412492 = FZpavHZNjJgUNmTKZArc47113513;     FZpavHZNjJgUNmTKZArc47113513 = FZpavHZNjJgUNmTKZArc96096665;     FZpavHZNjJgUNmTKZArc96096665 = FZpavHZNjJgUNmTKZArc69873826;     FZpavHZNjJgUNmTKZArc69873826 = FZpavHZNjJgUNmTKZArc38058472;     FZpavHZNjJgUNmTKZArc38058472 = FZpavHZNjJgUNmTKZArc68802533;     FZpavHZNjJgUNmTKZArc68802533 = FZpavHZNjJgUNmTKZArc80396326;     FZpavHZNjJgUNmTKZArc80396326 = FZpavHZNjJgUNmTKZArc16004578;     FZpavHZNjJgUNmTKZArc16004578 = FZpavHZNjJgUNmTKZArc69430478;     FZpavHZNjJgUNmTKZArc69430478 = FZpavHZNjJgUNmTKZArc56872861;     FZpavHZNjJgUNmTKZArc56872861 = FZpavHZNjJgUNmTKZArc63135297;     FZpavHZNjJgUNmTKZArc63135297 = FZpavHZNjJgUNmTKZArc74424529;     FZpavHZNjJgUNmTKZArc74424529 = FZpavHZNjJgUNmTKZArc58934673;     FZpavHZNjJgUNmTKZArc58934673 = FZpavHZNjJgUNmTKZArc89243889;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void deAlIDqlDQtJwDqeNPRB83689942() {     int AAIwoZHBMLYGITWgcPMF52674542 = -338014457;    int AAIwoZHBMLYGITWgcPMF84712432 = -393348638;    int AAIwoZHBMLYGITWgcPMF49353795 = -995718587;    int AAIwoZHBMLYGITWgcPMF75597963 = -416955410;    int AAIwoZHBMLYGITWgcPMF92728804 = -905060349;    int AAIwoZHBMLYGITWgcPMF21607436 = -235106013;    int AAIwoZHBMLYGITWgcPMF88970001 = -126919769;    int AAIwoZHBMLYGITWgcPMF25623103 = -22678925;    int AAIwoZHBMLYGITWgcPMF36195954 = -662541396;    int AAIwoZHBMLYGITWgcPMF76222442 = -701352634;    int AAIwoZHBMLYGITWgcPMF32786147 = -383236217;    int AAIwoZHBMLYGITWgcPMF41577058 = -44443508;    int AAIwoZHBMLYGITWgcPMF98882989 = -938910091;    int AAIwoZHBMLYGITWgcPMF3617301 = -674937461;    int AAIwoZHBMLYGITWgcPMF54207815 = -6597297;    int AAIwoZHBMLYGITWgcPMF14411577 = -463105582;    int AAIwoZHBMLYGITWgcPMF69759540 = -386075924;    int AAIwoZHBMLYGITWgcPMF11809986 = 63546073;    int AAIwoZHBMLYGITWgcPMF39422025 = -568269438;    int AAIwoZHBMLYGITWgcPMF88447575 = -448789984;    int AAIwoZHBMLYGITWgcPMF24175036 = -166280167;    int AAIwoZHBMLYGITWgcPMF23577468 = -109880218;    int AAIwoZHBMLYGITWgcPMF11460713 = -448436739;    int AAIwoZHBMLYGITWgcPMF43378105 = -1116958;    int AAIwoZHBMLYGITWgcPMF31405807 = -143376344;    int AAIwoZHBMLYGITWgcPMF76162345 = 53531003;    int AAIwoZHBMLYGITWgcPMF27925980 = -424364805;    int AAIwoZHBMLYGITWgcPMF57789131 = -375168752;    int AAIwoZHBMLYGITWgcPMF44836845 = 69381185;    int AAIwoZHBMLYGITWgcPMF52716233 = -999624671;    int AAIwoZHBMLYGITWgcPMF65753277 = -765243348;    int AAIwoZHBMLYGITWgcPMF83659663 = -934467158;    int AAIwoZHBMLYGITWgcPMF84992772 = -210482166;    int AAIwoZHBMLYGITWgcPMF4127891 = -927482451;    int AAIwoZHBMLYGITWgcPMF20953413 = -400934356;    int AAIwoZHBMLYGITWgcPMF24792329 = -217823016;    int AAIwoZHBMLYGITWgcPMF62305861 = -510679816;    int AAIwoZHBMLYGITWgcPMF30810613 = -753979091;    int AAIwoZHBMLYGITWgcPMF85786801 = -169836888;    int AAIwoZHBMLYGITWgcPMF17943505 = -178347619;    int AAIwoZHBMLYGITWgcPMF9797163 = -109129431;    int AAIwoZHBMLYGITWgcPMF91809889 = -70649582;    int AAIwoZHBMLYGITWgcPMF50549829 = -505151250;    int AAIwoZHBMLYGITWgcPMF25077765 = -807904541;    int AAIwoZHBMLYGITWgcPMF42956042 = -606344061;    int AAIwoZHBMLYGITWgcPMF16764790 = -288609797;    int AAIwoZHBMLYGITWgcPMF49905223 = -441458038;    int AAIwoZHBMLYGITWgcPMF83053107 = -372259530;    int AAIwoZHBMLYGITWgcPMF82419540 = -131978934;    int AAIwoZHBMLYGITWgcPMF60736593 = -170011880;    int AAIwoZHBMLYGITWgcPMF8484745 = -749144868;    int AAIwoZHBMLYGITWgcPMF5394114 = -842286124;    int AAIwoZHBMLYGITWgcPMF10325891 = -693959376;    int AAIwoZHBMLYGITWgcPMF9328468 = -290515425;    int AAIwoZHBMLYGITWgcPMF59514906 = -801594697;    int AAIwoZHBMLYGITWgcPMF29097074 = -128134239;    int AAIwoZHBMLYGITWgcPMF73251720 = -944911900;    int AAIwoZHBMLYGITWgcPMF5975691 = -894601630;    int AAIwoZHBMLYGITWgcPMF44192157 = -173579067;    int AAIwoZHBMLYGITWgcPMF16566459 = -858591352;    int AAIwoZHBMLYGITWgcPMF93681456 = -810741209;    int AAIwoZHBMLYGITWgcPMF31180871 = -751751017;    int AAIwoZHBMLYGITWgcPMF80786258 = 7939890;    int AAIwoZHBMLYGITWgcPMF83479721 = -662916725;    int AAIwoZHBMLYGITWgcPMF10469166 = -936109286;    int AAIwoZHBMLYGITWgcPMF49126484 = -448769060;    int AAIwoZHBMLYGITWgcPMF56584285 = -833961342;    int AAIwoZHBMLYGITWgcPMF94755098 = 88572360;    int AAIwoZHBMLYGITWgcPMF82663887 = -174003105;    int AAIwoZHBMLYGITWgcPMF29415487 = -788774282;    int AAIwoZHBMLYGITWgcPMF52105716 = -952425767;    int AAIwoZHBMLYGITWgcPMF38948928 = -632096834;    int AAIwoZHBMLYGITWgcPMF26023184 = -766617039;    int AAIwoZHBMLYGITWgcPMF21478521 = -289921819;    int AAIwoZHBMLYGITWgcPMF78650412 = -239660554;    int AAIwoZHBMLYGITWgcPMF32365146 = 4369415;    int AAIwoZHBMLYGITWgcPMF73027639 = -604728969;    int AAIwoZHBMLYGITWgcPMF86382947 = -640532198;    int AAIwoZHBMLYGITWgcPMF422063 = -394772898;    int AAIwoZHBMLYGITWgcPMF14641018 = -854766547;    int AAIwoZHBMLYGITWgcPMF26257123 = -505010959;    int AAIwoZHBMLYGITWgcPMF44872873 = 47894725;    int AAIwoZHBMLYGITWgcPMF75369591 = -143189818;    int AAIwoZHBMLYGITWgcPMF84100251 = -760606935;    int AAIwoZHBMLYGITWgcPMF44231488 = -150479804;    int AAIwoZHBMLYGITWgcPMF60359164 = -922957225;    int AAIwoZHBMLYGITWgcPMF73333772 = -140507782;    int AAIwoZHBMLYGITWgcPMF75664305 = -919966742;    int AAIwoZHBMLYGITWgcPMF44612984 = -25887754;    int AAIwoZHBMLYGITWgcPMF91856338 = -172800118;    int AAIwoZHBMLYGITWgcPMF51540609 = -272911117;    int AAIwoZHBMLYGITWgcPMF56330170 = -616078187;    int AAIwoZHBMLYGITWgcPMF86618455 = -480400024;    int AAIwoZHBMLYGITWgcPMF69220342 = -311245536;    int AAIwoZHBMLYGITWgcPMF24262049 = -367606411;    int AAIwoZHBMLYGITWgcPMF78616291 = -357378414;    int AAIwoZHBMLYGITWgcPMF11023632 = 21410528;    int AAIwoZHBMLYGITWgcPMF67070108 = -842234525;    int AAIwoZHBMLYGITWgcPMF14608600 = -871795256;    int AAIwoZHBMLYGITWgcPMF93829557 = -338014457;     AAIwoZHBMLYGITWgcPMF52674542 = AAIwoZHBMLYGITWgcPMF84712432;     AAIwoZHBMLYGITWgcPMF84712432 = AAIwoZHBMLYGITWgcPMF49353795;     AAIwoZHBMLYGITWgcPMF49353795 = AAIwoZHBMLYGITWgcPMF75597963;     AAIwoZHBMLYGITWgcPMF75597963 = AAIwoZHBMLYGITWgcPMF92728804;     AAIwoZHBMLYGITWgcPMF92728804 = AAIwoZHBMLYGITWgcPMF21607436;     AAIwoZHBMLYGITWgcPMF21607436 = AAIwoZHBMLYGITWgcPMF88970001;     AAIwoZHBMLYGITWgcPMF88970001 = AAIwoZHBMLYGITWgcPMF25623103;     AAIwoZHBMLYGITWgcPMF25623103 = AAIwoZHBMLYGITWgcPMF36195954;     AAIwoZHBMLYGITWgcPMF36195954 = AAIwoZHBMLYGITWgcPMF76222442;     AAIwoZHBMLYGITWgcPMF76222442 = AAIwoZHBMLYGITWgcPMF32786147;     AAIwoZHBMLYGITWgcPMF32786147 = AAIwoZHBMLYGITWgcPMF41577058;     AAIwoZHBMLYGITWgcPMF41577058 = AAIwoZHBMLYGITWgcPMF98882989;     AAIwoZHBMLYGITWgcPMF98882989 = AAIwoZHBMLYGITWgcPMF3617301;     AAIwoZHBMLYGITWgcPMF3617301 = AAIwoZHBMLYGITWgcPMF54207815;     AAIwoZHBMLYGITWgcPMF54207815 = AAIwoZHBMLYGITWgcPMF14411577;     AAIwoZHBMLYGITWgcPMF14411577 = AAIwoZHBMLYGITWgcPMF69759540;     AAIwoZHBMLYGITWgcPMF69759540 = AAIwoZHBMLYGITWgcPMF11809986;     AAIwoZHBMLYGITWgcPMF11809986 = AAIwoZHBMLYGITWgcPMF39422025;     AAIwoZHBMLYGITWgcPMF39422025 = AAIwoZHBMLYGITWgcPMF88447575;     AAIwoZHBMLYGITWgcPMF88447575 = AAIwoZHBMLYGITWgcPMF24175036;     AAIwoZHBMLYGITWgcPMF24175036 = AAIwoZHBMLYGITWgcPMF23577468;     AAIwoZHBMLYGITWgcPMF23577468 = AAIwoZHBMLYGITWgcPMF11460713;     AAIwoZHBMLYGITWgcPMF11460713 = AAIwoZHBMLYGITWgcPMF43378105;     AAIwoZHBMLYGITWgcPMF43378105 = AAIwoZHBMLYGITWgcPMF31405807;     AAIwoZHBMLYGITWgcPMF31405807 = AAIwoZHBMLYGITWgcPMF76162345;     AAIwoZHBMLYGITWgcPMF76162345 = AAIwoZHBMLYGITWgcPMF27925980;     AAIwoZHBMLYGITWgcPMF27925980 = AAIwoZHBMLYGITWgcPMF57789131;     AAIwoZHBMLYGITWgcPMF57789131 = AAIwoZHBMLYGITWgcPMF44836845;     AAIwoZHBMLYGITWgcPMF44836845 = AAIwoZHBMLYGITWgcPMF52716233;     AAIwoZHBMLYGITWgcPMF52716233 = AAIwoZHBMLYGITWgcPMF65753277;     AAIwoZHBMLYGITWgcPMF65753277 = AAIwoZHBMLYGITWgcPMF83659663;     AAIwoZHBMLYGITWgcPMF83659663 = AAIwoZHBMLYGITWgcPMF84992772;     AAIwoZHBMLYGITWgcPMF84992772 = AAIwoZHBMLYGITWgcPMF4127891;     AAIwoZHBMLYGITWgcPMF4127891 = AAIwoZHBMLYGITWgcPMF20953413;     AAIwoZHBMLYGITWgcPMF20953413 = AAIwoZHBMLYGITWgcPMF24792329;     AAIwoZHBMLYGITWgcPMF24792329 = AAIwoZHBMLYGITWgcPMF62305861;     AAIwoZHBMLYGITWgcPMF62305861 = AAIwoZHBMLYGITWgcPMF30810613;     AAIwoZHBMLYGITWgcPMF30810613 = AAIwoZHBMLYGITWgcPMF85786801;     AAIwoZHBMLYGITWgcPMF85786801 = AAIwoZHBMLYGITWgcPMF17943505;     AAIwoZHBMLYGITWgcPMF17943505 = AAIwoZHBMLYGITWgcPMF9797163;     AAIwoZHBMLYGITWgcPMF9797163 = AAIwoZHBMLYGITWgcPMF91809889;     AAIwoZHBMLYGITWgcPMF91809889 = AAIwoZHBMLYGITWgcPMF50549829;     AAIwoZHBMLYGITWgcPMF50549829 = AAIwoZHBMLYGITWgcPMF25077765;     AAIwoZHBMLYGITWgcPMF25077765 = AAIwoZHBMLYGITWgcPMF42956042;     AAIwoZHBMLYGITWgcPMF42956042 = AAIwoZHBMLYGITWgcPMF16764790;     AAIwoZHBMLYGITWgcPMF16764790 = AAIwoZHBMLYGITWgcPMF49905223;     AAIwoZHBMLYGITWgcPMF49905223 = AAIwoZHBMLYGITWgcPMF83053107;     AAIwoZHBMLYGITWgcPMF83053107 = AAIwoZHBMLYGITWgcPMF82419540;     AAIwoZHBMLYGITWgcPMF82419540 = AAIwoZHBMLYGITWgcPMF60736593;     AAIwoZHBMLYGITWgcPMF60736593 = AAIwoZHBMLYGITWgcPMF8484745;     AAIwoZHBMLYGITWgcPMF8484745 = AAIwoZHBMLYGITWgcPMF5394114;     AAIwoZHBMLYGITWgcPMF5394114 = AAIwoZHBMLYGITWgcPMF10325891;     AAIwoZHBMLYGITWgcPMF10325891 = AAIwoZHBMLYGITWgcPMF9328468;     AAIwoZHBMLYGITWgcPMF9328468 = AAIwoZHBMLYGITWgcPMF59514906;     AAIwoZHBMLYGITWgcPMF59514906 = AAIwoZHBMLYGITWgcPMF29097074;     AAIwoZHBMLYGITWgcPMF29097074 = AAIwoZHBMLYGITWgcPMF73251720;     AAIwoZHBMLYGITWgcPMF73251720 = AAIwoZHBMLYGITWgcPMF5975691;     AAIwoZHBMLYGITWgcPMF5975691 = AAIwoZHBMLYGITWgcPMF44192157;     AAIwoZHBMLYGITWgcPMF44192157 = AAIwoZHBMLYGITWgcPMF16566459;     AAIwoZHBMLYGITWgcPMF16566459 = AAIwoZHBMLYGITWgcPMF93681456;     AAIwoZHBMLYGITWgcPMF93681456 = AAIwoZHBMLYGITWgcPMF31180871;     AAIwoZHBMLYGITWgcPMF31180871 = AAIwoZHBMLYGITWgcPMF80786258;     AAIwoZHBMLYGITWgcPMF80786258 = AAIwoZHBMLYGITWgcPMF83479721;     AAIwoZHBMLYGITWgcPMF83479721 = AAIwoZHBMLYGITWgcPMF10469166;     AAIwoZHBMLYGITWgcPMF10469166 = AAIwoZHBMLYGITWgcPMF49126484;     AAIwoZHBMLYGITWgcPMF49126484 = AAIwoZHBMLYGITWgcPMF56584285;     AAIwoZHBMLYGITWgcPMF56584285 = AAIwoZHBMLYGITWgcPMF94755098;     AAIwoZHBMLYGITWgcPMF94755098 = AAIwoZHBMLYGITWgcPMF82663887;     AAIwoZHBMLYGITWgcPMF82663887 = AAIwoZHBMLYGITWgcPMF29415487;     AAIwoZHBMLYGITWgcPMF29415487 = AAIwoZHBMLYGITWgcPMF52105716;     AAIwoZHBMLYGITWgcPMF52105716 = AAIwoZHBMLYGITWgcPMF38948928;     AAIwoZHBMLYGITWgcPMF38948928 = AAIwoZHBMLYGITWgcPMF26023184;     AAIwoZHBMLYGITWgcPMF26023184 = AAIwoZHBMLYGITWgcPMF21478521;     AAIwoZHBMLYGITWgcPMF21478521 = AAIwoZHBMLYGITWgcPMF78650412;     AAIwoZHBMLYGITWgcPMF78650412 = AAIwoZHBMLYGITWgcPMF32365146;     AAIwoZHBMLYGITWgcPMF32365146 = AAIwoZHBMLYGITWgcPMF73027639;     AAIwoZHBMLYGITWgcPMF73027639 = AAIwoZHBMLYGITWgcPMF86382947;     AAIwoZHBMLYGITWgcPMF86382947 = AAIwoZHBMLYGITWgcPMF422063;     AAIwoZHBMLYGITWgcPMF422063 = AAIwoZHBMLYGITWgcPMF14641018;     AAIwoZHBMLYGITWgcPMF14641018 = AAIwoZHBMLYGITWgcPMF26257123;     AAIwoZHBMLYGITWgcPMF26257123 = AAIwoZHBMLYGITWgcPMF44872873;     AAIwoZHBMLYGITWgcPMF44872873 = AAIwoZHBMLYGITWgcPMF75369591;     AAIwoZHBMLYGITWgcPMF75369591 = AAIwoZHBMLYGITWgcPMF84100251;     AAIwoZHBMLYGITWgcPMF84100251 = AAIwoZHBMLYGITWgcPMF44231488;     AAIwoZHBMLYGITWgcPMF44231488 = AAIwoZHBMLYGITWgcPMF60359164;     AAIwoZHBMLYGITWgcPMF60359164 = AAIwoZHBMLYGITWgcPMF73333772;     AAIwoZHBMLYGITWgcPMF73333772 = AAIwoZHBMLYGITWgcPMF75664305;     AAIwoZHBMLYGITWgcPMF75664305 = AAIwoZHBMLYGITWgcPMF44612984;     AAIwoZHBMLYGITWgcPMF44612984 = AAIwoZHBMLYGITWgcPMF91856338;     AAIwoZHBMLYGITWgcPMF91856338 = AAIwoZHBMLYGITWgcPMF51540609;     AAIwoZHBMLYGITWgcPMF51540609 = AAIwoZHBMLYGITWgcPMF56330170;     AAIwoZHBMLYGITWgcPMF56330170 = AAIwoZHBMLYGITWgcPMF86618455;     AAIwoZHBMLYGITWgcPMF86618455 = AAIwoZHBMLYGITWgcPMF69220342;     AAIwoZHBMLYGITWgcPMF69220342 = AAIwoZHBMLYGITWgcPMF24262049;     AAIwoZHBMLYGITWgcPMF24262049 = AAIwoZHBMLYGITWgcPMF78616291;     AAIwoZHBMLYGITWgcPMF78616291 = AAIwoZHBMLYGITWgcPMF11023632;     AAIwoZHBMLYGITWgcPMF11023632 = AAIwoZHBMLYGITWgcPMF67070108;     AAIwoZHBMLYGITWgcPMF67070108 = AAIwoZHBMLYGITWgcPMF14608600;     AAIwoZHBMLYGITWgcPMF14608600 = AAIwoZHBMLYGITWgcPMF93829557;     AAIwoZHBMLYGITWgcPMF93829557 = AAIwoZHBMLYGITWgcPMF52674542;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void ebNIkcZkGYFLpZpxRavG90099081() {     int wQgeepKAYxuHIlHUOlly86763332 = -667412859;    int wQgeepKAYxuHIlHUOlly40065213 = 17828574;    int wQgeepKAYxuHIlHUOlly20599373 = -458958893;    int wQgeepKAYxuHIlHUOlly35723368 = -800532696;    int wQgeepKAYxuHIlHUOlly46453602 = -581834406;    int wQgeepKAYxuHIlHUOlly54432994 = -78477583;    int wQgeepKAYxuHIlHUOlly96231838 = -845485512;    int wQgeepKAYxuHIlHUOlly65935260 = -983721014;    int wQgeepKAYxuHIlHUOlly48303764 = -516171796;    int wQgeepKAYxuHIlHUOlly36767419 = -845487631;    int wQgeepKAYxuHIlHUOlly22319970 = -996508733;    int wQgeepKAYxuHIlHUOlly49946641 = -159045705;    int wQgeepKAYxuHIlHUOlly45025599 = -48433685;    int wQgeepKAYxuHIlHUOlly67650020 = -863726136;    int wQgeepKAYxuHIlHUOlly15089920 = -400799290;    int wQgeepKAYxuHIlHUOlly72570354 = -287492956;    int wQgeepKAYxuHIlHUOlly71616808 = -706918019;    int wQgeepKAYxuHIlHUOlly53438930 = -503862021;    int wQgeepKAYxuHIlHUOlly65577583 = -563031955;    int wQgeepKAYxuHIlHUOlly70288732 = -793718623;    int wQgeepKAYxuHIlHUOlly60088586 = 42998088;    int wQgeepKAYxuHIlHUOlly69540164 = -746531399;    int wQgeepKAYxuHIlHUOlly7721718 = -502709857;    int wQgeepKAYxuHIlHUOlly30389933 = -351223187;    int wQgeepKAYxuHIlHUOlly50522326 = -867196116;    int wQgeepKAYxuHIlHUOlly32311165 = -489935530;    int wQgeepKAYxuHIlHUOlly46774231 = -448494691;    int wQgeepKAYxuHIlHUOlly42565583 = -626639950;    int wQgeepKAYxuHIlHUOlly61606034 = -111978568;    int wQgeepKAYxuHIlHUOlly55315234 = -433623732;    int wQgeepKAYxuHIlHUOlly34340248 = -162146799;    int wQgeepKAYxuHIlHUOlly74152945 = -370140813;    int wQgeepKAYxuHIlHUOlly2472019 = -928591710;    int wQgeepKAYxuHIlHUOlly64913950 = -722976946;    int wQgeepKAYxuHIlHUOlly12946739 = -531649545;    int wQgeepKAYxuHIlHUOlly17680167 = -261462480;    int wQgeepKAYxuHIlHUOlly8205149 = 30698239;    int wQgeepKAYxuHIlHUOlly53340502 = -902220472;    int wQgeepKAYxuHIlHUOlly16819772 = -766682082;    int wQgeepKAYxuHIlHUOlly46253562 = -235616635;    int wQgeepKAYxuHIlHUOlly36233118 = -234096985;    int wQgeepKAYxuHIlHUOlly44124670 = -429439804;    int wQgeepKAYxuHIlHUOlly52634384 = -115075961;    int wQgeepKAYxuHIlHUOlly51828613 = -770574370;    int wQgeepKAYxuHIlHUOlly26686522 = 49468009;    int wQgeepKAYxuHIlHUOlly77709817 = -902250906;    int wQgeepKAYxuHIlHUOlly72641520 = -653613523;    int wQgeepKAYxuHIlHUOlly3888837 = -386739803;    int wQgeepKAYxuHIlHUOlly15543144 = -469750026;    int wQgeepKAYxuHIlHUOlly2380260 = -187964250;    int wQgeepKAYxuHIlHUOlly2051072 = -384846499;    int wQgeepKAYxuHIlHUOlly50665929 = 41611897;    int wQgeepKAYxuHIlHUOlly20803929 = -780863650;    int wQgeepKAYxuHIlHUOlly78039996 = -662865518;    int wQgeepKAYxuHIlHUOlly30010495 = -79283622;    int wQgeepKAYxuHIlHUOlly17223169 = -920881460;    int wQgeepKAYxuHIlHUOlly32343495 = -479461570;    int wQgeepKAYxuHIlHUOlly90209439 = -7735707;    int wQgeepKAYxuHIlHUOlly85201042 = -933336580;    int wQgeepKAYxuHIlHUOlly14142437 = 8101124;    int wQgeepKAYxuHIlHUOlly7658763 = -629982892;    int wQgeepKAYxuHIlHUOlly53666256 = -118845562;    int wQgeepKAYxuHIlHUOlly4329227 = -771742447;    int wQgeepKAYxuHIlHUOlly92988529 = 17451935;    int wQgeepKAYxuHIlHUOlly2427172 = -583340833;    int wQgeepKAYxuHIlHUOlly48167025 = -526367921;    int wQgeepKAYxuHIlHUOlly47474622 = -230453995;    int wQgeepKAYxuHIlHUOlly80111649 = -325456739;    int wQgeepKAYxuHIlHUOlly54703281 = -232076591;    int wQgeepKAYxuHIlHUOlly97409752 = -39336810;    int wQgeepKAYxuHIlHUOlly64365206 = -218191196;    int wQgeepKAYxuHIlHUOlly18276307 = -804697547;    int wQgeepKAYxuHIlHUOlly36619158 = -737179940;    int wQgeepKAYxuHIlHUOlly19324021 = -227415321;    int wQgeepKAYxuHIlHUOlly34055614 = -459621639;    int wQgeepKAYxuHIlHUOlly15963917 = -527562108;    int wQgeepKAYxuHIlHUOlly16905781 = -531455439;    int wQgeepKAYxuHIlHUOlly55893104 = -732135488;    int wQgeepKAYxuHIlHUOlly3703411 = -300691196;    int wQgeepKAYxuHIlHUOlly72812509 = -964945211;    int wQgeepKAYxuHIlHUOlly59669645 = -836322008;    int wQgeepKAYxuHIlHUOlly42885395 = 38245111;    int wQgeepKAYxuHIlHUOlly27022440 = -56889924;    int wQgeepKAYxuHIlHUOlly59225774 = -924014319;    int wQgeepKAYxuHIlHUOlly53264162 = 51222767;    int wQgeepKAYxuHIlHUOlly83674318 = -103758696;    int wQgeepKAYxuHIlHUOlly53349017 = -589277163;    int wQgeepKAYxuHIlHUOlly24432022 = -165726192;    int wQgeepKAYxuHIlHUOlly34903455 = -543693324;    int wQgeepKAYxuHIlHUOlly95723570 = -610768086;    int wQgeepKAYxuHIlHUOlly85336672 = -782000911;    int wQgeepKAYxuHIlHUOlly17995709 = -961566054;    int wQgeepKAYxuHIlHUOlly68139460 = -968883893;    int wQgeepKAYxuHIlHUOlly2677335 = -674783206;    int wQgeepKAYxuHIlHUOlly38594800 = -605633743;    int wQgeepKAYxuHIlHUOlly82566862 = -15251423;    int wQgeepKAYxuHIlHUOlly39795443 = -657697358;    int wQgeepKAYxuHIlHUOlly59645854 = -32527897;    int wQgeepKAYxuHIlHUOlly49401442 = -87233538;    int wQgeepKAYxuHIlHUOlly78519497 = -667412859;     wQgeepKAYxuHIlHUOlly86763332 = wQgeepKAYxuHIlHUOlly40065213;     wQgeepKAYxuHIlHUOlly40065213 = wQgeepKAYxuHIlHUOlly20599373;     wQgeepKAYxuHIlHUOlly20599373 = wQgeepKAYxuHIlHUOlly35723368;     wQgeepKAYxuHIlHUOlly35723368 = wQgeepKAYxuHIlHUOlly46453602;     wQgeepKAYxuHIlHUOlly46453602 = wQgeepKAYxuHIlHUOlly54432994;     wQgeepKAYxuHIlHUOlly54432994 = wQgeepKAYxuHIlHUOlly96231838;     wQgeepKAYxuHIlHUOlly96231838 = wQgeepKAYxuHIlHUOlly65935260;     wQgeepKAYxuHIlHUOlly65935260 = wQgeepKAYxuHIlHUOlly48303764;     wQgeepKAYxuHIlHUOlly48303764 = wQgeepKAYxuHIlHUOlly36767419;     wQgeepKAYxuHIlHUOlly36767419 = wQgeepKAYxuHIlHUOlly22319970;     wQgeepKAYxuHIlHUOlly22319970 = wQgeepKAYxuHIlHUOlly49946641;     wQgeepKAYxuHIlHUOlly49946641 = wQgeepKAYxuHIlHUOlly45025599;     wQgeepKAYxuHIlHUOlly45025599 = wQgeepKAYxuHIlHUOlly67650020;     wQgeepKAYxuHIlHUOlly67650020 = wQgeepKAYxuHIlHUOlly15089920;     wQgeepKAYxuHIlHUOlly15089920 = wQgeepKAYxuHIlHUOlly72570354;     wQgeepKAYxuHIlHUOlly72570354 = wQgeepKAYxuHIlHUOlly71616808;     wQgeepKAYxuHIlHUOlly71616808 = wQgeepKAYxuHIlHUOlly53438930;     wQgeepKAYxuHIlHUOlly53438930 = wQgeepKAYxuHIlHUOlly65577583;     wQgeepKAYxuHIlHUOlly65577583 = wQgeepKAYxuHIlHUOlly70288732;     wQgeepKAYxuHIlHUOlly70288732 = wQgeepKAYxuHIlHUOlly60088586;     wQgeepKAYxuHIlHUOlly60088586 = wQgeepKAYxuHIlHUOlly69540164;     wQgeepKAYxuHIlHUOlly69540164 = wQgeepKAYxuHIlHUOlly7721718;     wQgeepKAYxuHIlHUOlly7721718 = wQgeepKAYxuHIlHUOlly30389933;     wQgeepKAYxuHIlHUOlly30389933 = wQgeepKAYxuHIlHUOlly50522326;     wQgeepKAYxuHIlHUOlly50522326 = wQgeepKAYxuHIlHUOlly32311165;     wQgeepKAYxuHIlHUOlly32311165 = wQgeepKAYxuHIlHUOlly46774231;     wQgeepKAYxuHIlHUOlly46774231 = wQgeepKAYxuHIlHUOlly42565583;     wQgeepKAYxuHIlHUOlly42565583 = wQgeepKAYxuHIlHUOlly61606034;     wQgeepKAYxuHIlHUOlly61606034 = wQgeepKAYxuHIlHUOlly55315234;     wQgeepKAYxuHIlHUOlly55315234 = wQgeepKAYxuHIlHUOlly34340248;     wQgeepKAYxuHIlHUOlly34340248 = wQgeepKAYxuHIlHUOlly74152945;     wQgeepKAYxuHIlHUOlly74152945 = wQgeepKAYxuHIlHUOlly2472019;     wQgeepKAYxuHIlHUOlly2472019 = wQgeepKAYxuHIlHUOlly64913950;     wQgeepKAYxuHIlHUOlly64913950 = wQgeepKAYxuHIlHUOlly12946739;     wQgeepKAYxuHIlHUOlly12946739 = wQgeepKAYxuHIlHUOlly17680167;     wQgeepKAYxuHIlHUOlly17680167 = wQgeepKAYxuHIlHUOlly8205149;     wQgeepKAYxuHIlHUOlly8205149 = wQgeepKAYxuHIlHUOlly53340502;     wQgeepKAYxuHIlHUOlly53340502 = wQgeepKAYxuHIlHUOlly16819772;     wQgeepKAYxuHIlHUOlly16819772 = wQgeepKAYxuHIlHUOlly46253562;     wQgeepKAYxuHIlHUOlly46253562 = wQgeepKAYxuHIlHUOlly36233118;     wQgeepKAYxuHIlHUOlly36233118 = wQgeepKAYxuHIlHUOlly44124670;     wQgeepKAYxuHIlHUOlly44124670 = wQgeepKAYxuHIlHUOlly52634384;     wQgeepKAYxuHIlHUOlly52634384 = wQgeepKAYxuHIlHUOlly51828613;     wQgeepKAYxuHIlHUOlly51828613 = wQgeepKAYxuHIlHUOlly26686522;     wQgeepKAYxuHIlHUOlly26686522 = wQgeepKAYxuHIlHUOlly77709817;     wQgeepKAYxuHIlHUOlly77709817 = wQgeepKAYxuHIlHUOlly72641520;     wQgeepKAYxuHIlHUOlly72641520 = wQgeepKAYxuHIlHUOlly3888837;     wQgeepKAYxuHIlHUOlly3888837 = wQgeepKAYxuHIlHUOlly15543144;     wQgeepKAYxuHIlHUOlly15543144 = wQgeepKAYxuHIlHUOlly2380260;     wQgeepKAYxuHIlHUOlly2380260 = wQgeepKAYxuHIlHUOlly2051072;     wQgeepKAYxuHIlHUOlly2051072 = wQgeepKAYxuHIlHUOlly50665929;     wQgeepKAYxuHIlHUOlly50665929 = wQgeepKAYxuHIlHUOlly20803929;     wQgeepKAYxuHIlHUOlly20803929 = wQgeepKAYxuHIlHUOlly78039996;     wQgeepKAYxuHIlHUOlly78039996 = wQgeepKAYxuHIlHUOlly30010495;     wQgeepKAYxuHIlHUOlly30010495 = wQgeepKAYxuHIlHUOlly17223169;     wQgeepKAYxuHIlHUOlly17223169 = wQgeepKAYxuHIlHUOlly32343495;     wQgeepKAYxuHIlHUOlly32343495 = wQgeepKAYxuHIlHUOlly90209439;     wQgeepKAYxuHIlHUOlly90209439 = wQgeepKAYxuHIlHUOlly85201042;     wQgeepKAYxuHIlHUOlly85201042 = wQgeepKAYxuHIlHUOlly14142437;     wQgeepKAYxuHIlHUOlly14142437 = wQgeepKAYxuHIlHUOlly7658763;     wQgeepKAYxuHIlHUOlly7658763 = wQgeepKAYxuHIlHUOlly53666256;     wQgeepKAYxuHIlHUOlly53666256 = wQgeepKAYxuHIlHUOlly4329227;     wQgeepKAYxuHIlHUOlly4329227 = wQgeepKAYxuHIlHUOlly92988529;     wQgeepKAYxuHIlHUOlly92988529 = wQgeepKAYxuHIlHUOlly2427172;     wQgeepKAYxuHIlHUOlly2427172 = wQgeepKAYxuHIlHUOlly48167025;     wQgeepKAYxuHIlHUOlly48167025 = wQgeepKAYxuHIlHUOlly47474622;     wQgeepKAYxuHIlHUOlly47474622 = wQgeepKAYxuHIlHUOlly80111649;     wQgeepKAYxuHIlHUOlly80111649 = wQgeepKAYxuHIlHUOlly54703281;     wQgeepKAYxuHIlHUOlly54703281 = wQgeepKAYxuHIlHUOlly97409752;     wQgeepKAYxuHIlHUOlly97409752 = wQgeepKAYxuHIlHUOlly64365206;     wQgeepKAYxuHIlHUOlly64365206 = wQgeepKAYxuHIlHUOlly18276307;     wQgeepKAYxuHIlHUOlly18276307 = wQgeepKAYxuHIlHUOlly36619158;     wQgeepKAYxuHIlHUOlly36619158 = wQgeepKAYxuHIlHUOlly19324021;     wQgeepKAYxuHIlHUOlly19324021 = wQgeepKAYxuHIlHUOlly34055614;     wQgeepKAYxuHIlHUOlly34055614 = wQgeepKAYxuHIlHUOlly15963917;     wQgeepKAYxuHIlHUOlly15963917 = wQgeepKAYxuHIlHUOlly16905781;     wQgeepKAYxuHIlHUOlly16905781 = wQgeepKAYxuHIlHUOlly55893104;     wQgeepKAYxuHIlHUOlly55893104 = wQgeepKAYxuHIlHUOlly3703411;     wQgeepKAYxuHIlHUOlly3703411 = wQgeepKAYxuHIlHUOlly72812509;     wQgeepKAYxuHIlHUOlly72812509 = wQgeepKAYxuHIlHUOlly59669645;     wQgeepKAYxuHIlHUOlly59669645 = wQgeepKAYxuHIlHUOlly42885395;     wQgeepKAYxuHIlHUOlly42885395 = wQgeepKAYxuHIlHUOlly27022440;     wQgeepKAYxuHIlHUOlly27022440 = wQgeepKAYxuHIlHUOlly59225774;     wQgeepKAYxuHIlHUOlly59225774 = wQgeepKAYxuHIlHUOlly53264162;     wQgeepKAYxuHIlHUOlly53264162 = wQgeepKAYxuHIlHUOlly83674318;     wQgeepKAYxuHIlHUOlly83674318 = wQgeepKAYxuHIlHUOlly53349017;     wQgeepKAYxuHIlHUOlly53349017 = wQgeepKAYxuHIlHUOlly24432022;     wQgeepKAYxuHIlHUOlly24432022 = wQgeepKAYxuHIlHUOlly34903455;     wQgeepKAYxuHIlHUOlly34903455 = wQgeepKAYxuHIlHUOlly95723570;     wQgeepKAYxuHIlHUOlly95723570 = wQgeepKAYxuHIlHUOlly85336672;     wQgeepKAYxuHIlHUOlly85336672 = wQgeepKAYxuHIlHUOlly17995709;     wQgeepKAYxuHIlHUOlly17995709 = wQgeepKAYxuHIlHUOlly68139460;     wQgeepKAYxuHIlHUOlly68139460 = wQgeepKAYxuHIlHUOlly2677335;     wQgeepKAYxuHIlHUOlly2677335 = wQgeepKAYxuHIlHUOlly38594800;     wQgeepKAYxuHIlHUOlly38594800 = wQgeepKAYxuHIlHUOlly82566862;     wQgeepKAYxuHIlHUOlly82566862 = wQgeepKAYxuHIlHUOlly39795443;     wQgeepKAYxuHIlHUOlly39795443 = wQgeepKAYxuHIlHUOlly59645854;     wQgeepKAYxuHIlHUOlly59645854 = wQgeepKAYxuHIlHUOlly49401442;     wQgeepKAYxuHIlHUOlly49401442 = wQgeepKAYxuHIlHUOlly78519497;     wQgeepKAYxuHIlHUOlly78519497 = wQgeepKAYxuHIlHUOlly86763332;}
// Junk Finished
