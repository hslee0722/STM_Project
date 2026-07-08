#ifndef CONVEYOR_FSM_H
#define CONVEYOR_FSM_H

#include "app_state.h"

void Conveyor_Init_State(void);
void Conveyor_Start(void);
void Conveyor_Update(int dist);
ConveyorState Conveyor_Get_State(void);

#endif
