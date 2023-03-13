/**********************************************************************
* Asyn device support using SSH communications library                *
**********************************************************************/
#ifndef DRVASYNSSHPORT_H
#define DRVASYNSSHPORT_H

#include <shareLib.h>  

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

epicsShareFunc int drvAsynSSHPortConfigure(const char *portName,
                                           const char *hostaddress,
                                           const char *username,
                                           const char *password,
                                           unsigned int priority,
                                           int noAutoConnect,
                                           int noProcessEos);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif  /* DRVASYNSSHPORT_H */
