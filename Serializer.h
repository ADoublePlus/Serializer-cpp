#pragma once

#include <cstdint>
#include <type_traits>
#include <string.h>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>
#include <unordered_map>

namespace port
{
    static constexpr const bool kLittleEndian = true;
}

class Serializer_Base
{
    public:
        static int VariantLength(uint64_t v);

    protected:
        static void EncodeFixed32(char* buf, uint32_t v);
        static char* EncodeVariant32(char* buf, uint32_t v);
        static const char* GetVariant32Ptr(const char* p, const char* limit, uint32_t* v);
        static const char* GetVariant32PtrFallback(const char* p, const char* limit, uint32_t* v);
        static char* EncodeVariant64(char* dst, uint64_t v);
        static void EncodeFixed64(char* buf, uint64_t v);
        static const char* GetVariant64Ptr(const char* p, const char* limit, uint64_t* v);

        static inline uint32_t DecodeFixed32(const char* ptr)
        {
            if constexpr (port::kLittleEndian)
            {
                // Load the raw bytes
                uint32_t result;
                memcpy(&result, ptr, sizeof(result)); // gcc optimizes this to a plain load
                return result;
            }
            else 
                return ((static_cast<uint32_t>(static_cast<unsigned char>(ptr[0]))) |
                        (static_cast<uint32_t>(static_cast<unsigned char>(ptr[1])) << 8) |
                        (static_cast<uint32_t>(static_cast<unsigned char>(ptr[2])) << 16) |
                        (static_cast<uint32_t>(static_cast<unsigned char>(ptr[3])) << 24));
        }

        static inline uint64_t DecodeFixed64(const char* ptr)
        {
            if (port::kLittleEndian)
            {
                // Load the raw bytes
                uint64_t result;
                memcpy(&result, ptr, sizeof(result)); // gcc optimizes this to a plain load
                return result;
            }
            else 
            {
                uint64_t lo = DecodeFixed32(ptr);
                uint64_t hi = DecodeFixed32(ptr + 4);
                return (hi << 32) | lo;
            }
        }
};

class Serializer_Byte : public Serializer_Base
{
    protected:
        static void PutByte(std::string& dst, uint8_t v) { dst += *reinterpret_cast<char*>(&v); }
        static bool GetByte(std::string_view& input, uint8_t* v);
};

class Serializer_Variant32 : public Serializer_Base
{
    protected:
        static void PutVariant32(std::string& dst, uint32_t v);
        static bool GetVariant32(std::string_view& input, uint32_t* v);
};

class Serializer_Fixed32 : public Serializer_Base
{
    protected:
        static void PutFixed32(std::string& dst, uint32_t v);
        static bool GetFixed32(std::string_view& input, uint32_t* v);
};

class Serializer_Variant64 : public Serializer_Base
{
    protected:
        static void PutVariant64(std::string& dst, uint64_t v);
        static bool GetVariant64(std::string_view& input, uint64_t* v);
};

class Serializer_Fixed64 : public Serializer_Base
{
    protected:
        static void PutFixed64(std::string& dst, uint64_t v);
        static bool GetFixed64(std::string_view& input, uint64_t* v);
};

template <typename T, typename = std::enable_if_t<std::is_enum_v<T>>>
class Serializer_Enum : public Serializer_Variant32
{
    public:
        static void PutValue(std::string& dst, T v)
        {
            PutVariant32(dst, static_cast<uint32_t>(v));
        }

        static bool GetValue(std::string_view& src, T& v)
        {
            uint32_t u;

            if (GetVariant32(src, reinterpret_cast<uint32_t*>(&u)))
            {
                v = static_cast<T>(u);

                return true;
            }

            return false;
        }
};

// Serializer class
template <typename T>
class Serializer;

template <>
class Serializer<int64_t> : public Serializer_Variant64
{
    public:
        static void PutValue(std::string& dst, int64_t v)
        {
            PutVariant64(dst, v);
        }

        static bool GetValue(std::string_view& src, int64_t& v)
        {
            return GetVariant64(src, reinterpret_cast<uint64_t*>(&v));
        }
};

template <>
class Serializer<uint64_t> : public Serializer_Variant64 
{
    public:
        static void PutValue(std::string& dst, uint64_t v)
        {
            PutVariant64(dst, v);
        }

        static bool GetValue(std::string_view& src, uint64_t& v)
        {
            return GetVariant64(src, &v);
        }
};

template <>
class Serializer<int> : public Serializer_Variant32
{
    public:
        static void PutValue(std::string& dst, int v)
        {
            PutVariant32(dst, v);
        }

        static bool GetValue(std::string_view& src, int& v)
        {
            return GetVariant32(src, reinterpret_cast<uint32_t*>(&v));
        }
};

template <>
class Serializer<uint32_t> : public Serializer_Variant32
{
    public:
        static void PutValue(std::string& dst, uint32_t v)
        {
            PutVariant32(dst, v);
        }

        static bool GetValue(std::string_view& src, uint32_t& v)
        {
            return GetVariant32(src, &v);
        }
};

template <>
class Serializer<int16_t> : public Serializer_Variant32
{
    public:
        static void PutValue(std::string& dst, int16_t v)
        {
            PutVariant32(dst, static_cast<uint32_t>(v));
        }

        static bool GetValue(std::string_view& src, int16_t& v)
        {
            uint32_t u;

            if (GetVariant32(src, reinterpret_cast<uint32_t*>(&u)))
            {
                v = static_cast<int16_t>(u);
                
                return true;
            }

            return false;
        }
};

template <>
class Serializer<uint16_t> : public Serializer_Variant32
{
    public:
        static void PutValue(std::string& dst, uint16_t v)
        {
            PutVariant32(dst, static_cast<uint32_t>(v));
        }

        static bool GetValue(std::string_view& src, uint16_t& v)
        {
            uint32_t u;

            if (GetVariant32(src, reinterpret_cast<uint32_t*>(&u)))
            {
                v = static_cast<uint16_t>(u);

                return true;
            }

            return false;
        }
};

template <>
class Serializer<char> : public Serializer_Byte
{
    public:
        static void PutValue(std::string& dst, char v)
        {
            PutByte(dst, v);
        }

        static bool GetValue(std::string_view& src, char& v)
        {
            return GetByte(src, reinterpret_cast<uint8_t*>(&v));
        }
};

template <>
class Serializer<int8_t> : public Serializer_Byte
{
    public:
        static void PutValue(std::string& dst, int8_t v)
        {
            PutByte(dst, v);
        }

        static bool GetValue(std::string_view& src, int8_t& v)
        {
            return GetByte(src, reinterpret_cast<uint8_t*>(&v));
        }
};

template <>
class Serializer<uint8_t> : public Serializer_Byte
{
    public:
        static void PutValue(std::string& dst, uint8_t v)
        {
            PutByte(dst, v);
        }

        static bool GetValue(std::string_view& src, uint8_t& v)
        {
            return GetByte(src, &v);
        }
};

template <>
class Serializer<float> : public Serializer_Fixed32
{
    public:
        static void PutValue(std::string& dst, float v)
        {
            union 
            {
                float f;
                uint32_t u;
            };

            f = v;
            PutFixed32(dst, u);
        }

        static bool GetValue(std::string_view& src, float& v)
        {
            union 
            {
                float f;
                uint32_t u;
            };

            if (GetFixed32(src, &u))
            {
                v = f;
                src.remove_prefix(sizeof(float));

                return true;
            }

            return false;
        }
};

template <>
class Serializer<double> : public Serializer_Fixed64
{
    public:
        static void PutValue(std::string& dst, double v)
        {
            union 
            {
                double d;
                uint64_t l;
            };

            d = v;
            PutFixed64(dst, l);
        }

        static bool GetValue(std::string_view& src, double& v)
        {
            union 
            {
                double d;
                uint64_t l;
            };

            if (GetFixed64(src, &l))
            {
                v = d;
                src.remove_prefix(sizeof(double));

                return true;
            }

            return false;
        }
};

template <>
class Serializer<std::string> : public Serializer_Variant32
{
    public:
        static void PutValue(std::string& dst, const std::string& v)
        {
            PutVariant32(dst, static_cast<uint32_t>(v.size()));
            dst.append(v.data(), v.size());
        }

        static bool GetValue(std::string_view& src, std::string& v)
        {
            uint32_t len;

            if (GetVariant32(src, &len) && src.size() >= len)
            {
                v.assign(src.data(), len);
                src.remove_prefix(len);

                return true;
            }
            else 
                return false;
        }
};

template <typename T>
class Serializer<std::vector<T>> : public Serializer_Variant32
{
    public:
        static void PutValue(std::string& dst, const std::vector<T>& v)
        {
            PutVariant32(dst, static_cast<uint32_t>(v.size()));

            for (const auto& e : v)
            {
                Serializer<T>::PutValue(dst, e);
            }
        }

        static bool GetValue(std::string_view& src, std::vector<T>& v)
        {
            uint32_t vSize;

            if (!GetVariant32(src, &vSize))
                return false;

            for (uint32_t i = 0; i < vSize; i++)
            {
                if (!Serializer<T>::GetValue(src, v.emplace_back()))
                    return false;
            }

            return true;
        }
};

template <typename K, typename V>
class Serializer<std::unordered_map<K, V>> : public Serializer_Variant32
{
    public:
        static void PutValue(std::string& dst, const std::unordered_map<K, V>& v)
        {
            PutVariant32(dst, static_cast<uint32_t>(v.size()));

            for (const auto& kv : v)
            {
                Serializer<K>::PutValue(dst, kv.first);
                Serializer<V>::PutValue(dst, kv.second);
            }
        }

        static bool GetValue(std::string_view& src, std::unordered_map<K, V>& v)
        {
            uint32_t vSize;

            if (!GetVariant32(src, &vSize))
                return false;

            for (uint32_t i = 0; i < vSize; i++)
            {
                std::pair<K, V> kv;

                if (!Serializer<K>::GetValue(src, kv.first))
                    return false;

                if (!Serializer<V>::GetValue(src, kv.second))
                    return false;

                v.insert(std::move(kv));
            }

            return true;
        }
};

template <typename T, typename = std::enable_if_t<std::is_class_v<T>>>
class Serializer_Class
{
    public:
        static void PutValue(std::string& dst, const T& obj)
        {
            if constexpr (!std::is_same_v<typename T::SuperClass, void>)
            {
                Serializer<typename T::SuperClass>::PutValue(dst, obj);
            }

            constexpr auto memberSize = std::tuple_size_v<decltype(T::kMetaClassMember)>;
            PutMember(dst, &obj, T::kMetaClassMember, std::make_index_sequence<memberSize>{});
        }

        static bool GetValue(std::string_view& src, T& obj)
        {
            bool result = true;

            if constexpr (!std::is_same_v<typename T::SuperClass, void>)
            {
                result = result && Serializer<typename T::SuperClass>::GetValue(src, obj);
            }

            constexpr auto memberSize = std::tuple_size_v<decltype(T::kMetaClassMember)>;
            return result && GetMember(src, &obj, T::kMetaClassMember, std::make_index_sequence<memberSize>{});
        }

    protected:
        template <typename O, typename... Args, std::size_t... Idx>
        static void PutMember(std::string& dst, const O* o, const std::tuple<Args...>& t, std::index_sequence<Idx...>)
        {
            (Serializer<std::remove_const_t<std::remove_reference_t<decltype(o->*std::get<Idx>(t))>>>::PutValue(dst, o->*std::get<Idx>(t)), ...);
        }

        template <typename O, typename... Args, size_t... Idx>
        static bool GetMember(std::string_view& src, O* o, const std::tuple<Args...>& t, std::index_sequence<Idx...>)
        {
            return (Serializer<std::remove_const_t<std::remove_reference_t<decltype(o->*std::get<Idx>(t))>>>::GetValue(src, o->*std::get<Idx>(t)) && ...);
        }
};