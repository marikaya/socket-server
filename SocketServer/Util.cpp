#include "Util.h"

void Console_Print(std::string message)
{
	time_t Now = time(NULL);
	string Time = asctime(localtime(&Now));
	Time.erase(Time.size() - 1);
	cout << "[" << Time << "]" << message << endl;
}
