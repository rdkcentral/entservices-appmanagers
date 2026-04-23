//
//  Debugger.h
//  AppManager Gateway
//
//  Copyright © 2022 Sky UK. All rights reserved.
//

#ifndef DEBUGGER_H
#define DEBUGGER_H


// -----------------------------------------------------------------------------
/*!
    \class Debugger

    Base class for attached debuggers.

 */
// Spec: runtime-manager
class Debugger
{
public:
    virtual ~Debugger() = default;

    enum class Type { Gdb, WebInspector };

    virtual Type type() const = 0;
    virtual bool isAttached() const = 0;
    virtual int debugPort() const = 0;

};


#endif //DEBUGGER_H
