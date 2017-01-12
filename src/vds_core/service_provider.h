#ifndef __VDS_CORE_SERVICE_PROVIDER_H_
#define __VDS_CORE_SERVICE_PROVIDER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <typeinfo>
#include <memory>
#include <functional>
#include <map>
#include <stack>
#include <list>
#include <mutex>
#include "shutdown_event.h"
#include "string_format.h"
#include "types.h"

namespace vds {
    class service_provider
    {
    public:
        template <typename interface_type>
        interface_type get() const;

        template <typename interface_type>
        bool enum_collection(const std::function<bool(interface_type)> & visiter, bool throwIfEmpty = true) const;

        template <typename interface_type>
        std::list<interface_type> get_collection(bool throwIfEmpty = true)  const;

        service_provider create_scope() const;

        void on_complete(const std::function<void(void)> & done) const;

        const shutdown_event & get_shutdown_event() const;

    private:
        friend class iservice_provider_impl;
        friend class service_registrator_impl;

        service_provider(class iservice_provider_impl * impl);
        service_provider(const std::shared_ptr<iservice_provider_impl> & impl);

        std::shared_ptr<iservice_provider_impl> impl_;
    };

    class service_registrator;

    class iservice
    {
    public:
        virtual void register_services(service_registrator &) = 0;
        virtual void start(const service_provider &) = 0;
        virtual void stop(const service_provider &) = 0;
    };

    class service_registrator
    {
    public:
        service_registrator();

        template <typename interface_type, typename implement_type>
        void add_transient();

        template <typename interface_type, typename implement_type>
        void add_scoped();

        template <typename interface_type>
        void add_factory(const std::function<interface_type(bool &)> & factory);

        template <typename interface_type>
        void add_collection_factory(const std::function<interface_type()> & factory);

        void add(iservice & service);

        void shutdown();

        service_provider build() const;

    private:
        std::shared_ptr<class service_registrator_impl> impl_;
    };

    //////////////////////////////////////////////////////////////////////////////////
    class iservice_provider_impl : public std::enable_shared_from_this<iservice_provider_impl>
    {
    public:
        iservice_provider_impl();
        ~iservice_provider_impl();

        template <typename interface_type>
        interface_type get();

        template <typename interface_type>
        bool enum_collection(const std::function<bool(interface_type)> & visiter, bool throwIfEmpty = true);

        template <typename interface_type>
        std::list<interface_type> get_collection(bool throwIfEmpty = true);

        service_provider create_scope();

        void on_complete(const std::function<void(void)> & done);
        virtual const shutdown_event & get_shutdown_event() const = 0;

    protected:
        friend class scopped_service_provider;

        class iservice_factory {
        public:
            virtual ~iservice_factory();
        };

        template <typename interface_type>
        class service_factory : public iservice_factory {
        public:
            service_factory(const std::function<interface_type(bool & ) > & factory)
                : factory_(factory) {
            }

            interface_type get(bool & is_scoped) {
                return this->factory_(is_scoped);
            }

        private:
            std::function<interface_type(bool & ) > factory_;
        };

        template <typename interface_type>
        class service_collection_factory : public iservice_factory {
        public:
            void add(const std::function<interface_type() > & factory);
            bool enum_collection(const std::function<bool(interface_type)> & visiter);

        private:
            std::list<std::function<interface_type()>> factories_;
        };

        virtual iservice_factory * get_factory(size_t type) = 0;

        struct iobject_holder
        {
            virtual ~iobject_holder();
        };

        template <typename interface_type>
        struct object_holder : public iobject_holder
        {
            object_holder(const interface_type & v)
                : value(v)
            {
            }

            interface_type value;
        };

        std::recursive_mutex m_;
        std::map<size_t, iobject_holder *> scopped_objects_;
        std::list<std::function<void(void)>> done_handlers_;
    };
    /////////////////////////////////////////////////////////////////////////////////
    class scopped_service_provider : public iservice_provider_impl
    {
    public:
        scopped_service_provider(
          const std::shared_ptr<iservice_provider_impl> & parent);

        const shutdown_event & get_shutdown_event() const override;
    protected:
        iservice_factory * get_factory(size_t type) override;

    private:
        std::shared_ptr<iservice_provider_impl> parent_;
    };

    /////////////////////////////////////////////////////////////////////////////////
    class service_registrator_impl : public iservice_provider_impl
    {
    public:
        service_registrator_impl();
        ~service_registrator_impl();

        template <typename interface_type, typename implement_type>
        void add_transient() {
            this->add_factory<interface_type>([] (bool & is_scoped) {
                is_scoped = false;
                return std::shared_ptr<interface_type>(new implement_type()); });
        }

        template <typename interface_type, typename implement_type>
        void add_scoped() {
            this->add_factory<interface_type>([](bool & is_scoped) {
                is_scoped = true;
                return std::shared_ptr<interface_type>(new implement_type()); });
        }

        template <typename interface_type>
        void add_factory(const std::function<interface_type(bool &)> & factory) {
            this->factory_[types::get_type_id<interface_type>()] = new service_factory<interface_type>(factory);
        }

        template <typename interface_type>
        void add_collection_factory(const std::function<interface_type()> & factory) {
            service_collection_factory<interface_type> * collection;

            auto p = this->factory_.find(types::get_type_id<std::list<interface_type>>());
            if (p == this->factory_.end()) {
                collection = new service_collection_factory<interface_type>();
                this->factory_[types::get_type_id<std::list<interface_type>>()] = collection;
            }
            else {
                collection = static_cast<service_collection_factory<interface_type> *>(p->second);
            }

            collection->add(factory);
        }

        void add(iservice & service);
        void shutdown();

        const shutdown_event & get_shutdown_event() const {
            return this->shutdown_event_;
        }

        service_provider build();

    protected:
        iservice_factory * get_factory(size_t type) override
        {
            auto p = this->factory_.find(type);
            if (p == this->factory_.end()) {
                return nullptr;
            }

            return p->second;
        }

    private:
        std::map<size_t, iservice_factory *> factory_;
        std::list<iservice *> services_;
        shutdown_event shutdown_event_;
    };

    template<typename interface_type>
    inline interface_type service_provider::get() const
    {
        return this->impl_->get<interface_type>();
    }

    template<typename interface_type>
    inline bool service_provider::enum_collection(const std::function<bool(interface_type)>& visiter, bool throwIfEmpty) const
    {
        return this->impl_->enum_collection<interface_type>(visiter, throwIfEmpty);
    }

    template<typename interface_type>
    inline std::list<interface_type> service_provider::get_collection(bool throwIfEmpty) const
    {
        return this->impl_->get_collection<interface_type>(throwIfEmpty);
    }

    template<typename interface_type, typename implement_type>
    inline void service_registrator::add_transient()
    {
        this->impl_->add_transient<interface_type, implement_type>();
    }
    template<typename interface_type, typename implement_type>
    inline void service_registrator::add_scoped()
    {
        return this->impl_->add_scoped<interface_type, implement_type>();
    }
    template<typename interface_type>
    inline void service_registrator::add_factory(const std::function<interface_type(bool &)>& factory)
    {
        this->impl_->add_factory<interface_type>(factory);
    }
    template<typename interface_type>
    inline void service_registrator::add_collection_factory(const std::function<interface_type()>& factory)
    {
        this->impl_->add_collection_factory<interface_type>(factory);
    }
    template<typename interface_type>
    inline interface_type iservice_provider_impl::get()
    {
        std::lock_guard<std::recursive_mutex> lock(this->m_);

        auto type_id = types::get_type_id<interface_type>();
        auto p = this->scopped_objects_.find(type_id);
        if (this->scopped_objects_.end() != p) {
            return static_cast<object_holder<interface_type> *>(p->second)->value;
        }

        auto factory = this->get_factory(types::get_type_id<interface_type>());
        if (nullptr == factory) {
            throw new std::logic_error(
                string_format(
                    "interface %s not found",
                    typeid(interface_type).name()
                ));
        }

        bool is_scopped = false;
        auto result = static_cast<service_factory<interface_type> *>(factory)->get(is_scopped);
        if (is_scopped) {
            this->scopped_objects_[type_id] = new object_holder<interface_type>(result);
        }

        return result;
    }
    template<typename interface_type>
    inline bool iservice_provider_impl::enum_collection(const std::function<bool(interface_type)>& visiter, bool throwIfEmpty)
    {
        auto factory = this->get_factory(types::get_type_id<std::list<interface_type>>());
        if (nullptr == factory) {
            if (throwIfEmpty) {
                throw new std::logic_error(
                    string_format(
                        "collection of interface %s not found",
                        typeid(interface_type).name()
                    ));
            }

            return false;
        }
        else {
            return static_cast<service_collection_factory<interface_type> *>(factory)->enum_collection(visiter);
        }
    }
    template<typename interface_type>
    inline std::list<interface_type> iservice_provider_impl::get_collection(bool throwIfEmpty)
    {
        std::list<interface_type> result;
        this->enum_collection<interface_type>([&result](interface_type value) -> bool {
            result.push_back(value);
            return true;
        }, throwIfEmpty);

        return result;
    }

    template<typename interface_type>
    inline void iservice_provider_impl::service_collection_factory<interface_type>::add(const std::function<interface_type()>& factory)
    {
        this->factories_.push_back(factory);
    }
    template<typename interface_type>
    inline bool iservice_provider_impl::service_collection_factory<interface_type>::enum_collection(const std::function<bool(interface_type)>& visiter)
    {
        for (auto p : this->factories_) {
            if (!visiter(p())) {
                return false;
            }
        }

        return true;
    }
};

#endif // ! __VDS_CORE_SERVICE_PROVIDER_H_


