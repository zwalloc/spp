#pragma once

#include "packet.h"

#include <string>
#include <optional>
#include <vector>
#include <limits>

#ifdef  _DEBUG
#include <typeinfo>
#endif //  _DEBUG

#include <ulib/containers/list.h>

#undef max
#undef min

namespace spp
{
	struct DeserializeError
	{
		std::string type;
		std::string desc;
		uint line;
		spp::PacketView* packet;
	};

	inline void(*pfnDeserializeErrorHandler)(const DeserializeError& error) = nullptr;

#ifdef  _DEBUG
	inline void ReportDebugDeserializeError(const std::string& type, const std::string& desc, uint line, spp::PacketView& packet)
	{
		if (pfnDeserializeErrorHandler)
		{
			DeserializeError error;
			error.type = type;
			error.desc = desc;
			error.line = line;
			error.packet = &packet;

			pfnDeserializeErrorHandler(error);
		}
	}
#endif //  _DEBUG

}

#ifdef  _DEBUG
#define ReportDeserializeError(type, desc, packet) ReportDebugDeserializeError(typeid(type).name(), desc, __LINE__, packet)
#else
#define ReportDeserializeError(type, desc, packet) {}
#endif

namespace spp
{
	template<class CharT, uint maxChars, class SizePrefixT>
	struct sp_basic_string : public std::basic_string<CharT>
	{
		using StringT = std::basic_string<CharT>;

		sp_basic_string() : StringT() {}
		sp_basic_string(const CharT* str) : StringT(str) {}
		sp_basic_string(const StringT& str) : StringT(str) {}

		static constexpr uint MaxChars = maxChars;
	};

	template<class CharT, uint maxChars> // maxChars include zero char
	struct z_basic_string : public std::basic_string<CharT>
	{
		using StringT = std::basic_string<CharT>;

		z_basic_string() : StringT() {}
		z_basic_string(const CharT* str) : StringT(str) {}
		z_basic_string(const StringT& str) : StringT(str) {}

		static constexpr uint MaxChars = maxChars;
	};

	template<class CharT, uint staticSize> // size include zero char and requires CharT[MaxChars] buffer
	struct s_basic_string : public std::basic_string<CharT>
	{
		using StringT = std::basic_string<CharT>;

		s_basic_string() : StringT() {}
		s_basic_string(const CharT* str) : StringT(str) {}
		s_basic_string(const StringT& str) : StringT(str) {}

		static constexpr uint Size = staticSize;
	};

	template<uint maxChars> using sp_string = sp_basic_string<char, maxChars, uint>;
	template<uint maxChars> using sp_wstring = sp_basic_string<wchar_t, maxChars, uint>;

	template<uint maxChars> using z_string = z_basic_string<char, maxChars>;
	template<uint maxChars> using z_wstring = z_basic_string<wchar_t, maxChars>;

	template<uint size> using s_string = s_basic_string<char, size>;
	template<uint size> using s_wstring = s_basic_string<wchar_t, size>;

	template<class T, uint size>
	struct sp_list : public  ulib::List<T>
	{
		static constexpr uint Size = size;
	};

	template<class T, uint size>
	struct sp_vector : public  std::vector<T>
	{
		static constexpr uint Size = size;
	};

    /*
    class ModifierTag {};
	class ContainerTag {};

	class NullModifier 
	{
		using SchemaTag = ModifierTag;
	};

	template<class T, class NextModifierT = NullModifier>
	struct SizePrefix : public NextModifierT
	{
		using SchemaTag = ModifierTag;

		template<class ContainerT>
		inline static void Apply(class ContainerT& cont)
		{
			*(uint32_t*)buffer = bufferSize;
		}
	};

	template<class T, class NextModifierT = NullModifier>
	struct ZeroEnd : public NextModifierT
	{

	};

	template<class ElementT, class ModifiersT>
	struct Field : public ElementT
	{
		using Modifiers = ModifiersT;

		inline void Read(void* buffer)
		{
			Modifiers::Apply(buffer);
		}
	};

	struct TestSchema
	{
		// list with 4 byte size prefix
		spp::Field<ulib::List<int>, SizePrefix<int>> list;

		// size_t value with 1 byte size prefix like sizeof(size_t)
		spp::Field<size_t, SizePrefix<char>> vehicleCount;

		// size prefix counted by string size + sizeof(char)
		spp::Field<std::string, SizePrefix<int, ZeroEnd<char>>> str;

		// stirng
		std::string ky;
	};
    */
	

	template<class T>
	struct schema_walker;

	template<class T>
	struct schema_serializer
	{
		inline static void write(const T& elem, spp::Packet& out)
		{
			if constexpr (bpacs::has_bp_reflection<T>::value)
				schema_walker<T>::write(elem, out);
			else
				out.Write(&elem, sizeof(elem));
		}

		inline static bool read(spp::PacketView& packet, T& out)
		{
			if constexpr (bpacs::has_bp_reflection<T>::value)
				return schema_walker<T>::read(packet, out);

			if (!packet.CanRead<T>())
			{
				ReportDeserializeError(decltype(out), "", packet);
				return false;
			}

			packet.Read(&out, sizeof(T));
			return true;
		}

		inline static constexpr size_t max_size()
		{
			if constexpr (bpacs::has_bp_reflection<T>::value)
				return schema_walker<T>::max_size();

			return sizeof(T);
		}
	};

	template<class CharT, uint maxChars>
	struct schema_serializer<spp::z_basic_string<CharT, maxChars>>
	{
		inline static void write(const spp::z_basic_string<CharT, maxChars>& elem, spp::Packet& out)
		{
			assert("Schema contains so large string" && (elem.size() < maxChars));
			out.WriteString(elem.c_str());
		}

		inline static bool read(spp::PacketView& packet, spp::z_basic_string<CharT, maxChars>& out)
		{
			if (!packet.CanReadString<CharT>(size_t(maxChars)))
			{
				ReportDeserializeError(decltype(out), "", packet);
				return false;
			}

			out = packet.ReadString<CharT>();
			return true;
		}

		inline static size_t max_size() { return maxChars * sizeof(CharT); }
	};

	template<class CharT, uint maxChars, class SizePrefixT>
	struct schema_serializer<spp::sp_basic_string<CharT, maxChars, SizePrefixT>>
	{
		inline static void write(const spp::sp_basic_string<CharT, maxChars, SizePrefixT>& elem, spp::Packet& out)
		{
			assert("Schema contains so large string" && (elem.size() * sizeof(CharT)) < maxChars);
			assert("Schema SizePrefixT so low" && (elem.size() < std::numeric_limits<SizePrefixT>::max()));

			out.Write<SizePrefixT>(SizePrefixT(elem.size()));
			out.Write(elem.data(), elem.size() * sizeof(CharT));
		}

		inline static bool read(spp::PacketView& packet, spp::sp_basic_string<CharT, maxChars, SizePrefixT>& out)
		{
			if (!packet.CanRead<SizePrefixT>())
			{
				ReportDeserializeError(decltype(out), "", packet);
				return false;
			}

			SizePrefixT length = packet.Read<SizePrefixT>();
			if (!packet.CanRead(length * sizeof(CharT)))
			{
				ReportDeserializeError(decltype(out), std::string("string length: ") + std::to_string(length), packet);
				return false;
			}

			CharT* begin = packet.Read<CharT>(length * sizeof(CharT));
			CharT* end = begin + length;

			out.assign(begin, end);

			return true;
		}

		inline static constexpr size_t max_size()
		{
			return maxChars * sizeof(CharT) + sizeof(SizePrefixT);
		}
	};

	template<class CharT, uint size>
	struct schema_serializer<spp::s_basic_string<CharT, size>>
	{
		inline static void write(const spp::s_basic_string<CharT, size>& elem, spp::Packet& out)
		{
			assert("Schema contains so large string" && elem.size() + 1 < size);

			out.Write(elem.data(), elem.size() * sizeof(CharT));
			out.Write<CharT>(0);

			const size_t stepSize = sizeof(CharT) * (size - (uint(elem.size()) + 1));
			out.Step(stepSize);
		}

		inline static bool read(spp::PacketView& packet, spp::s_basic_string<CharT, size>& out)
		{
			if (!packet.CanReadString<CharT>(size_t(size)))
			{
				ReportDeserializeError(decltype(out), "", packet);
				return false;
			}

			out = packet.Read<CharT>(size * sizeof(CharT));

			return true;
		}

		inline static constexpr size_t max_size()
		{
			return size * sizeof(CharT);
		}
	};

	template<class ElemT, uint limit>
	struct schema_serializer<spp::sp_list<ElemT, limit>>
	{
		inline static void write(const spp::sp_list<ElemT, limit>& elem, spp::Packet& out)
		{
			assert(elem.Size() < limit);

			out.Write<uint>(elem.Size());
			for (auto& obj : elem)
				schema_serializer<ElemT>::write(obj, out);
		}

		inline static bool read(spp::PacketView& packet, spp::sp_list<ElemT, limit>& out)
		{
			if (!packet.CanRead<uint>())
			{
				ReportDeserializeError(decltype(out), "", packet);
				return false;
			}

			uint size = packet.Read<uint>();
			if (size > limit)
			{
				ReportDeserializeError(decltype(out), std::string("list size (") + std::to_string(size) + ") bigger than limit (" + std::to_string(limit) + ")", packet);
				return false;
			}

			out.Clear();
			for (uint i = 0; i != size; i++)
			{
				typename spp::sp_list<ElemT, limit>::value_type elem;
				if (!schema_serializer<ElemT>::read(packet, elem))
					return false;
				out.Add(elem);
			}

			return true;
		}

		inline static constexpr size_t max_size()
		{
			return sizeof(uint) + schema_serializer<ElemT>::max_size() * limit;
		}
	};

	template<class ElemT, uint limit>
	struct schema_serializer<spp::sp_vector<ElemT, limit>>
	{
		inline static void write(const spp::sp_vector<ElemT, limit>& elem, spp::Packet& out)
		{
			assert(elem.size() < limit);

			out.Write<uint>(elem.size());
			for (auto& obj : elem)
				schema_serializer<ElemT>::write(obj, out);
		}

		inline static bool read(spp::PacketView& packet, spp::sp_vector<ElemT, limit>& out)
		{
			if (!packet.CanRead<uint>())
			{
				ReportDeserializeError(decltype(out), "", packet);
				return false;
			}

			uint size = packet.Read<uint>();
			if (size > limit)
			{
				ReportDeserializeError(decltype(out), std::string("list size (") + std::to_string(size) + ") bigger than limit (" + std::to_string(limit) + ")", packet);
				return false;
			}

			out.clear();
			for (uint i = 0; i != size; i++)
			{
				typename spp::sp_vector<ElemT, limit>::value_type elem;
				if (!schema_serializer<ElemT>::read(packet, elem))
					return false;
				out.push_back(elem);
			}

			return true;
		}

		inline static constexpr size_t max_size()
		{
			return sizeof(uint) + schema_serializer<ElemT>::max_size() * limit;
		}
	};

	template<class T>
	struct schema_serializer<std::optional<T>>
	{
		inline static void write(const std::optional<T>& elem, spp::Packet& out)
		{
			if (elem.has_value())
			{
				out.Write<bool>(true);
				schema_serializer<T>::write(elem.value(), out);
			}
			else
			{
				out.Write<bool>(false);
			}
		}

		inline static bool read(spp::PacketView& packet, std::optional<T>& out)
		{
			if (!packet.CanRead<bool>())
			{
				ReportDeserializeError(decltype(out), "", packet);
				return false;
			}

			bool has = packet.Read<bool>();
			if (has)
			{
				out.emplace();
				schema_serializer<T>::read(packet, out.value());
			}
			else
			{
				out = {};
			}

			return true;
		}

		inline static constexpr size_t max_size()
		{
			return sizeof(bool) + schema_serializer<T>::max_size();
		}
	};

	template<class T>
	struct schema_walker
	{
		inline static void write(const T& elem, spp::Packet& out)
		{
			size_t size = 0;
			bpacs::iterate_object(elem, [&](auto field)
				{
					using ttype = typename std::remove_const_t<std::remove_reference_t<decltype(field.value())>>;
					schema_serializer<ttype>::write(field.value(), out);
					size += sizeof(field.value());
				});

			if (sizeof(T) == 1 && size == 0)
				return;

			constexpr size_t schemaSize = sizeof(T);
			assert(size == schemaSize);
		}

		inline static bool read(spp::PacketView& packet, T& out)
		{
			size_t size = 0;
			bool failed = false;
			bpacs::iterate_object(out, [&](auto field)
				{
					size += sizeof(field.value());
					if (!failed)
					{
						using ttype = typename std::remove_const_t<std::remove_reference_t<decltype(field.value())>>;
						if (!schema_serializer<ttype>::read(packet, field.value()))
						{
							failed = true;
							return;
						}
					}
				});


			if (sizeof(T) == 1 && size == 0)
			{
				if (packet.CanRead())
				{
					if (!failed)
						ReportDeserializeError(decltype(out), "packet is not fully read", packet);
					return false;
				}

				return !failed;
			}

			constexpr size_t schemaSize = sizeof(T);
			assert(size == schemaSize && "Maybe you forgot define schema's fields ?");

			if (packet.CanRead())
			{
				if (!failed)
					ReportDeserializeError(decltype(out), "packet is not fully read", packet);
				return false;
			}

			return !failed;
		}

		inline static constexpr size_t max_size()
		{
			T* pSchema = nullptr;
			size_t size = 0;
			size_t limit = 0;
			bool failed = false;
			bpacs::iterate_object(*pSchema, [&](auto field)
				{
					using ttype = typename std::remove_const_t<std::remove_reference_t<decltype(field.value())>>;

					size += sizeof(field.value());
					limit += schema_serializer<ttype>::max_size();
				});

			if (sizeof(T) == 1 && size == 0)
				return limit;

			constexpr size_t schemaSize = sizeof(T);
			assert(size == schemaSize && "Maybe you forgot define schema's fields ?");
			return limit;
		}
	};


	template<class T>
	void write_schema(const T& elem, spp::Packet& out)
	{
		if constexpr (bpacs::has_bp_reflection<T>::value)
			schema_walker<T>::write(elem, out);
	}

	template<class T>
	bool read_schema(spp::PacketView packet, T& out)
	{
		if constexpr (bpacs::has_bp_reflection<T>::value)
			return schema_walker<T>::read(packet, out);

		return true;
	}

	template<class T>
	constexpr size_t max_size_schema() { return schema_serializer<T>::max_size(); }

	// ky
	template<uint packetType>
	struct Schema
	{
		static constexpr uint PacketType = packetType;
	};
}