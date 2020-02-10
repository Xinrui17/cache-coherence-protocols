
#ifndef CACHE_H
#define CACHE_H

#include <cmath>
#include <iostream>

typedef unsigned long ulong;
typedef unsigned char uchar;
typedef unsigned int uint;

enum{
	INVALID = 0,
   SHARED,
   MODIFIED,
   EXCLUSIVE,
   S_CLEAN,
   S_MODIFIED
};

class cacheLine 
{
protected:
   ulong tag;
   ulong Flags;   // 0:invalid, 1:valid, 2:dirty 
   ulong seq; 
 
public:
   cacheLine()            { tag = 0; Flags = 0; }
   ulong getTag()         { return tag; }
   ulong getFlags()			{ return Flags;}
   ulong getSeq()         { return seq; }
   void setSeq(ulong Seq)			{ seq = Seq;}
   void setFlags(ulong flags)			{  Flags = flags;}
   void setTag(ulong a)   { tag = a; }
   void invalidate()      { tag = 0; Flags = INVALID; }
   bool isValid()         { return ((Flags) != INVALID); }
};

class Cache
{
protected:
   ulong size, lineSize, assoc, sets, log2Sets, log2Blk, tagMask, numLines;
   ulong reads,readMisses,writes,writeMisses,writeBacks;
   float missRate;

   ulong ccTransfer;
   ulong flushes;
   ulong invalidations;
   ulong interventions;
   ulong memTransact;
   ulong busRd;
   ulong busRdX;
   ulong busUpgr;

   cacheLine **cache;
   ulong calcTag(ulong addr)     { return (addr >> (log2Blk) );}
   ulong calcIndex(ulong addr)  { return ((addr >> log2Blk) & tagMask);}
   ulong calcAddr4Tag(ulong tag)   { return (tag << (log2Blk));}
   
public:
   ulong currentCycle;  
     
    Cache(int,int,int);
   ~Cache() { delete cache;}
   
   cacheLine *findLineToReplace(ulong addr);
   cacheLine *fillLine(ulong addr);
   cacheLine * findLine(ulong addr);
   cacheLine * getLRU(ulong);
   
   ulong getRM(){return readMisses;} 
   ulong getWM(){return writeMisses;} 
   ulong getReads(){return reads;}
   ulong getWrites(){return writes;}
   ulong getWB(){return writeBacks;}
   float getMR()
   {
      return (((float)readMisses+(float)writeMisses)*100/((float)reads+(float)writes));
   }
   ulong getCCT(){return ccTransfer;}
   ulong getMMT(){return memTransact;}
   ulong getINV(){return invalidations;}
   ulong getINT(){return interventions;}
   ulong getFlush(){return flushes;}
   ulong getBusRd(){return busRd;}
   ulong getBusRdX(){return busRdX;}
   ulong getBusUpgr(){return busUpgr;}

   void writeBack(ulong)   {writeBacks++;}
   void inc_flush()    {flushes++;}
   void inc_ccTransfer()    {ccTransfer++;}
   void inc_intervene()   {interventions++;}
   void inc_invalidate()   {invalidations++;}
   void inc_memTransact()   {memTransact++;}
   void inc_busRd()   {busRd++;}
   void inc_busRdX()   {busRdX++;}
   void inc_busUpgr()   {busUpgr++; }

   void Access(ulong,uchar);
   void updateLRU(cacheLine *);
};

#endif
