#ifndef __DIGSIG_PRDF_H__
#define __DIGSIG_PRDF_H__

#include <Event/Event.h>
#include <Event/EventTypes.h>

int process_event (Event *e); //++CINT 
int SaveFile(); //++CINT 
void SetSaveFileName(const char *); //++CINT 
int SetChannelMapFile(const char *); //++CINT 

#endif /* __V1742PRDF_H__ */
