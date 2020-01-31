#include "core.hpp"

static ui *interface = nullptr;


int main()
{
	livemedia_pp::ref();

	interface = new ui_platform("user interface");
	

	livemedia_pp::ref(false);
	return 0;
};

