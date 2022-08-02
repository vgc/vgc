// Copyright 2022 The VGC Developers
// See the COPYRIGHT file at the top-level directory of this distribution
// and at https://github.com/vgc/vgc/blob/master/COPYRIGHT
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef VGC_GRAPHICS_RESOURCE_H
#define VGC_GRAPHICS_RESOURCE_H

#include <atomic>
#include <mutex>
#include <unordered_set>
#include <utility>

#include <vgc/core/array.h>
#include <vgc/core/arithmetic.h>
#include <vgc/core/object.h>
#include <vgc/graphics/api.h>
#include <vgc/graphics/logcategories.h>

namespace vgc::graphics {

VGC_DECLARE_OBJECT(Engine);

class Resource;

// XXX 
/*

Before a graphics::Resource can be deleted, its underlying-api-specific
object must be released on the rendering thread.
Some engine implementations need to extend the lifetime of resources and this
implies concurrent reference counting. To minimize synchronization points we use
two different counts, one for the user thread, and another for the
rendering thread.

When the engine is stopped we can release them all. It is important if the engine is a temporary
wrapper (e.g. around Qt OpenGL), better not leak resources.

*/

namespace detail {

// ResourceRegistry is used for the garbage collection of resources.
class VGC_GRAPHICS_API ResourceRegistry {
public:
    ResourceRegistry() = default;

    // must be called from a thread in which we can release resources
    void releaseAndDeleteGarbagedResources(Engine* engine);

    // must be called from a thread in which we can release resources
    void releaseAllResources(Engine* engine);

private:
    friend Resource;

    ~ResourceRegistry() = default;

    void registerResource_(Resource* resource)
    {
        VGC_CORE_ASSERT(releasedByEngine_ == false);
        {
            std::lock_guard<std::mutex> lg(mutex_);
            resources_.insert(resource);
        }
        ++refCount_;
    }

    void garbageResource_(Resource* resource)
    {
        {
            std::lock_guard<std::mutex> lg(mutex_);
            if (!resources_.erase(resource)) {
                return;
            }
            garbagedResources_.append(resource);
        }
        decRef_();
    }

    void addRef_()
    {
        ++refCount_;
    }

    // can end up deleting the registry
    void decRef_();

    // It is the list of all resources created by the engine that owns this
    // registry and that are still referenced.
    // XXX could use another container, intrusive list ?
    //
    std::unordered_set<Resource*> resources_;

    // It is a list of all resources that are no longer referenced and thus
    // should be released, destroyed and deleted.
    //
    core::Array<Resource*> garbagedResources_;

    std::mutex mutex_;
    std::atomic<Int64> refCount_ = 1;
    std::atomic<bool> releasedByEngine_ = false;
};

} // namespace detail

/// \class vgc::graphics::Resource
/// \brief Abstract graphics resource.
///
class VGC_GRAPHICS_API Resource {
protected:
    using ResourceRegistry = detail::ResourceRegistry;

    // Should be called in the user thread, not the rendering thread.
    explicit Resource(ResourceRegistry* registry)
        : registry_(registry)
    {
        registry->registerResource_(this);
    }

    static constexpr Int64 uninitializedCountValue_ = core::Int64Min;

    static_assert(uninitializedCountValue_ < 0);

public:
    Resource(const Resource&) = delete;
    Resource& operator=(const Resource&) = delete;

    // XXX needed ?
    //template<typename T>
    //ResourcePtr<T> sharedFromThis();

protected:
    friend detail::ResourceRegistry;
    friend class Engine;

    template<typename T>
    friend class ResourcePtr;

    // Resources should only be destroyed by their registry.
    virtual ~Resource() {
#ifdef VGC_DEBUG
        if (!released_) {
            VGC_ERROR(LogVgcGraphics, "A resource has not been released before destruction");
        }
#endif
    }

    // This function is called when the resource is being garbaged.
    // You must override it to reset all inner ResourcePtr.
    //
    virtual void releaseSubResources_() {}

    // This function is called only in the rendering thread.
    // You must override it to release the actual underlying data and objects.
    //
    virtual void release_(Engine*) {
#ifdef VGC_DEBUG
        released_ = true;
#endif
    }

private:
    void initRef_()
    {
        int64_t uninitializedValue = uninitializedCountValue_;
        if (refCount_ == uninitializedValue) {
            refCount_ = 1;
        }
        else {
            throw core::LogicError("Resource: reference count already initialized.");
        }
    }

    void incRef_()
    { 
#ifdef VGC_DEBUG
        if (refCount_ <= 0) {
            throw core::LogicError(
                "Resource: trying to take shared ownership of an already garbaged resource.");
        }
#endif
        ++refCount_;
    }

    void decRef_()
    {
        if (--refCount_ == 0) {
            releaseSubResources_();
            registry_->garbageResource_(this);
        }
    }

    ResourceRegistry* registry_;
    std::atomic<Int64> refCount_ = uninitializedCountValue_;
#ifdef VGC_DEBUG
    bool released_ = false;
#endif

    // If this assert does not pass, we can use a boolean..
    static_assert(std::atomic<Int64>::is_always_lock_free);
};

namespace detail {

inline void ResourceRegistry::releaseAndDeleteGarbagedResources(Engine* engine)
{
    std::lock_guard<std::mutex> lg(mutex_);
    for (Resource* r : garbagedResources_) {
        r->release_(engine);
        delete r;
    }
    garbagedResources_.clear();
}

inline void ResourceRegistry::releaseAllResources(Engine* engine)
{
    if (releasedByEngine_.exchange(true)) {
        VGC_WARNING(LogVgcGraphics, "Trying to release a ResourceRegistry more than once.");
        return;
    }
    {
        std::lock_guard<std::mutex> lg(mutex_);
        for (Resource* r : resources_) {
            r->release_(engine);
        }
        for (Resource* r : garbagedResources_) {
            r->release_(engine);
            delete r;
        }
        garbagedResources_.clear();
    }
    decRef_();
}

inline void ResourceRegistry::decRef_()
{
    if (--refCount_ == 0) {
        VGC_CORE_ASSERT(resources_.empty());
        for (Resource* r : garbagedResources_) {
            delete r;
        }
        delete this;
    }
}

template<typename T>
class SmartPtrBase_ {
protected:
    ~SmartPtrBase_() = default;

    constexpr SmartPtrBase_(T* p) noexcept
        : p_(p) {
    }

public:
    constexpr SmartPtrBase_() noexcept
        : p_(nullptr) {
    }

    constexpr SmartPtrBase_(std::nullptr_t) noexcept
        : p_(nullptr) {
    }

    T* get() const {
        return p_;
    }

    template<typename U>
    U* get_static_cast() const {
        return static_cast<U*>(p_);
    }

    explicit operator bool() const noexcept
    {
        return p_ != nullptr;
    }

    T& operator*() const noexcept
    {
        return *p_;
    }

    T* operator->() const noexcept
    {
        return p_;
    }

protected:
    T* p_ = nullptr;
};

template<typename T, typename U>
bool operator==(const SmartPtrBase_<T>& lhs, const SmartPtrBase_<U>& rhs) noexcept
{
    return lhs.get() == rhs.get();
}

template<typename T, typename U>
bool operator!=(const SmartPtrBase_<T>& lhs, const SmartPtrBase_<U>& rhs) noexcept
{
    return lhs.get() != rhs.get();
}

} // namespace detail

/// \class vgc::graphics::ResourcePtr<T>
/// \brief Shared pointer to a graphics `Resource`.
///
/// When the reference count reaches zero, the resource gets queued for release
/// and destruction in the rendering thread by the Engine that created it.
///
template<typename T>
class ResourcePtr : public detail::SmartPtrBase_<T> {
protected:
    using Base = detail::SmartPtrBase_<T>;

    template<typename U>
    static constexpr bool isCompatible_ = std::is_convertible_v<U*, T*>;

    template<typename S, typename U>
    friend ResourcePtr<S> static_pointer_cast(const ResourcePtr<U>& r) noexcept;

    struct CastTag {};

    // For casts
    ResourcePtr(T* p, CastTag)
        : Base(p)
    {
        if (p_) {
            p_->incRef_();
        }
    }

public:
    template<typename U>
    friend class ResourcePtr;

    static_assert(std::is_base_of_v<Resource, T>);

    using Base::Base;

    explicit ResourcePtr(T* p)
        : Base(p)
    {
        if (p_) {
            p_->initRef_();
        }
    }

    ~ResourcePtr() {
        reset();
    }

    template<typename U, VGC_REQUIRES(isCompatible_<U>)>
    ResourcePtr(const ResourcePtr<U>& other)
        : Base(other.p_)
    {
        if (p_) {
            p_->incRef_();
        }
    }

    template<typename U, VGC_REQUIRES(isCompatible_<U>)>
    ResourcePtr(ResourcePtr<U>&& other) noexcept
        : Base(other.p_)
    {
        other.p_ = nullptr;
    }

    ResourcePtr(const ResourcePtr& other)
        : Base(other.p_)
    {
        if (p_) {
            p_->incRef_();
        }
    }

    ResourcePtr(ResourcePtr&& other) noexcept
        : Base(other.p_)
    {
        other.p_ = nullptr;
    }

    template<typename U>
    ResourcePtr& operator=(const ResourcePtr<U>& other)
    {
        if (p_ != other.p_) {
            if (other.p_) {
                other.p_->incRef_();
            }
            if (p_) {
                p_->decRef_();
            }
            p_ = other.p_;
        }
        return *this;
    }

    ResourcePtr& operator=(const ResourcePtr& other)
    {
        return operator=<T>(other);
    }

    template<typename U>
    ResourcePtr& operator=(ResourcePtr<U>&& other)
    {
        std::swap(p_, other.p_);
        return *this;
    }

    ResourcePtr& operator=(ResourcePtr&& other) noexcept
    {
        std::swap(p_, other.p_);
        return *this;
    }

    void reset()
    {
        if (p_) {
            p_->decRef_();
#ifdef VGC_DEBUG
            p_ = nullptr;
#endif
        }
    }

    void reset(T* p)
    {
        if (p) {
            p->initRef_();
        }
        if (p_) {
            p_->decRef_();
        }
        p_ = p;
    }

    Int64 useCount() const
    {
        return p_ ? p_->useCount() : 0;
    }
};

template<typename T, typename U>
ResourcePtr<T> static_pointer_cast(const ResourcePtr<U>& r) noexcept
{
    return ResourcePtr<T>(static_cast<T*>(r.get()), typename ResourcePtr<T>::CastTag{});
}

} // namespace vgc::graphics

#endif // VGC_GRAPHICS_RESOURCE_H
