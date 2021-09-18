#include <nusys.h>

NUContData pad_data[NU_CONT_MAXCONTROLLERS];

void ContInit()
{
	nuContInit();
}

void ContUpdate()
{
	nuContDataGetExAll(pad_data);
}