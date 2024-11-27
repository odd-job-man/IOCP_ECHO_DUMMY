#include "IOCP_ECHO_DUMMY.h"

IOCP_ECHO_DUMMY g_dummy;

int main()
{
	g_dummy.Start();
	while (true)
	{
		g_dummy.InitialConnect();
		Sleep(20);
	}
	Sleep(INFINITE);
	return 0;
}