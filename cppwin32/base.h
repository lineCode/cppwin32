#pragma once

#include <array>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <type_traits>

#ifdef _DEBUG

#define WIN32_ASSERT _ASSERTE
#define WIN32_VERIFY WIN32_ASSERT
#define WIN32_VERIFY_(result, expression) WIN32_ASSERT(result == expression)

#else

#define WIN32_ASSERT(expression) ((void)0)
#define WIN32_VERIFY(expression) (void)(expression)
#define WIN32_VERIFY_(result, expression) (void)(expression)

#endif

#ifndef WIN32_EXPORT
#define WIN32_EXPORT
#endif

#ifdef __IUnknown_INTERFACE_DEFINED__
#define WIN32_IMPL_IUNKNOWN_DEFINED
#endif

#ifdef _M_HYBRID
#define WIN32_IMPL_LINK(function, count) __pragma(comment(linker, "/alternatename:#WIN32_IMPL_" #function "@" #count "=#" #function "@" #count))
#elif _M_ARM64EC
#define WIN32_IMPL_LINK(function, count) __pragma(comment(linker, "/alternatename:#WIN32_IMPL_" #function "=#" #function))
#elif _M_IX86
#define WIN32_IMPL_LINK(function, count) __pragma(comment(linker, "/alternatename:_WIN32_IMPL_" #function "@" #count "=_" #function "@" #count))
#else
#define WIN32_IMPL_LINK(function, count) __pragma(comment(linker, "/alternatename:WIN32_IMPL_" #function "=" #function))
#endif

WIN32_EXPORT namespace win32
{
    struct hresult
    {
        int32_t value{};

        constexpr hresult() noexcept = default;

        constexpr hresult(int32_t const value) noexcept : value(value)
        {
        }

        constexpr operator int32_t() const noexcept
        {
            return value;
        }
    };

    struct guid
    {
        uint32_t Data1;
        uint16_t Data2;
        uint16_t Data3;
        uint8_t  Data4[8];

        guid() noexcept = default;

        constexpr guid(uint32_t const Data1, uint16_t const Data2, uint16_t const Data3, std::array<uint8_t, 8> const& Data4) noexcept :
            Data1(Data1),
            Data2(Data2),
            Data3(Data3),
            Data4{ Data4[0], Data4[1], Data4[2], Data4[3], Data4[4], Data4[5], Data4[6], Data4[7] }
        {
        }

#ifdef WIN32_IMPL_IUNKNOWN_DEFINED

        constexpr guid(GUID const& value) noexcept :
            Data1(value.Data1),
            Data2(value.Data2),
            Data3(value.Data3),
            Data4{ value.Data4[0], value.Data4[1], value.Data4[2], value.Data4[3], value.Data4[4], value.Data4[5], value.Data4[6], value.Data4[7] }
        {

        }

        operator GUID const& () const noexcept
        {
            return reinterpret_cast<GUID const&>(*this);
        }

#endif
    };

    inline bool operator==(guid const& left, guid const& right) noexcept
    {
        return !memcmp(&left, &right, sizeof(left));
    }

    inline bool operator!=(guid const& left, guid const& right) noexcept
    {
        return !(left == right);
    }

    inline bool operator<(guid const& left, guid const& right) noexcept
    {
        return memcmp(&left, &right, sizeof(left)) < 0;
    }
}

WIN32_EXPORT namespace win32::Microsoft::Windows::Sdk
{
    struct IUnknown;
}

namespace win32::_impl_
{
#ifdef WIN32_IMPL_IUNKNOWN_DEFINED
    using guid_type = GUID;
#else
    using guid_type = guid;
#endif
}

WIN32_EXPORT namespace win32
{
    void check_hresult(hresult const result);
    hresult to_hresult() noexcept;

    struct take_ownership_from_abi_t {};
    inline constexpr take_ownership_from_abi_t take_ownership_from_abi{};

    template <typename T>
    struct com_ptr;
}

namespace win32::_impl_
{
    template <typename T, typename Enable = void>
    struct abi
    {
        using type = T;
    };

    template <typename T>
    struct abi<T, std::enable_if_t<std::is_enum_v<T>>>
    {
        using type = std::underlying_type_t<T>;
    };

    template <typename T>
    using abi_t = typename abi<T>::type;

    template <typename T>
#ifdef WIN32_IMPL_IUNKNOWN_DEFINED
#ifdef __clang__
    inline const guid guid_v{ __uuidof(T) };
#else
    inline constexpr guid guid_v{ __uuidof(T) };
#endif
#else
    inline constexpr guid guid_v{};
#endif

    template <typename T>
    constexpr auto to_underlying_type(T const value) noexcept
    {
        return static_cast<std::underlying_type_t<T>>(value);
    }

    template <typename, typename = std::void_t<>>
    struct is_implements : std::false_type {};

    template <typename T>
    struct is_implements<T, std::void_t<typename T::implements_type>> : std::true_type {};

    template <typename T>
    inline constexpr bool is_implements_v = is_implements<T>::value;

    template <typename T>
    struct wrapped_type
    {
        using type = T;
    };

    template <typename T>
    struct wrapped_type<com_ptr<T>>
    {
        using type = T;
    };

    template <typename T>
    using wrapped_type_t = typename wrapped_type<T>::type;
}

namespace win32::_impl_
{
    template <> struct abi<Microsoft::Windows::Sdk::IUnknown>
    {
        struct __declspec(novtable) type
        {
            virtual int32_t __stdcall QueryInterface(guid const& id, void** object) noexcept = 0;
            virtual uint32_t __stdcall AddRef() noexcept = 0;
            virtual uint32_t __stdcall Release() noexcept = 0;
        };
    };

    using unknown_abi = abi_t<Microsoft::Windows::Sdk::IUnknown>;

    template <> inline constexpr guid guid_v<Microsoft::Windows::Sdk::IUnknown>{ 0x00000000, 0x0000, 0x0000, { 0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46 } };
}

namespace win32::_impl_
{
    template <typename T>
    using com_ref = std::conditional_t<std::is_base_of_v<Microsoft::Windows::Sdk::IUnknown, T>, T, com_ptr<T>>;

    template <typename T, std::enable_if_t<is_implements_v<T>, int> = 0>
    com_ref<T> wrap_as_result(void* result)
    {
        return { &static_cast<produce<T, typename default_interface<T>::type>*>(result)->shim(), take_ownership_from_abi };
    }

    template <typename T, std::enable_if_t<!is_implements_v<T>, int> = 0>
    com_ref<T> wrap_as_result(void* result)
    {
        return { result, take_ownership_from_abi };
    }

#ifdef WIN32_IMPL_IUNKNOWN_DEFINED
    template <typename T>
    struct is_com_interface : std::disjunction<std::is_base_of<Microsoft::Windows::Sdk::IUnknown, T>, std::is_base_of<unknown_abi, T>, is_implements<T>, std::is_base_of<::IUnknown, T>> {};
#else
    template <typename T>
    struct is_com_interface : std::disjunction<std::is_base_of<Microsoft::Windows::Sdk::IUnknown, T>, std::is_base_of<unknown_abi, T>, is_implements<T>> {};
#endif

    template <typename T>
    inline constexpr bool is_com_interface_v = is_com_interface<T>::value;

    // You must include <Unknwn.h> to use this overload.
    template <typename To, typename From, std::enable_if_t<!is_com_interface_v<To>, int> = 0>
    auto as(From* ptr);

    template <typename To, typename From, std::enable_if_t<is_com_interface_v<To>, int> = 0>
    com_ref<To> as(From* ptr)
    {
        if (!ptr)
        {
            return nullptr;
        }

        void* result{};
        check_hresult(ptr->QueryInterface(guid_of<To>(), &result));
        return wrap_as_result<To>(result);
    }

    // You must include <Unknwn.h> to use this overload.
    template <typename To, typename From, std::enable_if_t<!is_com_interface_v<To>, int> = 0>
    auto try_as(From* ptr) noexcept;

    template <typename To, typename From, std::enable_if_t<is_com_interface_v<To>, int> = 0>
    com_ref<To> try_as(From* ptr) noexcept
    {
        if (!ptr)
        {
            return nullptr;
        }

        void* result{};
        ptr->QueryInterface(guid_of<To>(), &result);
        return wrap_as_result<To>(result);
    }
}


WIN32_EXPORT namespace win32::Microsoft::Windows::Sdk
{
    struct IUnknown
    {
        IUnknown() noexcept = default;
        IUnknown(std::nullptr_t) noexcept {}
        void* operator new(size_t) = delete;

        IUnknown(void* ptr, take_ownership_from_abi_t) noexcept : m_ptr(static_cast<_impl_::unknown_abi*>(ptr))
        {
        }

        IUnknown(IUnknown const& other) noexcept : m_ptr(other.m_ptr)
        {
            add_ref();
        }

        IUnknown(IUnknown&& other) noexcept : m_ptr(std::exchange(other.m_ptr, {}))
        {
        }

        ~IUnknown() noexcept
        {
            release_ref();
        }

        IUnknown& operator=(IUnknown const& other) noexcept
        {
            if (this != &other)
            {
                release_ref();
                m_ptr = other.m_ptr;
                add_ref();
            }

            return*this;
        }

        IUnknown& operator=(IUnknown&& other) noexcept
        {
            if (this != &other)
            {
                release_ref();
                m_ptr = std::exchange(other.m_ptr, {});
            }

            return*this;
        }

        explicit operator bool() const noexcept
        {
            return nullptr != m_ptr;
        }

        IUnknown& operator=(std::nullptr_t) noexcept
        {
            release_ref();
            return*this;
        }

        template <typename To>
        auto as() const
        {
            return impl::as<To>(m_ptr);
        }

        template <typename To>
        auto try_as() const noexcept
        {
            return impl::try_as<To>(m_ptr);
        }

        template <typename To>
        void as(To& to) const
        {
            to = as<impl::wrapped_type_t<To>>();
        }

        template <typename To>
        bool try_as(To& to) const noexcept
        {
            if constexpr (impl::is_com_interface_v<To> || !std::is_same_v<To, impl::wrapped_type_t<To>>)
            {
                to = try_as<impl::wrapped_type_t<To>>();
                return static_cast<bool>(to);
            }
            else
            {
                auto result = try_as<To>();
                to = result.has_value() ? result.value() : impl::empty_value<To>();
                return result.has_value();
            }
        }

        hresult as(guid const& id, void** result) const noexcept
        {
            return m_ptr->QueryInterface(id, result);
        }

        friend void swap(IUnknown& left, IUnknown& right) noexcept
        {
            std::swap(left.m_ptr, right.m_ptr);
        }

    private:

        void add_ref() const noexcept
        {
            if (m_ptr)
            {
                m_ptr->AddRef();
            }
        }

        void release_ref() noexcept
        {
            if (m_ptr)
            {
                unconditional_release_ref();
            }
        }

        __declspec(noinline) void unconditional_release_ref() noexcept
        {
            std::exchange(m_ptr, {})->Release();
        }

        _impl_::unknown_abi* m_ptr{};
    };
}

WIN32_EXPORT namespace win32
{
    template <typename T, std::enable_if_t<!std::is_base_of_v<Microsoft::Windows::Sdk::IUnknown, T>, int> = 0>
    auto get_abi(T const& object) noexcept
    {
        return reinterpret_cast<impl::abi_t<T> const&>(object);
    }

    template <typename T, std::enable_if_t<!std::is_base_of_v<Microsoft::Windows::Sdk::IUnknown, T>, int> = 0>
    auto put_abi(T& object) noexcept
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            object = {};
        }

        return reinterpret_cast<impl::abi_t<T>*>(&object);
    }

    template <typename T, typename V, std::enable_if_t<!std::is_base_of_v<Microsoft::Windows::Sdk::IUnknown, T>, int> = 0>
    void copy_from_abi(T& object, V&& value)
    {
        object = reinterpret_cast<T const&>(value);
    }

    template <typename T, typename V, std::enable_if_t<!std::is_base_of_v<Microsoft::Windows::Sdk::IUnknown, T>, int> = 0>
    void copy_to_abi(T const& object, V& value)
    {
        reinterpret_cast<T&>(value) = object;
    }

    template <typename T, std::enable_if_t<!std::is_base_of_v<Microsoft::Windows::Sdk::IUnknown, std::decay_t<T>> && !std::is_convertible_v<T, std::wstring_view>, int> = 0>
    auto detach_abi(T&& object)
    {
        impl::abi_t<T> result{};
        reinterpret_cast<T&>(result) = std::move(object);
        return result;
    }

    inline void* get_abi(Microsoft::Windows::Sdk::IUnknown const& object) noexcept
    {
        return *(void**)(&object);
    }

    inline void** put_abi(Microsoft::Windows::Sdk::IUnknown& object) noexcept
    {
        object = nullptr;
        return reinterpret_cast<void**>(&object);
    }

    inline void attach_abi(Microsoft::Windows::Sdk::IUnknown& object, void* value) noexcept
    {
        object = nullptr;
        *put_abi(object) = value;
    }

    inline void* detach_abi(Microsoft::Windows::Sdk::IUnknown& object) noexcept
    {
        void* temp = get_abi(object);
        *reinterpret_cast<void**>(&object) = nullptr;
        return temp;
    }

    inline void* detach_abi(Microsoft::Windows::Sdk::IUnknown&& object) noexcept
    {
        void* temp = get_abi(object);
        *reinterpret_cast<void**>(&object) = nullptr;
        return temp;
    }

    constexpr void* detach_abi(std::nullptr_t) noexcept
    {
        return nullptr;
    }

    inline void copy_from_abi(Microsoft::Windows::Sdk::IUnknown& object, void* value) noexcept
    {
        object = nullptr;

        if (value)
        {
            static_cast<_impl_::unknown_abi*>(value)->AddRef();
            *put_abi(object) = value;
        }
    }

    inline void copy_to_abi(Microsoft::Windows::Sdk::IUnknown const& object, void*& value) noexcept
    {
        WIN32_ASSERT(value == nullptr);
        value = get_abi(object);

        if (value)
        {
            static_cast<_impl_::unknown_abi*>(value)->AddRef();
        }
    }

#ifdef WIN32_IMPL_IUNKNOWN_DEFINED

    inline ::IUnknown* get_unknown(Microsoft::Windows::Sdk::IUnknown const& object) noexcept
    {
        return static_cast<::IUnknown*>(get_abi(object));
    }

#endif
}

WIN32_EXPORT namespace win32::Microsoft::Windows::Sdk
{
    inline bool operator==(IUnknown const& left, IUnknown const& right) noexcept
    {
        if (get_abi(left) == get_abi(right))
        {
            return true;
        }
        if (!left || !right)
        {
            return false;
        }
        return get_abi(left.try_as<IUnknown>()) == get_abi(right.try_as<IUnknown>());
    }

    inline bool operator!=(IUnknown const& left, IUnknown const& right) noexcept
    {
        return !(left == right);
    }

    inline bool operator<(IUnknown const& left, IUnknown const& right) noexcept
    {
        if (get_abi(left) == get_abi(right))
        {
            return false;
        }
        if (!left || !right)
        {
            return get_abi(left) < get_abi(right);
        }
        return get_abi(left.try_as<IUnknown>()) < get_abi(right.try_as<IUnknown>());
    }

    inline bool operator>(IUnknown const& left, IUnknown const& right) noexcept
    {
        return right < left;
    }

    inline bool operator<=(IUnknown const& left, IUnknown const& right) noexcept
    {
        return !(right < left);
    }

    inline bool operator>=(IUnknown const& left, IUnknown const& right) noexcept
    {
        return !(left < right);
    }
}

WIN32_EXPORT namespace win32
{
    template <typename T>
    struct com_ptr
    {
        using type = impl::abi_t<T>;

        com_ptr(std::nullptr_t = nullptr) noexcept {}

        com_ptr(void* ptr, take_ownership_from_abi_t) noexcept : m_ptr(static_cast<type*>(ptr))
        {
        }

        com_ptr(com_ptr const& other) noexcept : m_ptr(other.m_ptr)
        {
            add_ref();
        }

        template <typename U>
        com_ptr(com_ptr<U> const& other) noexcept : m_ptr(other.m_ptr)
        {
            add_ref();
        }

        template <typename U>
        com_ptr(com_ptr<U>&& other) noexcept : m_ptr(std::exchange(other.m_ptr, {}))
        {
        }

        ~com_ptr() noexcept
        {
            release_ref();
        }

        com_ptr& operator=(com_ptr const& other) noexcept
        {
            copy_ref(other.m_ptr);
            return*this;
        }

        com_ptr& operator=(com_ptr&& other) noexcept
        {
            if (this != &other)
            {
                release_ref();
                m_ptr = std::exchange(other.m_ptr, {});
            }

            return*this;
        }

        template <typename U>
        com_ptr& operator=(com_ptr<U> const& other) noexcept
        {
            copy_ref(other.m_ptr);
            return*this;
        }

        template <typename U>
        com_ptr& operator=(com_ptr<U>&& other) noexcept
        {
            release_ref();
            m_ptr = std::exchange(other.m_ptr, {});
            return*this;
        }

        explicit operator bool() const noexcept
        {
            return m_ptr != nullptr;
        }

        auto operator->() const noexcept
        {
            return m_ptr;
        }

        T& operator*() const noexcept
        {
            return *m_ptr;
        }

        type* get() const noexcept
        {
            return m_ptr;
        }

        type** put() noexcept
        {
            WINRT_ASSERT(m_ptr == nullptr);
            return &m_ptr;
        }

        void** put_void() noexcept
        {
            return reinterpret_cast<void**>(put());
        }

        void attach(type* value) noexcept
        {
            release_ref();
            *put() = value;
        }

        type* detach() noexcept
        {
            return std::exchange(m_ptr, {});
        }

        friend void swap(com_ptr& left, com_ptr& right) noexcept
        {
            std::swap(left.m_ptr, right.m_ptr);
        }

        template <typename To>
        auto as() const
        {
            return impl::as<To>(m_ptr);
        }

        template <typename To>
        auto try_as() const noexcept
        {
            return impl::try_as<To>(m_ptr);
        }

        template <typename To>
        void as(To& to) const
        {
            to = as<impl::wrapped_type_t<To>>();
        }

        template <typename To>
        bool try_as(To& to) const noexcept
        {
            if constexpr (impl::is_com_interface_v<To> || !std::is_same_v<To, impl::wrapped_type_t<To>>)
            {
                to = try_as<impl::wrapped_type_t<To>>();
                return static_cast<bool>(to);
            }
            else
            {
                auto result = try_as<To>();
                to = result.has_value() ? result.value() : impl::empty_value<To>();
                return result.has_value();
            }
        }

        hresult as(guid const& id, void** result) const noexcept
        {
            return m_ptr->QueryInterface(id, result);
        }

        void copy_from(type* other) noexcept
        {
            copy_ref(other);
        }

        void copy_to(type** other) const noexcept
        {
            add_ref();
            *other = m_ptr;
        }

        template <typename F, typename...Args>
        bool try_capture(F function, Args&&...args)
        {
            return function(args..., guid_of<T>(), put_void()) >= 0;
        }

        template <typename O, typename M, typename...Args>
        bool try_capture(com_ptr<O> const& object, M method, Args&&...args)
        {
            return (object.get()->*(method))(args..., guid_of<T>(), put_void()) >= 0;
        }

        template <typename F, typename...Args>
        void capture(F function, Args&&...args)
        {
            check_hresult(function(args..., guid_of<T>(), put_void()));
        }

        template <typename O, typename M, typename...Args>
        void capture(com_ptr<O> const& object, M method, Args&&...args)
        {
            check_hresult((object.get()->*(method))(args..., guid_of<T>(), put_void()));
        }

    private:

        void copy_ref(type* other) noexcept
        {
            if (m_ptr != other)
            {
                release_ref();
                m_ptr = other;
                add_ref();
            }
        }

        void add_ref() const noexcept
        {
            if (m_ptr)
            {
                const_cast<std::remove_const_t<type>*>(m_ptr)->AddRef();
            }
        }

        void release_ref() noexcept
        {
            if (m_ptr)
            {
                unconditional_release_ref();
            }
        }

        __declspec(noinline) void unconditional_release_ref() noexcept
        {
            std::exchange(m_ptr, {})->Release();
        }

        template <typename U>
        friend struct com_ptr;

        type* m_ptr{};
    };
}
