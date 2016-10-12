/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   radio_timer.cpp
 * Author: fabiuz
 * 
 * Created on October 11, 2016, 11:17 AM
 */

#include "radio_timer.h"

using namespace miosix;

Thread* RadioTimer::tWaitingGPIO=nullptr;

RadioTimer::RadioTimer():b(HighResolutionTimerBase::instance()) {}

RadioTimer& RadioTimer::instance(){
    static RadioTimer instance;
    return instance;
}

RadioTimer::~RadioTimer() {}

