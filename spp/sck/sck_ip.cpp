#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")

#include "ip.h"

#include <winsock2.h>
#include <ws2tcpip.h>

static char* ui8toa_10(uchar val, char buf[4], char** pend)
{
	char* end = &buf[3];
	char* it = end;
	*it = 0;

	if (val < 0)
	{
		do {
			*--it = '0' - (val % 10);
			val /= 10;
		} while (val != 0);
		*--it = '-';
	}
	else
	{
		do {
			*--it = '0' + (val % 10);
			val /= 10;
		} while (val != 0);
	}

	*pend = end;
	return it;
}

static char* ui16toa_10(ushort val, char buf[7], char** pend)
{
	char* end = &buf[6];
	char* it = end;
	*it = 0;

	if (val < 0)
	{
		do {
			*--it = '0' - (val % 10);
			val /= 10;
		} while (val != 0);
		*--it = '-';
	}
	else
	{
		do {
			*--it = '0' + (val % 10);
			val /= 10;
		} while (val != 0);
	}

	*pend = end;
	return it;
}

namespace sck
{
	namespace ip
	{
		// v4::Addr v4::Addr::inet(const char* s) { return v4::Addr(isck_addr(s)); }

		void v4::Addr::ToString(char* buf) const
		{
			char i8buf[4];
			char* ptr, * end;

#define SCARY_WORK(b) ptr = ui8toa_10(b, i8buf, &end); while (ptr != end) { *buf = *ptr; ptr++; buf++; }
			SCARY_WORK(b1); *buf = '.'; buf++;
			SCARY_WORK(b2); *buf = '.'; buf++;
			SCARY_WORK(b3); *buf = '.'; buf++;
			SCARY_WORK(b4); *buf = 0;
#undef SCARY_WORK
		}

		void v4::Endpoint::ToString(char* buf) const // 22 siz
		{
			char ibuf[7];
			char* ptr, *end;

#define SCARY_WORK(b) ptr = ui8toa_10(b, ibuf, &end); while (ptr != end) { *buf = *ptr; ptr++; buf++; }
			SCARY_WORK(addr.b1); *buf = '.'; buf++;
			SCARY_WORK(addr.b2); *buf = '.'; buf++;
			SCARY_WORK(addr.b3); *buf = '.'; buf++;
			SCARY_WORK(addr.b4); *buf = ':'; buf++;
#undef SCARY_WORK
			
			ptr = ui16toa_10(port, ibuf, &end);
			while (ptr != end) { *buf = *ptr; ptr++; buf++; }
			*buf = 0;
		}
	}
}


