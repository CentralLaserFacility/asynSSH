#!../../bin/linux-x86/asynSSHTest

< envPaths
epicsEnvSet ("STREAM_PROTOCOL_PATH", "/mnt/c/Users/Admin/Downloads/asynSSH_V1-0/iocBoot/iocasynSSHTest")

cd ${TOP}

## Register all support components
dbLoadDatabase "dbd/asynSSHTest.dbd"
asynSSHTest_registerRecordDeviceDriver pdbbase

drvAsynSSHPortConfigure("SSH1", "localhost", "sailikhitha", "", "0", "0", "0")

## Load record instances
dbLoadRecords "db/testApp.db", ""

#asynSetTraceMask("SSH1", -1, 127)
#asynSetTraceIOMask("SSH1", -1, 127)

cd ${TOP}/iocBoot/${IOC}
iocInit

