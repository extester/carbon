/*
 *  Shell library
 *  Smart pointer for reference counter objects
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 07.05.2016 08:50:25
 *      Initial revision.
 *
 */

#ifndef __SHELL_DEC_PTR_H_INCLUDED__
#define __SHELL_DEC_PTR_H_INCLUDED__

#include "shell/types.h"
#include "shell/defines.h"

template<class T>
class dec_ptr
{
    private:
        T*      _ptr;

    public:
        dec_ptr(T* p = NULL) : _ptr(p) {}

        dec_ptr(const dec_ptr<T>& other) : _ptr(other._ptr) {}

        ~dec_ptr() {
            SAFE_RELEASE(_ptr);
        }

        dec_ptr<T>& operator=(const dec_ptr<T>& other) {
            if ( this != &other ) {
                SAFE_RELEASE(_ptr);
                _ptr = other._ptr;
            }

            return *this;
        }

        dec_ptr<T>& operator=(T* p)  {
            _ptr = p;

            return *this;
        }

        T& operator*() {
            return *_ptr;
        }

        const T& operator*() const {
            return *_ptr;
        }

        T* operator->() {
            return _ptr;
        }

        const T* operator->() const {
            return _ptr;
        }

        operator T*() {
            return _ptr;
        }

        operator const T*() {
            return _ptr;
        }

        T* ptr() const {
            return _ptr;
        }

        size_t getRefCount() const {
            return _ptr ? _ptr->getRefCount() : 0;
        }

        void release() {
            SAFE_RELEASE(_ptr);
        }
};

#endif /* __SHELL_DEC_PTR_H_INCLUDED__ */
