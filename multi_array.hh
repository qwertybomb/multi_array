//
// Created by qwerty-bomb on 5/25/20.
//
#pragma once
#ifndef TURTLE_MULTI_ARRAY_HH
#define TURTLE_MULTI_ARRAY_HH

#include <memory>
#include <vector>


namespace turtle {
    template<typename T, std::size_t N, bool Opt = false>
    class multi_array {
        /* a struct for holding the array sizes*/
        template<std::size_t SizeCount>
        struct size_list {
            std::size_t data[SizeCount];
        };
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

        template<typename Size, typename... Sizes, std::enable_if_t<sizeof...(Sizes)<N,int> = 0>
        explicit multi_array(const Size &size, const Sizes &... sizes) {
            resize_data(size, sizes...);
        }
       multi_array& operator=(const std::initializer_list<T>& list) {
            if constexpr (Opt) {
                //clear array
                std::fill(begin(),end(),T());
                std::copy(list.begin(),list.end(),begin());
                return *this;
            } else {
                static_assert(Opt,"Parent array cannot be set to initializer_list");
            }
        }
      
        /*Element access*/
        reference at(const size_type &index) { return data_[index]; }

        const_reference at(const size_type &index) const { return (*this)[index]; }

        template<typename Index, typename ... Indices, std::enable_if_t<sizeof...(Indices)<N,int> = 0>
        reference operator()(const Index &index, const Indices &... indices) {
            return data_[this->index(index, indices...)];
        }

        template<typename Index, typename ... Indices, std::enable_if_t<sizeof...(Indices)<N,int> = 0>
        const_reference operator()(const Index &index, const Indices &... indices) const {
            return data_[this->index(index, indices...)];
        }

        decltype(auto) operator[](const size_type &index) {
            if constexpr(N > 1) {
                auto temp = multi_array<T, N - 1, true>();
                temp.size_ = size_ / sizes_.data[0];
                temp.data_ = (begin() + temp.size_ * index);
                //copy sizes
                std::copy(sizes_.data + 1, sizes_.data + N, temp.sizes_.data);
                return temp;
            } else {
                //force it to return a reference
                return make_reference(data_[index]);
            }
        }

        reference front() { return *begin(); }

        const_reference front() const { return front(); }

        reference back() { return *(end() - 1); }

        const_reference back() const { return back(); }

        T *data() noexcept { return &data_[0]; }

        const T *data() const noexcept { return data(); }


        //Iterators
        iterator begin() noexcept { return &data_[0]; }

        iterator end() noexcept { return &data_[0] + size_; }

        iterator begin() const noexcept { return begin(); }

        iterator end() const noexcept { return end(); }

        iterator cbegin() const noexcept { return begin(); }

        iterator cend() const noexcept { return end(); }

        reverse_iterator rbegin() noexcept { return reverse_iterator(begin()); }

        reverse_iterator rend() noexcept { return reverse_iterator(end()); }

        reverse_iterator rbegin() const noexcept { return rbegin(); }

        reverse_iterator rend() const noexcept { return rend(); }

        reverse_iterator crbegin() const noexcept { return rbegin(); }

        reverse_iterator crend() const noexcept { return rend(); }
        //Capacity
        template<typename Size, typename... Sizes, std::enable_if_t<sizeof...(Sizes)<N,int> = 0>
        void resize(const Size& size, const Sizes&... sizes) {
            resize_data(size,sizes...);
        }

        [[nodiscard]] bool empty() const noexcept { return !size_;}
        size_type  size() const noexcept { return size_;}
        size_type max_size() const noexcept { return std::numeric_limits<size_type>::max()/sizeof(T);}

        //function for indexing
        template<typename Index, typename ... Indices, std::enable_if_t<sizeof...(Indices)<N,int> = 0>
        size_t index(const Index &index, const Indices &... indices) const {
            return get_index(0,multiply_sizes(0), index, indices...);
        }
        //Operations
        void fill(const T& value) {
            std::fill(begin(),end(),value);
        }

        void swap(std::conditional_t<Opt,multi_array,multi_array&> other) noexcept {
            /*from swap_ranges*/
            iterator first1 = data();
            iterator first2 = other.data();
            while (first1 != end()) {
                std::swap(*first1, *first2);
                ++first1; ++first2;
            }
        }
    private:

        /*store the dimensions for indexing*/
        size_list<N> sizes_;
        std::conditional_t<Opt, T *, std::vector<T>> data_;
        size_type size_ = 0;

        template<typename... Sizes>
        void resize_data(const Sizes &... sizes) {
            sizes_ = {static_cast<size_type>(sizes)...}; //store sizes
            data_.resize(size_ = (sizes*...)); //use fold expressions to get total size
        }

        size_type multiply_sizes(size_type pos) const {
            size_type to_return{1};
            while (pos < N)
                to_return *= sizes_.data[pos++];
            return to_return;
        }

        //helper function for indexing
        template<typename Index, typename ... Indices>
        std::size_t get_index(const size_type& size_index,const size_type& size, const Index &index, const Indices &... indices) const {
            //x + y*i + z*i*j + w*i*j*k ...
            size_type new_index = index * (size/sizes_.data[size_index]);
            if constexpr(sizeof...(indices)) {
                return get_index(size_index + 1,size/sizes_.data[size_index], indices...) + new_index;
            }
            return new_index;
        }
        template<typename U>
        U &make_reference(U &arg) { return arg; }
    };
}


#endif //TURTLE_MULTI_ARRAY_HH
