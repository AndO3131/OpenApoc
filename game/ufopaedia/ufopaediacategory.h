
#include "framework/stage.h"
#include "framework/includes.h"
#include "ufopaediaentry.h"

namespace OpenApoc {

class UfopaediaCategory : public Stage
{
	private:
		Form* menuform;
		StageCmd stageCmd;

	public:
		UString ID;
		UString Title;
		UString BodyInformation;
		UString BackgroundImageFilename;
		std::vector<std::shared_ptr<UfopaediaEntry>> Entries;

		UfopaediaCategory(Framework &fw, tinyxml2::XMLElement* Element);
		~UfopaediaCategory();

		// Stage control
		virtual void Begin();
		virtual void Pause();
		virtual void Resume();
		virtual void Finish();
		virtual void EventOccurred(Event *e);
		virtual void Update(StageCmd * const cmd);
		virtual void Render();
		virtual bool IsTransition();
};
}; //namespace OpenApoc