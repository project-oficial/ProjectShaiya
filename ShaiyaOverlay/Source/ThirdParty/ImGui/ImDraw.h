#pragma once
#include "imgui.h"
#include <cstdint>
#include "../core/structs.h"

namespace Draw
{
	void line_box(ImVec2 pos, ImVec2 dim, ImColor color, int thickness);
	auto String(const float font_size, const vec3 vec, const ImVec4 color, const bool b_center, const bool stroke, const char* text) -> void;
	void Stringf(float font_size, const vec3 vec, ImVec4 color, bool b_center, bool stroke, const char* p_text, ...);
	void box(const vec3 vec_start, const vec3 vec_end, ImVec4 color, float thickness);
	void line(const vec3 vec_start, const vec3 vec_end, ImVec4 color, float thickness = 1.0f);
	void circle(const vec3 vec_center, float radius, ImVec4 color, int num_seg, float thickness);
	void rect(const ImVec2& from, const ImVec2& to, const ImVec4& color, float rounding, uint32_t rounding_corners_flags, float thickness);
	void filled_box(const ImVec2& from, const ImVec2& to, const ImVec4& color);
	void box_ol(float x, float y, float w, float h, const ImVec4 color, bool outlined = true, float thickness = 0.4f);


	auto DrawCrossHair(const FLOAT aSize, ImU32 aColor) -> VOID;
	auto DrawLine(const ImVec2& aPoint1, const ImVec2 aPoint2, ImU32 aColor, const FLOAT aLineWidth) -> VOID;
	auto DrawBox(float x, float y, float w, float h, ImColor color) -> VOID;
	auto RectFilled(float x0, float y0, float x1, float y1, ImColor color, float rounding, int rounding_corners_flags) -> VOID;
	auto HealthBar(float x, float y, float w, float h, int phealth, bool Outlined, bool text = false) -> VOID;
	auto HealthBarHor(float x, float y, float w, float h, int phealth, bool Outlined, bool text = false) -> VOID;
	auto ShieldBar(float x, float y, float w, float h, int phealth, bool Outlined, bool text, int R, int G, int B) -> VOID;
	auto ShieldBarHor(float x, float y, float w, float h, int phealth, bool Outlined, bool text, int R, int G, int B) -> VOID;
	auto Draw3DCube(float X, float Y, float size, const ImU32& color, float thickness) -> VOID;
	auto DrawTriangle(float X, float Y, float W, float H, const ImU32& color, float thickness, bool filled) -> VOID;
	auto Box3D(uintptr_t CameraManager, FVector origin, FVector extends, ImU32 Color) -> VOID;
	auto Box3DMenu(FVector origin, FVector extends, ImU32 Color) -> VOID;
	auto Skeleton(std::vector<FVector>& boneList, ImU32 col, float thickness)->VOID;
	auto DrawCorneredBox(float X, float Y, float W, float H, const ImU32& color, float thickness, bool filled) -> VOID;

	auto DrawString(const ImVec2& aPos, const std::string& aString, ImU32 aColor) -> VOID;
	void DrawCircleFilled(int x, int y, int radius, ImColor color, int segments);
	void DrawCircle(int x, int y, int radius, ImColor color, int segments);
};
