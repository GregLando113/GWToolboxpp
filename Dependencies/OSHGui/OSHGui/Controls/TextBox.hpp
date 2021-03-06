/*
 * OldSchoolHack GUI
 *
 * by KN4CK3R http://www.oldschoolhack.me
 *
 * See license in OSHGui.hpp
 */

#ifndef OSHGUI_TEXTBOX_HPP
#define OSHGUI_TEXTBOX_HPP

#include "Control.hpp"
#include "../Misc/TextHelper.hpp"

namespace OSHGui {
	/**
	 * Tritt ein, wenn sich der Wert der Text-Eigenschaft �ndert.
	 */
	typedef Event<void(Control*)> TextChangedEvent;
	typedef EventHandler<void(Control*)> TextChangedEventHandler;

	/**
	 * Stellt ein Textfeld-Steuerelement dar.
	 */
	class OSHGUI_EXPORT TextBox : public Control {
	public:
		using Control::SetSize;

		/**
		 * Konstruktor der Klasse.
		 */
		TextBox(Control* parent);
		
		/**
		 * Legt die H�he und Breite des Steuerelements fest.
		 *
		 * \param size
		 */
		virtual void SetSize(const Drawing::SizeI &size) override;
		/**
		 * Legt die Schriftart des Texts im Steuerelement fest.
		 *
		 * \param font
		 */
		virtual void SetFont(const Drawing::FontPtr &font) override;
		/**
		 * Legt den Text fest.
		 *
		 * \param text
		 */
		virtual void SetText(const Misc::UnicodeString &text);
		/**
		 * Ruft den Text ab.
		 *
		 * \return der Text
		 */
		virtual const Misc::UnicodeString& GetText() const;
		/**
		 * Legt das Zeichen fest, das bei der Kennworteingabe angezeigt wird.
		 *
		 * \param passwordChar
		 */
		virtual void SetPasswordChar(const Misc::UnicodeChar &passwordChar);
		/**
		 * Ruft das Zeichen ab, das bei der Kennworteingabe angezeigt wird.
		 */
		virtual const Misc::UnicodeChar& GetPasswordChar() const;
		/**
		 * Ruft das TextChangedEvent f�r das Steuerelement ab.
		 *
		 * \return textChangedEvent
		 */
		TextChangedEvent& GetTextChangedEvent();

		virtual void ShowCaret(bool showCaret);

		virtual void CalculateAbsoluteLocation() override;
	
	protected:
		virtual void InjectTime(const Misc::DateTime &time) override;

		virtual void PopulateGeometry() override;

		virtual void OnMouseDown(const MouseMessage &mouse) override;
		virtual bool OnKeyPress(const KeyboardMessage &keyboard) override;
		virtual bool OnKeyDown(const KeyboardMessage &keyboard) override;
		virtual void OnTextChanged();

	private:
		static const Drawing::SizeI DefaultSize;
		static const Drawing::PointI DefaultTextOffset;

		void ResetCaretBlink();
		void PlaceCaret(int position);
		int CalculateVisibleCharacters();
			
		Misc::TextHelper textHelper_;
		
		Misc::UnicodeChar passwordChar_;
		Misc::UnicodeString realtext_;

		Drawing::RectangleI textRect_;
		Drawing::RectangleI caretRect_;
		
		bool showCaret_;
		bool drawCaret_;
		Misc::TimeSpan blinkTime_;
		Misc::DateTime nextBlinkTime_;
		int caretPosition_;
		int firstVisibleCharacter_;
		int visibleCharacterCount_;

		TextChangedEvent textChangedEvent_;
	};
}

#endif