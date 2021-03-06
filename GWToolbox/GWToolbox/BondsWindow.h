#pragma once

#include <OSHGui\OSHGui.hpp>

#include "ToolboxWindow.h"


class BondsWindow : public ToolboxWindow {
	static const int MAX_PLAYERS = 12;
	static const int MAX_BONDS = 3;

	class BondsMonitor : public DragButton {
		enum class Bond { Balth, Life, Prot };

		int GetBond(int xcoord);
		int GetPlayer(int ycoord);

		int img_size_;
		bool freezed;
		int hovered_player;
		int hovered_bond;
		bool pressed;
		int party_size;
		// allocate twice as much memory because something was overwriting 
		// the end of it. Maybe now it'll overwrite useless stuff.
		OSHGui::PictureBox* pics[MAX_PLAYERS * 2][MAX_BONDS];
		int buff_id[MAX_PLAYERS * 2][MAX_BONDS];
		bool show[MAX_PLAYERS * 2][MAX_BONDS];

		void DropUseBuff(int bond, int player);

	protected:
		virtual void Render(OSHGui::Drawing::RenderContext& context);
		virtual void PopulateGeometry() override;
		virtual void OnMouseDown(const OSHGui::MouseMessage &mouse) override;
		virtual void OnMouseMove(const OSHGui::MouseMessage &mouse) override;
		virtual void OnMouseUp(const OSHGui::MouseMessage &mouse) override;
		virtual void OnMouseLeave(const OSHGui::MouseMessage &mouse) override;
	public:
		BondsMonitor(OSHGui::Control* parent, int img_size);
		void SaveLocation();
		inline void SetFreeze(bool b) { freezed = b; }

		// Update. Will always be called every frame.
		void Main() {};

		// Draw user interface. Will be called every frame if the element is visible
		void Draw();
	};

public:
	BondsWindow();

	inline static const wchar_t* IniSection() { return L"bonds"; }
	inline static const wchar_t* IniKeyX() { return L"x"; }
	inline static const wchar_t* IniKeyY() { return L"y"; }
	inline static const wchar_t* IniKeyShow() { return L"show"; }
	inline static const char* ThemeKey() { return "bonds"; }

	// Update. Will always be called every frame.
	void Main() override {};

	// Draw user interface. Will be called every frame if the element is visible
	void Draw() override { if (monitor) monitor->Draw(); };

	inline void SetFreze(bool b) { monitor->SetFreeze(b); }
	void SetTransparentBackColor(bool b);

private:
	BondsMonitor* monitor;
};
