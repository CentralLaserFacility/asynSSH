#include <stdio.h>
#include <string.h>
#include <aSubRecord.h>
#include <epicsExport.h>
#include <registryFunction.h>

long parsels(struct aSubRecord *pgsub)
{
  char raw[2048];
  int maxCount = *(int *)pgsub->b;
  int start = *(int *)pgsub->c;
  int fileCount = *(int *)pgsub->d;
	//int index = *(int *)pgsub->b;
  int index1 = 0;
  int index2 = 0;
  int index3 = 0;
  char files[6][256] = {
    "",
    "",
    "",
    "",
    "",
    ""
  };
	
  strcpy(raw, (char *)pgsub->a);

  if (fileCount > 6){
    index2 = -1 * (fileCount-6) * start / 100;
  }
  // loop over all characters, splitting at \r\n
  while (raw[index1] != 0 && index2 < 6 && index1 < maxCount){
    if (raw[index1] == ','){
      if (index2 >= 0){
        files[index2][index3] = '\0';
      }
      index2++;
      index3 = 0;
      index1++;
    } else if (raw[index1] != '\r' && raw[index1] != '\n'){
      if (index2 >= 0){
        files[index2][index3] = raw[index1];
      }
      index3++;
    } else {
      // Here we have reached the end of the list
      if (raw[index1-2] != ','){
        index1 = maxCount;
      }
    }
    index1++;
  }  

  strcpy((char *)pgsub->vala, files[0]);
  strcpy((char *)pgsub->valb, files[1]);
  strcpy((char *)pgsub->valc, files[2]);
  strcpy((char *)pgsub->vald, files[3]);
  strcpy((char *)pgsub->vale, files[4]);
  strcpy((char *)pgsub->valf, files[5]);

	return 0;
}

long selectFile(struct aSubRecord *pgsub)
{
  int index = *(int *)pgsub->g;

  switch (index){
    case 1:
      strcpy((char *)pgsub->vala, (char *)pgsub->a);
      break;
    case 2:
      strcpy((char *)pgsub->vala, (char *)pgsub->b);
      break;
    case 3:
      strcpy((char *)pgsub->vala, (char *)pgsub->c);
      break;
    case 4:
      strcpy((char *)pgsub->vala, (char *)pgsub->d);
      break;
    case 5:
      strcpy((char *)pgsub->vala, (char *)pgsub->e);
      break;
    case 6:
      strcpy((char *)pgsub->vala, (char *)pgsub->f);
      break;
  }

	return 0;
}

epicsRegisterFunction( parsels );
epicsRegisterFunction( selectFile );
