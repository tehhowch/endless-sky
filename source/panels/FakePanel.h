
#ifndef FAKE_PANEL_H_
#define FAKE_PANEL_H_

#include <iostream>

class FakePanel {
public:
	inline void Log()
	{
		std::cout << "FakePanel Log called" << std::endl;
	}
};

#endif // FAKE_PANEL_H_
