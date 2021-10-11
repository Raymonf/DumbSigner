#ifndef LDID_HPP
#define LDID_HPP

#include <cstdlib>
#include <map>
#include <set>
#include <sstream>
#include <streambuf>
#include <string>
#include <vector>

namespace ldid {

// I wish Apple cared about providing quality toolchains :/

template <typename Function_>
class Functor;

template <typename Type_, typename... Args_>
class Functor<Type_ (Args_ ...)> {
public:
    virtual Type_ operator ()(Args_ ... args) const = 0;
};

template <typename Function_>
class FunctorImpl;

template <typename Value_, typename Type_, typename... Args_>
class FunctorImpl<Type_ (Value_::*)(Args_ ...) const> : public Functor<Type_ (Args_ ...)> {
private:
    const Value_* value_;

public:
    FunctorImpl(const Value_& value)
        : value_(&value) { }

    virtual Type_ operator ()(Args_ ... args) const
    {
        return (*value_)(args...);
    }
};

template <typename Function_>
FunctorImpl<decltype(&Function_::operator())> fun(const Function_& value)
{
    return value;
}

class __declspec(dllexport) Folder {
public:
    virtual void Save(const std::string& path, bool edit, const void* flag, const Functor<void (std::streambuf&)>& code) = 0;
    virtual bool Look(const std::string& path) const = 0;
    virtual void Open(const std::string& path, const Functor<void (std::streambuf&, size_t, const void*)>& code) const = 0;
    virtual void Find(const std::string& path, const Functor<void (const std::string&)>& code, const Functor<void (const std::string&, const Functor<std::string ()>&)>& link) const = 0;
};

class __declspec(dllexport) DiskFolder : public Folder {
private:
    const std::string path_;
    std::map<std::string, std::string> commit_;

protected:
    std::string Path(const std::string& path) const;

private:
    void Find(const std::string& root, const std::string& base, const Functor<void (const std::string&)>& code, const Functor<void (const std::string&, const Functor<std::string ()>&)>& link) const;

public:
    DiskFolder(const std::string& path);
    ~DiskFolder();

    virtual void Save(const std::string& path, bool edit, const void* flag, const Functor<void (std::streambuf&)>& code);
    virtual bool Look(const std::string& path) const;
    virtual void Open(const std::string& path, const Functor<void (std::streambuf&, size_t, const void*)>& code) const;
    virtual void Find(const std::string& path, const Functor<void (const std::string&)>& code, const Functor<void (const std::string&, const Functor<std::string ()>&)>& link) const;
};

class __declspec(dllexport) SubFolder : public Folder {
private:
    Folder& parent_;
    std::string path_;

public:
    SubFolder(Folder& parent, const std::string& path);

    virtual void Save(const std::string& path, bool edit, const void* flag, const Functor<void (std::streambuf&)>& code);
    virtual bool Look(const std::string& path) const;
    virtual void Open(const std::string& path, const Functor<void (std::streambuf&, size_t, const void*)>& code) const;
    virtual void Find(const std::string& path, const Functor<void (const std::string&)>& code, const Functor<void (const std::string&, const Functor<std::string ()>&)>& link) const;
};

struct __declspec(dllexport) Hash {
    uint8_t sha1_[0x14];
    uint8_t sha256_[0x20];
};

struct __declspec(dllexport) Bundle {
    std::string path;
    Hash hash;
};

__declspec(dllexport) Bundle Sign(const std::string& root, Folder& folder, const std::string& key, const std::string& requirement, const Functor<std::string (const std::string&, const std::string&)>& alter, const Functor<void (const std::string&)>& progress, const Functor<void (double)>& percent);

typedef std::map<uint32_t, Hash> Slots;

Hash Sign(const void* idata, size_t isize, std::streambuf& output, const std::string& identifier, const std::string& entitlements, const std::string& requirement, const std::string& key, const Slots& slots, const Functor<void (double)>& percent);

__declspec(dllexport) std::string Entitlements(std::string path);
}

#endif//LDID_HPP
