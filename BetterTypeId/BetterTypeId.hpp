#pragma once

#include <type_traits>
#include <string_view>
#include <tuple>
#include <algorithm>
#include <array>

namespace BetterTypeId
{
	namespace Detail
	{
		template<typename T>
		constexpr std::string_view compilerBasedTypeName()
		{
#if defined(__clang__)
			constexpr size_t prefixLength = 0;
			constexpr size_t postfixLength = 0;
			return std::string_view{ __PRETTY_FUNCTION__ + prefixLength, sizeof(__PRETTY_FUNCTION__) - prefixLength - postfixLength };
#elif defined(__GNUC__) || defined(__GNUG__)
			constexpr size_t prefixLength = 0;
			constexpr size_t postfixLength = 0;
			return std::string_view{ __PRETTY_FUNCTION__ + prefixLength, sizeof(__PRETTY_FUNCTION__) - prefixLength - postfixLength };
#elif defined(_MSC_VER)
			constexpr size_t prefixLength = 118;
			constexpr size_t postfixLength = 8;
			return std::string_view{ __FUNCSIG__ + prefixLength, sizeof(__FUNCSIG__) - prefixLength - postfixLength };
#endif
		}

		template<typename T>
		struct is_enum
		{
			static constexpr std::string_view substr = "enum ";
			static constexpr bool value = compilerBasedTypeName<T>().size() > substr.size() && compilerBasedTypeName<T>().substr(0, substr.size()) == substr;
		};
		template<typename T>
		constexpr bool is_enum_v = is_enum<T>::value;

		template<typename T>
		struct is_class
		{
			static constexpr std::string_view substr = "class ";
			static constexpr bool value = compilerBasedTypeName<T>().size() > substr.size() && compilerBasedTypeName<T>().substr(0, substr.size()) == substr;
		};
		template<typename T>
		constexpr bool is_class_v = is_class<T>::value;

		template<typename T>
		struct is_struct
		{
			static constexpr std::string_view substr = "struct ";
			static constexpr bool value = compilerBasedTypeName<T>().size() > substr.size() && compilerBasedTypeName<T>().substr(0, substr.size()) == substr;
		};
		template<typename T>
		constexpr bool is_struct_v = is_struct<T>::value;

		template<typename T>
		constexpr size_t type_specifier_length_v
			= is_class_v<T> ? is_class<T>::substr.size()
			: is_struct_v<T> ? is_struct<T>::substr.size()
			: is_enum_v<T> ? is_enum<T>::substr.size()
			: 0;

		template<typename T>
		struct nested_template_info
		{
			static constexpr size_t size = 0;
		};

		template<template<typename...> typename T, typename... Args>
		struct nested_template_info<T<Args...>>
		{
			static constexpr size_t size = sizeof...(Args);
			using type = std::tuple<Args...>;
		};

		template<typename T, size_t N>
		struct typeNameStorage
		{
			static std::array<char, N> data;
		};
		template<typename T, size_t N>
		std::array<char, N> typeNameStorage<T, N>::data = {};

		// forward declaration
		template<typename T>
		constexpr size_t typeNameSize(bool);

		// find size
		template<typename T>
		constexpr size_t simpleTypeNameSize()
		{
			return compilerBasedTypeName<T>().substr(type_specifier_length_v<T>).size();
		}

		template<typename T>
		constexpr size_t nestedBaseTypeNameSize()
		{
			auto name = compilerBasedTypeName<T>().substr(type_specifier_length_v<T>);
			return name.substr(0, name.find('<')).size();
		}

		template<typename T, typename... Args>
		constexpr size_t combineNestedTypeNamesSize(std::tuple<Args...>*)
		{
			size_t views[] = { nestedBaseTypeNameSize<T>(), typeNameSize<Args>(false)... };
			size_t i = 0;
			size_t n = 0;
			for (const auto& view : views)
			{
				if (n > 1)
				{
					i += 2;
				}
				i += view;
				if (n == 0)
				{
					i++;
				}
				n++;
			}
			return i + 1;
		}

		template<typename T>
		constexpr size_t nestedTypeNameSize()
		{
			using tuple_type = typename nested_template_info<T>::type;
			tuple_type* tuple{};
			return combineNestedTypeNamesSize<T>(tuple);
		}

		template<typename T>
		constexpr size_t typeNameSize(bool appendZeroTerminator)
		{
			if constexpr (std::is_const_v<T>)
			{
				return typeNameSize<typename std::remove_const_t<T>>(false) + 6 + (appendZeroTerminator ? 1 : 0);
			}
			else if constexpr (std::is_pointer_v<T>)
			{
				return typeNameSize<typename std::remove_pointer_t<T>>(false) + 1 + (appendZeroTerminator ? 1 : 0);
			}
			else if constexpr (std::is_reference_v<T>)
			{
				return typeNameSize<typename std::remove_reference_t<T>>(false) + 1 + (appendZeroTerminator ? 1 : 0);
			}
			if constexpr (nested_template_info<T>::size > 0)
			{
				return nestedTypeNameSize<T>() + (appendZeroTerminator ? 1 : 0);
			}
			return simpleTypeNameSize<T>() + (appendZeroTerminator ? 1 : 0);
		}

		// construct name
		template<typename T, size_t N>
		constexpr std::array<char, N> simpleTypeName()
		{
			auto str = compilerBasedTypeName<T>().substr(type_specifier_length_v<T>);
			std::array<char, N> name = {};
			size_t i = 0;
			for (const auto& c : str)
			{
				name[i++] = c;
			}
			return name;
		}

		template<typename T, size_t N = nestedBaseTypeNameSize<T>()>
		constexpr std::array<char, N> nestedBaseTypeName()
		{
			auto str = compilerBasedTypeName<T>().substr(type_specifier_length_v<T>);
			str = str.substr(0, str.find('<'));
			std::array<char, N> name = {};
			size_t i = 0;
			for (const auto& c : str)
			{
				name[i++] = c;
			}
			return name;
		}

		template<size_t N, typename T, typename... Args>
		constexpr void writeNestedTypeNames(std::array<char, N>& buffer, size_t& i, size_t& n)
		{
			if (n > 0)
			{
				buffer[i++] = ',';
				buffer[i++] = ' ';
			}
			for (const auto& c : typeName<T, typeNameSize<T>(false)>())
			{
				buffer[i++] = c;
			}
			n++;
			if constexpr (sizeof...(Args))
			{
				writeNestedTypeNames<N, Args...>(buffer, i, n);
			}
		}

		template<typename T, size_t N, typename... Args>
		constexpr void combineNestedTypeNames(std::array<char, N>& buffer, std::tuple<Args...>*)
		{
			size_t i = 0;
			for (const auto& c : nestedBaseTypeName<T>())
			{
				buffer[i++] = c;
			}

			buffer[i++] = '<';

			size_t n = 0;
			writeNestedTypeNames<N, Args...>(buffer, i, n);

			buffer[i++] = '>';
		}

		template<typename T, size_t N>
		constexpr std::array<char, N> nestedTypeName()
		{
			using tuple_type = typename nested_template_info<T>::type;
			tuple_type* tuple{};
			std::array<char, N> name = {};
			combineNestedTypeNames<T>(name, tuple);
			return name;
		}

		template<typename T, size_t N = typeNameSize<T>(true)>
		constexpr std::array<char, N> typeName()
		{
			if constexpr (std::is_const_v<T>)
			{
				std::array<char, N> name = {};
				size_t i = 0;
				for (const auto& c : std::string_view("const "))
				{
					name[i++] = c;
				}
				for (const auto& c : typeName<typename std::remove_const_t<T>, typeNameSize<typename std::remove_const_t<T>>(false)>())
				{
					name[i++] = c;
				}
				return name;
			}
			else if constexpr (std::is_pointer_v<T>)
			{
				std::array<char, N> name = {};
				size_t i = 0;
				for (const auto& c : typeName<typename std::remove_pointer_t<T>, typeNameSize<typename std::remove_pointer_t<T>>(false)>())
				{
					name[i++] = c;
				}
				for (const auto& c : std::string_view("*"))
				{
					name[i++] = c;
				}
				return name;
			}
			else if constexpr (std::is_reference_v<T>)
			{
				std::array<char, N> name = {};
				size_t i = 0;
				for (const auto& c : typeName<typename std::remove_reference_t<T>, typeNameSize<typename std::remove_reference_t<T>>(false)>())
				{
					name[i++] = c;
				}
				for (const auto& c : std::string_view("&"))
				{
					name[i++] = c;
				}
				return name;
			}
			else if constexpr (nested_template_info<T>::size > 0)
			{
				return nestedTypeName<T, N>();
			}
			return simpleTypeName<T, N>();
		}


		// create id
		template<typename T>
		constexpr size_t typeId()
		{
			size_t id = 0;
			for (const auto& c : typeName<T>())
			{
				id += c;
			}
			return id;
		}
	}

	template<typename T, size_t N = Detail::typeNameSize<T>(true)>
	constexpr std::array<char, N> typeName()
	{
		return Detail::typeName<T, N>();
	}


	// create id
	template<typename T>
	constexpr size_t typeId()
	{
		return Detail::typeId<T>();
	}
}