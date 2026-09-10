#ifndef DAQC_ROS_H
#define DAQC_ROS_H

#include <stdbool.h>

#include "daqc_control.h"

bool DaqcRosStart(daqc_control_t *control);
void DaqcRosRequestStatePublication(void);
void DaqcRosPublishCommunicationError(void);

#endif
