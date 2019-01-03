#ifndef __VDS_CORE_EXPECTED_H_
#define __VDS_CORE_EXPECTED_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <exception>
#include <memory>

namespace vds {

    class unexpected {
    public:
        unexpected(std::unique_ptr<std::exception> && error)
        : error_(std::move(error)){
        }

        const std::unique_ptr<std::exception> & error() const {
            return this->error_;
        }

        std::unique_ptr<std::exception> & error() {
            return this->error_;
        }

    private:
        std::unique_ptr<std::exception> error_;
    };


    template <typename error_type, class... _Args>
    inline unexpected make_unexpected(_Args&&... __args){
        return std::make_unique<error_type>(_VSTD::forward<_Args>(__args)...);
    }

    template <typename value_type>
    class [[nodiscard]] expected {
    public:
        expected(value_type && value)
        : value_holder_(std::make_unique<result_holder>(std::move(value))){
        }

        expected(unexpected && error)
                : value_holder_(std::make_unique<error_holder>(std::move(error.error()))){
        }

        expected(std::exception_ptr && error)
                : value_holder_(std::make_unique<exception_holder>(std::move(error))){
        }

        operator bool () const {
            return this->value_holder_->is_value();
        }

        bool operator !() const {
            return !this->value_holder_->is_value();
        }

        const value_type & value() const {
            return this->value_holder_->value();
        }

        value_type & value() {
            return this->value_holder_->value();
        }

        const std::unique_ptr<std::exception> & error() const {
            return this->value_holder_->error();
        }

        std::unique_ptr<std::exception> & error() {
            return this->value_holder_->error();
        }

    private:
        class value_holder {
        public:
            virtual ~value_holder() {}

            virtual bool is_value() const = 0;

            virtual const value_type & value() const = 0;
            virtual value_type & value() = 0;

            virtual const std::unique_ptr<std::exception> & error() const = 0;
            virtual std::unique_ptr<std::exception> & error() = 0;
        };

        std::unique_ptr<value_holder> value_holder_;

        class result_holder : public value_holder {
        public:
            result_holder(value_type && value)
            : value_(std::move(value)) {
            }

            virtual ~result_holder() {}

            bool is_value() const override{
                return true;
            }

            const value_type & value() const override {
                return this->value_;
            }

            value_type & value() override {
                return this->value_;
            }

            const std::unique_ptr<std::exception> & error() const override {
                throw std::runtime_error("Unexpected call expected::error")
            }
            std::unique_ptr<std::exception> & error() override {
                throw std::runtime_error("Unexpected call expected::error")
            }

        private:
            value_type value_;
        };

        class error_holder : public value_holder {
        public:
            error_holder(std::unique_ptr<std::exception> && error)
                    : error_(std::move(error)){
            }

            virtual ~error_holder() {}

            bool is_value() const override{
                return false;
            }

            const value_type & value() const override {
                throw std::runtime_error("Unexpected call expected::value");
            }

            value_type & value() override {
                throw std::runtime_error("Unexpected call expected::value");
            }

            const std::unique_ptr<std::exception> & error() const override {
                return this->error_;
            }
            std::unique_ptr<std::exception> & error() override {
                return this->error_;
            }

        private:
            std::unique_ptr<std::exception> error_;
        };

        class exception_holder : public value_holder {
        public:
            exception_holder(std::exception_ptr && error)
                    : error_(std::move(error)){
            }

            virtual ~exception_holder() {}

            bool is_value() const override{
                return false;
            }

            const value_type & value() const override {
                throw std::runtime_error("Unexpected call expected::value");
            }

            value_type & value() override {
                throw std::runtime_error("Unexpected call expected::value");
            }

            const std::unique_ptr<std::exception> & error() const override {
                std::rethrow_exception(this->error_);
            }

            std::unique_ptr<std::exception> & error() override {
                std::rethrow_exception(this->error_);
            }

        private:
            std::exception_ptr error_;
        };
    };

    template <>
    class [[nodiscard]] expected<void> {
    public:
        expected()
                : value_holder_(std::make_unique<result_holder>()){
        }

        expected(unexpected && error)
                : value_holder_(std::make_unique<error_holder>(std::move(error.error()))){
        }

        expected(std::exception_ptr && error)
                : value_holder_(std::make_unique<exception_holder>(std::move(error))){
        }

        operator bool () const {
            return this->value_holder_->is_value();
        }

        bool operator !() const {
            return !this->value_holder_->is_value();
        }

        void value() const {
            if(!this->value_holder_->is_value()){
                throw std::runtime_error("Unexpected call expected::value");
            }
        }

        void value() {
            if(!this->value_holder_->is_value()){
                throw std::runtime_error("Unexpected call expected::value");
            }
        }

        const std::unique_ptr<std::exception> & error() const {
            return this->value_holder_->error();
        }

        std::unique_ptr<std::exception> & error() {
            return this->value_holder_->error();
        }

    private:
        class value_holder {
        public:
            virtual ~value_holder() {}

            virtual bool is_value() const = 0;

            virtual const std::unique_ptr<std::exception> & error() const = 0;
            virtual std::unique_ptr<std::exception> & error() = 0;
        };

        std::unique_ptr<value_holder> value_holder_;

        class result_holder : public value_holder {
        public:
            result_holder() {
            }

            virtual ~result_holder() {}

            bool is_value() const override{
                return true;
            }

            const std::unique_ptr<std::exception> & error() const override {
                throw std::runtime_error("Unexpected call expected::error");
            }
            std::unique_ptr<std::exception> & error() override {
                throw std::runtime_error("Unexpected call expected::error");
            }
        };

        class error_holder : public value_holder {
        public:
            error_holder(std::unique_ptr<std::exception> && error)
                    : error_(std::move(error)){
            }

            virtual ~error_holder() {}

            bool is_value() const override{
                return false;
            }

            const std::unique_ptr<std::exception> & error() const override {
                return this->error_;
            }

            std::unique_ptr<std::exception> & error() override {
                return this->error_;
            }

        private:
            std::unique_ptr<std::exception> error_;
        };

        class exception_holder : public value_holder {
        public:
            exception_holder(std::exception_ptr && error)
                    : error_(std::move(error)){
            }

            virtual ~exception_holder() {}

            bool is_value() const override{
                return false;
            }

            const std::unique_ptr<std::exception> & error() const override {
                std::rethrow_exception(this->error_);
            }

            std::unique_ptr<std::exception> & error() override {
                std::rethrow_exception(this->error_);
            }

        private:
            std::exception_ptr error_;
        };
    };
}

#endif //__VDS_CORE_EXPECTED_H_
