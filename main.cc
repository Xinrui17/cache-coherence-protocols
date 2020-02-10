#include <stdlib.h>
#include <fstream>
#include <string.h>
#include <iostream>
#include <stdio.h>
#include <vector>
using namespace std;

#include "cache.h"

int cache_size=0, cache_assoc=0, blk_size=0, num_processors=0, protocol=0;
void MSIProtocol(vector <Cache*> caches, int p, char cmd, ulong address);
void MESIProtocol(vector <Cache*> caches, int p, char cmd, ulong address);
void DragonProtocol(vector <Cache*> caches, int p, char cmd, ulong address);
void printStats(int p, Cache* cache);

int main(int argc, char *argv[])
{	
	ifstream fin; 
	FILE * pFile;
	char fname[160], pname[10];
	int p;
	
	if(argv[1] == NULL){
		 printf("input format: ./smp_cache <cache_size> <assoc> <block_size> <num_processors> <protocol> <trace_file> \n");
		 exit(0);
    }
	cache_size = atoi(argv[1]);
	cache_assoc= atoi(argv[2]);
	blk_size   = atoi(argv[3]);
	num_processors = atoi(argv[4]);  //1, 2, 4, 8
	protocol   = atoi(argv[5]);	 	 //0:MSI, 1:MESI, 2:Dragon
 	strcpy(fname, argv[6]);
		
	//TEST
	//cache_size=8192;cache_assoc=8;blk_size=64;num_processors=4;protocol=0;
	//strcpy(fname, "trace/canneal.04t.longTrace");
	
	pFile = fopen(fname,"r");
	if (pFile==0) {
		printf("Trace file problem\n");
		exit(0);
	}

	vector <Cache*> caches;
	for (p=0; p<num_processors; p++)
	{
		Cache* cache = new Cache(cache_size, cache_assoc, blk_size);
		caches.push_back(cache);
	}

	uchar cmd; uint address; 	
	while(!feof(pFile)) {
		fscanf(pFile, "%d %c %x\n", &p, &cmd, &address);
		if (protocol==0) {
			MSIProtocol(caches, p, cmd, address);
			strcpy(pname, "MSI");
		}
		if (protocol==1) {
			MESIProtocol(caches, p, cmd, address);
			strcpy(pname, "MESI");
		}
		if (protocol==2) {
			DragonProtocol(caches, p, cmd, address);
			strcpy(pname, "Dragon");
		}
	}
		
	cout<<"===== 506 Personal information ====="
		<<"\nVarun Varadarajan\nvvarada2@ncsu.edu\nECE492 Students? NO";
	printf("\n===== 506 SMP Simulator configuration =====");
	cout<<"\nL1_SIZE: "<<cache_size
		<<"\nL1_ASSOC: "<<cache_assoc
		<<"\nL1_BLOCKSIZE: "<<blk_size
		<<"\nNUMBER OF PROCESSORS: "<<num_processors
		<<"\nCOHERENCE PROTOCOL: "<<pname
		<<"\nTRACE FILE: "<<fname;

	for (p=0; p<num_processors; p++ ) {
		printStats(p, caches[p]);
	}
	cout<<"\n";
}

void printStats(int p, Cache* cache) {
	printf("\n============ Simulation results (Cache %d) ============", p);
	cout<<"\n01. number of reads:				"<<cache->getReads()
    	<<"\n02. number of read misses:			"<<cache->getRM()
    	<<"\n03. number of writes:				"<<cache->getWrites()
    	<<"\n04. number of write misses:			"<<cache->getWM();
    printf("\n05. total miss rate:				%0.2f", cache->getMR());
    cout<<"%\n06. number of writebacks:			";
	if (protocol==2) {
      	cout<<cache->getWB()-cache->getFlush();
	} else {
		cout<<cache->getWB();
	}
	cout<<"\n07. number of cache-to-cache transfers:		"<<cache->getCCT()
      	<<"\n08. number of memory transactions:		";
	if (protocol==0) {
      	cout<<cache->getRM()+cache->getWM()+cache->getWB()+cache->getMMT();
	} else if (protocol==1) {
		cout<<cache->getRM()+cache->getWM()+cache->getWB()-cache->getCCT();
	} else if (protocol==2) {
		cout<<cache->getRM()+cache->getWM()+cache->getWB()-cache->getFlush();
	}
    cout<<"\n09. number of interventions:			"<<cache->getINT()
    	<<"\n10. number of invalidations:			"<<cache->getINV()
    	<<"\n11. number of flushes:				"<<cache->getFlush()
    	<<"\n12. number of BusRdX:				"<<cache->getBusRdX();
}

void MSIProtocol(vector <Cache*> caches, int p, char cmd, ulong address)
{
	int i;
	if (caches[p]->findLine(address) == NULL) {	
		//CURRENT CACHE
		caches[p]->Access(address, cmd);
		if (cmd == 'r') {
			caches[p]->inc_busRd();
			caches[p]->findLine(address)->setFlags(SHARED);
		}
		else if (cmd == 'w') {
			caches[p]->inc_busRdX();
			caches[p]->findLine(address)->setFlags(MODIFIED);
		}
		//SNOOPING CACHES
		for (i=0; i<num_processors; i++) {
			if (i!=p && caches[i]->findLine(address)!=NULL) {
				if (cmd == 'r') {
					if (caches[i]->findLine(address)->getFlags() == MODIFIED) {
						caches[i]->findLine(address)->setFlags(SHARED);
						caches[i]->inc_flush();
						caches[i]->inc_intervene();
						caches[i]->writeBack(address);
					}
				}
				else if (cmd == 'w') {
					caches[i]->findLine(address)->invalidate();
					caches[i]->inc_invalidate();
					caches[i]->inc_flush();
					caches[i]->writeBack(address);
				}
			}
		}
	}
	else if (caches[p]->findLine(address)->getFlags() == INVALID) {
		//CURRENT CACHE
		caches[p]->Access(address, cmd);
		if (cmd == 'r') {
			caches[p]->findLine(address)->setFlags(SHARED);
		}
		else if (cmd == 'w') {
			caches[p]->inc_busRdX();
			caches[p]->findLine(address)->setFlags(MODIFIED);
		}
		//SNOOPING CACHE
		for (i=0; i<num_processors; i++) {
			if (i!=p && caches[i]->findLine(address) != NULL) {
				if (cmd == 'r') {
					if (caches[i]->findLine(address)->getFlags() == MODIFIED) {
						caches[i]->findLine(address)->setFlags(SHARED);
						caches[i]->inc_flush();
						caches[i]->inc_intervene();
						caches[i]->writeBack(address);
					}
				}
				else if (cmd == 'w') {
					caches[i]->findLine(address)->invalidate();
					caches[i]->inc_invalidate();
				}
			}
		}
	}
	else if (caches[p]->findLine(address)->getFlags() == SHARED)
	{
		//CURRENT CACHE
		caches[p]->Access(address, cmd);
		if (cmd == 'w') {
			caches[p]->findLine(address)->setFlags(MODIFIED);
			caches[p]->inc_memTransact();
			caches[p]->inc_busRdX();
		}
		//SNOOPING CACHE - ONLY WR
		for (i=0; i<num_processors; i++) {
			if (i!=p && caches[i]->findLine(address) != NULL) {
				if (cmd == 'w') {
					if(caches[i]->findLine(address)->getFlags()!=INVALID) {
					    caches[i]->findLine(address)->invalidate();
						caches[i]->inc_invalidate();
					}
				}
			}
		}
	}
	else if (caches[p]->findLine(address)->getFlags() == MODIFIED) {
		//CURRENT CACHE
		caches[p]->Access(address, cmd);
		caches[p]->findLine(address)->setFlags(MODIFIED);
		//NO SNOOPING UPDATE REQUIRED
	}
}

void MESIProtocol(vector <Cache*> caches, int p, char cmd, ulong address)
{
	int i;
	bool C=false;
	if (caches[p]->findLine(address) == NULL)
	{	
		//CURRENT CACHE
		caches[p]->Access(address, cmd);
		for (i=0; i<num_processors; i++) {
			if (i!=p && caches[i]->findLine(address) != NULL) { C = true; }
		}
		if (cmd == 'r') {
			if (C) {
				caches[p]->inc_busRd();
				caches[p]->findLine(address)->setFlags(SHARED);
				caches[p]->inc_ccTransfer();
			}
			else {
				caches[p]->findLine(address)->setFlags(EXCLUSIVE);
			}
		}
		else if (cmd == 'w')
		{
			caches[p]->inc_busRdX();
			caches[p]->findLine(address)->setFlags(MODIFIED);
		}
		//SNOOPING CACHE
		for (i=0; i<num_processors; i++) {
			if (i!=p && caches[i]->findLine(address) != NULL) {
				if (cmd == 'r' && C)	{	
					if (caches[i]->findLine(address)->getFlags() == MODIFIED) {
						caches[i]->inc_flush();
						caches[i]->inc_intervene();
						caches[i]->writeBack(address);
					}
					else if (caches[i]->findLine(address)->getFlags() == EXCLUSIVE) {
						caches[i]->inc_intervene();
					}
					caches[i]->findLine(address)->setFlags(SHARED);
				}
				else if (cmd == 'w') {
					if (caches[i]->findLine(address)->getFlags() == SHARED) {
						caches[i]->inc_invalidate();
						caches[p]->inc_ccTransfer();
					}
					else if (caches[i]->findLine(address)->getFlags() == MODIFIED) {
						caches[i]->inc_flush();
						caches[i]->inc_invalidate();
						caches[p]->inc_ccTransfer();
						caches[i]->writeBack(address);
					}
					else {
						caches[i]->inc_invalidate();
					}
					caches[i]->findLine(address)->invalidate();
				}
			}
		}
	}
	else if (caches[p]->findLine(address)->getFlags() == EXCLUSIVE)
	{	
		//CURRENT CACHE
		caches[p]->Access(address, cmd);
		if (cmd == 'r') {
			caches[p]->findLine(address)->setFlags(EXCLUSIVE);
		}
		else if (cmd == 'w') {
			caches[p]->findLine(address)->setFlags(MODIFIED);
		}
	}
	else if (caches[p]->findLine(address)->getFlags() == SHARED)
	{			
		//CURRENT CACHE
		caches[p]->Access(address, cmd);
		if (cmd == 'w') {
			caches[p]->inc_busUpgr();
			caches[p]->findLine(address)->setFlags(MODIFIED);
		}
		//SNOOPING CACHE- ONLY WR
		for (i=0; i<num_processors; i++) {
			if (i!=p && caches[i]->findLine(address) != NULL) {
				if (cmd == 'w') {
					if (caches[i]->findLine(address)->getFlags() == SHARED) {
						caches[i]->inc_invalidate();
					}
					else {
						caches[i]->inc_invalidate();
					}
					caches[i]->findLine(address)->invalidate();
				}
			}
		}
	}
	else if (caches[p]->findLine(address)->getFlags() == MODIFIED) {
		//CURRENT CACHE
		caches[p]->Access(address, cmd);
		caches[p]->findLine(address)->setFlags(MODIFIED);
		//NO SNOOPING UPDATE REQUIRED
	}
}

void DragonProtocol(vector <Cache*> caches, int p, char cmd, ulong address)
{
	int i;
	bool C=false;
	for (i=0; i<num_processors; i++) {
		if (i!=p && caches[i]->findLine(address) != NULL 
			     && caches[i]->findLine(address)->getFlags() != INVALID) { C=true; }
	}

	if(caches[p]->findLine(address) == NULL || caches[p]->findLine(address)->getFlags() == INVALID)	{
		//CURRENT CACHE
		caches[p]->Access(address, cmd);
		if(cmd == 'r') {
            if(C)
                caches[p]->findLine(address)->setFlags(S_CLEAN);              
            else
                caches[p]->findLine(address)->setFlags(EXCLUSIVE);
		}
		else if(cmd == 'w') {
			if(C)
				caches[p]->findLine(address)->setFlags(S_MODIFIED);
			else
                caches[p]->findLine(address)->setFlags(MODIFIED);
		}
		//SNOOPING CACHE
		for (i=0; i<num_processors; i++) {
			if (i!=p && caches[i]->findLine(address) != NULL && caches[i]->findLine(address)->getFlags() != INVALID) {
				if (cmd == 'r' && C) {
					if (caches[i]->findLine(address)->getFlags() == EXCLUSIVE) {
 					    caches[i]->findLine(address)->setFlags(S_CLEAN);
					    caches[i]->inc_intervene();
					}
					else if (caches[i]->findLine(address)->getFlags() == MODIFIED) {
					    caches[i]->findLine(address)->setFlags(S_MODIFIED);
					    caches[i]->inc_intervene();
					    caches[i]->inc_flush();
					    caches[i]->writeBack(address); 
					    continue;
					}
					else if (caches[i]->findLine(address)->getFlags() == S_MODIFIED) {
                		caches[i]->inc_flush();
                		caches[i]->writeBack(address);
                	}
				}
				else if (cmd == 'w') {
					if (caches[i]->findLine(address)->getFlags() == EXCLUSIVE) {
					    caches[i]->inc_intervene();
				    }
					if (caches[i]->findLine(address)->getFlags() == MODIFIED) {
					    caches[i]->inc_intervene();
					    caches[i]->inc_flush();
					    caches[i]->writeBack(address);
				    }
				    if (caches[i]->findLine(address)->getFlags() == S_MODIFIED) {
				    	caches[i]->inc_flush();
				    	caches[i]->writeBack(address);
				    }
				    caches[i]->findLine(address)->setFlags(S_CLEAN);
				}
			}
		}
	}
    
    else if (caches[p]->findLine(address)->getFlags() == EXCLUSIVE)
	{	
		//CURRENT CACHE
		caches[p]->Access(address, cmd);
		if(cmd == 'r') {	
			caches[p]->findLine(address)->setFlags(EXCLUSIVE);
		}
		else if (cmd == 'w') {
			caches[p]->findLine(address)->setFlags(MODIFIED);
		}
		//NO SNOOPING UPDATE REQUIRED
	}

	else if (caches[p]->findLine(address)->getFlags() == MODIFIED)	{
		//CURRENT CACHE
		caches[p]->Access(address, cmd);
		caches[p]->findLine(address)->setFlags(MODIFIED);
		//NO SNOOPING UPDATE REQUIRED
	}

	else if (caches[p]->findLine(address)->getFlags() == S_CLEAN) {
        //CURRENT CACHE
		caches[p]->Access(address, cmd);
        if(cmd == 'r') {
        	caches[p]->findLine(address)->setFlags(S_CLEAN);
        }
        else if (cmd == 'w') {
        	if(C)
        		caches[p]->findLine(address)->setFlags(S_MODIFIED);
        	else
        		caches[p]->findLine(address)->setFlags(MODIFIED);
		}
		//SNOOPING CACHE - WR ONLY
       	for (i=0; i<num_processors; i++) {
		    if (i!=p && caches[i]->findLine(address) != NULL) {
				if (cmd == 'w') {
					if(caches[i]->findLine(address)->getFlags() == S_MODIFIED) {
                        caches[i]->findLine(address)->setFlags(S_CLEAN);
                    }
                }
            }
        } 
	}

	else if (caches[p]->findLine(address)->getFlags() == S_MODIFIED) {
		//CURRENT CACHE
		caches[p]->Access(address, cmd);
		if(cmd == 'r' || (cmd == 'w' && C)) {
            caches[p]->findLine(address)->setFlags(S_MODIFIED);
		}
		else if(cmd == 'w') {
           	caches[p]->findLine(address)->setFlags(MODIFIED);
		}
		//NO SNOOPING UPDATE
	}
}