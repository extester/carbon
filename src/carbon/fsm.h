/*
 *  Carbon framework
 *  Finite State Machine
 *
 *  Copyright (c) 2013 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 29.03.2013 16:25:16
 *      Initial revision.
 */

#ifndef __CARBON_FSM_H_INCLUDED__
#define __CARBON_FSM_H_INCLUDED__

#include "shell/atomic.h"

typedef uint32_t                fsm_t;
#define FSM_UNDEFINED     		((fsm_t)(-1))

class CStateMachine
{
    private:
        atomic_t		m_state;

    public:
        CStateMachine(fsm_t state = FSM_UNDEFINED)
        {
        	atomic_set(&m_state, state);
        }
        virtual ~CStateMachine() {}

        void setFsmState(fsm_t state)  { atomic_set(&m_state, state); }
        fsm_t getFsmState() const { return (fsm_t)atomic_get(&m_state); }
};

#endif /* __CARBON_FSM_H_INCLUDED__ */
