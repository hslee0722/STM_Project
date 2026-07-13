#ifndef CONVEYOR_FSM_H
#define CONVEYOR_FSM_H

#include "app_state.h"

/*
 * conveyor_fsm.h
 * ------------------------------------------------------------
 * 컨베이어 상태 머신 분리.
 */

void Conveyor_Init_State(void);
void Conveyor_Start(void);
void Conveyor_Update(int dist);
ConveyorState Conveyor_Get_State(void);

#endif
