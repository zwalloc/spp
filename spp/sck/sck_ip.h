#pragma once

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

namespace sck
{
#if defined _WIN64
	typedef unsigned long long socket_value;
#else
	typedef uint socket_value;
#endif

	namespace ip
	{
		namespace v4
		{
			struct Addr
			{
				static constexpr ulong any = 0;
				static constexpr ulong none = -1;

				// static Addr inet(const char* s);

				inline Addr() {}
				inline constexpr Addr(ulong ul) : ul(ul) {}
				inline constexpr Addr(uchar b1, uchar b2, uchar b3, uchar b4)
					: b1(b1), b2(b2), b3(b3), b4(b4) {}

				void ToString(char* buf) const;

				// example: std::string str = addr.ToString<std::string>();
				template<class T> T ToString() const { char buf[30]; ToString(buf); return buf; }

				union
				{
					struct { ulong ul; };
					struct { ushort w1, w2; };
					struct { uchar b1, b2, b3, b4; };
				};
			};

			struct Endpoint
			{
				inline Endpoint() {};
				inline constexpr Endpoint(Addr a, ushort p) : addr(a), port(p) {};
				
				void ToString(char* buf) const;

				// example: std::string str = endpoint.ToString<std::string>();
				template<class T> T ToString() const { char buf[30]; ToString(buf); return buf; }

				Addr addr;
				ushort port;
			};		
		}
	}
}