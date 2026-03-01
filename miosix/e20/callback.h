/***************************************************************************
 *   Copyright (C) 2012-2023 by Terraneo Federico                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/ 

#pragma once

#include <cstdint>

namespace miosix {

/**
 * This class contains all the code for Callback that
 * does not depend on the template parameter N. 
 */
class CallbackBase
{
protected:
    /**
     * Possible operations performed by TypeDependentOperation::operation()
     */
    enum Op
    {
        CALL,
        MOVE,
        DESTROY
    };
    /**
     * This class is part of the any idiom used by Callback.
     */
    template<typename T>
    class TypeDependentOperation
    {
    public:
        /**
         * Perform the type-dependent operations
         * \param a storage for the any object, stores the function object
         * \param b storage for the source object for the move constructor
         * \param op operation
         */
        static void operation(int32_t *a, int32_t *b, Op op)
        {
            T *o1=reinterpret_cast<T*>(a);
            T *o2=reinterpret_cast<T*>(b);
            switch(op)
            {
                case CALL:
                    (*o1)();
                    break;
                case MOVE:
                    new (o1) T(std::move(*o2));
                    break;
                case DESTROY:
                    o1->~T();
                    break;
            }
        }
    };
};

/**
 * A Callback works just like an std::function, but has some additional
 * <b>limitations</b>. First, it can only accept function objects that take void
 * as a parameter and return void. Second, if the size of the
 * implementation-defined type returned by bind is larger than N, a
 * compile-time error is generated. Also, calling an empty Callback does
 * nothing, while doing the same on a function results in an exception
 * being thrown.
 * 
 * The reason why one would want to use this class is because, other than the
 * limitations, this class also offers a guarantee: it will never allocate
 * data on the heap. It is not just a matter of code speed: in Miosix calling
 * new/delete/malloc/free from an interrupt routine (or with the GlobalIrqLock
 * taken) produces undefined behavior or even a deadlock, so this class enables
 * safely binding function calls from interrupt context.
 * 
 * \param N the size in bytes that an instance of this class reserves to
 * store the function objects. If the assert `sizeof(any)>=sizeof(T)'
 * starts failing it means it is time to increase this number. The size
 * of an instance of this object is N+sizeof(void (*)()), but with N rounded
 * by excess to four byte boundaries.
 */
template<unsigned N>
class Callback : private CallbackBase
{
public:
    /**
     * Default constructor. Produces an empty callback.
     */
    Callback() : operation(nullptr) {}

    /**
     * Deleted copy constructor. Callback cannot be copied, because doing that
     * may cause memory allocations.
     */
    Callback(Callback& rhs) = delete;

    /**
     * Deleted copy constructor. Callback cannot be copied, because doing that
     * may cause memory allocations.
     */
    Callback(const Callback& rhs) = delete;

    /**
     * Move constructor.
     * \param rhs Other Callback to be moved inside this one.
     */
    Callback(Callback&& rhs)
    {
        operation=rhs.operation;
        if(operation) operation(any,rhs.any,MOVE);
        rhs.operation=nullptr; //Ensure the rhs is made inert after the move
    }

    /**
     * Move conversion constructor. Not explicit by design.
     * \param functor Object which will be moved to inside this Callback.
     */
    template<typename T>
    Callback(T&& functor) : operation(nullptr)
    {
        *this=std::forward<T>(functor);
    }

    /**
     * Deleted assignment operators. Callback cannot be copied, because doing
     * that may cause memory allocations.
     */
    Callback& operator= (Callback& rhs) = delete;

    /**
     * Deleted assignment operators. Callback cannot be copied, because doing
     * that may cause memory allocations.
     */
    Callback& operator= (const Callback& rhs) = delete;

    /**
     * Assignment move operator.
     * \param rhs Object to move inside this Callback.
     * \return *this
     */
    Callback& operator= (Callback&& rhs);

    /**
     * Assignment move operator with conversion. Moves an object inside this
     * Callback.
     * \param functor Object which will be moved to inside this Callback.
     */
    template<typename T>
    Callback& operator= (T&& functor);

    /**
     * Removes any function object stored in this class
     */
    void clear()
    {
        if(operation) operation(any,nullptr,DESTROY);
        operation=nullptr;
    }

    /**
     * Call the callback, or do nothing if no callback is set
     */
    void operator() ()
    {
        if(operation) operation(any,nullptr,CALL);
    }
    
    /**
     * Call the callback, generating undefined behaviour if no callback is set
     */
    void call()
    {
        operation(any,nullptr,CALL);
    }
    
    /**
     * \return true if the object contains a callback
     */
    explicit operator bool() const
    {
        return operation!=nullptr;
    }

    /**
     * Destructor
     */
    ~Callback()
    {
        if(operation) operation(any,nullptr,DESTROY);
    }

private:
    /// This declaration is done like that to save memory. On 32 bit systems
    /// the size of a pointer is 4 bytes, but the strictest alignment is 8
    /// which is that of double and long long. Using an array of doubles would
    /// have guaranteed alignment but the array size would have been a multiple
    /// of 8 bytes, and by summing the 4 bytes of the operation pointer there
    /// would have been 4 bytes of slack space left unused when declaring arrays
    /// of callbacks. Therefore any is declared as an array of ints but aligned
    /// to 8 bytes. This allows i.e. declaring Callback<20> with 20 bytes of
    /// useful storage and 4 bytes of pointer, despite 20 is not a multiple of 8
    int32_t any[(N+3)/4] __attribute__((aligned(8)));
    void (*operation)(int32_t *a, int32_t *b, Op op);
};

template<unsigned N>
Callback<N>& Callback<N>::operator= (Callback<N>&& rhs)
{
    if(this==&rhs) return *this; //Handle assignmento to self
    if(operation) operation(any,nullptr,DESTROY);
    operation=rhs.operation;
    if(operation) operation(any,rhs.any,MOVE);
    rhs.operation=nullptr; //Ensure the rhs is made inert after the move
    return *this;
}

template<unsigned N>
template<typename T>
Callback<N>& Callback<N>::operator= (T&& functor)
{
    //If an error is reported about this line an attempt to store a too large
    //object is made. Increase N.
    static_assert(sizeof(any)>=sizeof(T),"N is too small");
    
    //This should not fail unless something has a stricter alignment than double
    static_assert(__alignof__(any)>=__alignof__(T),"Alignment mismatch");

    if(operation) operation(any,nullptr,DESTROY);

    new (reinterpret_cast<T*>(any)) T(std::forward<T>(functor));
    operation=TypeDependentOperation<T>::operation;
    return *this;
}

} //namespace miosix
