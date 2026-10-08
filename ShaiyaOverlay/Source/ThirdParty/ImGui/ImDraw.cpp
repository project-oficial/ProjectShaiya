#include <windows.h>
#include "imgui.h"
#include "ImDraw.h"
#include <stdio.h>
#include <WinNls.h>
#include <string>
#include "imgui_internal.h"
#include "../../Core/Structs.h"
#include "../../Config/Settings.h"

auto Draw::String(const float font_size, const vec3 vec, const ImVec4 color, const bool b_center, const bool stroke, const char* text) -> void
{
	auto vList = ImGui::GetCurrentWindow()->DrawList;
	auto draw_pos = vec;
	if (b_center)
	{
		const auto text_size = ImGui::CalcTextSize(text);
		draw_pos.x = vec.x - (text_size.x / 2);
		draw_pos.y = vec.y - text_size.y;
	}
	if (stroke)
	{
		vList->AddText(ImGui::GetFont(), font_size, ImVec2(draw_pos.x + 1, draw_pos.y + 1), ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 1)), text);
		vList->AddText(ImGui::GetFont(), font_size, ImVec2(draw_pos.x - 1, draw_pos.y - 1), ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 1)), text);
		vList->AddText(ImGui::GetFont(), font_size, ImVec2(draw_pos.x + 1, draw_pos.y - 1), ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 1)), text);
		vList->AddText(ImGui::GetFont(), font_size, ImVec2(draw_pos.x - 1, draw_pos.y + 1), ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 1)), text);
	}
	vList->AddText(ImGui::GetFont(), font_size, ImVec2(draw_pos.x, draw_pos.y), ImGui::GetColorU32(color), text);
}

auto Draw::Stringf(const float font_size, const vec3 vec, const ImVec4 color, const bool b_center, const bool stroke, const char* p_text, ...) -> void
{
	va_list va_a_list;
	char buf[1024] = { 0 };
	va_start(va_a_list, p_text);
	_vsnprintf_s(buf, sizeof(buf), p_text, va_a_list);
	va_end(va_a_list);
	return String(font_size, vec, color, b_center, stroke, buf);
}

void Draw::line_box(ImVec2 pos, ImVec2 dim, ImColor color, int thickness)
{
	auto vList = ImGui::GetCurrentWindow()->DrawList;
	float th = static_cast<float>(thickness);
	vList->AddLine(pos, ImVec2(pos.x - dim.x, pos.y), color, th);
	vList->AddLine(ImVec2(pos.x - dim.x, pos.y), ImVec2(pos.x - dim.x, pos.y + dim.y), color, th);
	vList->AddLine(ImVec2(pos.x - dim.x, pos.y + dim.y), ImVec2(pos.x, pos.y + dim.y), color, th);
	vList->AddLine(ImVec2(pos.x, pos.y + dim.y), ImVec2(pos.x, pos.y), color, th);
}

//void Renderer::DrawLine( ImVec2 dst, ImVec2 src, ImColor col, int thickness ) 
//{
//	this->GetDrawList( )->AddLine( src, dst, col, thickness );
//}
//
//void Renderer::DrawText( ImVec2 pos, ImColor col, const char* text )
//{
//	this->GetDrawList( )->AddText( pos, col, text, 0 );
//}
//
//void Renderer::DrawHealthBar( ImVec2 pos, ImVec2 dim, ImColor col )
//{
//	this->GetDrawList( )->AddLine( pos, ImVec2( pos.x, pos.y - dim.y ), col, dim.x );
//}


auto Draw::box(const vec3 vec_start, const vec3 vec_end, const ImVec4 color, const float thickness) -> void
{
	auto vList = ImGui::GetCurrentWindow()->DrawList;
	vList->AddRect(ImVec2(vec_start.x, vec_start.y), ImVec2(vec_start.x + vec_end.x, vec_start.y + vec_end.y), ImGui::GetColorU32(color), 0, 0, thickness);
}

auto Draw::line(const vec3 vec_start, const vec3 vec_end, const ImVec4 color, const float thickness) -> void
{
	auto vList = ImGui::GetCurrentWindow()->DrawList;
	vList->AddLine(ImVec2(vec_start.x, vec_start.y), ImVec2(vec_end.x, vec_end.y), ImGui::GetColorU32(color), thickness);
}

auto Draw::circle(const vec3 vec_center, const float radius, const ImVec4 color, const int num_seg, const float thickness) -> void
{
	auto vList = ImGui::GetCurrentWindow()->DrawList;
	vList->AddCircle(ImVec2(vec_center.x, vec_center.y), radius, ImGui::GetColorU32(color), num_seg, thickness);
}

auto Draw::rect(const ImVec2& from, const ImVec2& to, const ImVec4& color, const float rounding, const uint32_t rounding_corners_flags, const float thickness) -> void
{
	auto vList = ImGui::GetCurrentWindow()->DrawList;
	vList->AddRect(from, to, ImGui::GetColorU32(color), rounding, rounding_corners_flags, thickness);
}

auto Draw::filled_box(const ImVec2& from, const ImVec2& to, const ImVec4& color) -> void
{
	auto vList = ImGui::GetCurrentWindow()->DrawList;
	vList->AddRectFilled(from, to, ImGui::GetColorU32(color));
}

void Draw::box_ol(float x, float y, float w, float h, const ImVec4 color, bool outlined, float thickness)
{
	auto vList = ImGui::GetCurrentWindow()->DrawList;
	if (outlined)
	{
		auto outline_color = ImGui::GetColorU32({ 0, 0, 0, 1 });
		const auto b_x = x, b_y = y;
		x += 1;
		y += 1;
		vList->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), outline_color, 0.0f, 15, thickness);
		x -= 2;
		vList->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), outline_color, 0.0f, 15, thickness);
		x += 2;
		y -= 2;
		vList->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), outline_color, 0.0f, 15, thickness);
		x -= 2;
		vList->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), outline_color, 0.0f, 15, thickness);
		x = b_x;
		y = b_y;
	}
	vList->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), ImGui::GetColorU32(color), 0.0f, 15, thickness);
}

void DrawLine(int x1, int y1, int x2, int y2, float* color, int thickness)
{
	auto vList = ImGui::GetCurrentWindow()->DrawList;
	vList->AddLine(ImVec2(static_cast<float>(x1), static_cast<float>(y1)), ImVec2(static_cast<float>(x2), static_cast<float>(y2)), ImGui::ColorConvertFloat4ToU32(reinterpret_cast<ImVec4&>(color)), static_cast<float>(thickness));
}

//std::string string_To_UTF8( const std::string& str )
//{
//	const auto nw_len = ::MultiByteToWideChar( CP_ACP, 0, str.c_str( ), -1, nullptr, 0 );
//
//	auto* pw_buf = new wchar_t[ nw_len + 1 ];
//	ZeroMemory( pw_buf, nw_len * 2 + 2 );
//
//	::MultiByteToWideChar( CP_ACP, 0, str.c_str( ), str.length( ), pw_buf, nw_len );
//
//	const auto n_len = ::WideCharToMultiByte( CP_UTF8, 0, pw_buf, -1, nullptr, 0, nullptr, nullptr );
//
//	auto* p_buf = new char[ n_len + 1 ];
//	ZeroMemory( p_buf, n_len + 1 );
//
//	::WideCharToMultiByte( CP_UTF8, 0, pw_buf, nw_len, p_buf, n_len, nullptr, nullptr );
//
//	std::string ret_str( p_buf );
//
//	delete[ ]pw_buf;
//	delete[ ]p_buf;
//
//	pw_buf = nullptr;
//	p_buf = nullptr;
//
//	return ret_str;
//}
//
//void DrawNewText( int x, int y, float* color, const char* str )
//{
//	ImFont a;
//	const auto utf_8_1 = std::string( str );
//	const auto utf_8_2 = string_To_UTF8( utf_8_1 );
//	vList->AddText( ImVec2( x, y ), ImGui::ColorConvertFloat4ToU32( reinterpret_cast<ImVec4&>( color ) ), utf_8_2.c_str( ) );
//}



auto Draw::DrawCrossHair(const FLOAT aSize, ImU32 aColor)-> VOID
{
	auto vList = ImGui::GetCurrentWindow()->DrawList;
	auto display = ImGui::GetIO().DisplaySize;

	auto ScreenCenterX = display.x / 2;
	auto ScreenCenterY = display.y / 2;


	vList->AddLine({ ScreenCenterX, ScreenCenterY - (aSize + 1) }, { ScreenCenterX, ScreenCenterY + (aSize + 1) }, aColor, 2);
	vList->AddLine({ ScreenCenterX - (aSize + 1), ScreenCenterY }, { ScreenCenterX + (aSize + 1), ScreenCenterY }, aColor, 2);
}

auto Draw::DrawLine(const ImVec2& aPoint1, const ImVec2 aPoint2, ImU32 aColor, const FLOAT aLineWidth) -> VOID
{
	auto vList = ImGui::GetCurrentWindow()->DrawList;
	vList->AddLine(aPoint1, aPoint2, aColor, aLineWidth);
}


auto Draw::DrawBox(float x, float y, float w, float h, ImColor color)-> VOID
{
	DrawLine(ImVec2(x, y), ImVec2(x + w, y), color, 1.3f); // top 
	DrawLine(ImVec2(x, y - 1.3f), ImVec2(x, y + h + 1.4f), color, 1.3f); // left
	DrawLine(ImVec2(x + w, y - 1.3f), ImVec2(x + w, y + h + 1.4f), color, 1.3f);  // right
	DrawLine(ImVec2(x, y + h), ImVec2(x + w, y + h), color, 1.3f);   // bottom 
}

auto Draw::RectFilled(float x0, float y0, float x1, float y1, ImColor color, float rounding, int rounding_corners_flags)-> VOID
{
	auto vList = ImGui::GetCurrentWindow()->DrawList;
	vList->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), color, rounding, rounding_corners_flags);
}

#define max(a,b)            (((a) > (b)) ? (a) : (b))
#define min(a,b)            (((a) < (b)) ? (a) : (b))

auto Draw::HealthBar(float x, float y, float w, float h, int phealth, bool Outlined, bool text) -> VOID
{
	auto vList = ImGui::GetCurrentWindow()->DrawList;

	int healthValue = max(0, min(phealth, 100));

	ImColor barColor = ImColor(
		min(510 * (100 - healthValue) / 100, 255), min(510 * healthValue / 100, 255),
		25,
		255
	);

	// Add shadow to the bar
	ImVec2 shadow_offset(2.0f, 2.0f);
	ImColor shadow_color(0, 0, 0, 128);

	if (Outlined)
		vList->AddRect(ImVec2(x - 1, y - 1), ImVec2(x + w + 1, y + h + 1), shadow_color, 0.0f, 0, 1.0f);

	RectFilled(x + shadow_offset.x, y + shadow_offset.y + (h - (int)((h / 100.0f) * (float)phealth)),
		x + w + shadow_offset.x, y + h + shadow_offset.y, shadow_color, 0.0f, 0);

	// Draw the actual bar
	RectFilled(x, y + (h - (int)((h / 100.0f) * (float)phealth)), x + w, y + h, barColor, 0.0f, 0);

	if (text)
	{
		auto l = ((healthValue == 100) ? 20.f : (healthValue <= 9) ? 9.f : 15.f);
		Stringf(11.f, { x - l, y + (h - (int)((h / 100.0f) * (float)phealth)) }, { 1.f, 1.f, 1.f, 1.f }, false, true, "%d", healthValue);
	}
}

auto Draw::HealthBarHor(float x, float y, float w, float h, int phealth, bool Outlined, bool text) -> VOID
{
	auto vList = ImGui::GetCurrentWindow()->DrawList;

	int healthValue = max(0, min(phealth, 100));

	ImColor barColor = ImColor(
		min(510 * (100 - healthValue) / 100, 255), min(510 * healthValue / 100, 255),
		25,
		255
	);

	// Add shadow to the bar
	ImVec2 shadow_offset(2.0f, 2.0f);
	ImColor shadow_color(0, 0, 0, 128);

	if (Outlined)
		vList->AddRect(ImVec2(x - 1, y - 1), ImVec2(x + w + 1, y + 3.0f + 1), shadow_color, 0.0f, 0, 1.0f);

	RectFilled(x + shadow_offset.x, y + shadow_offset.y,
		x + w + shadow_offset.x, y + 3.0f + shadow_offset.y,
		shadow_color, 0.0f, 0);

	// Draw the actual bar
	RectFilled(x + (w - (int)((w / 100.0f) * (float)phealth)), y,
		x + w, y + 3.0f,
		barColor,
		0.0f,
		0);

	if (text)
		Stringf(11.0f, { x + (w - (int)((w / 100.0f) * (float)phealth)), y - 12.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, false, true, "%d", healthValue);
}

auto Draw::ShieldBar(float x, float y, float w, float h, int phealth, bool Outlined, bool text, int R, int G, int B) -> VOID
{
	auto vList = ImGui::GetCurrentWindow()->DrawList;

	int healthValue = max(0, min(phealth, 100));

	auto v = (510 * healthValue / 100);
	ImColor barColor = ImColor(
		min(v * 2, R),
		min(v * 2, G),
		min(v, B),
		255
	);

	// Add shadow to the bar
	ImVec2 shadow_offset(2.0f, 2.0f);
	ImColor shadow_color(0, 0, 0, 128);

	if (Outlined)
		vList->AddRect(ImVec2(x - 1, y - 1), ImVec2(x + w + 1, y + h + 1), shadow_color, 0.0f, 0, 1.0f);

	RectFilled(x + shadow_offset.x, y + shadow_offset.y,
		x + w + shadow_offset.x, y + h + shadow_offset.y,
		shadow_color, 0.0f, 0);

	// Draw the actual bar
	RectFilled(x, y + (h - (int)((h / 100.0f) * (float)phealth)), x + w, y + h, barColor, 0.0f, 0);

	if (text)
		Stringf(11.0f, { x + 7.0f, y + (h - (int)((h / 100.0f) * (float)phealth)) }, { 1.0f, 1.0f, 1.0f, 1.0f }, false, true, "%d", healthValue);
}

auto Draw::ShieldBarHor(float x, float y, float w, float h, int phealth, bool Outlined, bool text, int R, int G, int B) -> VOID
{
	auto vList = ImGui::GetCurrentWindow()->DrawList;

	int healthValue = max(0, min(phealth, 100));

	auto v = (510 * healthValue / 100);
	ImColor barColor = ImColor(
		min(v * 2, R),
		min(v * 2, G),
		min(v, B),
		255
	);

	// Add shadow to the bar
	ImVec2 shadow_offset(2.0f, 2.0f);
	ImColor shadow_color(0, 0, 0, 128);

	if (Outlined)
		vList->AddRect(ImVec2(x - 1, y - 1), ImVec2(x + w + 1, y + 3.f + 1), shadow_color, 0.0f, 0, 1.0f);

	RectFilled(x + shadow_offset.x, y + shadow_offset.y,
		x + w + shadow_offset.x, y + 3.f + shadow_offset.y,
		shadow_color, 0.0f, 0);

	// Draw the actual bar
	RectFilled(x + (w - (int)((w / 100.0f) * (float)phealth)), y,
		x + w, y + 3.f,
		barColor, 0.0f, 0);

	if (text)
		Stringf(11.0f, { x + (w - (int)((w / 100.0f) * (float)phealth)), y + 6.f }, { 1.0f, 1.0f, 1.0f, 1.0f }, false, true, "%d", healthValue);
}

auto Draw::Draw3DCube(float X, float Y, float size, const ImU32& color, float thickness) -> VOID
{
	float zOffset = 100.0f;

	// Pontos do cubo
	ImVec2 p0(X - size / 2, Y - size / 2);
	ImVec2 p1(X + size / 2, Y - size / 2);
	ImVec2 p2(X + size / 2, Y + size / 2);
	ImVec2 p3(X - size / 2, Y + size / 2);
	ImVec2 p4(X - size / 2 + zOffset, Y - size / 2 + zOffset);
	ImVec2 p5(X + size / 2 + zOffset, Y - size / 2 + zOffset);
	ImVec2 p6(X + size / 2 + zOffset, Y + size / 2 + zOffset);
	ImVec2 p7(X - size / 2 + zOffset, Y + size / 2 + zOffset);

	// Desenhar as linhas do cubo
	DrawLine(p0, p1, color, thickness);
	DrawLine(p1, p2, color, thickness);
	DrawLine(p2, p3, color, thickness);
	DrawLine(p3, p0, color, thickness);

	DrawLine(p4, p5, color, thickness);
	DrawLine(p5, p6, color, thickness);
	DrawLine(p6, p7, color, thickness);
	DrawLine(p7, p4, color, thickness);

	DrawLine(p0, p4, color, thickness);
	DrawLine(p1, p5, color, thickness);
	DrawLine(p2, p6, color, thickness);
	DrawLine(p3, p7, color, thickness);

	// Adicione as linhas adicionais para a perspectiva do cubo
	DrawLine(p0, p3, color, thickness);
	DrawLine(p1, p2, color, thickness);
	DrawLine(p4, p7, color, thickness);
	DrawLine(p5, p6, color, thickness);
}
void Line(FVector2D origin, FVector2D dest, ImU32 Color)
{
	if ((origin.x == 0.f && origin.y == 0.f) || (dest.x == 0.f && dest.y == 0.f)) return;

	ImGui::GetCurrentWindow()->DrawList->AddLine(ImVec2(static_cast<float>(origin.x), static_cast<float>(origin.y)), ImVec2(static_cast<float>(dest.x), static_cast<float>(dest.y)), Color, 1.0f);
}

auto Draw::DrawTriangle(float X, float Y, float W, float H, const ImU32& color, float thickness, bool filled) -> VOID
{
	auto vList = ImGui::GetCurrentWindow()->DrawList;

	// Define os v�rtices do tri�ngulo
	ImVec2 topPoint = ImVec2(X + W / 2, Y);             // Ponto superior
	ImVec2 bottomLeft = ImVec2(X, Y + H);               // Ponto inferior esquerdo
	ImVec2 bottomRight = ImVec2(X + W, Y + H);          // Ponto inferior direito

	// Cor principal
	ImU32 col = ImGui::GetColorU32(color);

	// Desenha o tri�ngulo preenchido, se necess�rio
	if (filled)
	{
		ImU32 shadowCol = IM_COL32(0, 0, 0, 128);
		vList->AddTriangleFilled(topPoint, bottomLeft, bottomRight, shadowCol);
	}

	// Desenha as bordas do tri�ngulo
	vList->AddTriangle(topPoint, bottomLeft, bottomRight, col, thickness);
}


auto Draw::Box3DMenu(FVector origin, FVector extends, ImU32 Color) -> VOID
{
	// Aplicando deslocamento para centralizar a origem
	origin -= extends / 2.f;

	// C�lculo dos 8 v�rtices da caixa 3D
	FVector one = origin;
	FVector two = origin; two.x += extends.x;
	FVector three = origin; three.x += extends.x; three.y += extends.y;
	FVector four = origin; four.y += extends.y;

	FVector five = one; five.z += extends.z;
	FVector six = two; six.z += extends.z;
	FVector seven = three; seven.z += extends.z;
	FVector eight = four; eight.z += extends.z;

	// Proje��o pseudo-3D: Ajusta Z para deslocar os v�rtices no espa�o 2D
	auto project = [](FVector vertex) -> FVector {
		const float zFactor = 0.5f; // Fator de compress�o no eixo Z
		return FVector(vertex.x + (vertex.z * zFactor), vertex.y - (vertex.z * zFactor), 0);
		};

	one = project(one);
	two = project(two);
	three = project(three);
	four = project(four);
	five = project(five);
	six = project(six);
	seven = project(seven);
	eight = project(eight);

	Line(FVector2D{ one.x,one.y }, FVector2D{ two.x,two.y }, Color);
	Line(FVector2D{ two.x,two.y }, FVector2D{ three.x,three.y }, Color);
	Line(FVector2D{ three.x,three.y }, FVector2D{ four.x,four.y }, Color);
	Line(FVector2D{ four.x,four.y }, FVector2D{ one.x,one.y }, Color);

	Line(FVector2D{ five.x,five.y }, FVector2D{ six.x,six.y }, Color);
	Line(FVector2D{ six.x,six.y }, FVector2D{ seven.x,seven.y }, Color);
	Line(FVector2D{ seven.x,seven.y }, FVector2D{ eight.x,eight.y }, Color);
	Line(FVector2D{ eight.x,eight.y }, FVector2D{ five.x,five.y }, Color);

	Line(FVector2D{ one.x,one.y }, FVector2D{ five.x,five.y }, Color);
	Line(FVector2D{ two.x,two.y }, FVector2D{ six.x,six.y }, Color);
	Line(FVector2D{ three.x,three.y }, FVector2D{ seven.x,seven.y }, Color);
	Line(FVector2D{ four.x,four.y }, FVector2D{ eight.x,eight.y }, Color);

}
auto Draw::Skeleton(std::vector<FVector>& boneList, ImU32 col,float thickness) -> VOID
{
	if (boneList.size() < 16)
		return;

	auto vDList = ImGui::GetCurrentWindow()->DrawList;

	auto DrawShadowLine = [&](const FVector& from, const FVector& to)
		{
			if (from.x <= 0.0f || from.y <= 0.0f || to.x <= 0.0f || to.y <= 0.0f)
				return;

			ImVec2 a = ImVec2(static_cast<float>(from.x), static_cast<float>(from.y));
			ImVec2 b = ImVec2(static_cast<float>(to.x), static_cast<float>(to.y));

			// Linha do esqueleto
			if (ProjectARCR::Config::Settings::GetInstance().Visuals.Skeleton)
				vDList->AddLine(a, b, col, thickness);
		};

	// Corpo
	DrawShadowLine(boneList[0], boneList[8]);
	DrawShadowLine(boneList[8], boneList[9]);

	// Bra�o direito
	DrawShadowLine(boneList[0], boneList[1]);
	DrawShadowLine(boneList[1], boneList[2]);
	DrawShadowLine(boneList[2], boneList[3]);

	// Bra�o esquerdo
	DrawShadowLine(boneList[0], boneList[4]);
	DrawShadowLine(boneList[4], boneList[5]);
	DrawShadowLine(boneList[5], boneList[6]);

	// Perna direita
	DrawShadowLine(boneList[9], boneList[10]);
	DrawShadowLine(boneList[10], boneList[11]);
	DrawShadowLine(boneList[11], boneList[12]);

	// Perna esquerda
	DrawShadowLine(boneList[9], boneList[13]);
	DrawShadowLine(boneList[13], boneList[14]);
	DrawShadowLine(boneList[14], boneList[15]);
}

auto Draw::DrawCorneredBox(float X, float Y, float W, float H, const ImU32& color, float thickness, bool filled) -> VOID
{
	auto vList = ImGui::GetCurrentWindow()->DrawList;

	// Configura��es da sombra
	float shadowOffset = 1.0f;
	ImU32 shadowCol = IM_COL32(0, 0, 0, 128); // Cor da sombra (preto com 50% de transpar�ncia)

	// Cor da linha principal
	ImU32 col = ImGui::GetColorU32(color);

	if (filled)
	{
		// Preenchimento no centro (preto transparente)
		ImU32 fillCol = IM_COL32(0, 0, 0, 150); // Preto com 50% de transpar�ncia
		vList->AddRectFilled(ImVec2(X, Y), ImVec2(X + W, Y + H), fillCol);
	}
	// Desenhar sombra nos cantos
	float lineW = (W);
	float lineH = (H);
	float shadowThickness = thickness + shadowOffset;

	vList->AddLine(ImVec2(X, Y - shadowThickness / 2), ImVec2(X, Y + lineH), shadowCol, shadowThickness); // Topo esquerdo
	vList->AddLine(ImVec2(X - shadowThickness / 2, Y), ImVec2(X + lineW, Y), shadowCol, shadowThickness);

	vList->AddLine(ImVec2(X + W - lineW, Y), ImVec2(X + W + shadowThickness / 2, Y), shadowCol, shadowThickness); // Topo direito horizontal
	vList->AddLine(ImVec2(X + W, Y - shadowThickness / 2), ImVec2(X + W, Y + lineH), shadowCol, shadowThickness);

	vList->AddLine(ImVec2(X, Y + H - lineH), ImVec2(X, Y + H + shadowThickness / 2), shadowCol, shadowThickness); // Inferior esquerdo
	vList->AddLine(ImVec2(X - shadowThickness / 2, Y + H), ImVec2(X + lineW, Y + H), shadowCol, shadowThickness);

	vList->AddLine(ImVec2(X + W - lineW, Y + H), ImVec2(X + W + shadowThickness / 2, Y + H), shadowCol, shadowThickness); // Inferior direito
	vList->AddLine(ImVec2(X + W, Y + H - lineH), ImVec2(X + W, Y + H + shadowThickness / 2), shadowCol, shadowThickness);

	// Desenhar linha principal
	vList->AddLine(ImVec2(X, Y - thickness / 2), ImVec2(X, Y + lineH), col, thickness); // Topo esquerdo
	vList->AddLine(ImVec2(X - thickness / 2, Y), ImVec2(X + lineW, Y), col, thickness);

	vList->AddLine(ImVec2(X + W - lineW, Y), ImVec2(X + W + thickness / 2, Y), col, thickness); // Topo direito horizontal
	vList->AddLine(ImVec2(X + W, Y - thickness / 2), ImVec2(X + W, Y + lineH), col, thickness);

	vList->AddLine(ImVec2(X, Y + H - lineH), ImVec2(X, Y + H + thickness / 2), col, thickness); // Inferior esquerdo
	vList->AddLine(ImVec2(X - thickness / 2, Y + H), ImVec2(X + lineW, Y + H), col, thickness);

	vList->AddLine(ImVec2(X + W - lineW, Y + H), ImVec2(X + W + thickness / 2, Y + H), col, thickness); // Bottom right
	vList->AddLine(ImVec2(X + W, Y + H - lineH), ImVec2(X + W, Y + H + (thickness / 2)), col, thickness);
}

auto Draw::DrawString(const ImVec2& aPos, const std::string& aString, ImU32 aColor) -> VOID
{
	auto vList = ImGui::GetCurrentWindow()->DrawList;
	vList->AddText(aPos, aColor, aString.data());
}

void Draw::DrawCircleFilled(int x, int y, int radius, ImColor color, int segments)
{
	auto vList = ImGui::GetCurrentWindow()->DrawList;
	vList->AddCircleFilled(ImVec2(static_cast<float>(x), static_cast<float>(y)), static_cast<float>(radius), ImGui::ColorConvertFloat4ToU32(color), segments);
}

void Draw::DrawCircle(int x, int y, int radius, ImColor color, int segments)
{
	auto vList = ImGui::GetCurrentWindow()->DrawList;
	vList->AddCircle(ImVec2(static_cast<float>(x), static_cast<float>(y)), static_cast<float>(radius), ImGui::ColorConvertFloat4ToU32(color), segments);
}