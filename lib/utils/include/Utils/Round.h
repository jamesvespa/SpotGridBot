//
// Created by james on 21/06/22.
//
#pragma once

namespace UTILS {
    //Helper to fix rounding issue with doubles
    template<class T>
    inline T Round(T value, const T c_multi = 10000.0) {
        bool sign = (value < 0); //ok see if the value has a sign

        value = (floor((fabs(value) * c_multi) + (T) 0.5) /
                 c_multi);  //regardless of sign make it positive(using fabs) then do rounding.

        return (sign) ? -value : value; //If it had a sign originally, add the sign back.
    }
}