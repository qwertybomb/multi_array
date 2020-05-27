//
// Created by qwerty-bomb on 5/25/20.
//
#pragma once
#ifndef TURTLE_MULTI_ARRAY_HH
#define TURTLE_MULTI_ARRAY_HH

#include <memory>
#include <vector>
#include <cassert>

namespace turtle {
    namespace detail {
        /* a struct for holding the array sizes*/
        template<std::size_t SizeCount>
        struct size_list {
            std::size_t data[SizeCount];
        };
    }
    template<typename T, std::size_t N, bool Opt = false>
    class multi_array {
        //allow different template instances to access private members of other template instances
        template<class A, std::size_t B, bool C>
        friend
        class multi_array;

    public:
        using value_type             = T;
        using pointer                = T *;
        using const_pointer          = const T *;
        using reference              = T &;
        using const_reference        = const T &;
        using size_type              = std::size_t;
        using difference_type        = std::ptrdiff_t;
        using iterator               = T *;
        using const_iterator         = const iterator;
        using reverse_iterator       = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;


        multi_array() = default;

        ~multi_array() { if (data_ && owner_) delete[] data_; }

        template<typename... Sizes, std::enable_if_t<sizeof...(Sizes) < N, int> = 0>
        explicit multi_array(const size_type &size, const Sizes &... sizes) {
            resize_data(size, sizes...);
        }

        template<bool OtherOpt>
        multi_array(const multi_array<T, N + 1, OtherOpt> &other, size_type pos) {
            size_ = other.size_ / other.sizes_.data[0];
            owner_ = false;
            data_ = (other.data_ + size_ * pos);
            std::copy(other.sizes_.data + 1, other.sizes_.data + N, sizes_.data);
        }

        multi_array(const multi_array<T,N,Opt>& other) : size_(other.size_), sizes_(other.sizes_) {
            if(other.empty()) { return ;}  //if other is empty don't do anything
            data_ = new T[size_];
            std::uninitialized_copy(other.begin(),other.end(),data_); //copy data
        }
        multi_array(const multi_array<T,N,!Opt>& other) : size_(other.size_), sizes_(other.sizes_) {
            if(other.empty()) { return ;}  //if other is empty don't do anything
            data_ = new T[size_];
            std::uninitialized_copy(other.begin(),other.end(),data_); //copy data
        }
        multi_array &operator=(const multi_array& other) {
              auto temp = multi_array(other);
              this->swap(temp);
              return *this;
        }
        multi_array &operator=(const std::initializer_list<T> &list) {
            if constexpr (Opt) {
                //clear array
                std::fill(begin(), end(), T());
                std::copy(list.begin(), list.end(), begin());
                return *this;
            } else {
                static_assert(Opt, "Parent array cannot be set to initializer_list");
            }
        }

        /*Element access*/
        reference at(const size_type &index) { return data_[index]; }

        const_reference at(const size_type &index) const { return data_[index]; }

        template<typename Index, typename ... Indices, std::enable_if_t<sizeof...(Indices) < N, int> = 0>
        decltype(auto) operator()(const Index &index, const Indices &... indices) {
            if constexpr(sizeof...(indices) == N - 1) {
                return data_[this->index(index, indices...)];
            } else if constexpr(!sizeof...(indices)) {
                return multi_array<T, N - 1, true>(*this, index);
            } else {
                return multi_array<T, N - 1, true>(*this, index)(indices...);
            }
        }

        template<typename Index, typename ... Indices, std::enable_if_t<sizeof...(Indices) < N, int> = 0>
        decltype(auto) operator()(const Index &index, const Indices &... indices) const {
            return (*this)(index, indices...);
        };

        decltype(auto) operator[](const size_type &index) {
            if constexpr(N > 1) {
                return multi_array<T, N - 1, true> (*this, index);
            } else {
                //force it to return a reference
                return data_[index];
            }
        }

        decltype(auto) operator[](const size_type &index) const {
            return (*this)[index];
        }

        reference front() { return data_[0]; }

        const_reference front() const { return data_[0]; }

        reference back() { return data_[size_ - 1]; }

        const_reference back() const { return data_[size_ - 1]; }

        T *data() noexcept { return data_; }

        const T *data() const noexcept { return data_; }


        //Iterators
        iterator begin() noexcept { return data_; }

        iterator end() noexcept { return data_ + size_; }

        const_iterator begin() const noexcept { return data_; }

        const_iterator end() const noexcept { return data_ + size_; }

        const_iterator cbegin() const noexcept { return begin(); }

        iterator cend() const noexcept { return end(); }

        reverse_iterator rbegin() noexcept { return reverse_iterator(begin()); }

        reverse_iterator rend() noexcept { return reverse_iterator(end()); }

        //Capacity
        template<typename Size, typename... Sizes, std::enable_if_t<sizeof...(Sizes) < N, int> = 0>
        void resize(const Size &size, const Sizes &... sizes) {
            assert(("cannot resize non owner",owner_));
            resize_data(size, sizes...);
        }

        bool empty() const noexcept { return !size_; }

        size_type size() const noexcept { return size_; }

        size_type max_size() const noexcept { return std::numeric_limits<size_type>::max() / sizeof(T); }

        //function for indexing
        template<typename Index, typename ... Indices, std::enable_if_t<sizeof...(Indices) < N, int> = 0>
        size_t index(const Index &index, const Indices &... indices) const {
            return get_index(0, multiply_sizes(0), index, indices...);
        }

        //Operations
        void fill(const T &value) {
            std::fill(begin(), end(), value);
        }

        //if the arrays have the same Opt
        template<typename _T, std::size_t _N, bool _Opt>
        friend void swap(multi_array<_T, _N, _Opt> &lhs, multi_array<_T, _N, _Opt> &rhs) noexcept;

        //if the arrays a different Opt
        template<typename _T, std::size_t _N, bool _Opt>
        friend void swap(multi_array<_T, _N, _Opt> &lhs, multi_array<_T, _N, !_Opt> &rhs) noexcept;

        template<typename _T, std::size_t _N, bool _Opt1, bool _Opt2>
        friend void swap(multi_array<_T, _N, _Opt1> &&lhs, multi_array<_T, _N, _Opt2> &&rhs) noexcept;

        template<typename _T, std::size_t _N, bool _Opt1, bool _Opt2>
        friend void swap(multi_array<_T, _N, _Opt1> &&lhs, multi_array<_T, _N, _Opt2> &rhs) noexcept;

        template<typename _T, std::size_t _N, bool _Opt1, bool _Opt2>
        friend void swap(multi_array<_T, _N, _Opt1> &lhs, multi_array<_T, _N, _Opt2> &&rhs) noexcept;

        template<typename _T, std::size_t _N, bool _Opt>
        void swap(multi_array<_T, _N, _Opt> &&rhs) noexcept;

        template<typename _T, std::size_t _N, bool _Opt>
        void swap(multi_array<_T, _N, _Opt> &rhs) noexcept;

    private:

        /*store the dimensions for indexing*/
        detail::size_list<N> sizes_;
        T *data_;
        size_type size_ = 0;
        bool owner_ = true;

        template<typename... Sizes>
        void resize_data(const Sizes &... sizes) {
            sizes_ = {static_cast<size_type>(sizes)...}; //store sizes
            data_ = (new T[size_ = (sizes*...)]); //use fold expressions to get total size
        }

        size_type multiply_sizes(size_type pos) const {
            size_type to_return{1};
            while (pos < N)
                to_return *= sizes_.data[pos++];
            return to_return;
        }

        //helper function for indexing
        template<typename Index, typename ... Indices>
        std::size_t get_index(const size_type &size_index, const size_type &size, const Index &index,
                              const Indices &... indices) const {
            //x + y*i + z*i*j + w*i*j*k ...
            size_type new_index = index * (size / sizes_.data[size_index]);
            if constexpr(sizeof...(indices)) {
                return get_index(size_index + 1, size / sizes_.data[size_index], indices...) + new_index;
            }
            return new_index;
        }

    };

    template<typename _T, size_t _N, bool _Opt>
    void swap(multi_array<_T, _N, _Opt> &lhs, multi_array<_T, _N, _Opt> &rhs) noexcept {

        bool both_owners = !_Opt && lhs.owner_ && rhs.owner_;
        if (both_owners) {
            std::swap(lhs.data_, rhs.data_); //both are true owners
            std::swap(lhs.size_,rhs.size_);
            std::swap(lhs.sizes_,rhs.sizes_);
        } else {
            assert(("array sizes must be the same",other.size() == size_));
            for (std::size_t i = 0; i < lhs.size_; ++i) {
                std::swap(lhs.data_[i], rhs.data_[i]); //at least one is not an owner
            }
        }
    }

    template<typename _T, size_t _N, bool _Opt>
    void swap(multi_array<_T, _N, _Opt> &lhs, multi_array<_T, _N, !_Opt> &rhs) noexcept {
        assert(("array sizes must be the same",other.size() == size_));
        for (std::size_t i = 0; i < lhs.size_; ++i) {
            std::swap(lhs.data_[i], rhs.data_[i]);
        }
    }

    template<typename _T, size_t _N, bool _Opt1, bool _Opt2>
    void swap(multi_array<_T, _N, _Opt1> &&lhs, multi_array<_T, _N, _Opt2> &&rhs) noexcept {
        swap(lhs, rhs);
    }

    template<typename _T, size_t _N, bool _Opt1, bool _Opt2>
    void swap(multi_array<_T, _N, _Opt1> &&lhs, multi_array<_T, _N, _Opt2> &rhs) noexcept {
        swap(lhs, rhs);
    }

    template<typename _T, size_t _N, bool _Opt1, bool _Opt2>
    void swap(multi_array<_T, _N, _Opt1> &lhs, multi_array<_T, _N, _Opt2> &&rhs) noexcept {
        swap(lhs, rhs);
    }

    template<typename T, size_t N, bool Opt>
    template<typename _T, size_t _N, bool _Opt>
    void multi_array<T, N, Opt>::swap(multi_array<_T, _N, _Opt> &&rhs) noexcept {
        turtle::swap(*this, rhs);
    }

    template<typename T, size_t N, bool Opt>
    template<typename _T, size_t _N, bool _Opt>
    void multi_array<T, N, Opt>::swap(multi_array<_T, _N, _Opt> &rhs) noexcept {
        turtle::swap(*this, rhs);
    }
}


#endif //TURTLE_MULTI_ARRAY_HH
