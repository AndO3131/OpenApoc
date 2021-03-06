
#pragma once

#include "control.h"

#include <typeinfo>

namespace OpenApoc
{

class Framework;

class Form : public Control
{

  protected:
	virtual void OnRender() override;

  public:
	Form(Framework &fw, tinyxml2::XMLElement *FormConfiguration);
	virtual ~Form();

	virtual void EventOccured(Event *e) override;
	virtual void Update() override;
	virtual void UnloadResources() override;

	virtual Control *CopyTo(Control *CopyParent) override;
};

}; // namespace OpenApoc
