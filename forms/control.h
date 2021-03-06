
#pragma once
#include "library/sp.h"

#include "framework/includes.h"
#include "library/colour.h"
#include "framework/font.h"
#include "framework/logger.h"

namespace OpenApoc
{

class Form;
class Event;
class Framework;
class Surface;
class Palette;

class Control
{
  private:
	sp<Surface> controlArea;

	void PreRender();
	void PostRender();

  protected:
	sp<Palette> palette;
	Control *owningControl;
	Control *focusedChild;
	bool mouseInside;
	bool mouseDepressed;
	Vec2<int> resolvedLocation;

	virtual void OnRender();

	void SetFocus(Control *Child);
	bool IsFocused();

	void ResolveLocation();
	void ConfigureFromXML(tinyxml2::XMLElement *Element);

	Control *GetRootControl();

	std::list<UString> WordWrapText(sp<OpenApoc::BitmapFont> UseFont, UString WrapText);

	Framework &fw;

	void CopyControlData(Control *CopyOf);

  public:
	UString Name;
	Vec2<int> Location;
	Vec2<int> Size;
	Colour BackgroundColour;
	bool takesFocus;
	bool showBounds;
	bool Visible;

	bool canCopy;
	Control *lastCopiedTo;

	std::vector<Control *> Controls;

	Control(Framework &fw, Control *Owner, bool takesFocus = true);
	virtual ~Control();

	Control *GetActiveControl();
	void Focus();

	virtual void EventOccured(Event *e);
	void Render();
	virtual void Update();
	virtual void UnloadResources();

	Control *operator[](int Index);
	Control *FindControl(UString ID);

	template <typename T> T *FindControlTyped(const UString &name)
	{
		Control *c = this->FindControl(name);
		if (!c)
		{
			LogError("Failed to find control \"%s\" within form \"%s\"", name.c_str(),
			         this->Name.c_str());
			return nullptr;
		}
		T *typedControl = dynamic_cast<T *>(c);
		if (!c)
		{
			LogError("Failed cast  control \"%s\" within form \"%s\" to type \"%s\"", name.c_str(),
			         this->Name.c_str(), typeid(T).name());
			return nullptr;
		}
		return typedControl;
	}

	Control *GetParent();
	Form *GetForm();
	void SetParent(Control *Parent);

	Vec2<int> GetLocationOnScreen();

	virtual Control *CopyTo(Control *CopyParent);
};

}; // namespace OpenApoc
