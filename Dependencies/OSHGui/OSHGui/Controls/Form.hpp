/*
 * OldSchoolHack GUI
 *
 * by KN4CK3R http://www.oldschoolhack.me
 *
 * See license in OSHGui.hpp
 */

#ifndef OSHGUI_FORM_HPP
#define OSHGUI_FORM_HPP

#include "Control.hpp"

namespace OSHGui {
	class Label;
	class Panel;

	/**
	 * Occurs when the form is to be closed.
	 */
	typedef Event<void(Control*, bool &canClose)> FormClosingEvent;
	typedef EventHandler<void(Control*, bool &canClose)> FormClosingEventHandler;

	/**
	 * Specifies identifiers that indicate the return value of a dialog box.
	 */
	enum class DialogResult {
		/**
		 * Der R�ckgabewert des Dialogfelds ist Nothing.
		 */
		None,
		/**
		 * Der R�ckgabewert des Dialogfelds ist OK (�blicherweise von der Schaltfl�che OK gesendet).
		 */
		OK,
		/**
		 * Der R�ckgabewert des Dialogfelds ist Cancel (�blicherweise von der Schaltfl�che Abbrechen gesendet).
		 */
		Cancel,
		/**
		 * Der R�ckgabewert des Dialogfelds ist Abort (�blicherweise von der Schaltfl�che Abbrechen gesendet).
		 */
		Abort,
		/**
		 * Der R�ckgabewert des Dialogfelds ist Retry (�blicherweise von der Schaltfl�che Wiederholen gesendet).
		 */
		Retry,
		/**
		 * Der R�ckgabewert des Dialogfelds ist Ignore (�blicherweise von der Schaltfl�che Ignorieren gesendet).
		 */
		Ignore,
		/**
		 * Der R�ckgabewert des Dialogfelds ist Yes (�blicherweise von der Schaltfl�che Ja gesendet).
		 */
		Yes,
		/**
		 * Der R�ckgabewert des Dialogfelds ist No (�blicherweise von der Schaltfl�che Nein gesendet).
		 */
		No
	};

	/**
	 * Represents a window that forms the user interface.
	 */
	class OSHGUI_EXPORT Form : public Control {
		class CaptionBar;

	public:
		using Control::SetSize;

		Form(bool create_caption);
		Form();
		/**
		 * Gets whether the form is displayed modally.
		 *
		 * \return modal
		 */
		bool IsModal() const;

		virtual void SetSize(const Drawing::SizeI &size) override;

		void SetText(const Misc::UnicodeString &text);
		/**
		 * Gibt den Text zur�ck.
		 *
		 * \return der Text
		 */
		const Misc::UnicodeString& GetText() const;

		virtual void SetForeColor(const Drawing::Color &color) override;
		
		/**
		 * Legt das DialogResult f�r das Fenster fest.
		 *
		 * \param result
		 */
		void SetDialogResult(DialogResult result);
		/**
		 * Ruft das DialogResult f�r das Fenster ab.
		 *
		 * \return dialogResult
		 */
		DialogResult GetDialogResult() const;
		/**
		 * Ruft das FormClosingEvent ab.
		 *
		 * \return formClosingEvent
		 */
		FormClosingEvent& GetFormClosingEvent();
		
		/**
		 * Shows the form.
		 *
		 * \param instance die aktuelle Instanz dieser Form
		 */
		void Show(const std::shared_ptr<Form> &instance);
		/**
		 * Displays the form to modal.
		 *
		 * \param instance die aktuelle Instanz dieser Form
		 */
		void ShowDialog(const std::shared_ptr<Form> &instance);
		/**
		 * Displays the form to modal.
		 *
		 * \param instance the current instance of this form
		 * \param closeFunction this function is performed when the mold is closed (can be null)
		 */
		void ShowDialog(const std::shared_ptr<Form> &instance, const std::function<void()> &closeFunction);
		/**
		 * Schlie�t die Form.
		 */
		void Close();

		virtual const std::deque<Control*>& GetControls() const override;
		virtual void AddControl(Control *control) override;
		virtual void RemoveControl(Control *control) override;

	protected:
		virtual void PopulateGeometry() override;

		CaptionBar *captionBar_;
		Panel *containerPanel_;

	private:
		static const Drawing::PointI DefaultLocation;
		static const Drawing::SizeI DefaultSize;

		std::weak_ptr<Form> instance_;

		bool isModal_;
		FormClosingEvent formClosingEvent_;

		DialogResult dialogResult_;

		class CaptionBar : public Control {
			class CaptionBarButton : public Control {
			public:
				static const Drawing::SizeI DefaultSize;

				CaptionBarButton(Control* parent);

				virtual void CalculateAbsoluteLocation() override;

			protected:
				virtual void PopulateGeometry() override;

				virtual void OnMouseUp(const MouseMessage &mouse) override;

			private:
				static const Drawing::PointI DefaultCrossOffset;

				Drawing::PointI crossAbsoluteLocation_;
			};

		public:
			static const int DefaultCaptionBarHeight = 17;

			CaptionBar(Control* parent);

			virtual void SetSize(const Drawing::SizeI &size) override;

			void SetText(const Misc::UnicodeString &text);
			const Misc::UnicodeString& GetText() const;
			virtual void SetForeColor(const Drawing::Color &color) override;

		protected:
			virtual void OnMouseDown(const MouseMessage &mouse) override;
			virtual void OnMouseMove(const MouseMessage &mouse) override;
			virtual void OnMouseUp(const MouseMessage &mouse) override;

		private:
			static const int DefaultButtonPadding = 6;
			static const Drawing::PointI DefaultTitleOffset;

			bool drag_;
			Drawing::PointI dragStart_;

			Label *titleLabel_;
			CaptionBarButton *closeButton_;
		};
	};
}

#endif