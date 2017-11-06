#include <future>
#include <iostream>
#include "XListener.h"

int main(){
	XListener * listener = new XListener(6112);
	bool initialized = listener->initialize();

	if (initialized)
	{
		listener->start();
	}


	
}