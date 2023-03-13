/***************************************************
* Asyn device support for SSH comms library        *
****************************************************/

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <osiUnistd.h>
#include <osiSock.h>
#include <cantProceed.h>
#include <errlog.h>
#include <iocsh.h>
#include <epicsAssert.h>
#include <epicsExit.h>
#include <epicsStdio.h>
#include <epicsString.h>
#include <epicsThread.h>
#include <epicsTime.h>
#include <osiUnistd.h>

#include <epicsExport.h>
#include "asynDriver.h"
#include "asynOctet.h"
#include "asynInterposeCom.h"
#include "asynInterposeEos.h"
#include "drvAsynSSHPort.h"
#include "sshDriver.h"

/*
 * This structure holds the hardware-specific information for a single
 * asyn link.  There is one for each IP socket.
 */
typedef struct {
    asynUser          *pasynUser;        /* Not currently used */
    char              *SSHDeviceName;
    char              *SSHHostName;
    char              *SSHUserName;
    char              *SSHPassword;
    char              *portName;
    SSHDriver         *driver;
    unsigned long      nRead;
    unsigned long      nWritten;
    int                haveAddress;
    osiSockAddr        farAddr;
    asynInterface      common;
    asynInterface      octet;
} sshController_t;

#define FLAG_BROADCAST                  0x1
#define FLAG_CONNECT_PER_TRANSACTION    0x2
#define FLAG_SHUTDOWN                   0x4


/*
 * Close a connection
 */
static void
closeConnection(asynUser *pasynUser,sshController_t *ssh,const char *why)
{
    asynPrint(pasynUser, ASYN_TRACE_FLOW,
              "Close %s connection (driver %d): %s\n", ssh->SSHDeviceName, ssh->driver, why);
    if (ssh->driver){
        ssh->driver->disconnectSSH();
        delete(ssh->driver);
        ssh->driver = 0;
    }
}

/*Beginning of asynCommon methods*/
/*
 * Report link parameters
 */
static void
asynCommonReport(void *drvPvt, FILE *fp, int details)
{
    sshController_t *ssh = (sshController_t *)drvPvt;

    assert(ssh);
    if (details >= 1) {
        fprintf(fp, "    Port %s: %sonnected\n",
                                                ssh->SSHDeviceName,
                                                ssh->driver ? "C" : "Disc");
    }
    if (details >= 2) {
        fprintf(fp, "    Characters written: %lu\n", ssh->nWritten);
        fprintf(fp, "       Characters read: %lu\n", ssh->nRead);
    }
}

/*
 * Clean up a connection on exit
 */
static void
cleanup (void *arg)
{
    asynStatus status;
    sshController_t *ssh = (sshController_t *)arg;

    if (!ssh) return;
    status=pasynManager->lockPort(ssh->pasynUser);
    if(status!=asynSuccess)
        asynPrint(ssh->pasynUser, ASYN_TRACE_ERROR, "%s: cleanup locking error\n", ssh->portName);

    if (ssh->driver){
        asynPrint(ssh->pasynUser, ASYN_TRACE_FLOW, "%s: shutdown socket\n", ssh->portName);
        ssh->driver->disconnectSSH();
        delete(ssh->driver);
    }

    if(status==asynSuccess)
        pasynManager->unlockPort(ssh->pasynUser);
}

/*
 * Create a link
*/
static asynStatus
connectIt(void *drvPvt, asynUser *pasynUser)
{
    sshController_t *ssh = (sshController_t *)drvPvt;

    /*
     * Sanity check
     */
    assert(ssh);
    asynPrint(pasynUser, ASYN_TRACE_FLOW,
              "Open connection to %s  reason:%d \n", ssh->SSHDeviceName,
                                                     pasynUser->reason);

    if (ssh->driver){
        epicsSnprintf(pasynUser->errorMessage,pasynUser->errorMessageSize,
                              "%s: Link already open!", ssh->SSHDeviceName);
        return asynError;
    }


    // Create the driver
    ssh->driver = new SSHDriver(ssh->SSHHostName);

    // Set the username
    ssh->driver->setUsername(ssh->SSHUserName);

    // If a password has been supplied then set it
    if (strcmp(ssh->SSHPassword, "")){
      ssh->driver->setPassword(ssh->SSHPassword);
    }

    // Connect to the remote host
    if (ssh->driver->connectSSH() != SSHDriverSuccess){
      epicsSnprintf(pasynUser->errorMessage,pasynUser->errorMessageSize,
                    "Can't connect to %s",
                    ssh->SSHDeviceName);
      ssh->haveAddress = 0;
      return asynError;
    }

    asynPrint(pasynUser, ASYN_TRACE_FLOW, "Opened connection to %s\n", ssh->SSHDeviceName);
    return asynSuccess;
}

static asynStatus
asynCommonConnect(void *drvPvt, asynUser *pasynUser)
{
    asynStatus status = asynSuccess;

    status = connectIt(drvPvt, pasynUser);

    if (status == asynSuccess)
        pasynManager->exceptionConnect(pasynUser);
    return status;
}

static asynStatus
asynCommonDisconnect(void *drvPvt, asynUser *pasynUser)
{
    sshController_t *ssh = (sshController_t *)drvPvt;

    assert(ssh);
    closeConnection(pasynUser,ssh,"Disconnect request");
    return asynSuccess;
}

/*Beginning of asynOctet methods*/
/*
 * Write to the TCP port
 */
static asynStatus writeIt(void *drvPvt, asynUser *pasynUser,
    const char *data, size_t numchars,size_t *nbytesTransfered)
{
    sshController_t *ssh = (sshController_t *)drvPvt;
    size_t thisWrite;
    asynStatus status = asynSuccess;

    assert(ssh);
    asynPrint(pasynUser, ASYN_TRACE_FLOW,
              "%s write.\n", ssh->SSHDeviceName);
    asynPrintIO(pasynUser, ASYN_TRACEIO_DRIVER, data, numchars,
                "%s write %d\n", ssh->SSHDeviceName, numchars);
    *nbytesTransfered = 0;

    // Here we will simply issue the write to the driver
    if (!ssh->driver){
        if ((status = connectIt(drvPvt, pasynUser)) != asynSuccess){
          return status;
        }
    }
    if (numchars == 0){
      return asynSuccess;
    }
    if (ssh->driver->write((char *)data, numchars, &thisWrite, (int)(pasynUser->timeout*1000.0)) != SSHDriverSuccess){
      epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                               "%s write error", ssh->SSHDeviceName);
      closeConnection(pasynUser,ssh,"Write error");
      status = asynError;
    }
    *nbytesTransfered = thisWrite;
    asynPrint(pasynUser, ASYN_TRACE_FLOW,
              "wrote %lu to %s, return %s.\n", (unsigned long)*nbytesTransfered,
                                               ssh->SSHDeviceName,
                                               pasynManager->strStatus(status));
    return status;
}

/*
 * Read from the TCP port
 */
static asynStatus readIt(void *drvPvt, asynUser *pasynUser,
    char *data, size_t maxchars,size_t *nbytesTransfered,int *gotEom)
{
    sshController_t *ssh = (sshController_t *)drvPvt;
    size_t thisRead;
    int reason = 0;
    asynStatus status = asynSuccess;
    SSHDriverStatus driverStatus = SSHDriverSuccess;

    assert(ssh);
    asynPrint(pasynUser, ASYN_TRACE_FLOW,
              "%s read.\n", ssh->SSHDeviceName);
    if (!ssh->driver){
      if ((status = connectIt(drvPvt, pasynUser)) != asynSuccess){
        return status;
      }
    }
    if (maxchars <= 0) {
        epicsSnprintf(pasynUser->errorMessage,pasynUser->errorMessageSize,
                  "%s maxchars %d. Why <=0?\n",ssh->SSHDeviceName,(int)maxchars);
        return asynError;
    }

    if (gotEom) *gotEom = 0;

    driverStatus = ssh->driver->read((char *)data, maxchars, &thisRead, (int)(pasynUser->timeout*1000.0));
    if (driverStatus != SSHDriverSuccess){
      if (driverStatus == SSHDriverError){
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                               "%s read error", ssh->SSHDeviceName);
        closeConnection(pasynUser,ssh,"Read error");
        status = asynError;
      } else if (driverStatus == SSHDriverTimeout){
        status = asynTimeout;
      }
    }

    if (thisRead < 0){
        thisRead = 0;
    }
    *nbytesTransfered = thisRead;

    /* If there is room add a null byte */
    if (thisRead < maxchars)
        data[thisRead] = 0;
    else 
        reason |= ASYN_EOM_CNT;

    if (gotEom) *gotEom = reason;

    if (thisRead == 0 && pasynUser->timeout == 0){
      status = asynTimeout;
    }

    return status;
}

/*
 * Flush pending input
 */
static asynStatus
flushIt(void *drvPvt,asynUser *pasynUser)
{
    sshController_t *ssh = (sshController_t *)drvPvt;

    assert(ssh);
    asynPrint(pasynUser, ASYN_TRACE_FLOW, "%s flush\n", ssh->SSHDeviceName);
    if (ssh->driver){
      ssh->driver->flush();
    }
    return asynSuccess;
}

/*
 * Clean up a sshController
 */
static void
sshCleanup(sshController_t *ssh)
{
    if (ssh) {
        if (ssh->driver)
        free(ssh->portName);
        free(ssh->SSHDeviceName);
        free(ssh);
    }
}

/*
 * asynCommon methods
 */
static const struct asynCommon drvAsynSSHPortAsynCommon = {
    asynCommonReport,
    asynCommonConnect,
    asynCommonDisconnect
};

/*
 * Configure and register an IP socket from a hostInfo string
 */
epicsShareFunc int
drvAsynSSHPortConfigure(const char *portName,
                              const char *hostName,
                              const char *userName,
                              const char *password,
                              unsigned int priority,
                              int noAutoConnect,
                              int noProcessEos)
{
    sshController_t *ssh;
    asynInterface *pasynInterface;
    asynStatus status;
    int nbytes;
    asynOctet *pasynOctet;
    int isCom = 0;

    /*
     * Check arguments
     */
    if (portName == NULL) {
        printf("asynSSH port name missing.\n");
        return -1;
    }
    if (hostName == NULL) {
        printf("asynSSH host name missing.\n");
        return -1;
    }
    if (userName == NULL) {
        printf("asynSSH username missing.\n");
        return -1;
    }


    /*
     * Create a driver
     */
    nbytes = sizeof(*ssh) + sizeof(asynOctet);
    ssh = (sshController_t *)callocMustSucceed(1, nbytes,
          "drvAsynSSHPortConfigure()");
    pasynOctet = (asynOctet *)(ssh+1);
    ssh->driver = NULL;
    ssh->SSHDeviceName = epicsStrDup(hostName);
    ssh->SSHHostName = epicsStrDup(hostName);
    ssh->SSHUserName = epicsStrDup(userName);
    ssh->SSHPassword = epicsStrDup(password);
    ssh->portName = epicsStrDup(portName);

    /*
     *  Link with higher level routines
     */
    pasynInterface = (asynInterface *)callocMustSucceed(2, sizeof *pasynInterface, "drvAsynSSHPortConfigure");
    ssh->common.interfaceType = asynCommonType;
    ssh->common.pinterface  = (void *)&drvAsynSSHPortAsynCommon;
    ssh->common.drvPvt = ssh;
    if (pasynManager->registerPort(ssh->portName,
                                   ASYN_CANBLOCK,
                                   !noAutoConnect,
                                   priority,
                                   0) != asynSuccess) {
        printf("drvAsynSSHPortConfigure: Can't register myself.\n");
        sshCleanup(ssh);
        return -1;
    }
    status = pasynManager->registerInterface(ssh->portName,&ssh->common);
    if(status != asynSuccess) {
        printf("drvAsynSSHPortConfigure: Can't register common.\n");
        sshCleanup(ssh);
        return -1;
    }
    pasynOctet->read = readIt;
    pasynOctet->write = writeIt;
    pasynOctet->flush = flushIt;
    ssh->octet.interfaceType = asynOctetType;
    ssh->octet.pinterface  = pasynOctet;
    ssh->octet.drvPvt = ssh;
    status = pasynOctetBase->initialize(ssh->portName,&ssh->octet, 0, 0, 1);
    if(status != asynSuccess) {
        printf("drvAsynSSHPortConfigure: pasynOctetBase->initialize failed.\n");
        sshCleanup(ssh);
        return -1;
    }
    if (isCom && (asynInterposeCOM(ssh->portName) != 0)) {
        printf("drvAsynSSHPortConfigure: asynInterposeCOM failed.\n");
        sshCleanup(ssh);
        return -1;
    }
    if (!noProcessEos)
        asynInterposeEosConfig(ssh->portName, -1, 1, 1);
    ssh->pasynUser = pasynManager->createAsynUser(0,0);
    status = pasynManager->connectDevice(ssh->pasynUser,ssh->portName,-1);
    if(status != asynSuccess) {
        printf("connectDevice failed %s\n",ssh->pasynUser->errorMessage);
        sshCleanup(ssh);
        return -1;
    }

    /*
     * Register for socket cleanup
     */
    epicsAtExit(cleanup, ssh);
    return 0;
}

/*
 * IOC shell command registration
 */
static const iocshArg drvAsynSSHPortConfigureArg0 = { "port name",iocshArgString};
static const iocshArg drvAsynSSHPortConfigureArg1 = { "host name",iocshArgString};
static const iocshArg drvAsynSSHPortConfigureArg2 = { "username",iocshArgString};
static const iocshArg drvAsynSSHPortConfigureArg3 = { "password",iocshArgString};
static const iocshArg drvAsynSSHPortConfigureArg4 = { "priority",iocshArgInt};
static const iocshArg drvAsynSSHPortConfigureArg5 = { "disable auto-connect",iocshArgInt};
static const iocshArg drvAsynSSHPortConfigureArg6 = { "noProcessEos",iocshArgInt};
static const iocshArg *drvAsynSSHPortConfigureArgs[] = {
    &drvAsynSSHPortConfigureArg0, &drvAsynSSHPortConfigureArg1,
    &drvAsynSSHPortConfigureArg2, &drvAsynSSHPortConfigureArg3,
    &drvAsynSSHPortConfigureArg4, &drvAsynSSHPortConfigureArg5,
    &drvAsynSSHPortConfigureArg6};
static const iocshFuncDef drvAsynSSHPortConfigureFuncDef =
                      {"drvAsynSSHPortConfigure",7,drvAsynSSHPortConfigureArgs};
static void drvAsynSSHPortConfigureCallFunc(const iocshArgBuf *args)
{
    drvAsynSSHPortConfigure(args[0].sval, args[1].sval, args[2].sval, args[3].sval, args[4].ival, args[5].ival, args[6].ival);
}

/*
 * This routine is called before multitasking has started, so there's
 * no race condition in the test/set of firstTime.
 */
static void
drvAsynSSHPortRegisterCommands(void)
{
    static int firstTime = 1;
    if (firstTime) {
        iocshRegister(&drvAsynSSHPortConfigureFuncDef,drvAsynSSHPortConfigureCallFunc);
        firstTime = 0;
    }
}
epicsExportRegistrar(drvAsynSSHPortRegisterCommands);

